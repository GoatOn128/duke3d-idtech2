#include "../game/q_shared.h"
#include "dnmap.h"
#include "dntransport.h"
#include <limits.h>

typedef char dn_sector_size[(sizeof(dn_sector_t) == 40) ? 1 : -1];
typedef char dn_wall_size[(sizeof(dn_wall_t) == 32) ? 1 : -1];
typedef char dn_sprite_size[(sizeof(dn_sprite_t) == 44) ? 1 : -1];

int DN_IsFloorSprite(const entity_state_t *state)
{
    return (state->renderfx & RF_DUKE_SPRITE) && (((uint32_t)state->skinnum >> 16) & 48) == 32;
}

static void DN_FloorSpriteAxes(const entity_state_t *state, vec3_t right, vec3_t up)
{
    float yaw = state->angles[YAW] * (float)M_PI / 180;
    VectorSet(right, -sinf(yaw), cosf(yaw), 0);
    VectorSet(up, cosf(yaw), sinf(yaw), 0);
}

void DN_FloorSpriteBounds(const entity_state_t *state, float *mins, float *maxs)
{
    vec3_t right, up;
    DN_FloorSpriteAxes(state, right, up);
    for (int axis = 0; axis < 3; axis++)
    {
        float r1 = right[axis] * state->mins[0], r2 = right[axis] * state->maxs[0];
        float u1 = up[axis] * state->mins[1], u2 = up[axis] * state->maxs[1];
        mins[axis] = state->origin[axis] + fminf(r1, r2) + fminf(u1, u2);
        maxs[axis] = state->origin[axis] + fmaxf(r1, r2) + fmaxf(u1, u2);
    }
}

/* Sweep a world-axis box against the exact rotated, zero-thickness sprite.
   The five separating axes include world X/Y so diagonal corners stay empty. */
trace_t DN_TraceFloorSprite(const entity_state_t *state, const float *start, const float *end,
                            const float *mins, const float *maxs)
{
    trace_t trace = {};
    vec3_t axes[5] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, center, extent, offset, velocity;
    float enter = -1e20f, leave = 1e20f, speed = 0;
    int start_inside = 1, end_inside = 1;
    unsigned cstat = (uint32_t)state->skinnum >> 16;
    trace.fraction = 1;
    VectorCopy(end, trace.endpos);
    if ((cstat & 64) && ((start[2] < state->origin[2]) == !(cstat & 8)))
    {
        return trace;
    }
    DN_FloorSpriteAxes(state, axes[3], axes[4]);
    VectorCopy(state->origin, center);
    VectorMA(center, (state->mins[0] + state->maxs[0]) * .5f, axes[3], center);
    VectorMA(center, (state->mins[1] + state->maxs[1]) * .5f, axes[4], center);
    for (int axis = 0; axis < 3; axis++)
    {
        offset[axis] = start[axis] + (mins[axis] + maxs[axis]) * .5f - center[axis];
        extent[axis] = (maxs[axis] - mins[axis]) * .5f;
        velocity[axis] = end[axis] - start[axis];
    }
    for (int axis = 0; axis < 5; axis++)
    {
        const float *normal = axes[axis];
        float radius =
            fabsf(DotProduct(normal, axes[3])) * (state->maxs[0] - state->mins[0]) * .5f +
            fabsf(DotProduct(normal, axes[4])) * (state->maxs[1] - state->mins[1]) * .5f;
        for (int component = 0; component < 3; component++)
        {
            radius += fabsf(normal[component]) * extent[component];
        }
        float pos = DotProduct(offset, normal), delta = DotProduct(velocity, normal);
        start_inside &= fabsf(pos) < radius - .001f;
        end_inside &= fabsf(pos + delta) < radius - .001f;
        if (fabsf(delta) < 1e-8f)
        {
            if (fabsf(pos) >= radius - .001f)
            {
                return trace; /* Permit sliding along a touching surface. */
            }
            continue;
        }
        float first = (-radius - pos) / delta, last = (radius - pos) / delta;
        if (first > last)
        {
            float swap = first;
            first = last;
            last = swap;
        }
        if (first > enter)
        {
            enter = first;
            VectorScale(normal, delta > 0 ? -1 : 1, trace.plane.normal);
            speed = fabsf(delta);
        }
        leave = fminf(leave, last);
        if (enter > leave)
        {
            return trace;
        }
    }
    if (leave < 0 || enter > 1 || (!start_inside && enter < -.001f))
    {
        return trace;
    }
    trace.startsolid = start_inside;
    trace.allsolid = start_inside && end_inside;
    trace.fraction = start_inside ? 0 : fmaxf(0, enter - .03125f / fmaxf(speed, .03125f));
    trace.contents = CONTENTS_SOLID;
    trace.plane.type = fabsf(trace.plane.normal[2]) > .5f ? 2 : 3;
    for (int axis = 0; axis < 3; axis++)
    {
        trace.endpos[axis] = start[axis] + trace.fraction * velocity[axis];
        if (trace.plane.normal[axis] < 0)
        {
            trace.plane.signbits |= 1 << axis;
        }
    }
    trace.plane.dist = DotProduct(trace.endpos, trace.plane.normal);
    return trace;
}

