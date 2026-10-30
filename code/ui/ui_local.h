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
#define CHAN_LOCAL_SOUND 5          /* channel the idTech 3 menus pass to startLocalSound */
#define PULSE_DIVISOR   75.0f       /* focus text pulse rate */
#define BLINK_DIVISOR   250         /* focus text blink switch in ms */
#define SAVE_INFOSTRING_LENGTH 256   /* savegame info line in the load/save feeders */

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
   dnMenuParser::Anims, seeded with built-in defaults and overridable
   from ui/resources.txt and ui/animations.txt */

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
