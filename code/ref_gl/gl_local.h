/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>

#include <glad/gl.h>
#include <math.h>
#include <stddef.h>

#include "../client/ref.h"

#define REF_VERSION "GL 0.02"

// up / down
#define PITCH 0

// left / right
#define YAW 1

// fall over
#define ROLL 2

typedef struct
{
    unsigned width, height; // coordinates from main game
} viddef_t;

extern viddef_t vid;

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum
{
    it_skin,
    it_sprite,
    it_wall,
    it_pic,
    it_sky,
    it_duke /* Native indexed tiles retain their texel grid without mipmapping. */
} imagetype_t;

typedef struct image_s
{
    char name[MAX_QPATH]; // game path, including extension
    imagetype_t type;
    int width, height;               // source image
    int upload_width, upload_height;  // after power of two and picmip
    int registration_sequence;       // 0 = free
    struct msurface_s *texturechain; // for sort-by-texture world drawing
    int texnum;                      // engine texture slot, resolved through R_TextureHandle
    float sl, tl, sh, th;            // 0,0 - 1,1 unless part of the scrap
    qboolean scrap;
    qboolean has_alpha;
} image_t;

// Engine-side texture slots. R_TextureHandle() turns them into GL texture
// names generated once at init.
#define TEXNUM_WHITE 1
#define TEXNUM_CINEMATIC 2
#define TEXNUM_LIGHTMAPS 1024
#define TEXNUM_SCRAPS 1152
#define TEXNUM_IMAGES 1153

#define MAX_GLTEXTURES 16384

//===================================================================

typedef enum
{
    rserr_ok,

    rserr_invalid_fullscreen,
    rserr_invalid_mode,

    rserr_unknown
} rserr_t;

#include "gl_model.h"

void GL_SetDefaultState(void);
void GL_UpdateSwapInterval(void);

extern float gldepthmin, gldepthmax;

#define BACKFACE_EPSILON 0.01

//====================================================

extern image_t gltextures[MAX_GLTEXTURES];
extern int numgltextures;

extern image_t *r_notexture;
extern image_t *r_particletexture;
extern entity_t *currententity;
extern model_t *currentmodel;
extern int r_visframecount;
extern int r_framecount;
extern cplane_t frustum[4];
extern int c_brush_polys, c_alias_polys;

extern int gl_filter_min, gl_filter_max;

//
// view origin
//
extern vec3_t vup;
extern vec3_t vpn;
extern vec3_t vright;
extern vec3_t r_origin;

//
// screen size info
//
extern refdef_t r_newrefdef;
extern int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern cvar_t *r_norefresh;
extern cvar_t *r_lefthand;
extern cvar_t *r_drawentities;
extern cvar_t *r_drawworld;
extern cvar_t *r_speeds;
extern cvar_t *r_fullbright;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lerpmodels;

extern cvar_t *r_lightlevel; // FIXME: This is a HACK to get the client's light level

extern cvar_t *gl_mode;
extern cvar_t *gl_lightmap;
extern cvar_t *gl_shadows;
extern cvar_t *gl_dynamic;
extern cvar_t *gl_nobind;
extern cvar_t *gl_round_down;
extern cvar_t *gl_picmip;
extern cvar_t *gl_skymip;
extern cvar_t *gl_showtris;
extern cvar_t *gl_finish;
extern cvar_t *gl_clear;
extern cvar_t *gl_cull;
extern cvar_t *gl_polyblend;
extern cvar_t *gl_flashblend;
extern cvar_t *gl_modulate;
extern cvar_t *gl_playermip;
extern cvar_t *gl_drawbuffer;
extern cvar_t *gl_swapinterval;
extern cvar_t *gl_texturemode;
extern cvar_t *gl_saturatelighting;
extern cvar_t *gl_lockpvs;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *vid_ref;

extern cvar_t *intensity;

extern float v_blend[4]; // final blending color

extern int c_visible_lightmaps;
extern int c_visible_textures;

extern float r_world_matrix[16];

void R_LightPoint(vec3_t p, vec3_t color);
void R_PushDlights(void);

//====================================================================

extern model_t *r_worldmodel;

extern unsigned d_8to24table[256];

extern int registration_sequence;

int R_Init(void *hinstance, void *hWnd);
void R_Shutdown(void);

void R_RenderView(refdef_t *fd);
void GL_ScreenShot_f(void);
void R_DrawAliasModel(entity_t *e);
void R_DrawBrushModel(entity_t *e);
void R_DrawSpriteModel(entity_t *e);
void R_DrawBeam(entity_t *e);
void R_DrawWorld(void);
void R_RenderDlights(void);
void R_DrawAlphaSurfaces(void);
void R_RenderBrushPoly(msurface_t *fa);
void R_InitParticleTexture(void);
void Draw_InitLocal(void);
void GL_SubdivideSurface(msurface_t *fa);
qboolean R_CullBox(vec3_t mins, vec3_t maxs);
void R_RotateForEntity(entity_t *e);
void R_MarkLeaves(void);