static int DN_Fail(dn_map_t *m, char *err, size_t n, const char *message)
{
    if (err && n)
    {
        snprintf(err, n, "%s", message);
    }
    DN_FreeMap(m);
    return 0;
}

void DN_FreeMap(dn_map_t *m)
{
    free(m->sectors);
    free(m->walls);
    free(m->sprites);
    memset(m, 0, sizeof(*m));
}

int DN_LoadMap(dn_map_t *m, const void *buffer, size_t length, char *err, size_t n)
{
    const unsigned char *p = static_cast<const unsigned char *>(buffer);
    const unsigned char *limit = p + length;
    int32_t version, xyz[3];
    int16_t angle, sector, count;
    int i, j;
    memset(m, 0, sizeof(*m));
    if (length < 22)
    {
        return DN_Fail(m, err, n, "Truncated Build header");
    }
    memcpy(&version, p, 4);
    p += 4;
    if (version != 7)
    {
        return DN_Fail(m, err, n, "Only Duke Build version 7 maps are supported");
    }
    memcpy(xyz, p, 12);
    p += 12;
    memcpy(&angle, p, 2);
    p += 2;
    memcpy(&sector, p, 2);
    p += 2;
    m->start[0] = xyz[0] / 16.f;
    m->start[1] = -xyz[1] / 16.f;
    m->start[2] = -xyz[2] / 256.f;
    m->angle = -angle * (360.f / 2048.f);
    m->startsector = sector;
#define READ_ARRAY(field, number, type, maxcount)                                                  \
    if ((size_t)(limit - p) < 2)                                                                   \
        return DN_Fail(m, err, n, "Missing Build object count");                                   \
    memcpy(&count, p, 2);                                                                          \
    p += 2;                                                                                        \
    if (count < 0 || count > maxcount || (size_t)(limit - p) < (size_t)count * sizeof(type))       \
        return DN_Fail(m, err, n, "Invalid Build object array");                                   \
    m->number = count;                                                                             \
    m->field = (type *)calloc(count ? count : 1, sizeof(type));                                            \
    if (!m->field)                                                                                 \
        return DN_Fail(m, err, n, "Out of memory loading Build map");                              \
    memcpy(m->field, p, (size_t)count * sizeof(type));                                             \
    p += (size_t)count * sizeof(type)
    READ_ARRAY(sectors, numsectors, dn_sector_t, DN_MAX_SECTORS);
    READ_ARRAY(walls, numwalls, dn_wall_t, DN_MAX_WALLS);
    READ_ARRAY(sprites, numsprites, dn_sprite_t, DN_MAX_SPRITES);
#undef READ_ARRAY
    if (!m->numsectors || sector < 0 || sector >= m->numsectors)
    {
        return DN_Fail(m, err, n, "Invalid starting sector");
    }
    for (i = 0; i < 3; i++)
    {
        m->mins[i] = 1e20f;
        m->maxs[i] = -1e20f;
    }
    for (i = 0; i < m->numsectors; i++)
    {
        dn_sector_t *s = &m->sectors[i];
        /* Shipped maps contain two-wall sectors used by moving geometry. */
        if (s->wallptr < 0 || s->wallnum < 2 || s->wallptr + s->wallnum > m->numwalls)
        {
            return DN_Fail(m, err, n, "Invalid sector wall range");
        }
        for (j = s->wallptr; j < s->wallptr + s->wallnum; j++)
        {
            dn_wall_t *w = &m->walls[j];
            float v[3];
            if (w->point2 < s->wallptr || w->point2 >= s->wallptr + s->wallnum ||
                w->nextsector < -1 || w->nextsector >= m->numsectors || w->nextwall < -1 ||
                w->nextwall >= m->numwalls)
            {
                return DN_Fail(m, err, n, "Invalid wall link");
            }
            /* Build permits asymmetric portals (several original levels use
               them). Bounds are checked above; reciprocity is not required. */
            v[0] = w->x / 16.f;
            v[1] = -w->y / 16.f;
            for (int k = 0; k < 2; k++)
            {
                if (v[k] < m->mins[k])
                {
                    m->mins[k] = v[k];
                }
                if (v[k] > m->maxs[k])
                {
                    m->maxs[k] = v[k];
                }
            }
            v[2] = DN_PlaneZ(m, i, 0, v[0], v[1]);
            if (v[2] < m->mins[2])
            {
                m->mins[2] = v[2];
            }
            v[2] = DN_PlaneZ(m, i, 1, v[0], v[1]);
            if (v[2] > m->maxs[2])
            {
                m->maxs[2] = v[2];
            }
        }
    }
    for (i = 0; i < m->numsprites; i++)
    {
        if (m->sprites[i].sectnum < 0 || m->sprites[i].sectnum >= m->numsectors ||
            m->sprites[i].picnum < 0 || m->sprites[i].picnum >= DN_MAX_TILES)
        {
            return DN_Fail(m, err, n, "Invalid sprite sector or tile");
        }
    }
    DN_InitTransportLinks(m);
    /* Build's loadboard updates the saved sector hint from XY. A few shipped
       maps contain an outdated editor sector at their starting coordinates. */
    if (!DN_Inside(m, m->startsector, m->start[0], m->start[1]))
    {
        for (i = 0; i < m->numsectors; i++)
        {
            if (DN_Inside(m, i, m->start[0], m->start[1]))
            {
                m->startsector = i;
                break;
            }
        }
    }
    return 1;
}

