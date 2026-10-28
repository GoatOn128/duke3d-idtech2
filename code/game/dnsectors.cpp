/* Authoritative sector motion and replicated wall/plane state. */
#include "dnlocal.h"
#include "dncon.h"
#include "../qcommon/dnmath.h"

typedef struct
{
    int32_t closedfloor, closedceiling, openfloor, openceiling, active, direction;
    int32_t kind, effector, ticks, parallax, skypic, skyshade;
    int32_t bridgewalls[2], bridgeposition;
    float progress;
} sector_motion_t;

static sector_motion_t *motions;
static edict_t **sector_markers;
static dn_wall_t *base_walls;
static int *previous_walls;
static edict_t **wall_markers;
static int *motion_order;

static int CompareEffectors(const void *left, const void *right)
{
    int a = *(const int *)left, b = *(const int *)right;
    return motions[b].effector - motions[a].effector;
}

/* Build nextsectorneighborz chooses the closest plane strictly on the requested
   side. Equal-height neighbors must not keep a platform at its current stop. */
static int NeighborPlane(int index, int32_t z, int ceiling, int direction)
{
    const dn_sector_t *sector = &dn_world.sectors[index];
    int best = -1;
    int32_t nearest = direction > 0 ? INT32_MAX : INT32_MIN;
    for (int i = sector->wallptr; i < sector->wallptr + sector->wallnum; i++)
    {
        int next = dn_world.walls[i].nextsector;
        if (next < 0)
        {
            continue;
        }
        int32_t value = ceiling ? dn_world.sectors[next].ceilingz : dn_world.sectors[next].floorz;
        if (direction > 0 ? value > z && value < nearest : value < z && value > nearest)
        {
            best = next;
            nearest = value;
        }
    }
    return best;
}

static void ActivatePlanes(int index)
{
    dn_sector_t *sector = &dn_world.sectors[index];
    sector_motion_t *motion = &motions[index];
    int tag = sector->lotag & 255, next;
    int32_t floor = sector->floorz, ceiling = sector->ceilingz;
    switch (tag)
    {
    case 16:
    case 17:
    case 18:
    case 19:
        if (motion->active)
        {
            return;
        }
        next = NeighborPlane(index, floor, 0, tag < 18 ? 1 : -1);
        if (next < 0)
        {
            next = NeighborPlane(index, floor, 0, tag < 18 ? -1 : 1);
        }
        if (next < 0)
        {
            return;
        }
        floor = dn_world.sectors[next].floorz;
        if (tag >= 18)
        {
            ceiling = floor + sector->ceilingz - sector->floorz;
        }
        break;
    case 20:
        if (sector->lotag & 32768)
        {
            ceiling = floor;
            for (int i = 0; i < dn_world.numsprites; i++)
            {
                const dn_sprite_t *sprite = &dn_world.sprites[i];
                if (sprite->sectnum == index && sprite->picnum == 1 && sprite->lotag == 9)
                {
                    ceiling = sprite->z;
                    break;
                }
            }
        }
        else
        {
            next = NeighborPlane(index, ceiling, 1, -1);
            if (next < 0)
            {
                return;
            }
            ceiling = dn_world.sectors[next].ceilingz;
        }
        sector->lotag ^= 32768;
        break;
    case 21:
        if (motion->active ? motion->openfloor == ceiling : floor == ceiling)
        {
            next = NeighborPlane(index, ceiling, 0, 1);
            if (next < 0)
            {
                return;
            }
            floor = dn_world.sectors[next].floorz;
        }
        else
        {
            floor = ceiling;
        }
        sector->lotag ^= 32768;
        break;
    case 22:
        if (sector->lotag & 32768)
        {
            floor = ceiling = (int32_t)(((int64_t)floor + ceiling) >> 1);
        }
        else
        {
            next = NeighborPlane(index, floor, 0, 1);
            int upper = NeighborPlane(index, ceiling, 1, -1);
            if (next < 0 || upper < 0)
            {
                return;
            }
            floor = dn_world.sectors[next].floorz;
            ceiling = dn_world.sectors[upper].ceilingz;
        }
        sector->lotag ^= 32768;
        break;
    default:
        return;
    }
    motion->closedfloor = motion->openfloor = floor;
    motion->closedceiling = motion->openceiling = ceiling;
    motion->active = 1;
}

