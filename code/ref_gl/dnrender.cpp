/* Build sectors rendered through the existing Quake refexport_t interface.
   Sector planes are tessellated with odd winding, preserving concavity and
   inner wall loops. There are deliberately no BSP surfaces or lightmaps. */
#include "gl_local.h"
#include "dnrender.h"
#include "../qcommon/dnvisibility.h"
#include "../qcommon/dnvisual.h"
#include "../qcommon/dnmath.h"
#include "../cgame/dnweaponview.h"

/* The GLU tessellator is the only GLU piece still needed, so declare the
   subset used instead of pulling in the whole GLU header. */
typedef struct GLUtesselator GLUtesselator;
#ifndef CALLBACK
#define CALLBACK
#endif
typedef void(CALLBACK *dn_glu_callback)(void);
#define GLU_TESS_BEGIN 100100
#define GLU_TESS_VERTEX 100101
#define GLU_TESS_END 100102
#define GLU_TESS_ERROR 100103
#define GLU_TESS_COMBINE 100105
#define GLU_TESS_WINDING_RULE 100140
#define GLU_TESS_WINDING_ODD 100130
extern "C" {
GLUtesselator *WINAPI gluNewTess(void);
void WINAPI gluTessCallback(GLUtesselator *tess, int which, dn_glu_callback CallBackFunc);
void WINAPI gluTessProperty(GLUtesselator *tess, int which, double data);
void WINAPI gluTessNormal(GLUtesselator *tess, double x, double y, double z);
void WINAPI gluTessVertex(GLUtesselator *tess, GLdouble *location, void *data);
void WINAPI gluTessBeginPolygon(GLUtesselator *tess, void *data);
void WINAPI gluTessBeginContour(GLUtesselator *tess);
void WINAPI gluTessEndContour(GLUtesselator *tess);
void WINAPI gluTessEndPolygon(GLUtesselator *tess);
void WINAPI gluDeleteTess(GLUtesselator *tess);
const GLubyte *WINAPI gluErrorString(GLenum error);
}

dn_map_t r_duke_map;
static dn_visibility_t visibility;
static dn_weapon_view_t last_weapon_view;

void R_DukeWeaponInfo_f(void)
{
    ri.Con_Printf(PRINT_ALL, "Duke weapon view: weapon %d phase %d base %d\n",
                  last_weapon_view.weapon, last_weapon_view.phase, last_weapon_view.base);
}

void R_DukeWorldInfo_f(void)
{
    ri.Con_Printf(PRINT_ALL, "Duke renderer world %08x\n", DN_WorldChecksum(&r_duke_map));
    ri.Con_Printf(PRINT_ALL, "Duke portals: %d/%d sectors, %d portal tests, fallback %d\n",
                  visibility.count, r_duke_map.numsectors, visibility.portal_tests,
                  visibility.fallback);
    ri.Con_Printf(PRINT_ALL, "Duke view: %.3f %.3f %.3f angles %.3f %.3f %.3f sector %d\n",
                  r_origin[0], r_origin[1], r_origin[2], r_newrefdef.viewangles[0],
                  r_newrefdef.viewangles[1], r_newrefdef.viewangles[2],
                  DN_FindSector(&r_duke_map, r_origin, -1));
}

typedef struct
{
    float *xy;
    int count, capacity;
} dn_mesh_t;

static dn_mesh_t *meshes;

static void ClipSectorWindow(int sector)
{
    const dn_window_t *window = &visibility.windows[sector];
    int left = (int)floorf((window->left + 1) * .5f * r_newrefdef.width);
    int right = (int)ceilf((window->right + 1) * .5f * r_newrefdef.width);
    int bottom = (int)floorf((window->bottom + 1) * .5f * r_newrefdef.height);
    int top = (int)ceilf((window->top + 1) * .5f * r_newrefdef.height);
    glScissor(r_newrefdef.x + left, vid.height - r_newrefdef.y - r_newrefdef.height + bottom,
              right - left, top - bottom);
}

static struct
{
    int16_t width, height;
    uint32_t anim;
} tiles[DN_MAX_TILES];

static byte remaps[256][256], shades[32][256];
static int palettes_loaded;

void R_DukeTextureCheck_f(void)
{
    int nearest = 0, console_fonts = 0, failures = 0;
    for (int i = 0; i < numgltextures; i++)
    {
        const image_t *image = &gltextures[i];
        GLint minification, magnification;
        if (!image->texnum)
        {
            continue;
        }
        GL_Bind(image->texnum);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minification);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magnification);
        int console = !strcmp(image->name, "pics/conchars.pcx");
        if (console)
        {
            console_fonts++;
        }
        else
        {
            nearest++;
        }
        if (console ? minification != GL_LINEAR || magnification != GL_LINEAR
                    : (minification != GL_NEAREST && minification != GL_NEAREST_MIPMAP_NEAREST) ||
                          magnification != GL_NEAREST)
        {
            failures++;
            ri.Con_Printf(PRINT_ALL, "Unexpected filtering: %s\n", image->name);
        }
        if (image->type == it_duke &&
            (image->width != image->upload_width || image->height != image->upload_height))
        {
            failures++;
            ri.Con_Printf(PRINT_ALL, "Resampled native tile: %s\n", image->name);
        }
    }
    ri.Con_Printf(PRINT_ALL,
                  "Duke texture checks: %d nearest, %d linear console fonts, %d failures\n",
                  nearest, console_fonts, failures);
}

