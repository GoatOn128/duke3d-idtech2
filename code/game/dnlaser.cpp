/* Assault Trooper FIRELASER shots and short-lived native bolt trails. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

void DN_FireLaser(edict_t *entity)
{
    edict_t *player = entity->client ? entity : DN_NearestPlayer(entity, NULL);
    if (!player)
    {
        return;
    }
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360) & 2047;
    vec3_t start;
    VectorCopy(entity->s.origin, start);
    int vertical;
    if (entity->client)
    {
        start[2] += entity->client->ps.viewoffset[2] - 2;
        angle = (int)lroundf(-entity->client->ps.viewangles[YAW] * 2048 / 360) & 2047;
        vertical = (int)(tanf(entity->client->ps.viewangles[PITCH] * (float)M_PI / 180) * 840 * 16);
    }
    else
    {
        start[2] += DN_ActorHeight(entity) / 2 + 5;
        start[0] += (DN_Sin(angle + 1024 + 96) >> 7) / 16.f;
        start[1] -= (DN_Sin(angle + 512 + 96) >> 7) / 16.f;
        angle += 16 - (DN_Random() & 31);
        float dx = (player->s.origin[0] - entity->s.origin[0]) * 16;
        float dy = (player->s.origin[1] - entity->s.origin[1]) * 16;
        int distance = (int)sqrtf(dx * dx + dy * dy) + 1;
        int difference =
            (int)((start[2] - player->s.origin[2] - player->client->ps.viewoffset[2] + 3) * 256);
        vertical = (int)((int64_t)difference * 840 / distance);
    }
    edict_t *laser = DN_SpawnSprite(entity, DN_ConSymbol("FIRELASER", 1625));
    laser->projectile = DN_PROJECTILE_FIRELASER;
    laser->actor = -1;
    laser->damage = laser->health + (DN_Random() & 7);
    laser->xrepeat = laser->yrepeat = entity->client ? 7 : 18;
    laser->cstat = 128;
    laser->clipdist = 4;
    laser->shade = -127;
    laser->firetime = dn_time + 8;
    laser->s.angles[YAW] = -(angle & 2047) * 360.f / 2048;
    VectorCopy(start, laser->s.origin);
    laser->velocity[0] = ((840 * DN_Sin(angle + 512)) >> 14) * DN_SIMULATION_HZ / 32.f;
    laser->velocity[1] = -((840 * DN_Sin(angle)) >> 14) * DN_SIMULATION_HZ / 32.f;
    laser->velocity[2] = -vertical * DN_SIMULATION_HZ / 512.f;
    /* shoot() consumes these even when scount is one; preserve its RNG ordering. */
    DN_Random();
    DN_Random();
    if (entity->health >= 0)
    {
        entity->shade = -96;
    }
    DN_Link(laser);
    if (gi.cvar("dn_debug_projectiles", "0", 0)->value)
    {
        gi.dprintf("Duke laser spawn: %.3f %.3f %.3f velocity %.3f %.3f %.3f damage %d\n",
                   laser->s.origin[0], laser->s.origin[1], laser->s.origin[2], laser->velocity[0],
                   laser->velocity[1], laser->velocity[2], laser->damage);
    }
}

void DN_LaserTick(edict_t *laser)
{
    if (laser->projectile == DN_PROJECTILE_LASERTRAIL)
    {
        if (++laser->count >= 2)
        {
            DN_Remove(laser);
        }
        return;
    }
    vec3_t end;
    VectorMA(laser->s.origin, 1.f / DN_SIMULATION_HZ, laser->velocity, end);
    trace_t trace =
        gi.trace(laser->s.origin, vec3_origin, vec3_origin, end, laser->owner, MASK_SHOT);
    DN_DamageWall(laser->owner, laser->s.origin, end, &trace);
    VectorCopy(trace.endpos, laser->s.origin);
    int angle = (int)lroundf(-laser->s.angles[YAW] * 2048 / 360) & 2047;
    int vertical = (int)lroundf(-laser->velocity[2] * 512 / DN_SIMULATION_HZ);
    for (int k = -3; k < 2; k++)
    {
        edict_t *trail = DN_SpawnSprite(laser, laser->tile);
        trail->actor = -1;
        trail->owner = laser->owner;
        trail->projectile = DN_PROJECTILE_LASERTRAIL;
        trail->xrepeat = laser->xrepeat;
        trail->yrepeat = laser->yrepeat;
        trail->cstat = 128;
        trail->pal = laser->pal;
        trail->shade = -40 + k * 4;
        trail->s.angles[YAW] = 0;
        trail->s.origin[0] += ((k * DN_Sin(angle + 512)) >> 9) / 16.f;
        trail->s.origin[1] -= ((k * DN_Sin(angle)) >> 9) / 16.f;
        trail->s.origin[2] -= k * (vertical / 24) / 256.f;
        DN_Link(trail);
    }
    if (trace.fraction < 1)
    {
        if (gi.cvar("dn_debug_projectiles", "0", 0)->value)
        {
            gi.dprintf("Duke laser hit: %.3f %.3f %.3f entity %d player %d startsolid %d\n",
                       trace.endpos[0], trace.endpos[1], trace.endpos[2],
                       trace.ent ? trace.ent->s.number : -1, trace.ent && trace.ent->client != NULL,
                       trace.startsolid);
        }
        if (trace.ent && trace.ent != dn_edicts)
        {
            DN_Damage(trace.ent, laser->owner, laser->damage, laser->tile);
        }
        DN_Remove(laser);
        return;
    }
    if (dn_time > laser->firetime)
    {
        DN_Remove(laser);
        return;
    }
    DN_Link(laser);
}