static void InitDemolition(int index, int sky)
{
    dn_sprite_t *effector = &dn_world.sprites[index];
    dn_sector_t *sector = &dn_world.sectors[effector->sectnum];
    sector_motion_t *motion = &motions[effector->sectnum];
    motion->kind = 13;
    motion->effector = index;
    motion->skypic = sky >= 0 ? dn_world.sectors[sky].ceilingpicnum : -1;
    motion->skyshade = sky >= 0 ? dn_world.sectors[sky].ceilingshade : 0;
    effector->yvel = sector->extra;
    effector->owner = abs(motion->openceiling - effector->z) < abs(motion->openfloor - effector->z);
    if (effector->ang == 512)
    {
        if (effector->owner)
        {
            sector->ceilingz = effector->z;
        }
        else
        {
            sector->floorz = effector->z;
        }
    }
    else
    {
        sector->floorz = sector->ceilingz = effector->z;
    }
    if (sector->ceilingstat & 1)
    {
        sector->ceilingstat ^= 1;
        motion->parallax = 1;
        if (!effector->owner && effector->ang == 512)
        {
            sector->ceilingstat ^= 1;
            motion->parallax = 0;
        }
        sector->ceilingshade = sector->floorshade;
        if (effector->ang == 512)
        {
            for (int i = sector->wallptr; i < sector->wallptr + sector->wallnum; i++)
            {
                int neighbor = dn_world.walls[i].nextsector;
                if (neighbor >= 0 && !(dn_world.sectors[neighbor].ceilingstat & 1))
                {
                    sector->ceilingpicnum = dn_world.sectors[neighbor].ceilingpicnum;
                    sector->ceilingshade = dn_world.sectors[neighbor].ceilingshade;
                    break;
                }
            }
        }
    }
    DN_PublishSector(effector->sectnum);
    DN_PublishSectorStyle(effector->sectnum);
}

void DN_TriggerDemolition(int tag)
{
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        sector_motion_t *motion = &motions[i];
        if (motion->kind == 13 && !motion->ticks && dn_world.sprites[motion->effector].hitag == tag)
        {
            motion->ticks = motion->active = 1;
        }
    }
}

static int32_t ApproachPlane(int32_t value, int32_t target, int32_t step)
{
    return value + (int32_t)fmax(-step, fmin(step, (double)target - value));
}

static void MoveDemolition(int index)
{
    sector_motion_t *motion = &motions[index];
    dn_sector_t *sector = &dn_world.sectors[index];
    dn_sprite_t *effector = &dn_world.sprites[motion->effector];
    int step = (effector->yvel * 32) | 1;
    if (step < 1)
    {
        step = 1;
    }
    if (effector->ang == 512)
    {
        if (effector->owner)
        {
            sector->ceilingz = ApproachPlane(sector->ceilingz, motion->openceiling, step);
        }
        else
        {
            sector->floorz = ApproachPlane(sector->floorz, motion->openfloor, step);
        }
    }
    else
    {
        sector->floorz = ApproachPlane(sector->floorz, motion->openfloor, step);
        sector->ceilingz = motion->openceiling;
    }
    if (motion->parallax == 1)
    {
        motion->parallax++;
        sector->ceilingstat ^= 1;
        if (effector->ang == 512)
        {
            for (int wall = sector->wallptr; wall < sector->wallptr + sector->wallnum; wall++)
            {
                dn_world.walls[wall].shade = effector->shade;
                DN_PublishWallStyle(wall);
            }
            sector->floorshade = effector->shade;
            if (motion->skypic >= 0)
            {
                sector->ceilingpicnum = motion->skypic;
                sector->ceilingshade = motion->skyshade;
            }
        }
        DN_PublishSectorStyle(index);
    }
    DN_PublishSector(index);
    if (++motion->ticks > 256)
    {
        motion->active = 0;
    }
}

static void PublishWall(int index, int32_t x, int32_t y)
{
    edict_t *entity;
    dn_wall_t *wall = &dn_world.walls[index];
    if (wall->x == x && wall->y == y)
    {
        return;
    }
    wall->x = x;
    wall->y = y;
    entity = wall_markers[index];
    if (!entity)
    {
        entity = wall_markers[index] = DN_NewEntity();
        entity->s.renderfx = RF_DUKE_WALL;
        entity->s.modelindex = dn_sprite_model;
        entity->s.frame = index;
    }
    entity->s.skinnum = x;
    entity->s.effects = (uint32_t)y;
    entity->s.origin[0] = x / 16.f;
    entity->s.origin[1] = -y / 16.f;
    DN_Link(entity);
}

