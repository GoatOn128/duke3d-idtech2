/* Native ACTORS.cpp enemies which have no CON actor: rats, Recon and green slime.
   Native counters live in the edict so save/restore preserves the state machine. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

static int Angle(const edict_t *entity)
{
    return (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
}

static void SetAngle(edict_t *entity, int angle)
{
    entity->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
}

static int TargetAngle(const edict_t *entity, const float *point)
{
    return DN_GetAngle((int)((point[0] - entity->s.origin[0]) * 16),
                       (int)((entity->s.origin[1] - point[1]) * 16));
}

static int Move(edict_t *entity)
{
    vec3_t start, end;
    VectorCopy(entity->s.origin, start);
    start[2] += fmaxf(4, DN_ActorHeight(entity) / 2);
    VectorCopy(start, end);
    int speed = (int)lroundf(entity->speed * 16), angle = Angle(entity);
    end[0] += ((speed * DN_Sin(angle + 512)) >> 14) / 32.f;
    end[1] -= ((speed * DN_Sin(angle)) >> 14) / 32.f;
    vec3_t mins = {-12, -12, -4}, maxs = {12, 12, 4};
    trace_t trace = gi.trace(start, mins, maxs, end, entity, MASK_MONSTERSOLID);
    entity->s.origin[0] = trace.endpos[0];
    entity->s.origin[1] = trace.endpos[1];
    entity->movflag = trace.fraction < 1 ? 32768 : 0;
    entity->sector = DN_FindSectorXY(&dn_world, entity->s.origin, entity->sector);
    if (trace.fraction < 1 && entity->tile == DN_ConSymbol("RECON", 1960))
    {
        /* Native Recon never runs the CON getglobalz countdown: movesprite
           turns it 135 degrees on a blocked wall/sprite move. */
        SetAngle(entity, angle + 768);
    }
    return trace.fraction == 1;
}

static void Kill(edict_t *entity, edict_t *player, const char *sound)
{
    if (player)
    {
        player->client->kills++;
    }
    DN_PlaySound(entity, DN_ConSymbol(sound, -1), 0, 0);
    DN_Remove(entity);
}

static int Visible(edict_t *entity, edict_t *player)
{
    if (!player || player->client->health <= 0)
    {
        return 0;
    }
    vec3_t source, target;
    VectorCopy(entity->s.origin, source);
    source[2] += 16;
    VectorCopy(player->s.origin, target);
    target[2] += player->client->ps.viewoffset[2];
    return DN_CanSee(&dn_world, source, entity->sector, target, player->sector);
}

static void Rat(edict_t *entity)
{
    DN_Fall(entity);
    int angle = Angle(entity);
    if (Move(entity))
    {
        if ((DN_Random() & 255) < 3)
        {
            DN_PlaySound(entity, DN_ConSymbol("RATTY", -1), 0, 0);
        }
        angle += (DN_Random() & 31) - 15 + (DN_Sin(entity->native_state[0] * 256) >> 11);
    }
    else if (++entity->native_state[0] > 1)
    {
        DN_Remove(entity);
        return;
    }
    else
    {
        angle = DN_Random() & 2047;
    }
    if (entity->speed < 8)
    {
        entity->speed += 2.f / 16;
    }
    SetAngle(entity, angle + (DN_Random() & 3) - 6);
}

static int Locator(int tag)
{
    for (int i = dn_world.numsprites - 1; i >= 0; i--)
    {
        if (dn_world.sprites[i].picnum == DN_ConSymbol("LOCATORS", 6) &&
            dn_world.sprites[i].lotag == tag)
        {
            return i;
        }
    }
    return -1;
}