static dn_mesh_t *tess_mesh;
static GLenum tess_mode;
static int tess_count;
static float tess_first[2], tess_previous[2], tess_previous2[2];

typedef struct dn_combine_s
{
    GLdouble p[3];
    struct dn_combine_s *next;
} dn_combine_t;

static dn_combine_t *combined;

static void MeshVertex(dn_mesh_t *m, const float *p)
{
    if (m->count + 2 >= m->capacity)
    {
        m->capacity = m->capacity ? m->capacity * 2 : 256;
        m->xy = (float *)(realloc(m->xy, m->capacity * sizeof(float)));
        if (!m->xy)
        {
            ri.Sys_Error(ERR_FATAL, "Build tessellation allocation failed");
        }
    }
    m->xy[m->count++] = p[0];
    m->xy[m->count++] = p[1];
}

static void CALLBACK TessBegin(GLenum mode)
{
    tess_mode = mode;
    tess_count = 0;
}

static void CALLBACK TessVertex(void *data)
{
    GLdouble *v = (GLdouble *)(data);
    float p[2] = {(float)v[0], (float)v[1]};
    if (tess_mode == GL_TRIANGLES)
    {
        MeshVertex(tess_mesh, p);
    }
    else if (tess_count >= 2)
    {
        MeshVertex(tess_mesh, tess_mode == GL_TRIANGLE_FAN ? tess_first : tess_previous2);
        MeshVertex(tess_mesh, tess_previous);
        MeshVertex(tess_mesh, p);
    }
    if (!tess_count)
    {
        memcpy(tess_first, p, sizeof(p));
    }
    memcpy(tess_previous2, tess_previous, sizeof(p));
    memcpy(tess_previous, p, sizeof(p));
    tess_count++;
}

static void CALLBACK TessEnd(void)
{
}

static void CALLBACK TessError(GLenum error)
{
    ri.Sys_Error(ERR_DROP, "Build polygon tessellation: %s", gluErrorString(error));
}

static void CALLBACK TessCombine(GLdouble p[3], void *input[4], GLfloat weight[4], void **output)
{
    dn_combine_t *c = (dn_combine_t *)(calloc(1, sizeof(*c)));
    if (!c)
    {
        ri.Sys_Error(ERR_FATAL, "Build tessellation allocation failed");
    }
    memcpy(c->p, p, sizeof(c->p));
    c->next = combined;
    combined = c;
    *output = c->p;
}