void DN_SetWallPosition(int index, int32_t x, int32_t y)
{
    int current, remaining;
    if (index < 0 || index >= dn_world.numwalls)
    {
        return;
    }
    /* Preserve shared portal vertices, following Build's dragpoint traversal. */
    current = index;
    remaining = dn_world.numwalls;
    do
    {
        PublishWall(current, x, y);
        int opposite = dn_world.walls[current].nextwall;
        if (opposite < 0)
        {
            current = index;
            do
            {
                opposite = dn_world.walls[previous_walls[current]].nextwall;
                if (opposite < 0)
                {
                    return;
                }
                current = opposite;
                PublishWall(current, x, y);
            } while (current != index && --remaining > 0);
            return;
        }
        current = dn_world.walls[opposite].point2;
    } while (current != index && --remaining > 0);
}

void DN_PublishSector(int sector)
{
    dn_sector_t *s = &dn_world.sectors[sector];
    edict_t *entity = sector_markers[sector];
    if (!entity)
    {
        entity = sector_markers[sector] = DN_NewEntity();
        entity->s.renderfx = RF_DUKE_SECTOR;
        entity->s.modelindex = dn_sprite_model;
        entity->s.frame = sector;
    }
    entity->s.skinnum = s->floorz;
    entity->s.effects = (uint32_t)s->ceilingz;
    entity->s.origin[0] = -s->floorz / 256.f;
    entity->s.origin[2] = -s->ceilingz / 256.f;
    entity->s.angles[0] = (s->floorshade + 128) * 360.f / 256;
    entity->s.angles[1] = (s->ceilingshade + 128) * 360.f / 256;
    entity->s.angles[2] = s->visibility * 360.f / 256;
    DN_Link(entity);
}

static void InitBridge(int index)
{
    dn_sprite_t *effector = &dn_world.sprites[index];
    const dn_sector_t *sector = &dn_world.sectors[effector->sectnum];
    sector_motion_t *motion = &motions[effector->sectnum];
    motion->kind = 20;
    motion->effector = index;
    motion->bridgewalls[0] = motion->bridgewalls[1] = -1;
    /* GAME.cpp selects the two nearest vertices with Build's distance estimate. */
    for (int point = 0; point < 2; point++)
    {
        int64_t nearest = INT64_MAX;
        for (int wall = sector->wallptr; wall < sector->wallptr + sector->wallnum; wall++)
        {
            if (point && wall == motion->bridgewalls[0])
            {
                continue;
            }
            int64_t dx = llabs((int64_t)effector->x - dn_world.walls[wall].x);
            int64_t dy = llabs((int64_t)effector->y - dn_world.walls[wall].y);
            if (dx < dy)
            {
                int64_t swap = dx;
                dx = dy;
                dy = swap;
            }
            dy += dy >> 1;
            int64_t distance = dx - (dx >> 5) - (dx >> 7) + (dy >> 2) + (dy >> 6);
            if (distance < nearest)
            {
                nearest = distance;
                motion->bridgewalls[point] = wall;
            }
        }
    }
}