int DN_Inside(const dn_map_t *m, int sector, float x, float y)
{
    const dn_sector_t *s;
    int inside = 0, i;
    if (sector < 0 || sector >= m->numsectors)
    {
        return 0;
    }
    s = &m->sectors[sector];
    x *= 16.f;
    y *= -16.f;
    for (i = s->wallptr; i < s->wallptr + s->wallnum; i++)
    {
        const dn_wall_t *a = &m->walls[i], *b = &m->walls[a->point2];
        if ((a->y > y) != (b->y > y) &&
            x < a->x + (double)(b->x - a->x) * (y - a->y) / (b->y - a->y))
        {
            inside ^= 1;
        }
    }
    return inside;
}

float DN_PlaneZ(const dn_map_t *m, int sector, int ceiling, float x, float y)
{
    const dn_sector_t *s = &m->sectors[sector];
    float z = -(ceiling ? s->ceilingz : s->floorz) / 256.f;
    unsigned stat = ceiling ? s->ceilingstat : s->floorstat;
    if (stat & 2)
    {
        const dn_wall_t *a = &m->walls[s->wallptr], *b = &m->walls[a->point2];
        double dx = (double)b->x - a->x, dy = (double)b->y - a->y, len = sqrt(dx * dx + dy * dy);
        int heinum = ceiling ? s->ceilingheinum : s->floorheinum;
        if (len > 0)
        {
            z -= (float)(heinum * (dx * (-y * 16 - a->y) - dy * (x * 16 - a->x)) / (65536.0 * len));
        }
    }
    return z;
}

void DN_PlaneNormal(const dn_map_t *m, int sector, int ceiling, float *normal)
{
    float z = DN_PlaneZ(m, sector, ceiling, 0, 0);
    normal[0] = z - DN_PlaneZ(m, sector, ceiling, 1, 0);
    normal[1] = z - DN_PlaneZ(m, sector, ceiling, 0, 1);
    normal[2] = 1;
    VectorNormalize(normal);
    if (ceiling)
    {
        VectorInverse(normal);
    }
}

