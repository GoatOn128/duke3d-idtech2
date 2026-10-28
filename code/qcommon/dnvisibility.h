/* Build wall-portal visibility, independent of the graphics API. */
#ifndef DN_VISIBILITY_H
#define DN_VISIBILITY_H
#include "dnmap.h"

typedef struct
{
    float origin[3], forward[3], right[3], up[3];
    float fov_x, fov_y;
} dn_view_t;

typedef struct
{
    float left, right, bottom, top;
} dn_window_t;

typedef struct
{
    unsigned char sectors[DN_MAX_SECTORS];
    dn_window_t windows[DN_MAX_SECTORS];
    int count, portal_tests, fallback;
} dn_visibility_t;

void DN_BuildVisibility(const dn_map_t *map, const dn_view_t *view, dn_visibility_t *result);
#endif