static void MoveBridge(int index)
{
    sector_motion_t *motion = &motions[index];
    dn_sector_t *sector = &dn_world.sectors[index];
    dn_sprite_t *effector = &dn_world.sprites[motion->effector];
    effector->xvel = motion->direction ? 8 : -8;
    int dx = (effector->xvel * DN_Sin(effector->ang + 512)) >> 14;
    int dy = (effector->xvel * DN_Sin(effector->ang)) >> 14;
    motion->bridgeposition += effector->xvel;
    /* Native SE20 consumes the endpoint tick without moving its vertices. */
    if (motion->bridgeposition <= 0 || (motion->bridgeposition >> 6) >= (effector->yvel >> 6))
    {
        motion->active = 0;
        return;
    }
    effector->x += dx;
    effector->y += dy;
    for (int entity = 1; entity < globals.num_edicts; entity++)
    {
        edict_t *rider = &dn_edicts[entity];
        if (!rider->inuse || rider->sector != index || (rider->s.renderfx & RF_DUKE_WORLD) ||
            (rider->client ? !(rider->client->ps.pmove.pm_flags & PMF_ON_GROUND)
                           : rider->zspeed != 0))
        {
            continue;
        }
        rider->s.origin[0] += dx / 16.f;
        rider->s.origin[1] -= dy / 16.f;
        if (rider->client)
        {
            rider->client->ps.pmove.origin[0] = lroundf(rider->s.origin[0] * 8);
            rider->client->ps.pmove.origin[1] = lroundf(rider->s.origin[1] * 8);
            rider->client->ps.prediction.motion_x += dx;
            rider->client->ps.prediction.motion_y += dy;
        }
        DN_Link(rider);
    }
    for (int point = 0; point < 2; point++)
    {
        int wall = motion->bridgewalls[point];
        if (wall >= 0)
        {
            DN_SetWallPosition(wall, dn_world.walls[wall].x + dx, dn_world.walls[wall].y + dy);
        }
    }
    sector->floorxpanning -= dx >> 3;
    sector->floorypanning -= dy >> 3;
    sector->ceilingxpanning -= dx >> 3;
    sector->ceilingypanning -= dy >> 3;
    DN_PublishSectorStyle(index);
}

void DN_InitSectors(void)
{
    int sector, i, sky = -1;
    DN_InitStyles();
    DN_InitLights();
    free(motions);
    free(sector_markers);
    free(base_walls);
    free(previous_walls);
    free(wall_markers);
    free(motion_order);
    motions = (sector_motion_t *)(calloc(dn_world.numsectors, sizeof(*motions)));
    sector_markers = (edict_t **)(calloc(dn_world.numsectors, sizeof(*sector_markers)));
    base_walls = (dn_wall_t *)(malloc(dn_world.numwalls * sizeof(*base_walls)));
    previous_walls = (int *)(calloc(dn_world.numwalls, sizeof(*previous_walls)));
    wall_markers = (edict_t **)(calloc(dn_world.numwalls, sizeof(*wall_markers)));
    motion_order = (int *)(malloc(dn_world.numsectors * sizeof(*motion_order)));
    if (!motions || !sector_markers || !base_walls || !previous_walls || !wall_markers ||
        !motion_order)
    {
        gi.error("Cannot allocate Duke moving-sector state");
    }
    memcpy(base_walls, dn_world.walls, dn_world.numwalls * sizeof(*base_walls));
    for (i = 0; i < dn_world.numwalls; i++)
    {
        previous_walls[dn_world.walls[i].point2] = i;
    }
    /* PREMAP sets a default speed and consumes editor GPSPEED markers. */
    for (i = 0; i < dn_world.numsectors; i++)
    {
        if (sky < 0 && (dn_world.sectors[i].ceilingstat & 1))
        {
            sky = i;
        }
        dn_world.sectors[i].extra = 256;
    }
    for (i = 0; i < dn_world.numsprites; i++)
    {
        const dn_sprite_t *sprite = &dn_world.sprites[i];
        if (sprite->picnum == DN_ConSymbol("GPSPEED", 10))
        {
            dn_world.sectors[sprite->sectnum].extra = sprite->lotag;
        }
    }
    for (sector = 0; sector < dn_world.numsectors; sector++)
    {
        dn_sector_t *s = &dn_world.sectors[sector];
        sector_motion_t *m = &motions[sector];
        m->effector = -1;
        m->closedfloor = m->openfloor = s->floorz;
        m->closedceiling = m->openceiling = s->ceilingz;
        if ((s->lotag & 255) != 20 && (s->lotag & 255) != 21 && (s->lotag & 255) != 22 &&
            (s->lotag & 255) != 18 && (s->lotag & 255) != 19)
        {
            continue;
        }
        for (i = s->wallptr; i < s->wallptr + s->wallnum; i++)
        {
            int next = dn_world.walls[i].nextsector;
            if (next < 0)
            {
                continue;
            }
            dn_sector_t *other = &dn_world.sectors[next];
            if (other->ceilingz < m->openceiling)
            {
                m->openceiling = other->ceilingz;
            }
            if (other->floorz > m->openfloor)
            {
                m->openfloor = other->floorz;
            }
        }
        if ((s->lotag & 255) == 20)
        {
            m->openfloor = s->floorz;
        }
        if ((s->lotag & 255) == 21)
        {
            m->openceiling = s->ceilingz;
        }
        DN_PublishSector(sector);
    }
    for (i = 0; i < dn_world.numsprites; i++)
    {
        dn_sprite_t *sprite = &dn_world.sprites[i];
        sector_motion_t *motion = &motions[sprite->sectnum];
        int tag = dn_world.sectors[sprite->sectnum].lotag & 255;
        if (sprite->picnum == 1 && sprite->lotag == 20 && tag == 27)
        {
            InitBridge(i);
            sprite->yvel = dn_world.sectors[sprite->sectnum].extra;
            continue;
        }
        if (sprite->picnum == 1 && sprite->lotag == 13)
        {
            InitDemolition(i, sky);
            continue;
        }
        if (sprite->picnum == 1 &&
            (sprite->lotag == 11 || sprite->lotag == 15 || (sprite->lotag == 0 && tag == 30)))
        {
            if (sprite->lotag == 0)
            {
                /* GAME.cpp moves an angle-512 SE0 onto its matching SE1 pivot
                   before recording ms() wall offsets. It is not a hinge. */
                int pivot = -1;
                for (int marker = 0; marker < dn_world.numsprites; marker++)
                {
                    const dn_sprite_t *candidate = &dn_world.sprites[marker];
                    if (candidate->picnum == 1 && candidate->lotag == 1 &&
                        candidate->hitag == sprite->hitag)
                    {
                        pivot = marker;
                        break;
                    }
                }
                if (pivot < 0)
                {
                    continue;
                }
                sprite->owner = pivot;
                sprite->clipdist = sprite->pal != 0;
                if (sprite->ang == 512)
                {
                    sprite->x = dn_world.sprites[pivot].x;
                    sprite->y = dn_world.sprites[pivot].y;
                }
            }
            motion->kind = sprite->lotag == 0 ? 30 : sprite->lotag;
            motion->effector = i;
            sprite->yvel = dn_world.sectors[sprite->sectnum].extra;
            DN_PublishSector(sprite->sectnum);
        }
    }
    DN_InitTransports();
    DN_InitDropFloors();
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        motion_order[i] = i;
    }
    /* Native insertion into stat 3 puts later map effectors first. Shared
       vertices must receive paired door transforms in that same order. */
    qsort(motion_order, dn_world.numsectors, sizeof(*motion_order), CompareEffectors);
}