int DN_FindSectorXY(const dn_map_t *map, const float *point, int hint)
{
    if (hint >= 0 && hint < map->numsectors)
    {
        if (DN_Inside(map, hint, point[0], point[1]))
        {
            return hint;
        }
        const dn_sector_t *sector = &map->sectors[hint];
        for (int i = sector->wallptr; i < sector->wallptr + sector->wallnum; i++)
        {
            int next = map->walls[i].nextsector;
            if (next >= 0 && DN_Inside(map, next, point[0], point[1]))
            {
                return next;
            }
        }
    }
    for (int i = 0; i < map->numsectors; i++)
    {
        if (DN_Inside(map, i, point[0], point[1]))
        {
            return i;
        }
    }
    return -1;
}

int DN_FindSector(const dn_map_t *m, const float *p, int hint)
{
    int i;
    if (hint >= 0 && hint < m->numsectors && DN_Inside(m, hint, p[0], p[1]) &&
        p[2] >= DN_PlaneZ(m, hint, 0, p[0], p[1]) - .1f &&
        p[2] <= DN_PlaneZ(m, hint, 1, p[0], p[1]) + .1f)
    {
        return hint;
    }
    for (i = 0; i < m->numsectors; i++)
    {
        if (i != hint && DN_Inside(m, i, p[0], p[1]) &&
            p[2] >= DN_PlaneZ(m, i, 0, p[0], p[1]) - .1f &&
            p[2] <= DN_PlaneZ(m, i, 1, p[0], p[1]) + .1f)
        {
            return i;
        }
    }
    return -1;
}

int DN_PointContents(const dn_map_t *m, const float *p)
{
    int sector = DN_FindSector(m, p, -1);
    if (sector < 0)
    {
        return CONTENTS_SOLID;
    }
    return m->sectors[sector].lotag == 2 ? CONTENTS_WATER : 0;
}

/* Swept AABB against a finite wall segment using separating axes. This
   includes endpoint contacts, so fast bullets and diagonal corners cannot
   tunnel through a wall. Returned normal points toward the moving box. */
static int DN_SweepWall(const float *a, const float *b, const float *mins, const float *maxs,
                        float x1, float y1, float x2, float y2, float *fraction, float *normal,
                        int *start_inside, int *end_inside)
{
    float nx = y2 - y1, ny = x1 - x2, len = sqrtf(nx * nx + ny * ny);
    float axes[3][2] = {{1, 0}, {0, 1}, {0, 0}}, enter = -1e20f, leave = 1e20f, best[2] = {0, 0};
    int k;
    *start_inside = *end_inside = 1;
    if (len < .0001f)
    {
        return 0;
    }
    axes[2][0] = nx / len;
    axes[2][1] = ny / len;
    for (k = 0; k < 3; k++)
    {
        float ax = axes[k][0], ay = axes[k][1];
        float v1 = x1 * ax + y1 * ay, v2 = x2 * ax + y2 * ay;
        float lo =
            fminf(v1, v2) - (ax >= 0 ? maxs[0] : mins[0]) * ax - (ay >= 0 ? maxs[1] : mins[1]) * ay;
        float hi =
            fmaxf(v1, v2) - (ax >= 0 ? mins[0] : maxs[0]) * ax - (ay >= 0 ? mins[1] : maxs[1]) * ay;
        float pos = a[0] * ax + a[1] * ay, vel = (b[0] - a[0]) * ax + (b[1] - a[1]) * ay, t1, t2;
        if (pos <= lo + .001f || pos >= hi - .001f)
        {
            *start_inside = 0;
        }
        if (pos + vel <= lo + .001f || pos + vel >= hi - .001f)
        {
            *end_inside = 0;
        }
        if (fabsf(vel) < 1e-8f)
        {
            /* Tangential contact has no entering collision. In particular,
               an endpoint axis must not stop a box sliding across the join
               between two collinear walls while its side touches both. */
            if (pos <= lo + .001f || pos >= hi - .001f)
            {
                return 0;
            }
            continue;
        }
        t1 = (lo - pos) / vel;
        t2 = (hi - pos) / vel;
        if (t1 > t2)
        {
            float t = t1;
            t1 = t2;
            t2 = t;
        }
        if (t1 > enter)
        {
            enter = t1;
            best[0] = vel > 0 ? -ax : ax;
            best[1] = vel > 0 ? -ay : ay;
        }
        if (t2 < leave)
        {
            leave = t2;
        }
        if (enter > leave)
        {
            return 0;
        }
    }
    if (leave < 0 || enter > 1)
    {
        return 0;
    }
    if (*start_inside)
    {
        *fraction = 0;
        VectorClear(normal);
        return 1;
    }
    if (enter < -.01f)
    {
        return 0; /* already touching: permit motion away */
    }
    *fraction = fmaxf(0, enter);
    normal[0] = best[0];
    normal[1] = best[1];
    normal[2] = 0;
    return 1;
}