static void BuildMesh(int sector)
{
    const dn_sector_t *s = &r_duke_map.sectors[sector];
    GLUtesselator *t = gluNewTess();
    GLdouble(*vertices)[3] = static_cast<GLdouble(*)[3]>(calloc(s->wallnum, sizeof(*vertices)));
    byte *visited = (byte *)(calloc(s->wallnum, 1));
    int i, j, steps;
    if (!t || !vertices || !visited)
    {
        ri.Sys_Error(ERR_FATAL, "Build polygon allocation failed");
    }
    tess_mesh = &meshes[sector];
    tess_mesh->count = 0;
    gluTessCallback(t, GLU_TESS_BEGIN, (dn_glu_callback)TessBegin);
    gluTessCallback(t, GLU_TESS_VERTEX, (dn_glu_callback)TessVertex);
    gluTessCallback(t, GLU_TESS_END, (dn_glu_callback)TessEnd);
    gluTessCallback(t, GLU_TESS_ERROR, (dn_glu_callback)TessError);
    gluTessCallback(t, GLU_TESS_COMBINE, (dn_glu_callback)TessCombine);
    gluTessProperty(t, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
    gluTessNormal(t, 0, 0, 1);
    for (i = 0; i < s->wallnum; i++)
    {
        vertices[i][0] = r_duke_map.walls[s->wallptr + i].x / 16.;
        vertices[i][1] = -r_duke_map.walls[s->wallptr + i].y / 16.;
    }
    gluTessBeginPolygon(t, NULL);
    for (i = 0; i < s->wallnum; i++)
    {
        if (!visited[i])
        {
            gluTessBeginContour(t);
            j = i;
            steps = 0;
            do
            {
                if (j < 0 || j >= s->wallnum || visited[j] || ++steps > s->wallnum)
                {
                    ri.Sys_Error(ERR_DROP, "Invalid Build polygon loop");
                }
                visited[j] = 1;
                gluTessVertex(t, vertices[j], vertices[j]);
                j = r_duke_map.walls[s->wallptr + j].point2 - s->wallptr;
            } while (j != i);
            gluTessEndContour(t);
        }
    }
    gluTessEndPolygon(t);
    gluDeleteTess(t);
    free(vertices);
    free(visited);
    while (combined)
    {
        dn_combine_t *next = combined->next;
        free(combined);
        combined = next;
    }
}

void R_DukeFree(void)
{
    int i;
    if (meshes)
    {
        for (i = 0; i < r_duke_map.numsectors; i++)
        {
            free(meshes[i].xy);
        }
        free(meshes);
        meshes = NULL;
    }
    DN_FreeMap(&r_duke_map);
    memset(&visibility, 0, sizeof(visibility));
}

static void LoadPalettes(void)
{
    byte *p;
    int length, i, j;
    if (palettes_loaded)
    {
        return;
    }
    for (i = 0; i < 256; i++)
    {
        for (j = 0; j < 256; j++)
        {
            remaps[i][j] = j;
        }
    }
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < 256; j++)
        {
            shades[i][j] = j;
        }
    }
    length = ri.FS_LoadFile("pics/palookup.dat", (void **)&p);
    if (p && length >= 2 + 32 * 256)
    {
        memcpy(shades, p + 2, sizeof(shades));
    }
    if (p)
    {
        ri.FS_FreeFile(p);
    }
    length = ri.FS_LoadFile("pics/lookup.dat", (void **)&p);
    if (p && length > 0)
    {
        for (i = 0; i < p[0]; i++)
        {
            if (1 + (i + 1) * 257 <= length)
            {
                memcpy(remaps[p[1 + i * 257]], p + 2 + i * 257, 256);
            }
        }
    }
    if (p)
    {
        ri.FS_FreeFile(p);
    }
    palettes_loaded = 1;
}

/* Cache the exact palette remap and sector shade in indexed WAL textures.
   Fog remains distance dependent and is set separately per sector. */
static image_t *TileImage(int tile, int pal, int shade)
{
    char name[MAX_QPATH], path[MAX_QPATH];
    miptex_t *wal;
    byte *pixels;
    image_t *im;
    int i, size, length;
    if (tile < 0 || tile >= DN_MAX_TILES || !tiles[tile].width || !tiles[tile].height)
    {
        return r_notexture;
    }
    if (pal < 0 || pal > 255)
    {
        pal = 0;
    }
    if (shade < 0)
    {
        shade = 0;
    }
    if (shade > 31)
    {
        shade = 31;
    }
    snprintf(name, sizeof(name), "duke/%d_p%d_s%d", tile, pal, shade);
    for (i = 0; i < numgltextures; i++)
    {
        if (gltextures[i].texnum && !strcmp(gltextures[i].name, name))
        {
            gltextures[i].registration_sequence = registration_sequence;
            return &gltextures[i];
        }
    }
    snprintf(path, sizeof(path), "textures/duke/tile%04d.wal", tile);
    length = ri.FS_LoadFile(path, (void **)&wal);
    if (!wal)
    {
        return r_notexture;
    }
    size = tiles[tile].width * tiles[tile].height;
    if (length < 100 + size)
    {
        ri.FS_FreeFile(wal);
        return r_notexture;
    }
    pixels = (byte *)(malloc(size));
    if (!pixels)
    {
        ri.Sys_Error(ERR_FATAL, "Build tile allocation failed");
    }
    for (i = 0; i < size; i++)
    {
        int c = ((byte *)wal)[100 + i];
        pixels[i] = c == 255 ? 255 : shades[shade][remaps[pal][c]];
    }
    im = GL_LoadPic(name, pixels, tiles[tile].width, tiles[tile].height, it_duke, 8);
    free(pixels);
    ri.FS_FreeFile(wal);
    return im;
}

static int AnimatedTile(int tile)
{
    unsigned anim;
    int frames, type, tick;
    if (tile < 0 || tile >= DN_MAX_TILES)
    {
        return 0;
    }
    anim = tiles[tile].anim;
    frames = anim & 63;
    type = anim & 192;
    if (!frames || !type)
    {
        return tile;
    }
    /* A fixed ART clock makes renderer comparisons independent of capture timing. */
    float time = ri.Cvar_Get("r_dn_animtime", "-1", 0)->value;
    tick = (int)((time >= 0 ? time : r_newrefdef.time) * 120) >> ((anim >> 24) & 15);
    if (type == 64)
    {
        tick %= frames * 2;
        if (tick > frames)
        {
            tick = frames * 2 - tick;
        }
    }
    else
    {
        tick %= frames + 1;
    }
    return tile + (type == 192 ? -tick : tick);
}

