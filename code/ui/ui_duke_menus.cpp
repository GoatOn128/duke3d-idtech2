/* Duke menus coded directly on the idTech 3 menu framework instead of
   parsed .menu files. The framework owns focus, navigation and hit
   testing; every Duke-styled line is an ownerdraw item that paints through
   the engine menu glyph helpers, reproducing the menu.cpp look. The
   options and customize controls menus keep the engine item set but draw
   in Duke glyphs. */

#include "ui_local.h"

/* ---- ownerdraw ids ---- */

/* two standalone tiles; line ownerdraws index the line table below */
#define DUKE_OD_MAIN_LOGO   0
#define DUKE_OD_GAME_HEADER 1
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
} dukeLine_t;

static dukeLine_t dukeLines[DL_COUNT] = {
    {"NEW GAME",           80,  67,  0, 0, 56},
    {"SAVE GAME",          80,  83,  0, 0, 56},
    {"LOAD GAME",          80,  99,  0, 0, 56},
    {"OPTIONS",            80,  115, 0, 0, 56},
    {"MULTIPLAYER",        80,  131, 0, 0, 56},
    {"VIDEO SETUP",        80,  147, 0, 0, 56},
    {"QUIT GAME",          80,  163, 0, 0, 56},

    {"SELECT AN EPISODE",  160, 24,  1, 0, 0},
    {"L.A. MELTDOWN",      160, 70,  0, 1, 0},
    {"LUNAR APOCALYPSE",   160, 90,  0, 1, 0},
    {"SHRAPNEL CITY",      160, 110, 0, 1, 0},
    {"THE BIRTH",          160, 130, 0, 1, 0},

    {"SELECT SKILL",       160, 24,  1, 0, 0},
    {"PIECE OF CAKE",      160, 70,  0, 1, 0},
    {"LET'S ROCK",         160, 90,  0, 1, 0},
    {"COME GET SOME",      160, 110, 0, 1, 0},
    {"DAMN I'M GOOD",      160, 130, 0, 1, 0},

    {"QUIT GAME?",         160, 90,  1, 0, 0},
    {"YES",                160, 120, 0, 0, 115},
    {"NO",                 160, 145, 0, 0, 115},

    {"OPTIONS",            160, 16,  1, 0, 0},
    {"CUSTOMIZE CONTROLS", 40,  200, 0, 1, 0},
    {"RESET DEFAULTS",     40,  214, 0, 1, 0},
    {"GO TO CONSOLE",      40,  228, 0, 1, 0},

    {"CUSTOMIZE CONTROLS", 160, 12,  1, 0, 0},
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
    menuDef_t *menu = Menu_GetFocused();
    itemDef_t *item = Menu_GetFocusedItem( menu );

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

    if ( ownerDraw == DUKE_OD_MAIN_LOGO ) {
        uii.UiTile( 160, 28, 2499, 0, 0, 0 );
        return;
    }
    if ( ownerDraw == DUKE_OD_GAME_HEADER ) {
        uii.UiTile( 160, 19, 2457, 16, 0, 0 );
        return;
    }
    if ( ownerDraw < DUKE_OD_LINE_BASE || ownerDraw - DUKE_OD_LINE_BASE >= DL_COUNT ) {
        return;
    }

    line = &dukeLines[ownerDraw - DUKE_OD_LINE_BASE];
    selected = line->bright || Duke_LineFocused( ownerDraw );
    start = uii.MenuText( line->x, line->y, line->label, selected );
    if ( !line->bright && selected ) {
        if ( line->cursorFromStart ) {
            uii.MenuCursor( start - 18, line->y - 7 );
        } else {
            uii.MenuCursor( line->cursorX, line->y - 7 );
        }
    }
}

/* ---- item builders ---- */

/* decoration item: paints through an ownerdraw, never focused or hit */
static void Duke_AddDeco( menuDef_t *menu, int ownerDraw ) {
    itemDef_t *item = Menu_AddItem( menu, NULL );

    if ( item == NULL ) {
        return;
    }
    item->type = ITEM_TYPE_OWNERDRAW;
    item->window.ownerDraw = ownerDraw;
    item->window.flags |= WINDOW_VISIBLE | WINDOW_DECORATION;
}

