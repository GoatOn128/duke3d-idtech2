/* Server-side damage, hitscan weapons and projectile creation. */
#include "dnlocal.h"
#include "dncon.h"
#include "../qcommon/dnmath.h"

int DN_CanShoot(edict_t *entity, float distance)
{
    if (distance <= 1024.f / 16)
    {
        return 1;
    }
    int large = entity->enemy && entity->xrepeat > 56;
    int turn = large ? 48 : 16;
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    const int offsets[3] = {0, turn, -turn};
    for (int ray = 0; ray < 3; ray++)
    {
        vec3_t start, end;
        VectorCopy(entity->s.origin, start);
        start[2] += entity->enemy ? 42 : entity->client ? 3 : 0;
        VectorCopy(start, end);
        end[0] += DN_Sin(angle + offsets[ray] + 512) * 4.f;
        end[1] -= DN_Sin(angle + offsets[ray]) * 4.f;
        trace_t trace = gi.trace(start, vec3_origin, vec3_origin, end, entity, MASK_SHOT);
        float dx = trace.endpos[0] - start[0], dy = trace.endpos[1] - start[1];
        float limit = ray == 2 ? 768.f / 16 : (large ? 3084.f : 768.f) / 16;
        if (sqrtf(dx * dx + dy * dy) <= limit ||
            (trace.ent && trace.ent != dn_edicts && trace.ent->tile == entity->tile))
        {
            return 0;
        }
    }
    return 1;
}

static void vectoangles(const vec3_t direction, vec3_t angles)
{
    angles[YAW] = atan2f(direction[1], direction[0]) * 180.f / (float)M_PI;
    angles[PITCH] =
        -atan2f(direction[2], sqrtf(direction[0] * direction[0] + direction[1] * direction[1])) *
        180.f / (float)M_PI;
    angles[ROLL] = 0;
}

void DN_Damage(edict_t *target, edict_t *attacker, int damage, int weapon)
{
    DN_DamageAt(target, attacker, damage, weapon, NULL);
}

void DN_DamageAt(edict_t *target, edict_t *attacker, int damage, int weapon, const float *impact)
{
    if (!target || !target->inuse || damage < 0 || (target->s.renderfx & (RF_DUKE_WORLD | RF_BEAM)))
    {
        return;
    }
    if (target->client)
    {
        gclient_t *c = target->client;
        if (c->god)
        {
            return;
        }
        int absorbed = (damage * 2) / 3;
        if (absorbed > c->armor)
        {
            absorbed = c->armor;
        }
        c->armor -= absorbed;
        int alive = c->health > 0;
        c->health -= damage - absorbed;
        target->health = c->health;
        if (alive && c->health <= 0)
        {
            DN_PlayerFrag(target, attacker);
        }
        c->ps.blend[0] = 1;
        c->ps.blend[3] = .25f;
        return;
    }
    if (target->projectile == DN_PROJECTILE_PIPEBOMB ||
        target->projectile == DN_PROJECTILE_TRIPBOMB)
    {
        target->firetime = dn_time;
        return;
    }
    if (DN_DamageFaller(target, damage, weapon) || DN_DamageDestructible(target, weapon))
    {
        return;
    }
    /* Native props have explicit checkhitsprite cases. An unsupported map
       decoration does not become an explosive just because its extra is -1. */
    if (target->actor < 0 && !target->enemy)
    {
        return;
    }
    DN_HitBlood(target, attacker, impact, weapon);
    if (attacker && attacker != dn_edicts && weapon != DN_ConSymbol("RADIUSEXPLOSION", 1670))
    {
        const float *point = impact ? impact : target->s.origin;
        int angle = DN_GetAngle((int)lroundf((point[0] - attacker->s.origin[0]) * 16),
                                (int)lroundf((attacker->s.origin[1] - point[1]) * 16));
        DN_ActorHitPush(target, damage, angle);
    }
    target->health -= damage;
    target->hitweapon = 1;
    target->lastweapon = weapon;
    if (target->dormant)
    {
        target->dormant = 0;
        target->sleep = 1536;
        if (attacker)
        {
            VectorCopy(attacker->s.origin, target->lastseen);
        }
    }
}