static int DN_BoxTouchesSector(const dn_map_t *map, int sector, const float *point,
                               const float *mins, const float *maxs)
{
    if (DN_Inside(map, sector, point[0], point[1]))
    {
        return 1;
    }
    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            if (DN_Inside(map, sector, point[0] + (x ? maxs[0] : mins[0]),
                          point[1] + (y ? maxs[1] : mins[1])))
            {
                return 1;
            }
        }
    }
    const dn_sector_t *area = &map->sectors[sector];
    for (int i = area->wallptr; i < area->wallptr + area->wallnum; i++)
    {
        const dn_wall_t *wall = &map->walls[i], *end = &map->walls[wall->point2];
        float fraction, normal[3];
        int inside, endpoint;
        if (DN_SweepWall(point, point, mins, maxs, wall->x / 16.f, -wall->y / 16.f, end->x / 16.f,
                         -end->y / 16.f, &fraction, normal, &inside, &endpoint) &&
            inside)
        {
            return 1;
        }
    }
    return 0;
}

static csurface_t dn_surface = {"duke", 0, 0};

/* Build pushmove resolves a pose against nearby blocking walls after a sector
   transfer. Q2's eighth-unit snap test alone cannot recover such overlaps. */
int DN_PushMove(const dn_map_t *map, float *point, const float *mins, const float *maxs, int sector,
                int mask)
{
    for (int pass = 0; pass < 32; pass++)
    {
        sector = DN_FindSector(map, point, sector);
        if (sector < 0)
        {
            return 0;
        }
        const dn_sector_t *area = &map->sectors[sector];
        int changed = 0;
        for (int j = 0; j < area->wallnum; j++)
        {
            int i = area->wallptr + ((pass & 1) ? area->wallnum - j - 1 : j);
            const dn_wall_t *wall = &map->walls[i], *end = &map->walls[wall->point2];
            float fraction, normal[3];
            int inside, endpoint;
            if (!DN_SweepWall(point, point, mins, maxs, wall->x / 16.f, -wall->y / 16.f,
                              end->x / 16.f, -end->y / 16.f, &fraction, normal, &inside,
                              &endpoint) ||
                !inside)
            {
                continue;
            }
            int next = wall->nextsector;
            if (next >= 0 &&
                !(wall->cstat & ((mask & (CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP)) ? 1 : 64)) &&
                point[2] + mins[2] >= DN_PlaneZ(map, next, 0, point[0], point[1]) - .05f &&
                point[2] + maxs[2] <= DN_PlaneZ(map, next, 1, point[0], point[1]) + .05f)
            {
                continue;
            }
            VectorSet(normal, -(end->y - (float)wall->y), -(end->x - (float)wall->x), 0);
            if (!VectorNormalize(normal))
            {
                continue;
            }
            float distance =
                (point[0] - wall->x / 16.f) * normal[0] + (point[1] + wall->y / 16.f) * normal[1];
            float support = -(normal[0] >= 0 ? mins[0] : maxs[0]) * normal[0] -
                            (normal[1] >= 0 ? mins[1] : maxs[1]) * normal[1];
            float push = support - distance + .125f;
            if (push > 0)
            {
                point[0] += normal[0] * push;
                point[1] += normal[1] * push;
                changed = 1;
            }
        }
        if (!changed)
        {
            trace_t valid = DN_Trace(map, point, point, mins, maxs, mask);
            return !valid.allsolid;
        }
    }
    return 0;
}

static uint32_t DN_HashInteger(uint32_t hash, int32_t value)
{
    for (int byte = 0; byte < 4; byte++)
    {
        hash = (hash ^ (((uint32_t)value >> (byte * 8)) & 255)) * 16777619u;
    }
    return hash;
}

