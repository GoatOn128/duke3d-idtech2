/* Build's scansector/drawalls visibility rules adapted to a hardware depth
   buffer: facing wall loops, nextsector traversal, one-way boundaries and
   clipped floor/ceiling openings. Portal windows replace software umost/dmost
   spans; OpenGL resolves opaque wall occlusion inside the admitted sectors. */
#include "../game/q_shared.h"
#include "dnvisibility.h"

#define DN_CLIP_VERTICES 16
#define DN_VIS_EPSILON .00001f

typedef struct
{
    float x, y, z;
} view_vertex_t;

static void IncludeAll(const dn_map_t *map, dn_visibility_t *result)
{
    memset(result->sectors, 1, map->numsectors);
    result->count = map->numsectors;
    result->fallback = 1;
}

static view_vertex_t ToView(const dn_view_t *view, float x, float y, float z)
{
    vec3_t delta = {x - view->origin[0], y - view->origin[1], z - view->origin[2]};
    view_vertex_t vertex = {DotProduct(delta, view->right), DotProduct(delta, view->up),
                            DotProduct(delta, view->forward)};
    return vertex;
}

static int ClipPolygon(const view_vertex_t *input, int count, view_vertex_t *output,
                       const float *plane, float offset)
{
    int written = 0;
    view_vertex_t previous = input[count - 1];
    float previous_distance =
        previous.x * plane[0] + previous.y * plane[1] + previous.z * plane[2] - offset;
    for (int i = 0; i < count; i++)
    {
        view_vertex_t current = input[i];
        float distance =
            current.x * plane[0] + current.y * plane[1] + current.z * plane[2] - offset;
        if ((previous_distance >= 0) != (distance >= 0))
        {
            float t = previous_distance / (previous_distance - distance);
            if (written >= DN_CLIP_VERTICES)
            {
                return -1;
            }
            output[written++] = (view_vertex_t){previous.x + t * (current.x - previous.x),
                                                previous.y + t * (current.y - previous.y),
                                                previous.z + t * (current.z - previous.z)};
        }
        if (distance >= 0)
        {
            if (written >= DN_CLIP_VERTICES)
            {
                return -1;
            }
            output[written++] = current;
        }
        previous = current;
        previous_distance = distance;
    }
    return written;
}

static int ProjectOpening(const dn_view_t *view, const dn_window_t *parent, float x1, float y1,
                          float x2, float y2, float floor1, float floor2, float ceiling1,
                          float ceiling2, dn_window_t *window)
{
    float tx = tanf(view->fov_x * (float)M_PI / 360);
    float ty = tanf(view->fov_y * (float)M_PI / 360);
    float planes[5][3] = {{0, 0, 1},
                          {1, 0, -parent->left * tx},
                          {-1, 0, parent->right * tx},
                          {0, 1, -parent->bottom * ty},
                          {0, -1, parent->top * ty}};
    view_vertex_t vertices[2][DN_CLIP_VERTICES];
    int count = 4, source = 0;
    vertices[0][0] = ToView(view, x1, y1, floor1);
    vertices[0][1] = ToView(view, x2, y2, floor2);
    vertices[0][2] = ToView(view, x2, y2, ceiling2);
    vertices[0][3] = ToView(view, x1, y1, ceiling1);
    for (int plane = 0; plane < 5; plane++)
    {
        count = ClipPolygon(vertices[source], count, vertices[1 - source], planes[plane],
                            plane == 0 ? .001f : 0);
        source = 1 - source;
        if (count < 0)
        {
            *window = *parent; /* Conservative overflow handling never hides geometry. */
            return 1;
        }
        if (!count)
        {
            return 0;
        }
    }
    *window = (dn_window_t){1, -1, 1, -1};
    for (int i = 0; i < count; i++)
    {
        const view_vertex_t *vertex = &vertices[source][i];
        float x = vertex->x / (vertex->z * tx), y = vertex->y / (vertex->z * ty);
        window->left = fminf(window->left, x - DN_VIS_EPSILON);
        window->right = fmaxf(window->right, x + DN_VIS_EPSILON);
        window->bottom = fminf(window->bottom, y - DN_VIS_EPSILON);
        window->top = fmaxf(window->top, y + DN_VIS_EPSILON);
    }
    window->left = fmaxf(window->left, parent->left);
    window->right = fminf(window->right, parent->right);
    window->bottom = fmaxf(window->bottom, parent->bottom);
    window->top = fminf(window->top, parent->top);
    return window->right > window->left && window->top > window->bottom;
}

