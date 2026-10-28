/* Build actor gravity is a per-tick Z velocity, separate from player Pmove. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

void DN_ActorHitPush(edict_t *entity, int damage, int angle)
{
    static const char *fixed[] = {"TANK", "BOSS1", "BOSS4", "BOSS2", "BOSS3", "RECON", "ROTATEGUN"};
    if (!entity->enemy)
    {
        return;
    }
    for (int i = 0; i < sizeof(fixed) / sizeof(fixed[0]); i++)
    {
        if (entity->tile == DN_ConSymbol(fixed[i], -1))
        {
            return;
        }
    }
    /* SECTOR.cpp checkhitsprite faces the victim into the shot and gives it
       negative xvel. pushmove resolves the pose before CON runs again. */
    if (!(entity->cstat & 48))
    {
        entity->s.angles[YAW] = -((angle + 1024) & 2047) * 360.f / 2048;
    }
    entity->speed = -damage / 4.f;
    vec3_t mins = {-8, -8, -4}, maxs = {8, 8, 4};
    DN_PushMove(&dn_world, entity->s.origin, mins, maxs, entity->sector, MASK_MONSTERSOLID);
    DN_Link(entity);
}

static void QueryZRange(edict_t *entity, float *floor, float *ceiling)
{
    vec3_t point, end;
    VectorCopy(entity->s.origin, point);
    point[2] += 1; /* getglobalz queries sprite.z - FOURSLEIGHT. */
    int distance = entity->projectile ? 4 : 127;
    *floor = *ceiling = entity->s.origin[2];
    if (!DN_SectorZRange(&dn_world, point, entity->sector, distance, floor, ceiling))
    {
        return;
    }
    float radius = distance / 16.f;
    vec3_t mins = {-radius, -radius, 0}, maxs = {radius, radius, 0};
    VectorCopy(point, end);
    end[2] = *floor - 1;
    trace_t trace = gi.trace(point, mins, maxs, end, entity, MASK_MONSTERSOLID);
    if (!trace.startsolid && trace.fraction < 1 && trace.ent && trace.ent != dn_edicts &&
        trace.plane.normal[2] > .5f)
    {
        *floor = fmaxf(*floor, trace.ent->s.origin[2] + trace.ent->maxs[2]);
    }
    end[2] = *ceiling + 1;
    trace = gi.trace(point, mins, maxs, end, entity, MASK_MONSTERSOLID);
    if (!trace.startsolid && trace.fraction < 1 && trace.ent && trace.ent != dn_edicts &&
        trace.plane.normal[2] < -.5f)
    {
        *ceiling = fminf(*ceiling, trace.ent->s.origin[2] + trace.ent->mins[2]);
    }
}

void DN_ActorZRange(edict_t *entity, float *floor, float *ceiling)
{
    if (!entity->client && entity->zrange_clock >= 0)
    {
        *floor = entity->cached_floor;
        *ceiling = entity->cached_ceiling;
        return;
    }
    QueryZRange(entity, floor, ceiling);
}

void DN_Fall(edict_t *entity)
{
    if (entity->sector < 0 || entity->sector >= dn_world.numsectors)
    {
        return;
    }
    const dn_sector_t *sector = &dn_world.sectors[entity->sector];
    int gravity = DN_ConSetting("GRAVITATIONALCONSTANT", 176);
    int floor_space = DN_ConCondition(entity, DN_OP_IFINOUTERSPACE, NULL, 0);
    int ceiling_space = DN_ConCondition(entity, DN_OP_IFINSPACE, NULL, 0);
    if (floor_space)
    {
        gravity = 0;
    }
    else if (ceiling_space || sector->lotag == 2)
    {
        gravity /= 6;
    }
    /* GAMEDEF.cpp fall shares its seven-tick getglobalz countdown with
       ACTORS.cpp movesprite's blocked-heading response. Slopes query every tick. */
    if (entity->zrange_clock <= 0 || (sector->floorstat & 2))
    {
        QueryZRange(entity, &entity->cached_floor, &entity->cached_ceiling);
        entity->zrange_clock = 6;
    }
    else
    {
        entity->zrange_clock--;
    }
    float floor = entity->cached_floor;
    if (entity->s.origin[2] > floor + 1)
    {
        int velocity = (int)lroundf(-entity->zspeed * 256);
        if (sector->lotag == 2 && velocity > 3122)
        {
            velocity = 3144;
        }
        velocity += gravity;
        entity->zspeed = -velocity / 256.f;
        entity->s.origin[2] += entity->zspeed;
        if (velocity > 6144)
        {
            entity->zspeed = -6144.f / 256;
        }
    }
    else
    {
        entity->s.origin[2] = floor + 1;
        entity->zspeed = 0;
    }
}