static void Recon(edict_t *entity, edict_t *player, float distance)
{
    int *t = entity->native_state;
    float floor, range_ceiling;
    DN_ActorZRange(entity, &floor, &range_ceiling);
    float ceiling =
        DN_PlaneZ(&dn_world, entity->sector, 1, entity->s.origin[0], entity->s.origin[1]);
    const dn_sector_t *sector = &dn_world.sectors[entity->sector];
    int shade = sector->ceilingstat & 1 ? sector->ceilingshade : sector->floorshade;
    entity->shade += (shade - entity->shade) >> 1;
    entity->s.origin[2] = fminf(entity->s.origin[2], ceiling - 32);
    if (entity->hitweapon)
    {
        entity->hitweapon = 0;
        if (entity->health < 0 && t[0] != -1)
        {
            t[0] = -1;
            entity->health = 0;
        }
        DN_PlaySound(entity, DN_ConSymbol("RECO_PAIN", -1), 0, 0);
        DN_SpawnScrap(entity, 1);
    }
    if (t[0] == -1)
    {
        entity->s.origin[2] -= 4;
        if ((++t[2] & 3) == 0)
        {
            DN_SpawnSprite(entity, DN_ConSymbol("EXPLOSION2", 1890));
        }
        SetAngle(entity, Angle(entity) + 96);
        entity->speed = 8;
        if (!Move(entity) || entity->s.origin[2] < floor)
        {
            DN_SpawnScrap(entity, 16);
            DN_SpawnSprite(entity, DN_ConSymbol("PIGCOP", 2000));
            Kill(entity, player, "LASERTRIP_EXPLODE");
        }
        return;
    }
    entity->s.origin[2] = fmaxf(entity->s.origin[2], floor + 48);
    int angle = Angle(entity);
    int visible = Visible(entity, player);
    if (t[0] >= 4)
    {
        if ((++t[2] & 15) == 0 && player)
        {
            SetAngle(entity, entity->native_angle);
            DN_PlaySound(entity, DN_ConSymbol("RECO_ATTACK", -1), 0, 0);
            DN_Shoot(entity, DN_ConSymbol("FIRELASER", 1625));
            SetAngle(entity, angle);
        }
        if (t[2] > 78 || !visible)
        {
            t[0] = t[2] = 0;
        }
        else
        {
            entity->native_angle +=
                DN_AngleDelta(entity->native_angle, TargetAngle(entity, player->s.origin)) / 3;
        }
    }
    else if (t[0] == 2 || t[0] == 3)
    {
        t[3] = 0;
        entity->speed = fmaxf(0, entity->speed - 1);
        if (player)
        {
            if (t[0] == 2)
            {
                float eye = player->s.origin[2] + player->client->ps.viewoffset[2];
                if (fabsf(eye - entity->s.origin[2]) < 48)
                {
                    t[0] = 3;
                }
                else
                {
                    entity->s.origin[2] += eye > entity->s.origin[2] ? 4 : -4;
                }
            }
            else if (++t[2] > 78 || !visible)
            {
                t[0] = 1;
                t[2] = 0;
            }
            else if ((t[2] & 15) == 0)
            {
                DN_PlaySound(entity, DN_ConSymbol("RECO_ATTACK", -1), 0, 0);
                DN_Shoot(entity, DN_ConSymbol("FIRELASER", 1625));
            }
            angle += DN_AngleDelta(angle, TargetAngle(entity, player->s.origin)) >> 2;
        }
    }
    if (t[0] != 2 && t[0] != 3)
    {
        /* ACTORS.cpp steers toward the old locator before selecting the next
           one. Changing this order makes the craft snap into each corner. */
        int goal = angle;
        if (t[4] >= 0 && t[4] < dn_world.numsprites)
        {
            const dn_sprite_t *locator = &dn_world.sprites[t[4]];
            vec3_t point = {locator->x / 16.f, -locator->y / 16.f, -locator->z / 256.f};
            float dx = point[0] - entity->s.origin[0];
            float dy = point[1] - entity->s.origin[1];
            float dz = point[2] - entity->s.origin[2];
            float reach = sqrtf(dx * dx + dy * dy);
            if (reach <= 1524.f / 16)
            {
                entity->speed = ((int)lroundf(entity->speed * 16) >> 1) / 16.f;
            }
            else
            {
                goal = TargetAngle(entity, point);
            }
            if (t[0] == 1 || t[0] == 4)
            {
                if (sqrtf(dx * dx + dy * dy + dz * dz) <= 1524.f / 16)
                {
                    t[0] = t[0] == 1 ? 0 : 5;
                }
                else if (entity->speed < 16)
                {
                    entity->speed += 2;
                }
                if (t[0] < 2)
                {
                    t[2]++;
                }
                if (t[0] < 2 && t[2] > 104 && distance < 6144.f / 16)
                {
                    t[0] = 2 + (DN_Random() & 2);
                    t[2] = 0;
                    entity->native_angle = angle;
                }
            }
        }
        if (t[0] == 0 || t[0] == 5)
        {
            t[0] = t[0] == 0 ? 1 : 4;
            int locator = Locator(entity->hitag);
            if (locator < 0)
            {
                entity->hitag = t[5];
                locator = Locator(entity->hitag);
            }
            else
            {
                entity->hitag++;
            }
            if (locator < 0)
            {
                /* Native Recon requires a locator route. Retire malformed
                   placements rather than dereferencing sprite[-1]. */
                DN_Remove(entity);
                return;
            }
            t[4] = locator;
        }
        if (t[4] < 0 || t[4] >= dn_world.numsprites)
        {
            DN_Remove(entity);
            return;
        }
        const dn_sprite_t *locator = &dn_world.sprites[t[4]];
        vec3_t point = {locator->x / 16.f, -locator->y / 16.f, -locator->z / 256.f};
        t[3] = DN_AngleDelta(angle, goal);
        angle += t[3] >> 3;
        entity->s.origin[2] += entity->s.origin[2] > point[2] ? -4 : 4;
    }
    SetAngle(entity, angle);
    DN_PlaySound(entity, DN_ConSymbol("RECO_ROAM", -1), 1, 0);
    Move(entity);
}