static void MergeWindow(dn_window_t *to, const dn_window_t *from)
{
    to->left = fminf(to->left, from->left);
    to->right = fmaxf(to->right, from->right);
    to->bottom = fminf(to->bottom, from->bottom);
    to->top = fmaxf(to->top, from->top);
}

static int PortalWindow(const dn_map_t *map, const dn_view_t *view, int sector,
                        const dn_wall_t *wall, const dn_window_t *parent, dn_window_t *window)
{
    const dn_wall_t *end = &map->walls[wall->point2];
    const dn_sector_t *from = &map->sectors[sector], *to = &map->sectors[wall->nextsector];
    float x1 = wall->x / 16.f, y1 = -wall->y / 16.f;
    float dx = (end->x - (double)wall->x) / 16.f, dy = -(end->y - (double)wall->y) / 16.f;
    float length2 = dx * dx + dy * dy;
    float t = length2 ? ((view->origin[0] - x1) * dx + (view->origin[1] - y1) * dy) / length2 : 0;
    float facing = dx * (view->origin[1] - y1) - dy * (view->origin[0] - x1);
    float floor[2][2], ceiling[2][2], cuts[4] = {0, 1};
    int cut_count = 2, visible = 0;
    if (!length2 || !DN_WallFacesPoint(wall, end, view->origin[0], view->origin[1]))
    {
        return 0;
    }
    for (int side = 0; side < 2; side++)
    {
        int index = side ? wall->nextsector : sector;
        for (int endpoint = 0; endpoint < 2; endpoint++)
        {
            float x = x1 + dx * endpoint, y = y1 + dy * endpoint;
            floor[side][endpoint] = DN_PlaneZ(map, index, 0, x, y);
            ceiling[side][endpoint] = DN_PlaneZ(map, index, 1, x, y);
            if ((from->ceilingstat & to->ceilingstat) & 1)
            {
                ceiling[side][endpoint] = view->origin[2] + 1000000;
            }
            if ((from->floorstat & to->floorstat) & 1)
            {
                floor[side][endpoint] = view->origin[2] - 1000000;
            }
        }
    }
    /* scansector admits a neighboring sector when the eye crosses its wall.
       This avoids near-plane popping during movement across an open portal. */
    if (t >= 0 && t <= 1 && facing * facing <= .0001f * length2)
    {
        float bottom = fmaxf(floor[0][0] + t * (floor[0][1] - floor[0][0]),
                             floor[1][0] + t * (floor[1][1] - floor[1][0]));
        float top = fminf(ceiling[0][0] + t * (ceiling[0][1] - ceiling[0][0]),
                          ceiling[1][0] + t * (ceiling[1][1] - ceiling[1][0]));
        if (top > bottom && view->origin[2] >= bottom && view->origin[2] <= top)
        {
            *window = *parent;
            return 1;
        }
    }
    /* Slopes can exchange which floor/ceiling bounds the opening midway
       along a wall. Split there instead of rejecting an open middle span. */
    for (int plane = 0; plane < 2; plane++)
    {
        float (*heights)[2] = plane ? ceiling : floor;
        float a = heights[0][0] - heights[1][0], b = heights[0][1] - heights[1][1];
        if (a != b)
        {
            float crossing = a / (a - b);
            if (crossing > 0 && crossing < 1)
            {
                cuts[cut_count++] = crossing;
            }
        }
    }
    for (int i = 1; i < cut_count; i++)
    {
        for (int j = i; j > 0 && cuts[j] < cuts[j - 1]; j--)
        {
            float value = cuts[j];
            cuts[j] = cuts[j - 1];
            cuts[j - 1] = value;
        }
    }
    for (int i = 1; i < cut_count; i++)
    {
        float a = cuts[i - 1], b = cuts[i], bottom[2], top[2];
        dn_window_t opening;
        for (int j = 0; j < 2; j++)
        {
            float at = j ? b : a;
            bottom[j] = fmaxf(floor[0][0] + at * (floor[0][1] - floor[0][0]),
                              floor[1][0] + at * (floor[1][1] - floor[1][0]));
            top[j] = fminf(ceiling[0][0] + at * (ceiling[0][1] - ceiling[0][0]),
                           ceiling[1][0] + at * (ceiling[1][1] - ceiling[1][0]));
        }
        float gap0 = top[0] - bottom[0], gap1 = top[1] - bottom[1];
        if (gap0 <= 0 && gap1 <= 0)
        {
            continue;
        }
        if (gap0 < 0 || gap1 < 0)
        {
            float fraction = gap0 / (gap0 - gap1), at = a + (b - a) * fraction;
            float height = bottom[0] + (bottom[1] - bottom[0]) * fraction;
            int endpoint = gap0 < 0 ? 0 : 1;
            if (endpoint == 0)
            {
                a = at;
            }
            else
            {
                b = at;
            }
            bottom[endpoint] = top[endpoint] = height;
        }
        if (ProjectOpening(view, parent, x1 + dx * a, y1 + dy * a, x1 + dx * b, y1 + dy * b,
                           bottom[0], bottom[1], top[0], top[1], &opening))
        {
            if (!visible)
            {
                *window = opening;
            }
            else
            {
                MergeWindow(window, &opening);
            }
            visible = 1;
        }
    }
    return visible;
}