void DN_ActivateSector(int sector)
{
    if (sector < 0 || sector >= dn_world.numsectors)
    {
        return;
    }
    if (DN_ActivateDropFloor(sector))
    {
        return;
    }
    sector_motion_t *selected = &motions[sector];
    if (selected->kind == 20)
    {
        dn_world.sectors[sector].lotag ^= 32768;
        selected->direction = (dn_world.sectors[sector].lotag & 32768) != 0;
        selected->active = 1;
        return;
    }
    if (!selected->kind)
    {
        ActivatePlanes(sector);
        return;
    }
    int group = selected->effector >= 0 ? dn_world.sprites[selected->effector].hitag : 0;
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        sector_motion_t *motion = &motions[i];
        if (motion->kind != 13 &&
            (i == sector || (motion->kind == selected->kind && motion->effector >= 0 &&
                             dn_world.sprites[motion->effector].hitag == group)))
        {
            motion->active = 1;
            motion->direction ^= 1;
        }
    }
}

int DN_SectorMoving(int sector)
{
    return sector >= 0 && sector < dn_world.numsectors &&
           (motions[sector].active || DN_DropFloorMoving(sector));
}

void DN_MoveSectors(void)
{
    DN_TickLights();
    DN_TickDropFloors();
    for (int order = 0; order < dn_world.numsectors; order++)
    {
        int i = motion_order[order];
        if (motions[i].active)
        {
            sector_motion_t *m = &motions[i];
            dn_sector_t *s = &dn_world.sectors[i];
            if (m->kind == 20)
            {
                MoveBridge(i);
                continue;
            }
            if (m->kind == 13)
            {
                MoveDemolition(i);
                continue;
            }
            if (m->kind)
            {
                dn_sprite_t *effector = &dn_world.sprites[m->effector];
                float target = m->direction ? 1 : 0;
                float step = m->kind == 15   ? 1.f / fmaxf(1, effector->yvel >> 3)
                             : m->kind == 11 ? fmaxf(1, effector->yvel >> 3) / 256.f
                                             : 1.f / 64;
                float previous = m->progress;
                m->progress += fmaxf(-step, fminf(step, target - m->progress));
                float pivot_x = effector->x / 16.f, pivot_y = -effector->y / 16.f;
                for (int wall = s->wallptr; wall < s->wallptr + s->wallnum; wall++)
                {
                    if (m->kind == 11 || m->kind == 30)
                    {
                        /* Operate reverses T4 before ms rotates native XY.
                           World Y has the opposite sign to Build's Y. */
                        int native_angle =
                            m->kind == 11
                                ? (int)lroundf(m->progress * 512) * (effector->ang > 1024 ? -1 : 1)
                                : (int)lroundf(m->progress * 64) * (s->extra >> 5) *
                                      (effector->clipdist ? 1 : -1);
                        int64_t x = (int64_t)base_walls[wall].x - effector->x;
                        int64_t y = (int64_t)base_walls[wall].y - effector->y;
                        int cosine = DN_Sin(native_angle + 512), sine = DN_Sin(native_angle);
                        DN_SetWallPosition(wall,
                                           effector->x + (int32_t)((x * cosine - y * sine) >> 14),
                                           effector->y + (int32_t)((y * cosine + x * sine) >> 14));
                        continue;
                    }
                    if (m->kind == 15)
                    {
                        int ticks = (int)lroundf(m->progress * DN_Max(1, effector->yvel >> 3));
                        int dx = (16 * DN_Sin(effector->ang + 1024 + 512)) >> 14;
                        int dy = (16 * DN_Sin(effector->ang + 1024)) >> 14;
                        DN_SetWallPosition(wall, base_walls[wall].x + dx * ticks,
                                           base_walls[wall].y + dy * ticks);
                        continue;
                    }
                }
                if (m->kind == 30 && (s->floorstat & 64))
                {
                    float delta = (m->progress - previous) * 64 * (s->extra >> 5) *
                                  (effector->clipdist ? -1 : 1) * (float)M_PI / 1024;
                    for (int entity = 1; entity < globals.num_edicts; entity++)
                    {
                        edict_t *rider = &dn_edicts[entity];
                        if (!rider->inuse || rider->sector != i ||
                            rider->s.renderfx & RF_DUKE_WORLD)
                        {
                            continue;
                        }
                        float x = rider->s.origin[0] - pivot_x, y = rider->s.origin[1] - pivot_y;
                        rider->s.origin[0] = pivot_x + x * cosf(delta) - y * sinf(delta);
                        rider->s.origin[1] = pivot_y + x * sinf(delta) + y * cosf(delta);
                        if (rider->client)
                        {
                            for (int axis = 0; axis < 3; axis++)
                            {
                                rider->client->ps.pmove.origin[axis] =
                                    lroundf(rider->s.origin[axis] * 8);
                            }
                        }
                        DN_Link(rider);
                    }
                }
                if (m->progress == target)
                {
                    m->active = 0;
                }
                continue;
            }
            int old_floor = s->floorz;
            int targetf = m->direction ? m->openfloor : m->closedfloor,
                targetc = m->direction ? m->openceiling : m->closedceiling;
            int *targets[2] = {&targetf, &targetc};
            int32_t *values[2] = {&s->floorz, &s->ceilingz};
            for (int j = 0; j < 2; j++)
            {
                int delta = *targets[j] - *values[j];
                int speed = (s->extra > 0 ? s->extra : 256) * DN_TICKS_PER_STEP;
                if (delta > speed)
                {
                    delta = speed;
                }
                if (delta < -speed)
                {
                    delta = -speed;
                }
                *values[j] += delta;
            }
            DN_PublishSector(i);
            float floor_delta = -(s->floorz - old_floor) / 256.f;
            for (int entity = 1; entity < globals.num_edicts && floor_delta; entity++)
            {
                edict_t *rider = &dn_edicts[entity];
                if (!rider->inuse || rider->sector != i || (rider->s.renderfx & RF_DUKE_WORLD))
                {
                    continue;
                }
                float old_height =
                    DN_PlaneZ(&dn_world, i, 0, rider->s.origin[0], rider->s.origin[1]) -
                    floor_delta;
                /* Native doanimations carries all ordinary sector sprites and
                   players within 64 units of the floor, including crouching. */
                if (!rider->client || rider->s.origin[2] + 12 - old_height < 64)
                {
                    rider->s.origin[2] += floor_delta;
                    if (rider->client)
                    {
                        rider->client->ps.pmove.origin[2] = lroundf(rider->s.origin[2] * 8);
                        rider->client->ps.pmove.velocity[2] = 0;
                        rider->client->ps.prediction.motion_z -= s->floorz - old_floor;
                        rider->client->ps.prediction.motion_vz = 0;
                    }
                    DN_Link(rider);
                }
            }
            if (s->floorz == targetf && s->ceilingz == targetc)
            {
                m->active = 0;
            }
        }
    }
}