static void SectorFog(int sector, int shade)
{
    float start, end;
    if (!ri.Cvar_Get("r_dn_fog", "1", 0)->value ||
        !DN_SectorFogRange(r_duke_map.sectors[sector].visibility, shade, &start, &end))
    {
        RB_Fog(false, 0, 1);
        return;
    }
    RB_Fog(true, start, end);
}

void R_DukeLoad(model_t *model, void *buffer, int length)
{
    char error[160];
    void *meta;
    int size, i;
    R_DukeFree();
    LoadPalettes();
    if (!DN_LoadMap(&r_duke_map, buffer, length, error, sizeof(error)))
    {
        ri.Sys_Error(ERR_DROP, "%s: %s", model->name, error);
    }
    size = ri.FS_LoadFile("sprites/tiles.dat", &meta);
    if (size != sizeof(tiles))
    {
        ri.Sys_Error(ERR_DROP, "Missing Duke tile metadata; run convertgrp");
    }
    memcpy(tiles, meta, sizeof(tiles));
    ri.FS_FreeFile(meta);
    size = ri.FS_LoadFile("sprites/tables.dat", &meta);
    if (size < 0 || !DN_MathInit(meta, size))
    {
        ri.Sys_Error(ERR_DROP, "Missing Build TABLES.DAT; run convertgrp");
    }
    ri.FS_FreeFile(meta);
    meshes = (dn_mesh_t *)(calloc(r_duke_map.numsectors, sizeof(*meshes)));
    if (!meshes)
    {
        ri.Sys_Error(ERR_FATAL, "Build mesh allocation failed");
    }
    for (i = 0; i < r_duke_map.numsectors; i++)
    {
        BuildMesh(i);
    }
    model->type = mod_duke;
    model->numframes = 1;
    model->numleafs = 1;
    model->leafs = (mleaf_t *)(Hunk_Alloc(sizeof(mleaf_t)));
    memset(model->leafs, 0, sizeof(mleaf_t));
    VectorCopy(r_duke_map.mins, model->mins);
    VectorCopy(r_duke_map.maxs, model->maxs);
    ri.Con_Printf(PRINT_ALL, "Native Build renderer: %d sectors (sector lighting, no lightmaps)\n",
                  r_duke_map.numsectors);
}

void R_DukeRegister(void)
{
    int i;
    for (i = 0; i < r_duke_map.numsectors; i++)
    {
        dn_sector_t *s = &r_duke_map.sectors[i];
        TileImage(s->floorpicnum, s->floorpal, s->floorshade);
        TileImage(s->ceilingpicnum, s->ceilingpal, s->ceilingshade);
    }
    for (i = 0; i < r_duke_map.numwalls; i++)
    {
        dn_wall_t *w = &r_duke_map.walls[i];
        TileImage(w->picnum, w->pal, w->shade);
        if (w->cstat & 16)
        {
            TileImage(w->overpicnum, w->pal, w->shade);
        }
    }
}

static void DrawPlane(int sector, int ceiling)
{
    dn_sector_t *s = &r_duke_map.sectors[sector];
    int stat = ceiling ? s->ceilingstat : s->floorstat,
        tile = AnimatedTile(ceiling ? s->ceilingpicnum : s->floorpicnum), i;
    int pal = ceiling ? s->ceilingpal : s->floorpal,
        shade = ceiling ? s->ceilingshade : s->floorshade;
    float panx = (ceiling ? s->ceilingxpanning : s->floorxpanning) / 256.f,
          pany = (ceiling ? s->ceilingypanning : s->floorypanning) / 256.f;
    if (stat & 1)
    {
        return; /* Parallax sky is drawn at infinity before sector geometry. */
    }
    image_t *im = TileImage(tile, pal, shade);
    GL_Bind(im->texnum);
    SectorFog(sector, shade);
    RB_Begin(GL_TRIANGLES);
    for (i = 0; i < meshes[sector].count; i += 2)
    {
        float x = meshes[sector].xy[i], y = meshes[sector].xy[i + 1], u = x * 2, v = -y * 2,
              z = DN_PlaneZ(&r_duke_map, sector, ceiling, x, y), swap;
        if (stat & 64)
        {
            dn_wall_t *a = &r_duke_map.walls[s->wallptr], *b = &r_duke_map.walls[a->point2];
            float dx = (b->x - a->x) / 16.f, dy = -(b->y - a->y) / 16.f,
                  len = sqrtf(dx * dx + dy * dy);
            if (len)
            {
                float px = x - a->x / 16.f, py = y + a->y / 16.f;
                u = (px * dx + py * dy) * 2 / len;
                v = (py * dx - px * dy) * 2 / len;
            }
        }
        if (stat & 4)
        {
            swap = u;
            u = v;
            v = swap;
        }
        if (stat & 8)
        {
            u *= 2;
            v *= 2;
        }
        if (stat & 16)
        {
            u = -u;
        }
        if (stat & 32)
        {
            v = -v;
        }
        if (stat & 1)
        {
            u = atan2f(y - r_origin[1], x - r_origin[0]) * (4.f / (float)M_PI);
            v = .5f - (z - r_origin[2]) / fmaxf(256, sqrtf((x - r_origin[0]) * (x - r_origin[0]) +
                                                           (y - r_origin[1]) * (y - r_origin[1])));
            RB_TexCoord2f(u, v);
        }
        else
        {
            RB_TexCoord2f(u / im->width + panx, v / im->height + pany);
        }
        RB_Vertex3f(x, y, z);
    }
    RB_End();
    c_brush_polys += meshes[sector].count / 6;
}

