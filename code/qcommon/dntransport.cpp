/* Shared SE7/SE23 dry player transfer rules from ACTORS.cpp movetransports(). */
#include "../game/q_shared.h"
#include "dntransport.h"

static int IsTransport(const dn_sprite_t *sprite)
{
    return sprite->picnum == 1 && (sprite->lotag == 7 || sprite->lotag == 23);
}

void DN_InitTransportLinks(dn_map_t *map)
{
    for (int i = 0; i < map->numsprites; i++)
    {
        dn_sprite_t *source = &map->sprites[i];
        if (!IsTransport(source))
        {
            continue;
        }
        /* SE7 owner is its paired map index. Its unused yvel records the
           original on-floor test so moving planes cannot change its kind. */
        source->yvel = source->z == map->sectors[source->sectnum].floorz;
        source->owner = source->lotag == 23 ? i : -1;
        if (source->lotag == 23)
        {
            continue;
        }
        for (int j = 0; j < map->numsprites; j++)
        {
            if (i != j && IsTransport(&map->sprites[j]) && source->hitag == map->sprites[j].hitag)
            {
                source->owner = j;
                break;
            }
        }
    }
}

static int CrossedAirTransport(const dn_map_t *map, const dn_transport_query_t *query,
                               const dn_sprite_t *source)
{
    float eye = query->point[2] + query->eyeheight, height = source->z / -256.f;
    if (fabsf(eye - height) < 24)
    {
        return 1;
    }
    if (query->has_previous)
    {
        float oldeye = query->previous[2] + query->eyeheight;
        if ((oldeye < height && eye > height) || (oldeye > height && eye < height))
        {
            float fraction = (height - oldeye) / (eye - oldeye);
            return DN_Inside(map, source->sectnum,
                             query->previous[0] + (query->point[0] - query->previous[0]) * fraction,
                             query->previous[1] +
                                 (query->point[1] - query->previous[1]) * fraction);
        }
    }
    return 0;
}

int DN_QueryPlayerTransport(const dn_map_t *map, const dn_transport_query_t *query,
                            dn_transport_result_t *result)
{
    if (query->hold || query->sector < 0 || query->sector >= map->numsectors)
    {
        return 0;
    }
    for (int i = map->numsprites - 1; i >= 0; i--)
    {
        const dn_sprite_t *source = &map->sprites[i];
        if (!IsTransport(source) || source->owner < 0 || source->owner >= map->numsprites ||
            source->owner == i || source->sectnum != query->sector ||
            map->sectors[source->sectnum].lotag != 0)
        {
            continue;
        }
        const dn_sprite_t *destination = &map->sprites[source->owner];
        memset(result, 0, sizeof(*result));
        result->source = i;
        result->destination = source->owner;
        result->onfloor = source->yvel != 0;
        if (result->onfloor)
        {
            if (!query->grounded || query->jumping || query->jetpack)
            {
                continue;
            }
            VectorSet(result->point, destination->x / 16.f, -destination->y / 16.f,
                      -destination->z / 256.f + 38 - query->eyeheight);
            result->yaw = -destination->ang * 360.f / 2048;
            result->hold = destination->owner == source->owner ? 0 : 13;
        }
        else
        {
            if (!CrossedAirTransport(map, query, source) || (query->jetpack && !query->upmove))
            {
                continue;
            }
            result->point[0] = query->point[0] + (destination->x - (double)source->x) / 16;
            result->point[1] = query->point[1] - (destination->y - (double)source->y) / 16;
            result->point[2] =
                -destination->z / 256.f - query->eyeheight +
                (query->jetpack && (query->upmove > 0 || query->jetpack < 11) ? 24 : -24);
        }
        return 1;
    }
    return 0;
}
