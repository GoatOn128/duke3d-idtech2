/* Opening scripted aircraft/car motion: original ACTORS.cpp HELECOPT/DUKECAR. */
#include "dncon.h"

static int IsVehicle(const edict_t *entity)
{
    return entity->tile == DN_ConSymbol("DUKECAR", 2491) ||
           entity->tile == DN_ConSymbol("HELECOPT", 1346);
}

void DN_InitVehicle(edict_t *entity)
{
    if (IsVehicle(entity))
    {
        entity->cstat = 0;
        entity->health = 1;
        entity->speed = 292.f / 16;
        entity->zspeed = 360.f / 256;
        entity->count = 0;
    }
}

int DN_VehicleTick(edict_t *entity)
{
    if (!IsVehicle(entity))
    {
        return 0;
    }
    int entered = 0;
    for (int i = 1; i <= dn_maxclients; i++)
    {
        entered |= dn_edicts[i].inuse && dn_edicts[i].client && dn_clients[i - 1].connected;
    }
    if (!entered)
    {
        return 1; /* Server setup runs frames before clients can hear the opening sequence. */
    }
    entity->s.origin[2] -= entity->zspeed;
    if (++entity->count == 4)
    {
        DN_PlaySound(entity, DN_ConSymbol("WAR_AMBIENCE2", 233), 0, 0);
    }
    if (entity->count > 26 * 8)
    {
        DN_PlaySound(entity, DN_ConSymbol("RPG_EXPLODE", 9), 0, 1);
        DN_SpawnScrap(entity, 32);
        DN_Remove(entity);
        return 1;
    }
    if (!(entity->count & 3))
    {
        DN_SpawnSprite(entity, DN_ConSymbol("EXPLOSION2", 1890));
    }
    vec3_t end;
    float angle = entity->s.angles[1] * (float)M_PI / 180;
    VectorSet(end, entity->s.origin[0] + cosf(angle) * entity->speed,
              entity->s.origin[1] + sinf(angle) * entity->speed,
              entity->s.origin[2] - entity->zspeed);
    trace_t trace = gi.trace(entity->s.origin, vec3_origin, vec3_origin, end, entity, MASK_SOLID);
    VectorCopy(trace.endpos, entity->s.origin);
    DN_Link(entity);
    return 1;
}