static void WallQuad(int sector, const dn_wall_t *w, float x1, float y1, float x2, float y2,
                     float bottom1, float bottom2, float top1, float top2, int tile, float anchor)
{
    image_t *im;
    float u1, u2, vscale, pan;
    if (top1 <= bottom1 && top2 <= bottom2)
    {
        return;
    }
    im = TileImage(AnimatedTile(tile), w->pal, w->shade);
    GL_Bind(im->texnum);
    SectorFog(sector, w->shade);
    u1 = w->xpanning / (float)im->width;
    u2 = u1 + w->xrepeat * 8.f / im->width;
    if (w->cstat & 8)
    {
        float t = u1;
        u1 = u2;
        u2 = t;
    }
    vscale = w->yrepeat / (8.f * im->height);
    if (w->cstat & 256)
    {
        vscale = -vscale;
    }
    pan = w->ypanning / 256.f;
    RB_Begin(GL_QUADS);
    RB_TexCoord2f(u1, (anchor - bottom1) * vscale + pan);
    RB_Vertex3f(x1, y1, bottom1);
    RB_TexCoord2f(u2, (anchor - bottom2) * vscale + pan);
    RB_Vertex3f(x2, y2, bottom2);
    RB_TexCoord2f(u2, (anchor - top2) * vscale + pan);
    RB_Vertex3f(x2, y2, top2);
    RB_TexCoord2f(u1, (anchor - top1) * vscale + pan);
    RB_Vertex3f(x1, y1, top1);
    RB_End();
    c_brush_polys += 2;
}