int DN_WallFacesPoint(const dn_wall_t *wall, const dn_wall_t *end, float x, float y)
{
    double dx = (end->x - (double)wall->x) / 16;
    double dy = -(end->y - (double)wall->y) / 16;
    return dx * (y + wall->y / 16.0) - dy * (x - wall->x / 16.0) <= .01;
}

int DN_CanSee(const dn_map_t *map, const float *start, int startsector, const float *end,
              int endsector)
{
    int queue[DN_MAX_SECTORS], count = 1;
    unsigned char seen[DN_MAX_SECTORS] = {};
    if (startsector < 0 || endsector < 0 || startsector >= map->numsectors ||
        endsector >= map->numsectors)
    {
        return 0;
    }
    if (start[0] == end[0] && start[1] == end[1])
    {
        return startsector == endsector;
    }
    /* ENGINE.cpp cansee traverses crossed portals. Physical blocking flags
       1/64 and sprite collision do not block this visibility query. */
    double x1 = start[0] * 16, y1 = -start[1] * 16;
    double dx = (end[0] - start[0]) * 16, dy = -(end[1] - start[1]) * 16;
    queue[0] = startsector;
    seen[startsector] = 1;
    for (int head = 0; head < count; head++)
    {
        int index = queue[head];
        const dn_sector_t *sector = &map->sectors[index];
        for (int i = sector->wallptr; i < sector->wallptr + sector->wallnum; i++)
        {
            const dn_wall_t *wall = &map->walls[i], *other = &map->walls[wall->point2];
            double x31 = wall->x - x1, y31 = wall->y - y1;
            double x34 = wall->x - (double)other->x, y34 = wall->y - (double)other->y;
            double denominator = dy * x34 - dx * y34;
            double along_wall = dy * x31 - dx * y31;
            double along_ray = y31 * x34 - x31 * y34;
            if (denominator <= 0 || along_wall < 0 || along_wall >= denominator || along_ray < 0 ||
                along_ray >= denominator)
            {
                continue;
            }
            int next = wall->nextsector;
            if (next < 0 || (wall->cstat & 32))
            {
                return 0;
            }
            double fraction = along_ray / denominator;
            float x = (float)(start[0] + (end[0] - start[0]) * fraction);
            float y = (float)(start[1] + (end[1] - start[1]) * fraction);
            float z = (float)(start[2] + (end[2] - start[2]) * fraction);
            if (z <= DN_PlaneZ(map, index, 0, x, y) || z >= DN_PlaneZ(map, index, 1, x, y) ||
                z <= DN_PlaneZ(map, next, 0, x, y) || z >= DN_PlaneZ(map, next, 1, x, y))
            {
                return 0;
            }
            if (!seen[next])
            {
                seen[next] = 1;
                queue[count++] = next;
            }
        }
    }
    return seen[endsector];
}

uint32_t DN_WorldChecksum(const dn_map_t *map)
{
    uint32_t hash = 2166136261u;
    hash = DN_HashInteger(hash, map->numsectors);
    hash = DN_HashInteger(hash, map->numwalls);
    for (int i = 0; i < map->numsectors; i++)
    {
        const dn_sector_t *sector = &map->sectors[i];
        hash = DN_HashInteger(hash, sector->floorz);
        hash = DN_HashInteger(hash, sector->ceilingz);
        hash = DN_HashInteger(hash, sector->floorshade);
        hash = DN_HashInteger(hash, sector->ceilingshade);
        hash = DN_HashInteger(hash, sector->visibility);
        hash = DN_HashInteger(hash, sector->ceilingstat);
        hash = DN_HashInteger(hash, sector->floorstat);
        hash = DN_HashInteger(hash, sector->ceilingpicnum);
        hash = DN_HashInteger(hash, sector->floorpicnum);
        hash = DN_HashInteger(hash, sector->ceilingpal);
        hash = DN_HashInteger(hash, sector->floorpal);
        hash = DN_HashInteger(hash, sector->ceilingxpanning);
        hash = DN_HashInteger(hash, sector->ceilingypanning);
        hash = DN_HashInteger(hash, sector->floorxpanning);
        hash = DN_HashInteger(hash, sector->floorypanning);
    }
    for (int i = 0; i < map->numwalls; i++)
    {
        hash = DN_HashInteger(hash, map->walls[i].x);
        hash = DN_HashInteger(hash, map->walls[i].y);
        hash = DN_HashInteger(hash, map->walls[i].picnum);
        hash = DN_HashInteger(hash, map->walls[i].overpicnum);
        hash = DN_HashInteger(hash, map->walls[i].cstat);
        hash = DN_HashInteger(hash, map->walls[i].shade);
        hash = DN_HashInteger(hash, map->walls[i].pal);
    }
    return hash;
}