static void Slime(edict_t *entity, edict_t *player, float distance)
{
    int *t = entity->native_state, base = DN_ConSymbol("GREENSLIME", 2370);
    t[1] = (t[1] + 128) & 2047;
    const dn_sector_t *sector = &dn_world.sectors[entity->sector];
    if (sector->floorstat & 1)
    {
        DN_Remove(entity);
        return;
    }
    if (t[0] == -5)
    {
        if (++t[3] > 280)
        {
            entity->pal = t[0] = 0;
        }
        else
        {
            DN_Fall(entity);
            entity->cstat = 257;
            entity->tile = base + 2;
            entity->health = entity->pal = 1;
            if (entity->hitweapon && entity->lastweapon != DN_ConSymbol("FREEZEBLAST", 1641))
            {
                int args[] = {30};
                DN_ConCommand(entity, DN_OP_LOTSOFGLASS, args, 1);
                Kill(entity, NULL, "GLASS_BREAKING");
            }
            entity->hitweapon = 0;
            return;
        }
    }
    entity->cstat = distance < 1596.f / 16 ? 0 : 257;
    if (entity->hitweapon)
    {
        entity->hitweapon = 0;
        if (entity->lastweapon == DN_ConSymbol("FREEZEBLAST", 1641))
        {
            t[0] = -5;
            t[3] = 0;
            if (player)
            {
                player->client->kills++;
            }
            DN_PlaySound(entity, DN_ConSymbol("SOMETHINGFROZE", -1), 0, 0);
        }
        else
        {
            DN_SpawnScrap(entity, 8);
            Kill(entity, player, "SLIM_DYING");
        }
        return;
    }
    if (t[0] == -4 && player)
    {
        if (player->client->health <= 0)
        {
            t[0] = 0;
            return;
        }
        if (player->client->buttons & BUTTON_ATTACK)
        {
            DN_SpawnScrap(entity, 8);
            Kill(entity, player, "SLIM_DYING");
            return;
        }
        int angle = (int)lroundf(-player->client->ps.viewangles[YAW] * 2048 / 360) & 2047;
        entity->s.origin[0] = player->s.origin[0] + (DN_Sin(angle + 512) >> 7) / 16.f;
        entity->s.origin[1] = player->s.origin[1] - (DN_Sin(angle) >> 7) / 16.f;
        entity->s.origin[2] = player->s.origin[2] + player->client->ps.viewoffset[2] - 8;
        SetAngle(entity, angle);
        static const int frames[] = {5, 5, 6, 6, 7, 7, 6, 5};
        entity->tile = base + frames[t[3] & 7];
        if (t[3] == 5)
        {
            DN_Damage(player, entity, 5 + (DN_Random() & 3), base);
            DN_PlaySound(entity, DN_ConSymbol("SLIM_ATTACK", -1), 0, 0);
        }
        if (t[3])
        {
            t[3] = (t[3] + 1) & 7;
        }
        else if ((DN_Random() >> 8) >= 223)
        {
            t[3] = 1;
        }
        entity->xrepeat = 20 + (DN_Sin(t[1]) >> 13);
        entity->yrepeat = 15 + (DN_Sin(t[1]) >> 13);
        return;
    }
    if (player && player->client->health > 0 && entity->speed < 4 && distance < 48 &&
        Visible(entity, player))
    {
        int attached = 0;
        for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
        {
            attached |= dn_edicts[i].inuse && &dn_edicts[i] != entity &&
                        dn_edicts[i].native_state[0] == -4 &&
                        dn_edicts[i].native_state[4] == player->s.number;
        }
        if (!attached)
        {
            t[0] = -4;
            t[4] = player->s.number;
            return;
        }
    }
    float floor, ceiling;
    DN_ActorZRange(entity, &floor, &ceiling);
    if (t[0] == -1)
    {
        DN_Fall(entity);
        entity->cstat &= ~8;
        entity->tile = base + 4;
        if (entity->xrepeat > 32)
        {
            entity->xrepeat -= DN_Random() & 7;
        }
        if (entity->yrepeat > 16)
        {
            entity->yrepeat -= DN_Random() & 7;
        }
        else
        {
            entity->xrepeat = 40;
            entity->yrepeat = 16;
            t[5] = -1;
            t[0] = 0;
        }
        return;
    }
    if (t[0] == -2)
    {
        int index = t[5];
        if (index <= dn_maxclients || index >= globals.num_edicts || !dn_edicts[index].inuse)
        {
            t[0] = -1;
            return;
        }
        edict_t *victim = &dn_edicts[index];
        victim->speed = 0;
        VectorCopy(victim->s.origin, entity->s.origin);
        int angle = Angle(victim);
        entity->s.origin[0] += (DN_Sin(angle + 512) >> 11) / 16.f;
        entity->s.origin[1] -= (DN_Sin(angle) >> 11) / 16.f;
        entity->tile = base + 2 + (entity->count & 1);
        if (entity->yrepeat < 64)
        {
            entity->yrepeat += 2;
        }
        else if (entity->xrepeat < 32)
        {
            entity->xrepeat += 4;
        }
        else
        {
            t[0] = -1;
            DN_Remove(victim);
        }
        return;
    }
    if ((DN_Random() >> 8) >= 223)
    {
        for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
        {
            edict_t *victim = &dn_edicts[i];
            if (!victim->inuse || victim->sector != entity->sector || victim->health <= 0 ||
                (victim->tile != DN_ConSymbol("LIZTROOP", 1680) &&
                 victim->tile != DN_ConSymbol("LIZMAN", 2120) &&
                 victim->tile != DN_ConSymbol("PIGCOP", 2000) &&
                 victim->tile != DN_ConSymbol("NEWBEAST", 4610)))
            {
                continue;
            }
            float dx = victim->s.origin[0] - entity->s.origin[0];
            float dy = victim->s.origin[1] - entity->s.origin[1];
            if (dx * dx + dy * dy < 48 * 48 &&
                fabsf(victim->s.origin[2] - entity->s.origin[2]) < 32)
            {
                t[5] = i;
                t[0] = -2;
                t[1] = 0;
                return;
            }
        }
    }
    if (t[0] == 0 || t[0] == 2)
    {
        entity->tile = base;
        if (t[0] == 2)
        {
            entity->zspeed = 0;
            entity->cstat &= ~8;
            if ((sector->ceilingstat & 1) || entity->s.origin[2] < ceiling - 24)
            {
                entity->s.origin[2] -= 8;
                t[0] = 3;
                return;
            }
        }
        else
        {
            entity->cstat |= 8;
            DN_Fall(entity);
        }
        if (entity->count & 1)
        {
            Move(entity);
        }
        entity->speed = (64 - (DN_Sin(t[1] + 512) >> 9)) / 16.f;
        if (player)
        {
            SetAngle(
                entity,
                Angle(entity) +
                    (DN_AngleDelta(Angle(entity), TargetAngle(entity, player->s.origin)) >> 3));
        }
        entity->xrepeat = 36 + (DN_Sin(t[1] + 512) >> 11);
        entity->yrepeat = 16 + (DN_Sin(t[1]) >> 13);
        if ((DN_Random() >> 8) >= 251 && !(sector->ceilingstat & 1) && ceiling - floor < 192)
        {
            entity->zspeed = 0;
            t[0]++;
        }
    }
    if (t[0] == 1)
    {
        entity->tile = base;
        entity->yrepeat = DN_Min(40, entity->yrepeat + 8);
        entity->xrepeat = DN_Max(8, entity->xrepeat - 4);
        entity->zspeed = fminf(12, entity->zspeed + 348.f / 256);
        entity->s.origin[2] += entity->zspeed;
        if (entity->s.origin[2] > ceiling - 16)
        {
            entity->s.origin[2] = ceiling - 16;
            entity->speed = 0;
            t[0] = 2;
        }
    }
    if (t[0] == 3)
    {
        entity->tile = base + 1;
        DN_Fall(entity);
        entity->yrepeat = DN_Min(36, entity->yrepeat + 8);
        entity->xrepeat = DN_Max(8, entity->xrepeat - 4);
        if (entity->s.origin[2] < floor + 8)
        {
            entity->s.origin[2] = floor + 8;
            entity->speed = 0;
            t[0] = 0;
        }
    }
}

