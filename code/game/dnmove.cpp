/* CON movement rules from GAMEDEF.cpp move()/alterang(), in Build integer units. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

static int AngleTo(const edict_t *entity, const float *point)
{
    return DN_GetAngle((int)((point[0] - entity->s.origin[0]) * 16),
                       (int)((entity->s.origin[1] - point[1]) * 16));
}

int DN_DodgeProjectile(edict_t *entity)
{
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    int mx = DN_Sin(angle + 512), my = DN_Sin(angle);
    for (int i = dn_maxclients + 1; i < globals.num_edicts; i++)
    {
        edict_t *shot = &dn_edicts[i];
        if (!shot->inuse || shot == entity || shot->owner == entity ||
            shot->sector != entity->sector || !shot->projectile ||
            shot->projectile == DN_PROJECTILE_LASERTRAIL ||
            shot->projectile == DN_PROJECTILE_BEAM || shot->projectile == DN_PROJECTILE_TRIPBOMB ||
            (!shot->velocity[0] && !shot->velocity[1]))
        {
            continue;
        }
        int bx = (int)lroundf((shot->s.origin[0] - entity->s.origin[0]) * 16);
        int by = (int)lroundf((entity->s.origin[1] - shot->s.origin[1]) * 16);
        int heading = DN_GetAngle((int)lroundf(shot->velocity[0] * 16),
                                  (int)lroundf(-shot->velocity[1] * 16));
        int vx = DN_Sin(heading + 512), vy = DN_Sin(heading);
        /* GAMEDEF.cpp dodge(): in front, approaching, within a 256-unit
           native XY corridor. Widen the products to avoid map-size overflow. */
        int64_t cross = (int64_t)vx * by - (int64_t)vy * bx;
        if ((int64_t)mx * bx + (int64_t)my * by >= 0 && (int64_t)vx * bx + (int64_t)vy * by < 0 &&
            cross > -65536LL * 64 && cross < 65536LL * 64)
        {
            angle -= 512 + (DN_Random() & 1024);
            entity->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
            return 1;
        }
    }
    return 0;
}

static trace_t LookAhead(edict_t *entity, int angle, float height)
{
    vec3_t start, end;
    VectorCopy(entity->s.origin, start);
    start[2] += height;
    VectorCopy(start, end);
    end[0] += DN_Sin(angle + 512) * 4.f;
    end[1] -= DN_Sin(angle) * 4.f;
    return gi.trace(start, vec3_origin, vec3_origin, end, entity, MASK_SHOT);
}

static int FurthestAngle(edict_t *entity, int angle)
{
    if ((entity->count & 63) > 2)
    {
        return (angle + 1024) & 2047;
    }
    int best = angle;
    float greatest = -1;
    for (int i = 0; i < 2; i++)
    {
        int candidate = (angle + i * 1024) & 2047;
        trace_t trace = LookAhead(entity, candidate, 8);
        float distance = fabsf(trace.endpos[0] - entity->s.origin[0]) +
                         fabsf(trace.endpos[1] - entity->s.origin[1]);
        if (distance > greatest)
        {
            greatest = distance;
            best = candidate;
        }
    }
    return best;
}

