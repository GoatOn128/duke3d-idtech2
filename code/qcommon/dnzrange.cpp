/* Build ENGINE.cpp getzrange sector traversal, using native XY/Z units.
   Sprite surfaces remain supplied through the game's existing trace service. */
#include "../game/q_shared.h"
#include "dnmap.h"

int DN_SectorZRange(const dn_map_t *map, const float *point, int sector, int distance, float *floor,
                    float *ceiling)
{
    if (sector < 0 || sector >= map->numsectors || distance < 0 || distance > 16384)
    {
        return 0;
    }
    const int padding = 1024;
    int64_t x = (int32_t)lroundf(point[0] * 16), y = (int32_t)lroundf(-point[1] * 16);
    int64_t z = (int32_t)lroundf(-point[2] * 256);
    int64_t radius = distance + padding + 1;
    int64_t xmin = x - radius, xmax = x + radius, ymin = y - radius, ymax = y + radius;
    int queue[DN_MAX_SECTORS], head = 0, tail = 1;
    uint8_t visited[DN_MAX_SECTORS] = {};
    queue[0] = sector;
    visited[sector] = 1;
    *floor = DN_PlaneZ(map, sector, 0, point[0], point[1]);
    *ceiling = DN_PlaneZ(map, sector, 1, point[0], point[1]);
    while (head < tail)
    {
        const dn_sector_t *current = &map->sectors[queue[head++]];
        for (int index = current->wallptr; index < current->wallptr + current->wallnum; index++)
        {
            const dn_wall_t *wall = &map->walls[index];
            int next = wall->nextsector;
            if (next < 0 || wall->cstat & 1)
            {
                continue;
            }
            const dn_wall_t *end = &map->walls[wall->point2];
            int64_t x1 = wall->x, x2 = end->x, y1 = wall->y, y2 = end->y;
            if ((x1 < xmin && x2 < xmin) || (x1 > xmax && x2 > xmax) || (y1 < ymin && y2 < ymin) ||
                (y1 > ymax && y2 > ymax))
            {
                continue;
            }
            int64_t dx = x2 - x1, dy = y2 - y1;
            if (dx * (y - y1) < (x - x1) * dy)
            {
                continue;
            }
            int64_t dax = dx * ((dx > 0 ? ymin : ymax) - y1);
            int64_t day = dy * ((dy > 0 ? xmax : xmin) - x1);
            if (dax >= day)
            {
                continue;
            }
            const dn_sector_t *neighbor = &map->sectors[next];
            if ((!(neighbor->ceilingstat & 1) && z <= (int64_t)neighbor->ceilingz + 768) ||
                (!(neighbor->floorstat & 1) && z >= (int64_t)neighbor->floorz - 768))
            {
                continue;
            }
            if (!visited[next])
            {
                visited[next] = 1;
                queue[tail++] = next;
            }
            if ((x1 < xmin + padding && x2 < xmin + padding) ||
                (x1 > xmax - padding && x2 > xmax - padding) ||
                (y1 < ymin + padding && y2 < ymin + padding) ||
                (y1 > ymax - padding && y2 > ymax - padding))
            {
                continue;
            }
            dax += dx > 0 ? dx * padding : -dx * padding;
            day += dy > 0 ? -dy * padding : dy * padding;
            if (dax >= day)
            {
                continue;
            }
            *floor = fmaxf(*floor, DN_PlaneZ(map, next, 0, point[0], point[1]));
            *ceiling = fminf(*ceiling, DN_PlaneZ(map, next, 1, point[0], point[1]));
        }
    }
    return 1;
}