static int AimHitscan(edict_t *shooter, const vec3_t start, vec3_t angles)
{
    edict_t *best = NULL;
    float nearest = 4096;
    int facing = (int)lroundf(-angles[YAW] * 2048 / 360) & 2047;
    for (int index = dn_maxclients + 1; index < globals.num_edicts; index++)
    {
        edict_t *target = &dn_edicts[index];
        if (!target->inuse || !target->enemy || target->health < 0 || !target->xrepeat ||
            (target->cstat & (257 | 32768)) != 257)
        {
            continue;
        }
        vec3_t direction, point;
        VectorSubtract(target->s.origin, start, direction);
        float distance = sqrtf(direction[0] * direction[0] + direction[1] * direction[1]);
        int angle = DN_GetAngle((int)(direction[0] * 16), (int)(-direction[1] * 16));
        if (distance <= 32 || distance >= nearest || abs(DN_AngleDelta(facing, angle)) > 48)
        {
            continue;
        }
        float look = tanf(angles[PITCH] * (float)M_PI / 180) * 128;
        if (fabsf(-direction[2] * 160 / distance - look) >= 100)
        {
            continue;
        }
        VectorCopy(target->s.origin, point);
        point[2] += DN_ActorHeight(target) * target->xrepeat / DN_Max(1, target->yrepeat) / 2 + 5;
        trace_t trace =
            gi.trace((float *)start, vec3_origin, vec3_origin, point, shooter, MASK_SHOT);
        if (trace.fraction < 1 && trace.ent != target)
        {
            continue;
        }
        best = target;
        nearest = distance;
    }
    if (!best)
    {
        return 0;
    }
    vec3_t direction;
    VectorSubtract(best->s.origin, start, direction);
    direction[2] += DN_ActorHeight(best) * best->xrepeat / DN_Max(1, best->yrepeat) / 2 + 5;
    vectoangles(direction, angles);
    return 1;
}

static void FireHitscan(edict_t *ent, int damage, int weapon, float spread)
{
    vec3_t start, end, forward, right, angles;
    trace_t tr;
    VectorCopy(ent->s.origin, start);
    if (ent->client)
    {
        start[2] += ent->client->ps.viewoffset[2] - 2;
        VectorCopy(ent->client->ps.viewangles, angles);
        int aimed = weapon != DN_ConSymbol("KNEE", 2521) && AimHitscan(ent, start, angles);
        if (!aimed || weapon != DN_ConSymbol("SHOTSPARK1", 2595))
        {
            angles[YAW] -= (16 - (DN_Random() & 31)) * 360.f / 2048;
            angles[PITCH] += (128 - (DN_Random() & 255)) * 180.f / ((float)M_PI * 4096);
        }
    }
    else
    {
        edict_t *p = DN_NearestPlayer(ent, NULL);
        if (!p)
        {
            return;
        }
        int angle = (int)lroundf(-ent->s.angles[YAW] * 2048 / 360) & 2047;
        start[2] += DN_ActorHeight(ent) / 2 + 7;
        if (ent->tile == DN_ConSymbol("ROTATEGUN", 2360))
        {
            start[2] -= 7;
        }
        else
        {
            start[0] += (DN_Sin(angle + 1024 + 96) >> 7) / 16.f;
            start[1] -= (DN_Sin(angle + 512 + 96) >> 7) / 16.f;
        }
        VectorSubtract(p->s.origin, start, forward);
        forward[2] += p->client->ps.viewoffset[2];
        vectoangles(forward, angles);
        if (ent->tile != DN_ConSymbol("BOSS1", 2630) && weapon != DN_ConSymbol("KNEE", 2521))
        {
            angles[YAW] = ent->s.angles[YAW];
        }
    }
    if (!ent->client)
    {
        angles[0] += ((DN_Random() & 255) / 255.f - .5f) * spread;
        angles[1] += ((DN_Random() & 255) / 255.f - .5f) * spread;
    }
    AngleVectors(angles, forward, right, NULL);
    VectorMA(start, weapon == DN_ConSymbol("KNEE", 2521) ? 64 : 4096, forward, end);
    tr = gi.trace(start, vec3_origin, vec3_origin, end, ent, MASK_SHOT);
    if (ent->client)
    {
        damage += DN_Random() % 6;
    }
    if (gi.cvar("dn_debug_hitscan", "0", 0)->value)
    {
        gi.dprintf(
            "Duke shot: weapon %d eye %.3f %.3f %.3f angles %.3f %.3f hit %d tile %d fraction %.4f "
            "health %d damage %d startsolid %d allsolid %d end %.3f %.3f %.3f cstat %u\n",
            weapon, start[0], start[1], start[2], angles[0], angles[1],
            tr.ent ? tr.ent->s.number : -1, tr.ent ? tr.ent->s.frame : -1, tr.fraction,
            tr.ent ? tr.ent->health : 0, damage, tr.startsolid, tr.allsolid, tr.endpos[0],
            tr.endpos[1], tr.endpos[2], tr.ent ? (unsigned)tr.ent->cstat : 0);
    }
    if (tr.ent && tr.ent != dn_edicts)
    {
        DN_DamageAt(tr.ent, ent, damage, weapon, tr.endpos);
    }
    else
    {
        DN_DamageWall(ent, start, end, &tr);
    }
    if (tr.fraction < 1 && weapon != DN_ConSymbol("KNEE", 2521))
    {
        edict_t *spark = DN_SpawnSprite(ent, DN_ConSymbol("SHOTSPARK1", 2595));
        VectorCopy(tr.endpos, spark->s.origin);
        spark->shade = -15;
        spark->xrepeat = spark->yrepeat = ent->client ? 10 : 24;
        spark->cstat = 0;
        DN_Link(spark);
        edict_t *smoke = DN_SpawnSprite(spark, DN_ConSymbol("SMALLSMOKE", 2329));
        DN_Link(smoke);
    }
}