void EmitWaterPolys(msurface_t *fa);
void R_AddSkySurface(msurface_t *fa);
void R_ClearSkyBox(void);
void R_DrawSkyBox(void);
void R_MarkLights(dlight_t *light, int bit, mnode_t *node);

void Draw_GetPicSize(int *w, int *h, char *name);
void Draw_Pic(int x, int y, char *name);
void Draw_StretchPic(int x, int y, int w, int h, char *name);
void Draw_Char(int x, int y, int c);
void Draw_TileClear(int x, int y, int w, int h, char *name);
void Draw_Fill(int x, int y, int w, int h, int c);
void Draw_FadeScreen(void);
void Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data);
void Draw_StretchRaw32(int x, int y, int w, int h, int cols, int rows, byte *data);

void R_BeginFrame(float camera_separation);
void R_SetPalette(const unsigned char *palette);

int Draw_GetPalette(void);

void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth,
                        int outheight);

struct image_s *R_RegisterSkin(char *name);

void LoadPCX(char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic(char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t *GL_FindImage(char *name, imagetype_t type);
void GL_TextureMode(char *string);
void GL_ImageList_f(void);

void GL_SelectTexture(GLenum unit);
void GL_EnableMultitexture(qboolean enable);
void GL_Bind(int texnum);
void GL_MBind(GLenum target, int texnum);

void GL_InitImages(void);
void GL_ShutdownImages(void);

void GL_FreeUnusedImages(void);

/*
** Batched core-profile drawing (gl_shaders.cpp)
*/

void R_InitShaders(void);
void R_ShutdownShaders(void);

// RB_* mirrors the old immediate-mode calls; RB_End converts and draws.
// Vertices accumulate in this format; light coords feed the lightmap sampler.
typedef struct
{
    float pos[3];
    float tex[2];
    float light[2];
    float color[4];
} rbVertex_t;

void RB_Begin(GLenum mode);
void RB_End(void);
void RB_Color3f(float r, float g, float b);
void RB_Color3fv(const float *rgb);
void RB_Color4f(float r, float g, float b, float a);
void RB_Color4fv(const float *rgba);
void RB_Color4ubv(const byte *rgba);
void RB_TexCoord2f(float s, float t);
void RB_LightCoord2f(float s, float t);
void RB_Vertex2f(float x, float y);
void RB_Vertex2fv(const float *v);
void RB_Vertex3f(float x, float y, float z);
void RB_Vertex3fv(const float *v);

// Replacements for state the core profile removed.
void RB_AlphaTest(qboolean enable); // glAlphaFunc(GL_GREATER, 0.666)
void RB_Textured(qboolean enable);  // white texture instead of GL_TEXTURE_2D off
void RB_Fog(qboolean enable, float start, float end); // linear black fog

// Column-major matrices, the same layout GL uniforms expect.
typedef float mat4_t[16];

extern mat4_t r_mat_projection;
extern mat4_t r_mat_modelview;

void Mat_Identity(mat4_t m);
void Mat_Copy(mat4_t dst, const mat4_t src);
void Mat_Multiply(mat4_t out, const mat4_t a, const mat4_t b);
void Mat_Translate(mat4_t m, float x, float y, float z);
void Mat_Scale(mat4_t m, float x, float y, float z);
void Mat_Rotate(mat4_t m, float degrees, float x, float y, float z);
void Mat_Ortho(mat4_t m, float left, float right, float bottom, float top, float nearz,
               float farz);
void Mat_Frustum(mat4_t m, float left, float right, float bottom, float top, float nearz,
                 float farz);
void R_PushProjection(void);
void R_PopProjection(void);
void R_PushModelview(void);
void R_PopModelview(void);

// Texture slot -> GL name resolution, plus slot helpers.
GLuint R_TextureHandle(int texnum);
void GL_GenTextureHandles(void);

/*
** GL config stuff
*/
typedef struct
{
    const char *renderer_string;
    const char *vendor_string;
    const char *version_string;
} glconfig_t;

typedef struct
{
    float inverse_intensity;
    qboolean fullscreen;

    int prev_mode;

    int lightmap_textures;

    int currenttextures[2];
    int currenttmu;
} glstate_t;

extern glconfig_t gl_config;
extern glstate_t gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern refimport_t ri;

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void GLimp_BeginFrame(void);
void GLimp_EndFrame(void);
qboolean GLimp_Init(void *hinstance, void *hWnd);
void GLimp_Shutdown(void);
rserr_t GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen);
void GLimp_AppActivate(qboolean active);
void GLimp_ListDisplays_f(void);
