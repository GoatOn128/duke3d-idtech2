/* Minimal id Tech 3 renderer-facing types for the UI module, adapted
   from the RTCW SP GPL source (src/cgame/tr_types.h plus the font and
   tokenizer pieces of src/game/q_shared.h). Handles are opaque pointers:
   the engine's Q2 2D drawing goes through name-based registration, and
   int handles would also truncate on x64 builds. */
#ifndef DN_UI_TR_TYPES_H
#define DN_UI_TR_TYPES_H

#include "../game/q_shared.h"

typedef void *qhandle_t;
typedef void *sfxHandle_t;

/* the engine Q2 q_shared stops at vec3, but the menu framework colors are vec4 */
typedef vec_t vec4_t[4];

/* idTech 3 boolean constants; the engine q_shared declares qboolean without them */
#define qtrue  1
#define qfalse 0

/* color prefixes as in the idTech 3 shared code; the engine prints them raw */
#define S_COLOR_RED      "^1"
#define S_COLOR_YELLOW   "^3"

#ifndef M_PI
#define M_PI        3.14159265358979323846f
#endif

/* refdef flags; the RF_ bits sit far away from the engine Q2 RF_ range */
/* guarded: some engine headers already define this flag */
#ifndef RDF_NOWORLDMODEL
#define RDF_NOWORLDMODEL    1
#endif
#define RF_LIGHTING_ORIGIN  0x20000000
#define RF_NOSHADOW         0x40000000

typedef struct {
    int reType;             /* refEntityType_t, only menu models later */
    int renderfx;
    qhandle_t hModel;
    vec3_t lightingOrigin;
    vec3_t axis[3];
    float origin[3];
    float oldorigin[3];
    int frame;
    int oldframe;
    float backlerp;
} refEntity_t;

typedef struct {
    int x, y, width, height;
    float fov_x, fov_y;
    vec3_t vieworg;
    vec3_t viewaxis[3];
    int time;
    int rdflags;
} refdef_t;

typedef struct {
    int vidWidth, vidHeight;
    float windowAspect;
} glconfig_t;

/* idTech 3 font records; the Duke menus draw through the MenuText
   ownerdraw instead, so these stay empty until real fonts land */
#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPHS_PER_FONT ( GLYPH_END - GLYPH_START + 1 )

typedef struct {
    int height;
    int top;
    int bottom;
    int pitch;
    int xSkip;
    int imageWidth;
    int imageHeight;
    float s;
    float t;
    float s2;
    float t2;
    qhandle_t glyph;
    char shaderName[32];
} glyphInfo_t;

typedef struct {
    glyphInfo_t glyphs[GLYPHS_PER_FONT];
    float glyphScale;
    char name[MAX_QPATH];
} fontInfo_t;

/* idTech 3 tokenizer token; the .menu parser sits behind stubs because the
   Duke menus are code-defined, but the framework types stay in place */
#define MAX_TOKENLENGTH 1024
#define TT_STRING       1
#define TT_LITERAL      2
#define TT_NUMBER       3
#define TT_NAME         4
#define TT_PUNCTUATION  5

typedef struct {
    int type;
    int subtype;
    int intvalue;
    float floatvalue;
    char string[MAX_TOKENLENGTH];
} pc_token_t;

/* clears a 3x3 rotation matrix */
inline void AxisClear( vec3_t axis[3] ) {
    axis[0][0] = axis[0][1] = axis[0][2] = 0;
    axis[1][0] = axis[1][1] = axis[1][2] = 0;
    axis[2][0] = axis[2][1] = axis[2][2] = 0;
}

#endif