void DN_Shoot(edict_t *ent, int tile)
{
    if (DN_IsBloodSplat(tile))
    {
        DN_ShootBlood(ent, tile);
        return;
    }
    if (tile == DN_ConSymbol("FIRELASER", 1625))
    {
        DN_FireLaser(ent);
        return;
    }
    if (tile == DN_ConSymbol("SPIT", 1636) || tile == DN_ConSymbol("COOLEXPLOSION1", 1360) ||
        tile == DN_ConSymbol("MORTER", 1650) || tile == DN_ConSymbol("BOUNCEMINE", 940))
    {
        DN_FireEnemyProjectile(ent, tile);
        return;
    }
    int shotgun = tile == DN_ConSymbol("SHOTGUN", 2613), rpg = tile == DN_ConSymbol("RPG", 2605);
    int pipebomb = tile == DN_ConSymbol("HEAVYHBOMB", 26);
    if (tile == DN_ConSymbol("HANDHOLDINGLASER", 2563))
    {
        DN_PlaceTripbomb(ent);
        return;
    }
    if (tile == DN_ConSymbol("SHRINKER", 2556))
    {
        tile = DN_ConSymbol("SHRINKSPARK", 1646);
    }
    if (rpg || tile == DN_ConSymbol("FREEZEBLAST", 1641) ||
        tile == DN_ConSymbol("SHRINKSPARK", 1646) || pipebomb)
    {
        edict_t *rocket = DN_SpawnSprite(ent, tile);
        vec3_t angles, forward;
        rocket->projectile = pipebomb                                    ? DN_PROJECTILE_PIPEBOMB
                             : rpg                                       ? DN_PROJECTILE_ROCKET
                             : tile == DN_ConSymbol("FREEZEBLAST", 1641) ? DN_PROJECTILE_FREEZE
                                                                         : DN_PROJECTILE_SHRINK;
        rocket->damage = DN_ConSymbol(pipebomb ? "HANDBOMB_WEAPON_STRENGTH"
                                      : rpg    ? "RPG_WEAPON_STRENGTH"
                                      : rocket->projectile == DN_PROJECTILE_FREEZE
                                          ? "FREEZETHROWER_WEAPON_STRENGTH"
                                          : "SHRINKER_WEAPON_STRENGTH",
                                      rpg || pipebomb ? 140 : 20);
        rocket->actor = -1;
        rocket->cstat = 0;
        rocket->xrepeat = rocket->yrepeat = 12;
        rocket->firetime = pipebomb ? 0 : dn_time + 8;
        rocket->count = 0;
        rocket->s.origin[2] += ent->client ? ent->client->ps.viewoffset[2] : 20;
        if (ent->client)
        {
            VectorCopy(ent->client->ps.viewangles, angles);
        }
        else
        {
            edict_t *p = DN_NearestPlayer(ent, NULL);
            if (!p)
            {
                DN_Remove(rocket);
                return;
            }
            VectorSubtract(p->s.origin, rocket->s.origin, forward);
            vectoangles(forward, angles);
        }
        AngleVectors(angles, forward, NULL, NULL);
        VectorScale(forward,
                    pipebomb ? 262.5f + (ent->client ? ent->client->weapon_hold * 60 : 0)
                    : rpg    ? 1207.5f
                             : 1440.f,
                    rocket->velocity);
        if (pipebomb)
        {
            rocket->velocity[2] += 60;
        }
        DN_Link(rocket);
        return;
    }
    /* Enemy CON emits each pellet explicitly (Pig Cops issue five shoots).
       Only a player's weapon command expands into the seven-pellet volley. */
    for (int i = 0; i < (shotgun && ent->client ? 7 : 1); i++)
    {
        int damage =
            DN_ConSymbol(shotgun                                   ? "SHOTGUN_WEAPON_STRENGTH"
                         : tile == DN_ConSymbol("KNEE", 2521)      ? "KNEE_WEAPON_STRENGTH"
                         : tile == DN_ConSymbol("GROWSPARK", 2448) ? "GROWSPARK_WEAPON_STRENGTH"
                         : tile == DN_ConSymbol("CHAINGUN", 2536)  ? "CHAINGUN_WEAPON_STRENGTH"
                                                                   : "PISTOL_WEAPON_STRENGTH",
                         6);
        FireHitscan(ent, damage, tile, shotgun ? 5 : 1);
    }
}
