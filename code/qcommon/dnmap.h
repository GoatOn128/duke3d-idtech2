/* Native Build v7 maps. Fixed-width disk types are essential on Win64. */
#ifndef DN_MAP_H
#define DN_MAP_H
#include <stdint.h>
#include <stddef.h>
#define DN_MAX_SECTORS 4096
#define DN_MAX_WALLS 32767
#define DN_MAX_SPRITES 16384
#define DN_MAX_TILES 6144
#pragma pack(push, 1)

typedef struct
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning, visibility, filler;
    int16_t lotag, hitag, extra;
} dn_sector_t;

typedef struct
{
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t lotag, hitag, extra;
} dn_wall_t;

typedef struct
{
    int32_t x, y, z;
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, filler, xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum, ang, owner, xvel, yvel, zvel, lotag, hitag, extra;
} dn_sprite_t;

#pragma pack(pop)

typedef struct
{
    int numsectors, numwalls, numsprites;
    float start[3], angle;
    int startsector;
    dn_sector_t *sectors;
    dn_wall_t *walls;
    dn_sprite_t *sprites;
    float mins[3], maxs[3];
} dn_map_t;

int DN_LoadMap(dn_map_t *map, const void *buffer, size_t length, char *error, size_t errorsize);
void DN_FreeMap(dn_map_t *map);
int DN_Inside(const dn_map_t *map, int sector, float x, float y);
int DN_WallFacesPoint(const dn_wall_t *wall, const dn_wall_t *end, float x, float y);
int DN_CanSee(const dn_map_t *map, const float *start, int startsector, const float *end,
              int endsector);
int DN_FindSector(const dn_map_t *map, const float *point, int hint);
int DN_FindSectorXY(const dn_map_t *map, const float *point, int hint);
float DN_PlaneZ(const dn_map_t *map, int sector, int ceiling, float x, float y);
void DN_PlaneNormal(const dn_map_t *map, int sector, int ceiling, float *normal);
int DN_SectorZRange(const dn_map_t *map, const float *point, int sector, int distance, float *floor,
                    float *ceiling);
/* Caller includes q_shared.h before this header when using the trace API. */
#ifdef MAX_QPATH
/* Floor-sprite mins/maxs are in the renderer's right/up basis, with Z = 0. */
int DN_IsFloorSprite(const entity_state_t *state);
void DN_FloorSpriteBounds(const entity_state_t *state, float *mins, float *maxs);
trace_t DN_TraceFloorSprite(const entity_state_t *state, const float *start, const float *end,
                            const float *mins, const float *maxs);
trace_t DN_Trace(const dn_map_t *map, const float *start, const float *end, const float *mins,
                 const float *maxs, int mask);
/* Same collision query, with the blocking wall index or -1 for a plane/no hit. */
trace_t DN_TraceWall(const dn_map_t *map, const float *start, const float *end, const float *mins,
                     const float *maxs, int mask, int *hitwall);
int DN_PushMove(const dn_map_t *map, float *point, const float *mins, const float *maxs, int sector,
                int mask);
int DN_PointContents(const dn_map_t *map, const float *point);
int DN_ApplyWorldState(dn_map_t *map, const entity_state_t *state);
uint32_t DN_WorldChecksum(const dn_map_t *map);
#endif
#endif
