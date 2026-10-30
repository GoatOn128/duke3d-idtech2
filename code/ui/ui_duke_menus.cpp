/* Duke menu glue on the idTech 3 menu framework. The menus themselves
   load from the .menu script files under ui/ so they stay moddable game
   data; this file owns the Duke-styled line ownerdraws, the module state
   (episode choice, pending skill voice, the sound registry) and the
   engine-facing entry points. */

#include "ui_local.h"

/* ---- ownerdraw ids ---- */

/* two standalone tiles; line ownerdraws index the line table below */
#define DUKE_OD_MAIN_LOGO   0
#define DUKE_OD_GAME_HEADER 1
/* the options and controls screens ride the same bar higher: their own
   header placement in the original menues.c (MENUBAR at y 10) */
#define DUKE_OD_GAME_HEADER_HIGH 2
#define DUKE_OD_LINE_BASE   16

/* every ownerdraw line in the menus, in dukeLines order */
enum {
    DL_MAIN_NEW,
    DL_MAIN_SAVE,
    DL_MAIN_LOAD,
    DL_MAIN_OPTIONS,
    DL_MAIN_MULTI,
    DL_MAIN_VIDEO,
    DL_MAIN_QUIT,

    DL_EPISODE_TITLE,
    DL_EPISODE_0,
    DL_EPISODE_1,
    DL_EPISODE_2,
    DL_EPISODE_3,

    DL_SKILL_TITLE,
    DL_SKILL_0,
    DL_SKILL_1,
    DL_SKILL_2,
    DL_SKILL_3,

    DL_QUIT_TITLE,
    DL_QUIT_YES,
    DL_QUIT_NO,

    DL_OPTIONS_TITLE,
    DL_OPTIONS_CONTROLS,
    DL_OPTIONS_DEFAULTS,
    DL_OPTIONS_CONSOLE,

    DL_KEYS_TITLE,

    DL_COUNT
};

typedef struct {
    const char *label;
    int x;                  /* Duke menu x, 160 means centered */
    int y;                  /* Duke menu y, the glyph top is y - 12 */
    int bright;             /* always drawn in the selected shade */
    int cursorFromStart;    /* cursor sits 18 pixels left of the text start */
    int cursorX;            /* fixed cursor x when cursorFromStart is 0 */
    int cursorBothSides;    /* centered lines carry the nuke pair at fixed columns */
} dukeLine_t;

static dukeLine_t dukeLines[DL_COUNT] = {
    {"NEW GAME",           160, 67,  0, 1, 0,  1},
    {"SAVE GAME",          160, 83,  0, 1, 0,  1},
    {"LOAD GAME",          160, 99,  0, 1, 0,  1},
    {"OPTIONS",            160, 115, 0, 1, 0,  1},
    {"MULTIPLAYER",        160, 131, 0, 1, 0,  1},
    {NULL,                 0,   0,   0, 0, 0,  0},
    {"QUIT GAME",          160, 163, 0, 1, 0,  1},

    {"SELECT AN EPISODE",  160, 24,  1, 0, 0,  0},
    {"L.A. MELTDOWN",      160, 70,  0, 1, 0,  1},
    {"LUNAR APOCALYPSE",   160, 90,  0, 1, 0,  1},
    {"SHRAPNEL CITY",      160, 110, 0, 1, 0,  1},
    {"THE BIRTH",          160, 130, 0, 1, 0,  1},

    {"SELECT SKILL",       160, 24,  1, 0, 0,  0},
    {"PIECE OF CAKE",      160, 70,  0, 1, 0,  1},
    {"LET'S ROCK",         160, 90,  0, 1, 0,  1},
    {"COME GET SOME",      160, 110, 0, 1, 0,  1},
    {"DAMN I'M GOOD",      160, 130, 0, 1, 0,  1},

    {"QUIT GAME?",         160, 90,  1, 0, 0,  0},
    {"YES",                160, 120, 0, 0, 115, 0},
    {"NO",                 160, 145, 0, 0, 115, 0},

    {"OPTIONS",            160, 15,  1, 0, 0,  0},
    {"CUSTOMIZE CONTROLS", 160, 181, 0, 1, 0,  1},
    {"RESET DEFAULTS",     160, 195, 0, 1, 0,  1},
    {NULL,                 0,   0,   0, 0, 0,  0},

    {"CUSTOMIZE CONTROLS", 160, 15,  1, 0, 0,  0},
};

/* the skill voice that plays before the level loads, per skill choice */
static const char *dukeSkillVoices[4] = {"piece02.wav", "letsrk03.wav", "getsom1a.wav", "imgood12.wav"};

static char s_pendingVoice[64];

/* ---- sound registry ---- */

/* the engine import plays local sounds by name; the framework hands out
   sfxHandle_t cookies, so cache the names behind them */
#define MAX_DUKE_SOUNDS 8

typedef struct {
    char name[64];
} dukeSound_t;