void DN_FreeSectors(void)
{
    DN_FreeStyles();
    DN_FreeLights();
    DN_FreeDropFloors();
    free(motions);
    free(sector_markers);
    free(base_walls);
    free(previous_walls);
    free(wall_markers);
    free(motion_order);
    motions = NULL;
    sector_markers = NULL;
    base_walls = NULL;
    previous_walls = NULL;
    wall_markers = NULL;
    motion_order = NULL;
}

/* Controller state contains values only. Marker pointers are rebuilt after
   the save's entities have been restored to their stable edict indices. */
void DN_WriteSectorState(FILE *file)
{
    DN_WriteSaveData(file, motions, sizeof(*motions), dn_world.numsectors);
    DN_WriteSaveData(file, base_walls, sizeof(*base_walls), dn_world.numwalls);
    DN_WriteLightState(file);
    DN_WriteDropState(file);
}

void DN_ReadSectorState(FILE *file)
{
    DN_ReadSaveData(file, motions, sizeof(*motions), dn_world.numsectors);
    DN_ReadSaveData(file, base_walls, sizeof(*base_walls), dn_world.numwalls);
    DN_ReadLightState(file);
    DN_ReadDropState(file);
    for (int i = 0; i < dn_world.numsectors; i++)
    {
        const sector_motion_t *motion = &motions[i];
        if ((motion->active != 0 && motion->active != 1) ||
            (motion->direction != 0 && motion->direction != 1) || !isfinite(motion->progress) ||
            motion->progress < 0 || motion->progress > 1 || motion->effector < -1 ||
            motion->effector >= dn_world.numsprites || (motion->kind && motion->effector < 0))
        {
            fclose(file);
            gi.error("Invalid saved sector controller %d", i);
        }
        if (motion->kind == 20 &&
            (motion->bridgewalls[0] < 0 || motion->bridgewalls[0] >= dn_world.numwalls ||
             motion->bridgewalls[1] < 0 || motion->bridgewalls[1] >= dn_world.numwalls ||
             motion->bridgewalls[0] == motion->bridgewalls[1]))
        {
            fclose(file);
            gi.error("Invalid saved bridge controller %d", i);
        }
    }
}