void DN_MoveActor(edict_t *entity, float previous_z)
{
    float distance;
    edict_t *player = DN_NearestPlayer(entity, &distance);
    int flags = entity->moveflags == -1 ? 0 : entity->moveflags;
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    int goal = player ? AngleTo(entity, player->s.origin) : angle;
    if (flags & 1)
    {
        int delta = DN_AngleDelta(angle, goal) >> 2;
        if (delta > -8 && delta < 0)
        {
            delta = 0;
        }
        angle += delta;
    }
    if (flags & 32)
    {
        angle += DN_Sin(entity->count * 8) >> 6;
    }
    if (flags & 16)
    {
        int delta = DN_AngleDelta(angle, goal);
        angle += delta > 0 ? 32 : delta < 0 ? -32 : 0;
    }
    int vertical = (int)lroundf(-entity->zspeed * 256);
    if ((flags & 257) == 257 && entity->count < 16)
    {
        vertical -= DN_Sin(512 + entity->count * 16) >> 5;
    }
    if (flags & 64 && player)
    {
        vec3_t predicted;
        VectorCopy(player->s.origin, predicted);
        /* posxv/768 projects approximately two movement ticks ahead. */
        predicted[0] += player->client->ps.pmove.velocity[0] / 8.f / 15;
        predicted[1] += player->client->ps.pmove.velocity[1] / 8.f / 15;
        int delta = DN_AngleDelta(angle, AngleTo(entity, predicted)) >> 2;
        angle += delta > -8 && delta < 0 ? 0 : delta;
    }
    entity->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
    entity->zspeed = -vertical / 256.f;
    if (entity->move <= 0 || entity->move >= 1024 || !flags)
    {
        return;
    }
    const dn_move_t *move = &dn_moves[entity->move];
    int horizontal = (int)lroundf(entity->speed * 16);
    if (flags & 2)
    {
        horizontal += (move->speed - horizontal) >> 1;
    }
    if (flags & 4)
    {
        vertical += (move->zspeed * 16 - vertical) >> 1;
    }
    if (flags & 4096)
    {
        DN_DodgeProjectile(entity);
        angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    }
    horizontal += (move->speed - horizontal) / 5;
    if (vertical < 648)
    {
        vertical += (move->zspeed * 16 - vertical) / 5;
    }
    int phase = entity->count & 31;
    if (flags & 512 && player)
    {
        goal = AngleTo(entity, entity->lastseen);
        if (horizontal && entity->tile != DN_ConSymbol("DRONE", 1880))
        {
            int delta = DN_AngleDelta(angle, goal);
            if (phase < 2 && abs(delta) < 256)
            {
                int turn = 128 - (DN_Random() & 256);
                trace_t trace = LookAhead(entity, angle + turn, 0);
                float reach = fabsf(trace.endpos[0] - entity->s.origin[0]) +
                              fabsf(trace.endpos[1] - entity->s.origin[1]);
                if (reach >= 844.f / 16)
                {
                    angle += turn;
                }
            }
            else if (phase > 18 && phase < 26)
            {
                angle = abs(delta >> 2) < 128 ? goal : angle + (delta >> 2);
            }
        }
        else
        {
            angle = goal;
        }
    }
    if (phase < 1 && flags & (1024 | 128))
    {
        angle = FurthestAngle(entity, angle);
    }
    if (horizontal > -6 && horizontal < 6)
    {
        horizontal = 0;
    }
    entity->speed = horizontal / 16.f;
    entity->zspeed = -vertical / 256.f;
    entity->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
    if (!horizontal && !vertical)
    {
        return;
    }
    if (entity->enemy && entity->sector >= 0)
    {
        float floor =
            DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]);
        float ceiling =
            DN_PlaneZ(&dn_world, entity->sector, 1, entity->s.origin[0], entity->s.origin[1]);
        int commander = entity->tile == DN_ConSymbol("COMMANDER", 1920);
        int drone = entity->tile == DN_ConSymbol("DRONE", 1880);
        if (entity->health > 0 && (commander || drone))
        {
            float top = ceiling - (commander ? 80 : 50);
            float bottom = floor + (commander ? 8 : 30);
            if ((commander || vertical > 0) && entity->s.origin[2] < bottom)
            {
                entity->s.origin[2] = bottom;
                if (commander)
                {
                    vertical = 0;
                }
            }
            if ((commander || vertical <= 0) && entity->s.origin[2] > top)
            {
                entity->s.origin[2] = top;
                vertical = 0;
            }
            entity->zspeed = -vertical / 256.f;
        }
        else if (vertical < 0 && ceiling - entity->s.origin[2] < 66 &&
                 entity->tile != DN_ConSymbol("ORGANTIC", 1820))
        {
            entity->s.origin[2] = ceiling - 66;
            entity->zspeed = -(vertical >> 1) / 256.f;
        }
    }
    int movement = horizontal;
    int movement_angle = angle;
    if (entity->enemy && entity->tile != DN_ConSymbol("ROTATEGUN", 2360))
    {
        if (player && distance < 960.f / 16 && entity->xrepeat > 16)
        {
            movement = -(1024 - (int)(distance * 16));
            movement_angle = AngleTo(entity, player->s.origin);
        }
        else if (entity->tile != DN_ConSymbol("DRONE", 1880) &&
                 entity->tile != DN_ConSymbol("SHARK", 1550) &&
                 entity->tile != DN_ConSymbol("COMMANDER", 1920))
        {
            int interval = previous_z != entity->s.origin[2] ||
                                   (dn_maxclients < 2 && gi.cvar("skill", "1", 0)->value < 1)
                               ? 2
                               : 4;
            if (entity->count & (interval - 1))
            {
                return;
            }
            movement *= interval;
        }
    }
    vec3_t end, previous;
    VectorCopy(entity->s.origin, previous);
    VectorCopy(previous, end);
    /* movesprite uses half of xchange/ychange and zchange per 30 Hz tick. */
    end[0] += ((movement * DN_Sin(movement_angle + 512)) >> 14) / 32.f;
    end[1] -= ((movement * DN_Sin(movement_angle)) >> 14) / 32.f;
    /* Native movesprite clips XY at the sprite's half-height, with four-unit
       plane clearance. An upright box incorrectly treats small curbs as walls. */
    vec3_t probe;
    VectorCopy(previous, probe);
    float height = DN_ActorHeight(entity) / 2;
    probe[2] += height;
    end[2] = probe[2];
    vec3_t mins = {entity->mins[0], entity->mins[1], -4};
    vec3_t maxs = {entity->maxs[0], entity->maxs[1], 4};
    trace_t trace = gi.trace(probe, mins, maxs, end, entity, MASK_MONSTERSOLID);
    entity->s.origin[0] = trace.endpos[0];
    entity->s.origin[1] = trace.endpos[1];
    entity->movflag =
        trace.fraction < 1 ? (trace.ent && trace.ent != dn_edicts ? 49152 : 32768) : 0;
    int sector = DN_FindSectorXY(&dn_world, entity->s.origin, entity->sector);
    if (sector < 0 || (entity->stayput >= 0 && sector != entity->stayput) ||
        (entity->tile == DN_ConSymbol("LIZTROOP", 1680) && !vertical &&
         dn_world.sectors[sector].lotag == 1))
    {
        VectorCopy(previous, entity->s.origin);
        entity->movflag = 16384 | entity->sector;
        return;
    }
    entity->sector = sector;
    if (entity->enemy && (entity->movflag & 49152) >= 32768 && entity->zrange_clock == 0)
    {
        /* Native movesprite turns 135 degrees after a blocked wall/sprite
           move when the shared getglobalz countdown expires. */
        angle = (angle + 768) & 2047;
        entity->s.angles[YAW] = -angle * 360.f / 2048;
    }
    float floor, ceiling;
    DN_ActorZRange(entity, &floor, &ceiling);
    float z = entity->s.origin[2] + entity->zspeed / 2;
    if (z >= floor && z <= ceiling)
    {
        entity->s.origin[2] = z;
    }
    else if (!entity->movflag)
    {
        entity->movflag = 16384 | entity->sector;
    }
    if (entity->enemy && entity->sector >= 0)
    {
        const dn_sector_t *area = &dn_world.sectors[entity->sector];
        int shade = area->ceilingstat & 1 ? area->ceilingshade : area->floorshade;
        entity->shade += (shade - entity->shade) >> 1;
    }
}