void DN_BuildVisibility(const dn_map_t *map, const dn_view_t *view, dn_visibility_t *result)
{
    int queue[DN_MAX_SECTORS], head = 0, tail = 0, pending = 0;
    unsigned char queued[DN_MAX_SECTORS] = {};
    int sector = DN_FindSector(map, view->origin, -1);
    memset(result, 0, sizeof(*result));
    if (sector < 0 || !isfinite(view->fov_x) || !isfinite(view->fov_y) || view->fov_x <= 0 ||
        view->fov_x >= 179 || view->fov_y <= 0 || view->fov_y >= 179)
    {
        IncludeAll(map, result);
        return;
    }
    result->sectors[sector] = 1;
    result->count = 1;
    result->windows[sector] = (dn_window_t){-1, 1, -1, 1};
    queue[tail++] = sector;
    queued[sector] = 1;
    pending = 1;
    while (pending)
    {
        sector = queue[head];
        head = (head + 1) % DN_MAX_SECTORS;
        pending--;
        queued[sector] = 0;
        const dn_sector_t *s = &map->sectors[sector];
        dn_window_t parent = result->windows[sector];
        for (int i = s->wallptr; i < s->wallptr + s->wallnum; i++)
        {
            const dn_wall_t *wall = &map->walls[i];
            dn_window_t opening;
            int next = wall->nextsector;
            if (next < 0 || (wall->cstat & 32))
            {
                continue;
            }
            if (++result->portal_tests > DN_MAX_WALLS * 16)
            {
                IncludeAll(map, result);
                return;
            }
            if (!PortalWindow(map, view, sector, wall, &parent, &opening))
            {
                continue;
            }
            if (result->sectors[next])
            {
                const dn_window_t *previous = &result->windows[next];
                if (opening.left >= previous->left - DN_VIS_EPSILON &&
                    opening.right <= previous->right + DN_VIS_EPSILON &&
                    opening.bottom >= previous->bottom - DN_VIS_EPSILON &&
                    opening.top <= previous->top + DN_VIS_EPSILON)
                {
                    continue;
                }
                MergeWindow(&result->windows[next], &opening);
            }
            else
            {
                result->sectors[next] = 1;
                result->windows[next] = opening;
                result->count++;
            }
            if (!queued[next])
            {
                queue[tail] = next;
                tail = (tail + 1) % DN_MAX_SECTORS;
                queued[next] = 1;
                pending++;
            }
        }
    }
}
