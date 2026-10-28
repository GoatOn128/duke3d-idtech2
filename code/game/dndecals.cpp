/* Native BLOODPOOL/PUKE initialization and their floor-aligned growth. */
#include "dncon.h"
#include "../qcommon/dnmath.h"

int DN_IsBloodSplat(int tile)
{
    return tile == DN_ConSymbol("BLOODSPLAT1", 2296) || tile == DN_ConSymbol("BLOODSPLAT2", 2297) ||
           tile == DN_ConSymbol("BLOODSPLAT3", 2298) || tile == DN_ConSymbol("BLOODSPLAT4", 2299);
}

static int IsPool(const edict_t *entity)
{
    return entity->tile == DN_ConSymbol("BLOODPOOL", 1226) ||
           entity->tile == DN_ConSymbol("PUKE", 4389);
}

void DN_InitDecal(edict_t *entity, const edict_t *parent)
{
    if (DN_IsBloodSplat(entity->tile))
    {
        entity->actor = -1;
        entity->cstat = 16;
        entity->xrepeat = 7 + (DN_Random() & 7);
        entity->yrepeat = 7 + (DN_Random() & 7);
        entity->s.origin[2] += 16;
        entity->pal = parent && parent->pal == 6 ? 6 : 0;
        return;
    }
    if (!IsPool(entity) || entity->sector < 0)
    {
        return;
    }
    /* GAME.cpp rejects pools that straddle different floor heights. */
    const dn_sector_t *floor = &dn_world.sectors[entity->sector];
    for (int corner = 0; corner < 4; corner++)
    {
        vec3_t point;
        VectorCopy(entity->s.origin, point);
        point[0] += (corner & 1 ? 108 : -108) / 16.f;
        point[1] += (corner & 2 ? 108 : -108) / 16.f;
        int sector = DN_FindSector(&dn_world, point, entity->sector);
        if (sector < 0 || dn_world.sectors[sector].floorz != floor->floorz)
        {
            entity->xrepeat = entity->yrepeat = 0;
            return;
        }
    }
    if (floor->lotag == 1)
    {
        entity->xrepeat = entity->yrepeat = 0;
        return;
    }
    if (parent && entity->tile != DN_ConSymbol("PUKE", 4389))
    {
        if (parent->pal == 1)
        {
            entity->pal = 1;
        }
        else if (parent->pal != 6 && parent->tile != DN_ConSymbol("NUKEBARREL", 1227) &&
                 parent->tile != DN_ConSymbol("TIRE", 990))
        {
            entity->pal = parent->tile == DN_ConSymbol("FECES", 2200) ? 7 : 2;
        }
        else
        {
            entity->pal = 0;
        }
        if (parent->tile == DN_ConSymbol("TIRE", 990))
        {
            entity->shade = 127;
        }
    }
    entity->cstat = (entity->cstat & ~(1 | 16 | 128 | 256)) | 32;
    if (parent)
    {
        entity->xrepeat = entity->yrepeat = 1;
    }
    entity->s.origin[2] =
        DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]) + 1;
}

int DN_DecalTick(edict_t *entity)
{
    if (DN_IsBloodSplat(entity->tile))
    {
        if (entity->count < 7 * 26)
        {
            entity->s.origin[2] -= (16 + (DN_Random() & 15)) / 256.f;
            if (++entity->count % 9 == 0)
            {
                ++entity->yrepeat;
            }
            DN_Link(entity);
        }
        return 1;
    }
    if (!IsPool(entity))
    {
        return 0;
    }
    if (entity->sector < 0 || dn_world.sectors[entity->sector].floorstat & 2)
    {
        DN_Remove(entity);
        return 1;
    }
    entity->s.origin[2] =
        DN_PlaneZ(&dn_world, entity->sector, 0, entity->s.origin[0], entity->s.origin[1]) + 1;
    if (entity->count < 32 && entity->xrepeat && entity->yrepeat)
    {
        ++entity->count;
        int limit = entity->spawnedby == DN_ConSymbol("TIRE", 990) ? 64 : 32;
        if (entity->xrepeat < limit && entity->yrepeat < limit)
        {
            entity->xrepeat += DN_Random() & 3;
            entity->yrepeat += DN_Random() & 3;
        }
    }
    DN_Link(entity);
    return 1;
}