int DN_ApplyWorldState(dn_map_t *map, const entity_state_t *state)
{
    int index = state->frame;
    if ((state->renderfx & RF_DUKE_SECTOR_STYLE) && index >= 0 && index < map->numsectors)
    {
        dn_sector_t *sector = &map->sectors[index];
        sector->ceilingpicnum = (uint32_t)state->skinnum & 65535;
        sector->floorpicnum = (uint32_t)state->skinnum >> 16;
        sector->ceilingstat = state->effects & 65535;
        sector->floorstat = state->effects >> 16;
        sector->ceilingpal = (int)lroundf(state->angles[0] * 256.f / 360) & 255;
        sector->floorpal = (int)lroundf(state->angles[1] * 256.f / 360) & 255;
        sector->ceilingxpanning = (int)lroundf(state->angles[2] * 256.f / 360) & 255;
        sector->ceilingypanning = (int)lroundf(state->origin[0]) & 255;
        sector->floorxpanning = (int)lroundf(state->origin[1]) & 255;
        sector->floorypanning = (int)lroundf(state->origin[2]) & 255;
        return RF_DUKE_SECTOR_STYLE;
    }
    if ((state->renderfx & RF_DUKE_WALL_STYLE) && index >= 0 && index < map->numwalls)
    {
        dn_wall_t *wall = &map->walls[index];
        wall->picnum = (uint32_t)state->skinnum & 65535;
        wall->cstat = (uint32_t)state->skinnum >> 16;
        wall->overpicnum = state->effects & 65535;
        wall->shade = (state->effects >> 16) & 255;
        wall->pal = state->effects >> 24;
        return RF_DUKE_WALL_STYLE;
    }
    if ((state->renderfx & RF_DUKE_SECTOR) && index >= 0 && index < map->numsectors)
    {
        dn_sector_t *sector = &map->sectors[index];
        sector->floorz = state->skinnum;
        sector->ceilingz = (int32_t)state->effects;
        sector->floorshade = ((int)lroundf(state->angles[0] * 256.f / 360) & 255) - 128;
        sector->ceilingshade = ((int)lroundf(state->angles[1] * 256.f / 360) & 255) - 128;
        sector->visibility = (int)lroundf(state->angles[2] * 256.f / 360) & 255;
        return RF_DUKE_SECTOR;
    }
    if ((state->renderfx & RF_DUKE_WALL) && index >= 0 && index < map->numwalls)
    {
        dn_wall_t *wall = &map->walls[index];
        if (wall->x != state->skinnum || wall->y != (int32_t)state->effects)
        {
            wall->x = state->skinnum;
            wall->y = (int32_t)state->effects;
            return RF_DUKE_WALL;
        }
    }
    return 0;
}

static void DN_Hit(trace_t *tr, float fraction, const float *normal, const float *point)
{
    float f = fmaxf(0, fraction - .0001f);
    if (f >= tr->fraction)
    {
        return;
    }
    tr->fraction = f;
    VectorCopy(normal, tr->plane.normal);
    tr->plane.dist = DotProduct(normal, point);
    tr->plane.type = 3;
    tr->plane.signbits = (normal[0] < 0) | ((normal[1] < 0) << 1) | ((normal[2] < 0) << 2);
    tr->contents = CONTENTS_SOLID;
}