static void DrawParallaxSky(void)
{
    static const int moon_offsets[8] = {0, 2, 3, 0, 2, 0, 1, 0};
    static const int orbit_offsets[8] = {0, 0, 4, 0, 0, 1, 2, 3};
    static const int city_offsets[8] = {1, 2, 1, 3, 4, 0, 2, 3};
    int sector = DN_FindSector(&r_duke_map, r_origin, -1);
    int tile = -1;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (sector >= 0 && (r_duke_map.sectors[sector].ceilingstat & 1))
    {
        tile = r_duke_map.sectors[sector].ceilingpicnum;
    }
    if (tile < 0)
    {
        for (int i = 0; i < r_duke_map.numsectors; i++)
        {
            if (visibility.sectors[i] && (r_duke_map.sectors[i].ceilingstat & 1))
            {
                tile = r_duke_map.sectors[i].ceilingpicnum;
                break;
            }
        }
    }
    if (tile < 0)
    {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    RB_Fog(false, 0, 1);
    for (int segment = 0; segment < 8; segment++)
    {
        int offset = tile == 80   ? moon_offsets[segment]
                     : tile == 84 ? orbit_offsets[segment]
                     : tile == 89 ? city_offsets[segment]
                                  : 0;
        image_t *image = TileImage(tile + offset, 0, 0);
        GL_Bind(image->texnum);
        RB_Begin(GL_QUAD_STRIP);
        for (int sample = 0; sample <= 8; sample++)
        {
            float u = sample / 8.f;
            float angle = (segment + u) * (float)M_PI / 4;
            float x = r_origin[0] + cosf(angle) * 1024;
            float y = r_origin[1] + sinf(angle) * 1024;
            RB_TexCoord2f(u, 0);
            RB_Vertex3f(x, y, r_origin[2] + 1024);
            RB_TexCoord2f(u, 1);
            RB_Vertex3f(x, y, r_origin[2] - 1024);
        }
        RB_End();
    }
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void R_DukeDrawWorld(void)
{
    int sector, i, pass;
    if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
    {
        return;
    }
    dn_view_t view = {};
    VectorCopy(r_origin, view.origin);
    VectorCopy(vpn, view.forward);
    VectorCopy(vright, view.right);
    VectorCopy(vup, view.up);
    view.fov_x = r_newrefdef.fov_x;
    view.fov_y = r_newrefdef.fov_y;
    DN_BuildVisibility(&r_duke_map, &view, &visibility);
    if (!ri.Cvar_Get("r_dn_portals", "1", 0)->value)
    {
        memset(visibility.sectors, 1, r_duke_map.numsectors);
        visibility.count = r_duke_map.numsectors;
        for (int i = 0; i < r_duke_map.numsectors; i++)
        {
            visibility.windows[i] = (dn_window_t){-1, 1, -1, 1};
        }
    }
    GL_EnableMultitexture(false);
    glDisable(GL_CULL_FACE);
    RB_Color4f(1, 1, 1, 1);
    DrawParallaxSky();
    glEnable(GL_SCISSOR_TEST);
    for (pass = 0; pass < 2; pass++)
    {
        for (sector = 0; sector < r_duke_map.numsectors; sector++)
        {
            dn_sector_t *s = &r_duke_map.sectors[sector];
            if (!visibility.sectors[sector])
            {
                continue;
            }
            /* Membership alone admits geometry below/above the opening that
               revealed a sector. Build's portal clip also bounds its pixels. */
            ClipSectorWindow(sector);
            if (!pass)
            {
                DrawPlane(sector, 0);
                DrawPlane(sector, 1);
            }
            for (i = s->wallptr; i < s->wallptr + s->wallnum; i++)
            {
                dn_wall_t *w = &r_duke_map.walls[i], *w2 = &r_duke_map.walls[w->point2];
                /* Build scansector discards back-facing walls. Drawing the
                   reverse side exposes building brick through parallax skies. */
                if (!DN_WallFacesPoint(w, w2, r_origin[0], r_origin[1]))
                {
                    continue;
                }
                float x1 = w->x / 16.f, y1 = -w->y / 16.f, x2 = w2->x / 16.f, y2 = -w2->y / 16.f;
                float f1 = DN_PlaneZ(&r_duke_map, sector, 0, x1, y1),
                      f2 = DN_PlaneZ(&r_duke_map, sector, 0, x2, y2);
                float c1 = DN_PlaneZ(&r_duke_map, sector, 1, x1, y1),
                      c2 = DN_PlaneZ(&r_duke_map, sector, 1, x2, y2);
                if (w->nextsector < 0)
                {
                    if (!pass)
                    {
                        WallQuad(sector, w, x1, y1, x2, y2, f1, f2, c1, c2, w->picnum,
                                 w->cstat & 4 ? f1 : c1);
                    }
                }
                else
                {
                    float nf1 = DN_PlaneZ(&r_duke_map, w->nextsector, 0, x1, y1),
                          nf2 = DN_PlaneZ(&r_duke_map, w->nextsector, 0, x2, y2);
                    float nc1 = DN_PlaneZ(&r_duke_map, w->nextsector, 1, x1, y1),
                          nc2 = DN_PlaneZ(&r_duke_map, w->nextsector, 1, x2, y2);
                    if (!pass)
                    {
                        if (!((s->floorstat & r_duke_map.sectors[w->nextsector].floorstat) & 1))
                        {
                            WallQuad(sector, w, x1, y1, x2, y2, f1, f2, fminf(nf1, c1),
                                     fminf(nf2, c2), w->picnum, w->cstat & 4 ? c1 : nf1);
                        }
                        if (!((s->ceilingstat & r_duke_map.sectors[w->nextsector].ceilingstat) & 1))
                        {
                            WallQuad(sector, w, x1, y1, x2, y2, fmaxf(nc1, f1), fmaxf(nc2, f2), c1,
                                     c2, w->picnum, w->cstat & 4 ? c1 : nc1);
                        }
                        /* A one-way texture covers only the portal opening. The
                           solid upper/lower pieces still use picnum (ENGINE.cpp).
                           E1L2's closed revolving door has no portal opening. */
                        if (w->cstat & 32)
                        {
                            WallQuad(sector, w, x1, y1, x2, y2, fmaxf(f1, nf1), fmaxf(f2, nf2),
                                     fminf(c1, nc1), fminf(c2, nc2), w->overpicnum,
                                     w->cstat & 4 ? c1 : nc1);
                        }
                    }
                    else if ((w->cstat & (16 | 32)) == 16)
                    {
                        RB_AlphaTest(true);
                        if (w->cstat & 128)
                        {
                            glEnable(GL_BLEND);
                            RB_Color4f(1, 1, 1, w->cstat & 512 ? .33f : .66f);
                            glDepthMask(GL_FALSE);
                        }
                        WallQuad(sector, w, x1, y1, x2, y2, fmaxf(f1, nf1), fmaxf(f2, nf2),
                                 fminf(c1, nc1), fminf(c2, nc2), w->overpicnum,
                                 w->cstat & 4 ? f1 : c1);
                        glDepthMask(GL_TRUE);
                        glDisable(GL_BLEND);
                        RB_AlphaTest(false);
                        RB_Color4f(1, 1, 1, 1);
                    }
                }
            }
        }
    }
    glDisable(GL_SCISSOR_TEST);
    RB_Fog(false, 0, 1);
    RB_Color4f(1, 1, 1, 1);
}

void R_DukeLight(const float *p, float *color)
{
    int sector = DN_FindSector(&r_duke_map, p, -1);
    float light = 1;
    if (sector >= 0)
    {
        light = powf(2, -r_duke_map.sectors[sector].floorshade / 16.f);
    }
    if (light > 1)
    {
        light = 1;
    }
    if (light < .05f)
    {
        light = .05f;
    }
    VectorSet(color, light, light, light);
}

void R_DukeDrawSprite(entity_t *e)
{
    if (e->flags & RF_DUKE_WORLD)
    {
        return;
    }
    int tile = e->frame, cstat = (unsigned)e->skinnum >> 16, rx = e->skinnum & 255,
        ry = (e->skinnum >> 8) & 255;
    int sector, pal = (int)(e->angles[2] * 256.f / 360.f + .5f) & 255,
                shade = ((int)lroundf(e->angles[0] * 256.f / 360.f) & 255) - 128;
    float right[3], up[3] = {0, 0, 1}, p[3], base[3], width, height, left, bottom,
                    alpha = e->flags & RF_TRANSLUCENT ? e->alpha : 1;
    image_t *im;
    if (tile < 0 || tile >= DN_MAX_TILES || !rx || !ry || cstat & 32768)
    {
        return;
    }
    {
        int views = (unsigned)e->skinnum >> 26 & 15;
        int angle = (int)lroundf(-e->angles[YAW] * 2048 / 360) & 2047;
        int view_angle = (int)lroundf(-r_newrefdef.viewangles[YAW] * 2048 / 360) & 2047;
        int dx = (int)((e->origin[0] - r_origin[0]) * 16);
        int dy = (int)((r_origin[1] - e->origin[1]) * 16);
        tile += DN_SpriteRotation(angle, view_angle, dx, dy, views, &cstat);
        /* Only directional actors can fall back to an earlier animation frame.
           Empty effect tiles (e.g. FRAMEEFFECT1) must not walk back through the
           ART bank to the unrelated USER ART sign at tile 3584. */
        while (views > 1 && tile > 0 && tile < DN_MAX_TILES && !tiles[tile].width)
        {
            tile -= views;
        }
        cstat &= 0xc3ff;
    }
    sector = DN_FindSector(&r_duke_map, e->origin, -1);
    if (sector >= 0 && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && !visibility.sectors[sector])
    {
        return;
    }
    if (sector >= 0)
    {
        const dn_sector_t *area = &r_duke_map.sectors[sector];
        shade = DN_SpriteSectorShade(shade, e->flags, area->ceilingstat, area->floorshade,
                                     area->ceilingshade);
        if (!(e->flags & RF_DUKE_OWN_PALETTE) && area->floorpal)
        {
            pal = area->floorpal;
        }
    }
    tile = AnimatedTile(tile);
    im = TileImage(tile, pal, shade);
    if (im == r_notexture)
    {
        return;
    }
    if (sector >= 0)
    {
        SectorFog(sector, shade);
    }
    width = tiles[tile].width * rx / 64.f;
    height = tiles[tile].height * ry / 64.f;
    DN_SpriteOffsets(tiles[tile].width, tiles[tile].height, tiles[tile].anim, rx, ry, cstat, &left,
                     &bottom);
    VectorCopy(e->origin, base);
    if (cstat & 48)
    {
        float yaw = e->angles[1] * (float)M_PI / 180;
        VectorSet(right, -sinf(yaw), cosf(yaw), 0);
        if (cstat & 32)
        {
            VectorSet(up, cosf(yaw), sinf(yaw), 0);
        }
    }
    else
    {
        VectorSet(right, vright[0], vright[1], 0);
        VectorNormalize(right);
    }
    GL_EnableMultitexture(false);
    GL_Bind(im->texnum);
    glDisable(GL_CULL_FACE);
    RB_AlphaTest(true);
    RB_Color4f(1, 1, 1, alpha);
    if (cstat & 48)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-1, -1);
    }
    if (alpha < 1)
    {
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
    }
    RB_Begin(GL_QUADS);
    for (int i = 0; i < 4; i++)
    {
        int r = i >= 2, t = i == 1 || i == 2;
        float u = r ? 1 : 0, v = t ? 0 : 1;
        if (cstat & 4)
        {
            u = 1 - u;
        }
        if (cstat & 8)
        {
            v = 1 - v;
        }
        VectorMA(base, left + (r ? width : 0), right, p);
        VectorMA(p, bottom + (t ? height : 0), up, p);
        RB_TexCoord2f(u, v);
        RB_Vertex3fv(p);
    }
    RB_End();
    if (cstat & 48)
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    RB_AlphaTest(false);
    RB_Fog(false, 0, 1);
    RB_Color4f(1, 1, 1, 1);
}