int DN_NativeEnemyTick(edict_t *entity)
{
    int rat = entity->tile == DN_ConSymbol("RAT", 1267);
    int recon = entity->tile == DN_ConSymbol("RECON", 1960);
    int slime = entity->tile - DN_ConSymbol("GREENSLIME", 2370);
    if (!rat && !recon && (slime < 0 || slime > 7))
    {
        return 0;
    }
    if (entity->sector < 0 || entity->sector >= dn_world.numsectors)
    {
        DN_Remove(entity);
        return 1;
    }
    float distance;
    edict_t *player = DN_NearestPlayer(entity, &distance);
    /* Once attached, retain this player even if another player gets closer. */
    int target = entity->native_state[4];
    if (!rat && !recon && entity->native_state[0] == -4 && target > 0 && target <= dn_maxclients)
    {
        if (dn_edicts[target].inuse && dn_clients[target - 1].connected)
        {
            player = &dn_edicts[target];
        }
        else
        {
            entity->native_state[0] = 0;
        }
    }
    if (rat)
    {
        Rat(entity);
    }
    else if (recon)
    {
        Recon(entity, player, distance);
    }
    else
    {
        Slime(entity, player, distance);
    }
    entity->count++;
    if (entity->inuse)
    {
        DN_Link(entity);
    }
    return 1;
}
