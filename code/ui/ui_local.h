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
#define PULSE_DIVISOR   75.0f        /* focus text pulse rate */
#define BLINK_DIVISOR   250          /* focus text blink switch in ms */
#define SAVE_INFOSTRING_LENGTH 256   /* savegame info line in the load/save feeders */

/* module-local replacements for the idTech 3 shared helpers the menu
   framework expects but the engine Q2 q_shared lacks */
void    UI_Printf( const char *fmt, ... );
int     UI_PC_ReadToken( int handle, pc_token_t *token );
int     UI_PC_SourceFileAndLine( int handle, char *filename, int *line );
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

/* framework helper without a ui_shared.h prototype */
itemDef_t *Menu_GetFocusedItem( menuDef_t *menu );

/* Duke menus coded on the framework (ui_duke_menus.cpp) */
void        Duke_MenusInit( void );
void        Duke_Refresh( void );
void        Duke_KeyEvent( int key, int down );
void        Duke_MouseEvent( int dx, int dy );
void        Duke_SetActiveMenu( dn_uimenu_t menu );
int         Duke_IsFullscreen( void );
void        Duke_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color );
sfxHandle_t Duke_RegisterSound( const char *name );
void        Duke_PlaySound( sfxHandle_t sfx );

/* Second Duke menu set: save/load, multiplayer, video, credits and
   player setup coded on the framework (ui_duke_menus2.cpp) */
void    Duke_Menus2Init( void );
void    Duke2_Refresh( void );
void    Duke2_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color );
qboolean Duke_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key );
void    Duke_RunScript( char **args );

/* ownerdraw ids at or above this belong to ui_duke_menus2.cpp */
#define DUKE2_OD_BASE 64

/* engine CVAR_ARCHIVE without pulling the engine qcommon header in */
#define DN_CVAR_ARCHIVE 1

#endif