void R_DukeApplyWorldStates(const refdef_t *frame)
{
    byte dirty[DN_MAX_SECTORS] = {};
    for (int i = 0; i < frame->num_entities; i++)
    {
        const entity_t *entity = &frame->entities[i];
        entity_state_t state = {};
        int changed;
        if (!(entity->flags & RF_DUKE_WORLD))
        {
            continue;
        }
        state.frame = entity->frame;
        state.renderfx = entity->flags;
        VectorCopy(entity->angles, state.angles);
        if (entity->flags & RF_DUKE_DISCRETE)
        {
            /* Styles change at snapshot boundaries, never between tile IDs. */
            state.skinnum = entity->skinnum;
            state.effects = (uint32_t)entity->oldframe;
            VectorCopy(entity->origin, state.origin);
        }
        else if (entity->flags & RF_DUKE_SECTOR)
        {
            state.skinnum = (int)lroundf(-entity->origin[0] * 256);
            state.effects = (int)lroundf(-entity->origin[2] * 256);
        }
        else
        {
            state.skinnum = (int)lroundf(entity->origin[0] * 16);
            state.effects = (int)lroundf(-entity->origin[1] * 16);
        }
        changed = DN_ApplyWorldState(&r_duke_map, &state);
        if (changed & RF_DUKE_WALL)
        {
            for (int sector = 0; sector < r_duke_map.numsectors; sector++)
            {
                const dn_sector_t *s = &r_duke_map.sectors[sector];
                if (state.frame >= s->wallptr && state.frame < s->wallptr + s->wallnum)
                {
                    dirty[sector] = 1;
                }
            }
        }
    }
    for (int sector = 0; sector < r_duke_map.numsectors; sector++)
    {
        if (dirty[sector])
        {
            BuildMesh(sector);
        }
    }
}