/* selectable line item: the rect is the hit zone in 640x480 virtual
   coordinates, the ownerdraw paints the line itself */
static itemDef_t *Duke_AddLine( menuDef_t *menu, int lineId, const char *name,
                                float x, float y, float w, float h, const char *action ) {
    itemDef_t *item = Menu_AddItem( menu, name );

    if ( item == NULL ) {
        return NULL;
    }
    item->type = ITEM_TYPE_OWNERDRAW;
    item->window.ownerDraw = DUKE_OD_LINE_BASE + lineId;
    item->window.rect.x = x;
    item->window.rect.y = y;
    item->window.rect.w = w;
    item->window.rect.h = h;
    item->window.flags |= WINDOW_VISIBLE;
    item->action = action;
    return item;
}

/* label plus value item (MULTI, YESNO or BIND) drawn with the framework
   text painting in Duke glyphs */
static itemDef_t *Duke_AddSetting( menuDef_t *menu, int type, const char *text,
                                   const char *cvar, const char *name,
                                   float x, float y, float w, float h ) {
    itemDef_t *item = Menu_AddItem( menu, name );

    if ( item == NULL ) {
        return NULL;
    }
    item->type = type;
    item->text = text;
    item->cvar = cvar;
    item->textscale = 1.0f;
    item->window.rect.x = x;
    item->window.rect.y = y;
    item->window.rect.w = w;
    item->window.rect.h = h;
    item->window.flags |= WINDOW_VISIBLE;
    item->window.foreColor[0] = 0.45f;
    item->window.foreColor[1] = 0.45f;
    item->window.foreColor[2] = 0.45f;
    item->window.foreColor[3] = 1.0f;
    return item;
}

/* fill a MULTI item with numeric settings */
static void Duke_SetMulti( itemDef_t *item, const char **labels, const float *values, int count ) {
    multiDef_t *multi;
    int i;

    if ( item == NULL ) {
        return;
    }
    multi = ( multiDef_t * )UI_Alloc( sizeof( multiDef_t ) );
    if ( multi == NULL ) {
        return;
    }
    memset( multi, 0, sizeof( multiDef_t ) );
    if ( count > MAX_MULTI_CVARS ) {
        count = MAX_MULTI_CVARS;
    }
    for ( i = 0; i < count; i++ ) {
        multi->cvarList[i] = labels[i];
        multi->cvarValue[i] = values[i];
    }
    multi->count = count;
    multi->strDef = qfalse;
    item->typeData = multi;
}

/* every menu uses the bright-white focused look the Duke cursor lines use */
static void Duke_MenuSetup( menuDef_t *menu ) {
    menu->fullScreen = 1;
    menu->focusColor[0] = 1.0f;
    menu->focusColor[1] = 1.0f;
    menu->focusColor[2] = 1.0f;
    menu->focusColor[3] = 1.0f;
}

/* ---- menus ---- */

static void Duke_Menu_Main( void ) {
    static const char *names[7] = {"main_new", "main_save", "main_load", "main_options",
                                   "main_multi", "main_video", "main_quit"};
    static const char *actions[7] = {
        "play kickhit.wav ; close main ; open game_episodes",
        "play kickhit.wav ; close main ; exec menu_savegame",
        "play kickhit.wav ; close main ; exec menu_loadgame",
        "play kickhit.wav ; close main ; open options",
        "play kickhit.wav ; close main ; exec menu_multiplayer",
        "play kickhit.wav ; close main ; exec menu_video",
        "play kickhit.wav ; close main ; open quit"
    };
    menuDef_t *menu = Menu_Alloc( "main" );
    itemDef_t *item;
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus main_new";

    Duke_AddDeco( menu, DUKE_OD_MAIN_LOGO );
    for ( i = 0; i < 7; i++ ) {
        item = Duke_AddLine( menu, DL_MAIN_NEW + i, names[i], 144, 106 + i * 32, 416, 32, actions[i] );
        if ( item && i == 1 ) {
            /* the engine only offers saving with a server running */
            item->cvarTest = "dn_serveractive";
            item->enableCvar = "1";
            item->cvarFlags = CVAR_DISABLE;
        }
    }
    Menu_PostParse( menu );
}

