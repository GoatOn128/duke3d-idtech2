/* Module-private UI declarations. The whole module calls the engine
   import table directly through uii (like the cgame module with cg); there
   is no trap wrapper layer. */
#ifndef DN_UI_LOCAL_H
#define DN_UI_LOCAL_H
#include "ui.h"
#include "ui_shared.h"

extern dn_ui_import_t uii;

/* idTech 3 menu constants the framework code references (from the RTCW ui_local) */
#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

/* the old hard-coded behavior values moved to ui/config.txt, read into
   dnMenuParser::Config: sound channel, focus pulse and blink, savegame
   info line length; the file is the only source */
void    UI_LoadBindList( const char *file );
float   UI_PulseDivisor( void );
int     UI_BlinkDivisor( void );

/* module-local replacements for the idTech 3 shared helpers the menu
   framework expects but the engine Q2 q_shared lacks; the .menu parser
   in qcommon/dnmenuparser.cpp reaches these through this header too */
void    UI_Printf( const char *fmt, ... );
void    Q_strncpyz( char *dest, const char *src, int destsize );
void    Q_strcat( char *dest, int size, const char *src );
char    *Q_strupr( char *s1 );
int     Q_strncmp( const char *s1, const char *s2, int n );
int     Q_isalpha( int c );
int     Q_isupper( int c );
int     Q_isnumeric( int c );
int     Q_isforfilename( int c );
char    *COM_ParseExt( char **data_p, qboolean allowLineBreak );
void    AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

/* Duke menus coded on the framework (ui_duke_menus.cpp) */
void        Duke_MenusInit( void );
void        Duke_Refresh( void );
void        Duke_KeyEvent( int key, int down );
void        Duke_MouseEvent( int x, int y );
void        Duke_SetActiveMenu( dn_uimenu_t menu );
void        Duke_OpenMenu( const char *name );
int         Duke_IsFullscreen( void );
void        Duke_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color );
sfxHandle_t Duke_RegisterSound( const char *name );
void        Duke_PlaySound( sfxHandle_t sfx );

/* the ownerdraw line rows and tiles paint against the item rect, mapped
   from the 640x480 virtual menu space into the 320x200 Duke menu grid;
   the line tables below then only carry the look, not the placement */
typedef struct {
    const char *label;
    int bright;             /* always drawn in the selected shade */
    int leftAnchor;         /* text sits on the rect left edge, not its center */
    int cursorFromStart;    /* cursor sits 18 pixels left of the text start */
    int cursorX;            /* fixed cursor x when cursorFromStart is 0 */
    int cursorBothSides;    /* centered rows carry the nuke pair at fixed columns */
} dukeLine_t;

typedef struct {
    int leftX;              /* rect left edge in Duke menu space */
    int centerX;            /* rect center in Duke menu space, 160 means screen center */
    int rightX;             /* rect right edge in Duke menu space */
    int topY;               /* glyph anchor, already carries the +12 baseline shift */
} dukeRect_t;

dukeRect_t Duke_MapRect( float x, float y, float w, float h );
void       Duke_PaintLine( const dukeLine_t *line, const dukeRect_t *rect, int selected );
void        Duke_SlideBegin( void );
void        Duke_SetSlideMode( int mode );

/* menu screen slide directions: the root menu script picks the screen
   change direction through the slide keyword, left to right is the
   classic Duke3D screen change */
#define DUKE_SLIDE_NONE  0
#define DUKE_SLIDE_LEFT  1
#define DUKE_SLIDE_UP    2

/* the Duke tiles and motion parameters (nuke cursor spin, screen slide)
   come from the parser data tables: dnMenuParser::Tile and
   dnMenuParser::Anims; the tables start empty, ui/resources.txt and
   ui/animations.txt are the only source */

/* Second Duke menu set (ui_duke_menus2.cpp): the menus live in .menu
   script files under ui/, the module owns the ownerdraws, the engine data
   readers and the custom script commands */
void    Duke_Menus2Init( void );
void    Duke_MenusAttachDynamic( void );
void    Duke2_Refresh( void );
void    Duke2_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color );
qboolean Duke_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key );
void    Duke_RunScript( char **args );

/* ownerdraw ids at or above this belong to ui_duke_menus2.cpp */
#define DUKE2_OD_BASE 64

/* engine CVAR_ARCHIVE without pulling the engine qcommon header in */
#define DN_CVAR_ARCHIVE 1

#endif