void R_DukeDrawWeapon(entity_t *entity)
{
    dn_weapon_view_t view = {};
    dn_weapon_sprites_t sprites;
    float sx = r_newrefdef.width / 320.f;
    float sy = vid.height / 200.f * r_newrefdef.width / vid.width;
    int sector = DN_FindSector(&r_duke_map, r_origin, -1);
    view.weapon = entity->skinnum & 255;
    view.side = (entity->skinnum >> 8) & 1;
    view.base = entity->frame;
    view.phase = entity->oldframe;
    view.sway = (int)entity->origin[0];
    view.position = (int)entity->origin[1];
    view.access = (int)entity->origin[2];
    view.shade = sector >= 0 ? r_duke_map.sectors[sector].floorshade : 0;
    view.palette = sector >= 0 ? r_duke_map.sectors[sector].floorpal : 0;
    last_weapon_view = view;
    ri.DukeWeaponView(&view, &sprites);

    R_PushProjection();
    Mat_Ortho(r_mat_projection, 0, r_newrefdef.width, r_newrefdef.height, 0, -1, 1);
    R_PushModelview();
    Mat_Identity(r_mat_modelview);
    glDisable(GL_DEPTH_TEST);
    RB_Fog(false, 0, 1);
    glDisable(GL_CULL_FACE);
    RB_AlphaTest(true);
    GL_EnableMultitexture(false);
    for (int i = 0; i < sprites.count; i++)
    {
        const dn_weapon_sprite_t *sprite = &sprites.sprites[i];
        int tile = sprite->tile;
        if (tile < 0 || tile >= DN_MAX_TILES || !tiles[tile].width)
        {
            continue;
        }
        image_t *image = TileImage(tile, sprite->palette, sprite->shade);
        float ox = tiles[tile].width / 2 + (int8_t)(tiles[tile].anim >> 8);
        float oy = tiles[tile].height / 2 + (int8_t)(tiles[tile].anim >> 16);
        if (sprite->mirror)
        {
            ox = tiles[tile].width - ox;
        }
        float x = (sprite->x - ox) * sx;
        float y = r_newrefdef.height * .5f + (sprite->y - 100 - oy) * sy;
        float width = tiles[tile].width * sx, height = tiles[tile].height * sy;
        GL_Bind(image->texnum);
        RB_Begin(GL_QUADS);
        RB_TexCoord2f(sprite->mirror ? 1 : 0, 0);
        RB_Vertex2f(x, y);
        RB_TexCoord2f(sprite->mirror ? 0 : 1, 0);
        RB_Vertex2f(x + width, y);
        RB_TexCoord2f(sprite->mirror ? 0 : 1, 1);
        RB_Vertex2f(x + width, y + height);
        RB_TexCoord2f(sprite->mirror ? 1 : 0, 1);
        RB_Vertex2f(x, y + height);
        RB_End();
    }
    RB_AlphaTest(false);
    glEnable(GL_DEPTH_TEST);
    R_PopModelview();
    R_PopProjection();
}