static dukeSound_t dukeSounds[MAX_DUKE_SOUNDS];
static int dukeSoundCount;

sfxHandle_t Duke_RegisterSound( const char *name ) {
    int i;

    if ( !name || !name[0] ) {
        return NULL;
    }
    for ( i = 0; i < dukeSoundCount; i++ ) {
        if ( Q_strcasecmp( dukeSounds[i].name, name ) == 0 ) {
            return ( dukeSound_t * )&dukeSounds[i];
        }
    }
    if ( dukeSoundCount >= MAX_DUKE_SOUNDS ) {
        return NULL;
    }
    Q_strncpyz( dukeSounds[dukeSoundCount].name, name, sizeof( dukeSounds[0].name ) );
    return ( dukeSound_t * )&dukeSounds[dukeSoundCount++];
}

void Duke_PlaySound( sfxHandle_t sfx ) {
    dukeSound_t *sound = ( dukeSound_t * )sfx;

    if ( sound ) {
        uii.StartLocalSound( sound->name );
    }
}

/* ---- ownerdraws ---- */

/* the focused item of the focused menu, used to light the lines */
static qboolean Duke_LineFocused( int ownerDraw ) {
    dnMenu *menu = g_menus.GetFocused();
    dnItem *item = menu ? menu->GetFocusedItem() : NULL;

    return ( item != NULL && item->window.ownerDraw == ownerDraw ) ? qtrue : qfalse;
}

void Duke_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color ) {
    dukeLine_t *line;
    int selected, start;

    /* the framework passes the virtual rect but the lines draw straight
       through the engine Duke helpers in Duke menu coordinates */
    ( void )x;
    ( void )y;
    ( void )w;
    ( void )h;
    ( void )color;

    /* the second menu set owns its own ownerdraw ids */
    if ( ownerDraw >= DUKE2_OD_BASE ) {
        Duke2_OwnerDraw( ownerDraw, x, y, w, h, color );
        return;
    }

    if ( ownerDraw == DUKE_OD_MAIN_LOGO ) {
        uii.UiTile( 160, 28, 2499, 0, 0, 0 );
        return;
    }
    if ( ownerDraw == DUKE_OD_GAME_HEADER ) {
        uii.UiTile( 160, 19, 2457, 16, 0, 0 );
        return;
    }
    if ( ownerDraw == DUKE_OD_GAME_HEADER_HIGH ) {
        uii.UiTile( 160, 10, 2457, 16, 0, 0 );
        return;
    }
    if ( ownerDraw < DUKE_OD_LINE_BASE || ownerDraw - DUKE_OD_LINE_BASE >= DL_COUNT ) {
        return;
    }

    line = &dukeLines[ownerDraw - DUKE_OD_LINE_BASE];
    if ( line->label == NULL ) {
        return;
    }
    selected = line->bright || Duke_LineFocused( ownerDraw );
    start = uii.MenuText( line->x, line->y, line->label, selected );
    if ( !line->bright && selected ) {
        if ( line->cursorBothSides && line->x == 160 ) {
            /* centered menus park the nuke pair at the fixed columns like
               the original probe(): the left one spins forward and the
               right one mirrors it, instead of hugging the line's text */
            int frame = ( uii.Milliseconds() / 66 ) % 7;
            uii.UiTile( DUKE_NUKE_LEFT_X, line->y - 7, DUKE_SPINNING_NUKE + frame, 0, 0, 0 );
            uii.UiTile( DUKE_NUKE_RIGHT_X, line->y - 7, DUKE_SPINNING_NUKE + ( 7 - frame ) % 7, 0, 0, 0 );
        } else if ( line->cursorFromStart ) {
            uii.MenuCursor( start - 18, line->y - 7 );
        } else {
            uii.MenuCursor( line->cursorX, line->y - 7 );
        }
    }
}

/* ---- module entry points ---- */

void Duke_MenusInit( void ) {
    /* module-owned episode state and the save-menu gate; the menus
       themselves are parsed from the ui/ .menu script files after this */
    uii.Cvar_Register( "dn_episode", "1", 0 );
    uii.Cvar_Register( "dn_skill_choice", "0", 0 );
    uii.Cvar_Register( "dn_serveractive", "0", 0 );

    Duke_Menus2Init();
}

/* start the chosen episode like the engine StartGame in menu.cpp */
static void Duke_StartGame( void ) {
    char command[128];
    int episode;

    episode = ( int )uii.Cvar_Value( "dn_episode" );
    if ( episode < 1 || episode > 4 ) {
        episode = 1;
    }

    g_menus.CloseAll();
    uii.Cvar_SetValue( "deathmatch", 0 );
    uii.Cvar_SetValue( "coop", 0 );
    uii.Cvar_SetValue( "gamerules", 0 );
    snprintf( command, sizeof( command ), "loading ; killserver ; wait ; map e%dl1\n", episode );
    uii.ExecuteText( DN_EXEC_APPEND, command );
}