trace_t DN_TraceWall(const dn_map_t *m, const float *a, const float *b, const float *mins,
                     const float *maxs, int mask, int *hitwall)
{
    trace_t tr;
    unsigned char seen[DN_MAX_SECTORS] = {};
    int queue[DN_MAX_SECTORS], head = 0, tail = 0, sector = DN_FindSector(m, a, -1), i, k;
    memset(&tr, 0, sizeof(tr));
    tr.fraction = 1;
    tr.surface = &dn_surface;
    *hitwall = -1;
    VectorCopy(b, tr.endpos);
    if (!(mask & CONTENTS_SOLID))
    {
        return tr;
    }
    if (sector < 0)
    {
        tr.startsolid = true;
        tr.allsolid = DN_FindSector(m, b, -1) < 0;
        tr.fraction = 0;
        VectorCopy(a, tr.endpos);
        return tr;
    }
    queue[tail++] = sector;
    seen[sector] = 1;
    while (head < tail)
    {
        const dn_sector_t *s;
        sector = queue[head++];
        s = &m->sectors[sector];
        for (k = 0; k < 2; k++)
        {
            float normal[3], z0 = DN_PlaneZ(m, sector, k, 0, 0), dist, d1, d2, offset = 0, p[3], f;
            DN_PlaneNormal(m, sector, k, normal);
            dist = z0 * normal[2];
            for (i = 0; i < 3; i++)
            {
                offset += (normal[i] >= 0 ? mins[i] : maxs[i]) * normal[i];
            }
            d1 = DotProduct(a, normal) + offset - dist;
            d2 = DotProduct(b, normal) + offset - dist;
            if (d1 < -.1f && DN_BoxTouchesSector(m, sector, a, mins, maxs))
            {
                tr.startsolid = true;
                if (d2 < 0)
                {
                    tr.allsolid = true;
                }
            }
            if (d1 >= -.1f && d2 < d1 && d2 < 0)
            {
                f = fmaxf(0, (d1 - .03125f) / (d1 - d2));
                for (i = 0; i < 3; i++)
                {
                    p[i] = a[i] + (b[i] - a[i]) * f;
                }
                /* Floors/ceilings support the whole footprint. Center-only
                   tests let a straddling player sink into a ledge, after which
                   stationary overlap rejects every snapped movement command. */
                if (DN_BoxTouchesSector(m, sector, p, mins, maxs))
                {
                    float previous = tr.fraction;
                    DN_Hit(&tr, f, normal, p);
                    if (tr.fraction < previous)
                    {
                        *hitwall = -1;
                    }
                }
            }
        }
        for (i = s->wallptr; i < s->wallptr + s->wallnum; i++)
        {
            const dn_wall_t *w = &m->walls[i], *w2 = &m->walls[w->point2];
            float f, normal[3], p[3], lo, hi;
            int start_inside, end_inside;
            int blocking =
                w->nextsector < 0 ||
                (w->cstat & ((mask & (CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP)) ? 1 : 64));
            if (!DN_SweepWall(a, b, mins, maxs, w->x / 16.f, -w->y / 16.f, w2->x / 16.f,
                              -w2->y / 16.f, &f, normal, &start_inside, &end_inside))
            {
                continue;
            }
            for (k = 0; k < 3; k++)
            {
                p[k] = a[k] + (b[k] - a[k]) * f;
            }
            if (!blocking)
            {
                lo = DN_PlaneZ(m, w->nextsector, 0, p[0], p[1]);
                hi = DN_PlaneZ(m, w->nextsector, 1, p[0], p[1]);
                blocking = p[2] + mins[2] < lo - .05f || p[2] + maxs[2] > hi + .05f;
            }
            if (blocking)
            {
                if (start_inside)
                {
                    tr.startsolid = true;
                    tr.allsolid |= end_inside;
                    tr.contents = CONTENTS_SOLID;
                }
                else
                {
                    float previous = tr.fraction;
                    DN_Hit(&tr, f, normal, p);
                    if (tr.fraction < previous)
                    {
                        *hitwall = i;
                    }
                }
            }
            else if (!seen[w->nextsector])
            {
                seen[w->nextsector] = 1;
                queue[tail++] = w->nextsector;
            }
        }
    }
    if (tr.allsolid)
    {
        tr.fraction = 0;
    }
    for (i = 0; i < 3; i++)
    {
        tr.endpos[i] = a[i] + tr.fraction * (b[i] - a[i]);
    }
    return tr;
}

trace_t DN_Trace(const dn_map_t *m, const float *a, const float *b, const float *mins,
                 const float *maxs, int mask)
{
    int hitwall;
    return DN_TraceWall(m, a, b, mins, maxs, mask, &hitwall);
}