void DN_ShootBlood(edict_t *entity, int tile)
{
    /* PLAYER.cpp's BLOODSPLAT shoot case paints a nearby wall. It never fires
       a damaging weapon; CON invokes it repeatedly during enemy hit states. */
    vec3_t start, end;
    VectorCopy(entity->s.origin, start);
    start[2] +=
        entity->client ? entity->client->ps.viewoffset[2] - 4 : DN_ActorHeight(entity) / 2 + 3;
    int angle = (int)lroundf(-entity->s.angles[YAW] * 2048 / 360);
    angle += (entity->client ? 0 : 1024) + 64 - (DN_Random() & 127);
    int vertical = 1024 - (DN_Random() & 2047);
    VectorSet(end, start[0] + DN_Sin(angle + 512) / 256.f, start[1] - DN_Sin(angle) / 256.f,
              start[2] - vertical / 64.f);
    trace_t trace = gi.trace(start, vec3_origin, vec3_origin, end, entity, MASK_SHOT);
    if (trace.fraction == 1 || trace.startsolid || fabsf(trace.plane.normal[2]) > .1f ||
        (trace.ent && trace.ent != dn_edicts))
    {
        return;
    }
    int wall_index = -1, sector_index = -1;
    float closest = .25f;
    for (int sector = 0; sector < dn_world.numsectors; sector++)
    {
        const dn_sector_t *s = &dn_world.sectors[sector];
        for (int index = s->wallptr; index < s->wallptr + s->wallnum; index++)
        {
            const dn_wall_t *wall = &dn_world.walls[index], *next = &dn_world.walls[wall->point2];
            float x = wall->x / 16.f, y = -wall->y / 16.f;
            float dx = next->x / 16.f - x, dy = -next->y / 16.f - y;
            float length = dx * dx + dy * dy;
            if (!length)
            {
                continue;
            }
            float along = ((trace.endpos[0] - x) * dx + (trace.endpos[1] - y) * dy) / length;
            if (along < 0 || along > 1)
            {
                continue;
            }
            float distance =
                fabsf((trace.endpos[0] - x) * dy - (trace.endpos[1] - y) * dx) / sqrtf(length);
            if (distance < closest)
            {
                closest = distance;
                wall_index = index;
                sector_index = sector;
            }
        }
    }
    if (wall_index < 0)
    {
        return;
    }
    const dn_wall_t *wall = &dn_world.walls[wall_index];
    const dn_sector_t *sector = &dn_world.sectors[sector_index];
    if (wall->hitag || (wall->cstat & 16) || sector->lotag ||
        wall->overpicnum == DN_ConSymbol("BIGFORCE", 230) ||
        (wall->nextwall >= 0 && dn_world.walls[wall->nextwall].hitag))
    {
        return;
    }
    if (wall->nextsector >= 0)
    {
        const dn_sector_t *other = &dn_world.sectors[wall->nextsector];
        if (other->lotag || sector->floorz - other->floorz <= 16 * 256)
        {
            return;
        }
        for (int index = 0; index < dn_world.numsprites; index++)
        {
            const dn_sprite_t *sprite = &dn_world.sprites[index];
            if (sprite->sectnum == wall->nextsector && sprite->picnum == 1 && sprite->lotag == 13)
            {
                return;
            }
        }
    }
    edict_t *splat = DN_SpawnSprite(entity, tile);
    VectorMA(trace.endpos, .75f, trace.plane.normal, splat->s.origin);
    splat->s.angles[YAW] = atan2f(trace.plane.normal[1], trace.plane.normal[0]) * 180 / (float)M_PI;
    splat->cstat |= DN_Random() & 4;
    splat->sector = sector_index;
    DN_Link(splat);
}