void Duke_Refresh( void ) {
    dnMenu *keys;
    /* the screen that last held focus: a new name means the screens
       switched and the new one slides up from below */
    static char slideMenu[64] = "";

    {
        dnMenu *focused = g_menus.GetFocused();
        if ( focused && focused->window.name && focused->window.name[0] ) {
            if ( Q_strcasecmp( slideMenu, focused->window.name ) != 0 ) {
                /* the slide is the screen to screen transition: a screen
                   appearing from nothing, like the main menu after the
                   intro video, draws already in place */
                if ( slideMenu[0] ) {
                    Duke_SlideBegin();
                }
                Q_strncpyz( slideMenu, focused->window.name, sizeof( slideMenu ) );
            }
        } else {
            slideMenu[0] = '\0';
        }
    }

    /* mirror the connection state so the main menu can gate SAVE GAME */
    uii.Cvar_Set( "dn_serveractive", uii.GetConnState() == DN_CONN_ACTIVE ? "1" : "0" );

    /* the skill voice must finish before the level loads, like menu.cpp */
    {
        int choice = ( int )uii.Cvar_Value( "dn_skill_choice" );
        if ( choice >= 1 && choice <= 4 ) {
            Q_strncpyz( s_pendingVoice, dukeSkillVoices[choice - 1], sizeof( s_pendingVoice ) );
            uii.Cvar_Set( "dn_skill_choice", "0" );
        }
    }
    if ( s_pendingVoice[0] && !uii.IsLocalSoundPlaying( s_pendingVoice ) ) {
        s_pendingVoice[0] = 0;
        Duke_StartGame();
    }

    /* keep the bind list current while the keys menu is up */
    keys = g_menus.FindByName( "keys" );
    if ( keys && ( keys->window.flags & WINDOW_VISIBLE ) ) {
        Controls_GetConfig();
    }

    Duke2_Refresh();

    if ( g_menus.AnyFullScreenVisible() ) {
        /* the old engine dims the running game behind in-game menus and
           otherwise draws the menu background tile */
        if ( uii.GetConnState() == DN_CONN_ACTIVE ) {
            uii.FadeScreen();
        } else {
            uii.MenuBackground();
        }
        g_menus.PaintAll();
    }
}

void Duke_KeyEvent( int key, int down ) {
    displayContextDef_t *dc = Display_GetContext();
    dnMenu *menu;

    /* the engine menus treat right click as escape, but the bind capture
       must hear the real key so mouse2 can be bound */
    if ( key == K_MOUSE2 && !Display_KeyBindPending() ) {
        key = K_ESCAPE;
    }

    /* the skill voice plays out before the game starts: swallow input */
    if ( s_pendingVoice[0] ) {
        return;
    }

    menu = g_menus.GetFocused();
    if ( down && key == K_ESCAPE && menu && menu->window.name &&
         Q_strcasecmp( menu->window.name, "main" ) == 0 ) {
        if ( uii.GetConnState() == DN_CONN_ACTIVE ) {
            /* leave the menu and go back to the game */
            uii.MenuSound( DN_UISOUND_BACK );
            g_menus.CloseAll();
        }
        /* stay on the main menu when no game is running */
        return;
    }

    g_menus.HandleKey( key, down, dc->cursorx, dc->cursory );
}

void Duke_MouseEvent( int x, int y ) {
    displayContextDef_t *dc = Display_GetContext();
    int width = 640, height = 480;

    uii.GetVideoMode( &width, &height );
    if ( width <= 0 ) {
        width = 640;
    }
    if ( height <= 0 ) {
        height = 480;
    }

    /* absolute window coordinates arrive, the framework works in the
       640x480 virtual menu space */
    dc->cursorx = x * SCREEN_WIDTH / width;
    dc->cursory = y * SCREEN_HEIGHT / height;
    if ( dc->cursorx < 0 ) {
        dc->cursorx = 0;
    }
    if ( dc->cursorx > SCREEN_WIDTH - 1 ) {
        dc->cursorx = SCREEN_WIDTH - 1;
    }
    if ( dc->cursory < 0 ) {
        dc->cursory = 0;
    }
    if ( dc->cursory > SCREEN_HEIGHT - 1 ) {
        dc->cursory = SCREEN_HEIGHT - 1;
    }
    g_menus.MouseMove( NULL, dc->cursorx, dc->cursory );
}

void Duke_SetActiveMenu( dn_uimenu_t menu ) {
    g_menus.CloseAll();
    if ( menu != DN_UIMENU_NONE ) {
        g_menus.OpenByName( "main" );
    }
}

/* engine console commands open a module menu by name */
void Duke_OpenMenu( const char *name ) {
    g_menus.CloseAll();
    if ( name && name[0] ) {
        g_menus.OpenByName( name );
    }
}

int Duke_IsFullscreen( void ) {
    return g_menus.AnyFullScreenVisible() ? 1 : 0;
}