static void Duke_Menu_Episodes( void ) {
    static const char *actions[4] = {
        "play kickhit.wav ; setcvar dn_episode 1 ; close game_episodes ; open game_skills",
        "play kickhit.wav ; setcvar dn_episode 2 ; close game_episodes ; open game_skills",
        "play kickhit.wav ; setcvar dn_episode 3 ; close game_episodes ; open game_skills",
        "play kickhit.wav ; setcvar dn_episode 4 ; close game_episodes ; open game_skills"
    };
    static const char *names[4] = {"episode_0", "episode_1", "episode_2", "episode_3"};
    menuDef_t *menu = Menu_Alloc( "game_episodes" );
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus episode_0";
    menu->onESC = "play item15.wav ; close game_episodes ; open main";

    Duke_AddDeco( menu, DUKE_OD_GAME_HEADER );
    Duke_AddDeco( menu, DUKE_OD_LINE_BASE + DL_EPISODE_TITLE );
    for ( i = 0; i < 4; i++ ) {
        Duke_AddLine( menu, DL_EPISODE_0 + i, names[i], 70, 112 + i * 40, 500, 40, actions[i] );
    }
    Menu_PostParse( menu );
}

static void Duke_Menu_Skills( void ) {
    /* the action strings are formatted once at build time so the script
       pointers stay valid */
    static char skillActions[4][256];
    static const char *names[4] = {"skill_0", "skill_1", "skill_2", "skill_3"};
    menuDef_t *menu = Menu_Alloc( "game_skills" );
    int i;

    if ( menu == NULL ) {
        return;
    }

    for ( i = 0; i < 4; i++ ) {
        snprintf( skillActions[i], sizeof( skillActions[0] ),
                  "play kickhit.wav ; setcvar skill %d ; setcvar respawn_monsters %d ; setcvar dn_skill_choice %d ; play %s",
                  i, i == 3 ? 1 : 0, i + 1, dukeSkillVoices[i] );
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus skill_0";
    menu->onESC = "play item15.wav ; close game_skills ; open game_episodes";

    Duke_AddDeco( menu, DUKE_OD_GAME_HEADER );
    Duke_AddDeco( menu, DUKE_OD_LINE_BASE + DL_SKILL_TITLE );
    for ( i = 0; i < 4; i++ ) {
        Duke_AddLine( menu, DL_SKILL_0 + i, names[i], 70, 112 + i * 40, 500, 40, skillActions[i] );
    }
    Menu_PostParse( menu );
}

static void Duke_Menu_Quit( void ) {
    menuDef_t *menu = Menu_Alloc( "quit" );

    if ( menu == NULL ) {
        return;
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus quit_no";
    menu->onESC = "play item15.wav ; close quit ; open main";

    Duke_AddDeco( menu, DUKE_OD_LINE_BASE + DL_QUIT_TITLE );
    Duke_AddLine( menu, DL_QUIT_YES, "quit_yes", 250, 212, 140, 40,
                  "play kickhit.wav ; exec quit" );
    Duke_AddLine( menu, DL_QUIT_NO, "quit_no", 250, 262, 140, 40,
                  "play item15.wav ; close quit ; open main" );
    Menu_PostParse( menu );
}

static void Duke_Menu_Options( void ) {
    static const char *volLabels[11] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    static const float volValues[11] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    static const char *crossLabels[4] = {"none", "cross", "dot", "angle"};
    static const float crossValues[4] = {0.0f, 1.0f, 2.0f, 3.0f};
    static const char *invertLabels[2] = {"no", "yes"};
    static const float invertValues[2] = {0.022f, -0.022f};
    /* mouse sensitivity runs 1.0 to 11.0 in 0.5 steps like the engine slider */
    static char sensLabels[21][8];
    static float sensValues[21];
    menuDef_t *menu = Menu_Alloc( "options" );
    itemDef_t *item;
    int i;

    if ( menu == NULL ) {
        return;
    }

    for ( i = 0; i < 21; i++ ) {
        snprintf( sensLabels[i], sizeof( sensLabels[0] ), "%.1f", 1.0f + 0.5f * i );
        sensValues[i] = 1.0f + 0.5f * i;
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus opt_sfx";
    menu->onESC = "play item15.wav ; close options ; open main";

    Duke_AddDeco( menu, DUKE_OD_LINE_BASE + DL_OPTIONS_TITLE );

    item = Duke_AddSetting( menu, ITEM_TYPE_MULTI, "effects volume", "s_volume", "opt_sfx", 80, 64, 480, 24 );
    Duke_SetMulti( item, volLabels, volValues, 11 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "menu music", "s_menu_music", "opt_menumusic", 80, 92, 480, 24 );
    item = Duke_AddSetting( menu, ITEM_TYPE_MULTI, "music volume", "s_musicvolume", "opt_musicvol", 80, 120, 480, 24 );
    Duke_SetMulti( item, volLabels, volValues, 11 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "music", "s_music", "opt_music", 80, 148, 480, 24 );
    item = Duke_AddSetting( menu, ITEM_TYPE_MULTI, "mouse speed", "sensitivity", "opt_sens", 80, 176, 480, 24 );
    Duke_SetMulti( item, ( const char ** )sensLabels, sensValues, 21 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "always run", "cl_run", "opt_run", 80, 204, 480, 24 );
    item = Duke_AddSetting( menu, ITEM_TYPE_MULTI, "invert mouse", "m_pitch", "opt_invert", 80, 232, 480, 24 );
    Duke_SetMulti( item, invertLabels, invertValues, 2 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "lookspring", "lookspring", "opt_lookspring", 80, 260, 480, 24 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "lookstrafe", "lookstrafe", "opt_lookstrafe", 80, 288, 480, 24 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "freelook", "freelook", "opt_freelook", 80, 316, 480, 24 );
    item = Duke_AddSetting( menu, ITEM_TYPE_MULTI, "crosshair", "crosshair", "opt_crosshair", 80, 344, 480, 24 );
    Duke_SetMulti( item, crossLabels, crossValues, 4 );
    Duke_AddSetting( menu, ITEM_TYPE_YESNO, "use joystick", "in_joystick", "opt_joystick", 80, 372, 480, 24 );

    Duke_AddLine( menu, DL_OPTIONS_CONTROLS, "opt_controls", 80, 400, 480, 24,
                  "play kickhit.wav ; close options ; open keys" );
    Duke_AddLine( menu, DL_OPTIONS_DEFAULTS, "opt_defaults", 80, 428, 480, 24,
                  "play kickhit.wav ; exec default.cfg" );
    Duke_AddLine( menu, DL_OPTIONS_CONSOLE, "opt_console", 80, 456, 480, 24,
                  "play kickhit.wav ; close options ; exec toggleconsole" );
    Menu_PostParse( menu );
}

static void Duke_Menu_Keys( void ) {
    /* the engine customize controls list: label plus bound command */
    static const struct {
        const char *label;
        const char *command;
    } binds[23] = {
        {"attack",         "+attack"},
        {"next weapon",    "weapnext"},
        {"walk forward",   "+forward"},
        {"backpedal",      "+back"},
        {"turn left",      "+left"},
        {"turn right",     "+right"},
        {"run",            "+speed"},
        {"step left",      "+moveleft"},
        {"step right",     "+moveright"},
        {"sidestep",       "+strafe"},
        {"look up",        "+lookup"},
        {"look down",      "+lookdown"},
        {"center view",    "centerview"},
        {"mouse look",     "+mlook"},
        {"keyboard look",  "+klook"},
        {"up/jump",        "+moveup"},
        {"down/crouch",    "+movedown"},
        {"inventory",      "inven"},
        {"use item",       "invuse"},
        {"drop item",      "invdrop"},
        {"prev item",      "invprev"},
        {"next item",      "invnext"},
        {"help computer",  "cmd help"}
    };
    menuDef_t *menu = Menu_Alloc( "keys" );
    char name[16];
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke_MenuSetup( menu );
    menu->onOpen = "setfocus key_0";
    menu->onESC = "play item15.wav ; close keys ; open options";

    Duke_AddDeco( menu, DUKE_OD_LINE_BASE + DL_KEYS_TITLE );
    for ( i = 0; i < 23; i++ ) {
        snprintf( name, sizeof( name ), "key_%d", i );
        Duke_AddSetting( menu, ITEM_TYPE_BIND, binds[i].label, binds[i].command, name,
                         48, 40 + i * 18, 544, 16 );
    }
    Menu_PostParse( menu );
}

/* ---- module entry points ---- */

void Duke_MenusInit( void ) {
    /* module-owned episode state and the save-menu gate */
    uii.Cvar_Register( "dn_episode", "1", 0 );
    uii.Cvar_Register( "dn_skill_choice", "0", 0 );
    uii.Cvar_Register( "dn_serveractive", "0", 0 );

    Duke_Menu_Main();
    Duke_Menu_Episodes();
    Duke_Menu_Skills();
    Duke_Menu_Quit();
    Duke_Menu_Options();
    Duke_Menu_Keys();
}

/* start the chosen episode like the engine StartGame in menu.cpp */
static void Duke_StartGame( void ) {
    char command[128];
    int episode;

    episode = ( int )uii.Cvar_Value( "dn_episode" );
    if ( episode < 1 || episode > 4 ) {
        episode = 1;
    }

    Menus_CloseAll();
    uii.Cvar_SetValue( "deathmatch", 0 );
    uii.Cvar_SetValue( "coop", 0 );
    uii.Cvar_SetValue( "gamerules", 0 );
    snprintf( command, sizeof( command ), "loading ; killserver ; wait ; map e%dl1\n", episode );
    uii.ExecuteText( DN_EXEC_APPEND, command );
}

void Duke_Refresh( void ) {
    menuDef_t *keys;

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
    keys = Menus_FindByName( "keys" );
    if ( keys && ( keys->window.flags & WINDOW_VISIBLE ) ) {
        Controls_GetConfig();
    }

    if ( Menus_AnyFullScreenVisible() ) {
        if ( uii.GetConnState() != DN_CONN_ACTIVE ) {
            uii.MenuBackground();
        }
        Menu_PaintAll();
    }
}

void Duke_KeyEvent( int key, int down ) {
    displayContextDef_t *dc = Display_GetContext();
    menuDef_t *menu;

    /* the engine menus treat right click as escape */
    if ( key == K_MOUSE2 ) {
        key = K_ESCAPE;
    }

    /* the skill voice plays out before the game starts: swallow input */
    if ( s_pendingVoice[0] ) {
        return;
    }

    menu = Menu_GetFocused();
    if ( down && key == K_ESCAPE && menu && menu->window.name &&
         Q_strcasecmp( menu->window.name, "main" ) == 0 ) {
        if ( uii.GetConnState() == DN_CONN_ACTIVE ) {
            /* leave the menu and go back to the game */
            uii.MenuSound( DN_UISOUND_BACK );
            Menus_CloseAll();
        }
        /* stay on the main menu when no game is running */
        return;
    }

    Display_HandleKey( key, down, dc->cursorx, dc->cursory );
}

void Duke_MouseEvent( int dx, int dy ) {
    displayContextDef_t *dc = Display_GetContext();
    int width = 640, height = 480;

    uii.GetVideoMode( &width, &height );
    if ( width <= 0 ) {
        width = 640;
    }
    if ( height <= 0 ) {
        height = 480;
    }

    /* mouse deltas arrive in device pixels, the framework works in the
       640x480 virtual menu space */
    dc->cursorx += ( int )( dx * ( 640.0f / width ) );
    dc->cursory += ( int )( dy * ( 480.0f / height ) );
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
    Display_MouseMove( NULL, dc->cursorx, dc->cursory );
}

void Duke_SetActiveMenu( dn_uimenu_t menu ) {
    Menus_CloseAll();
    if ( menu != DN_UIMENU_NONE ) {
        Menus_OpenByName( "main" );
    }
}

int Duke_IsFullscreen( void ) {
    return Menus_AnyFullScreenVisible() ? 1 : 0;
}