void DN_RestoreSectorMarkers(void)
{
    DN_InitStyles();
    memset(sector_markers, 0, dn_world.numsectors * sizeof(*sector_markers));
    memset(wall_markers, 0, dn_world.numwalls * sizeof(*wall_markers));
    for (int i = 1; i < globals.num_edicts; i++)
    {
        edict_t *entity = &dn_edicts[i];
        int index = entity->s.frame;
        if (!entity->inuse || !(entity->s.renderfx & RF_DUKE_WORLD))
        {
            continue;
        }
        if (entity->s.renderfx == RF_DUKE_SECTOR && index >= 0 && index < dn_world.numsectors &&
            !sector_markers[index])
        {
            sector_markers[index] = entity;
        }
        else if (entity->s.renderfx == RF_DUKE_WALL && index >= 0 && index < dn_world.numwalls &&
                 !wall_markers[index])
        {
            wall_markers[index] = entity;
        }
        else if ((entity->s.renderfx & RF_DUKE_DISCRETE) && DN_RestoreStyle(entity))
        {
            continue;
        }
        else
        {
            gi.error("Invalid saved world marker %d", i);
        }
    }
    for (int i = 0; i < dn_world.numwalls; i++)
    {
        previous_walls[dn_world.walls[i].point2] = i;
    }
}
