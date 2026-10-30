/* Second Duke menu set as moddable data: save/load games, the multiplayer
   tree, the server browser, player setup, download options, deathmatch
   flags and the video setup live in .menu script files under ui/, loaded
   through ui/menus.txt. This file keeps what cannot be scripted: the
   ownerdraw painters, the engine data readers and caches, the custom
   "duke" script commands and the cvars. The earlier compromises still
   hold: saves carry the level name instead of an editable string and
   there is no player model preview, both waiting on the Quake 3
   renderer. */

#include "ui_local.h"
#include "../qcommon/dnmenuparser.h"

/* ---- ownerdraw ids, all at or above DUKE2_OD_BASE ---- */

/* save slot rows, slot index 0..10 */
#define DUKE2_OD_SLOT0   0
/* LAN server rows, index 0..7 */
#define DUKE2_OD_SERVER0 16
/* the credits scroller */
#define DUKE2_OD_CREDITS 24
/* dmflags rows, index 0..14 */
#define DUKE2_OD_DMFLAG0 32
/* the dmflags value readout */
#define DUKE2_OD_DMVALUE 47
/* static lines from duke2Lines below */
#define DUKE2_OD_LINE0   48

enum {
    DL2_SAVE_TITLE,
    DL2_LOAD_TITLE,
    DL2_MP_TITLE,
    DL2_MP_JOIN,
    DL2_MP_START,
    DL2_MP_SETUP,
    DL2_JOIN_TITLE,
    DL2_JOIN_ADDRESSBOOK,
    DL2_JOIN_REFRESH,
    DL2_JOIN_CONNECTTO,
    DL2_ADDR_TITLE,
    DL2_START_TITLE,
    DL2_START_DMOPTIONS,
    DL2_START_BEGIN,
    DL2_DM_TITLE,
    DL2_DL_TITLE,
    DL2_PC_TITLE,
    DL2_PC_DOWNLOADS,
    DL2_VIDEO_TITLE,
    DL2_VIDEO_APPLY,
    DL2_COUNT
};

/* the line rows share the dukeLine_t shape from ui_local.h: the item
   rect places the row, the table carries the look */
static dukeLine_t duke2Lines[DL2_COUNT] = {
    {"SAVE GAME",           1, 0, 0, 0, 0},
    {"LOAD GAME",           1, 0, 0, 0, 0},
    {"MULTIPLAYER",         1, 0, 0, 0, 0},
    {"JOIN NETWORK SERVER", 0, 0, 1, 0, 1},
    {"START NETWORK SERVER", 0, 0, 1, 0, 1},
    {"PLAYER SETUP",        0, 0, 1, 0, 1},
    {"JOIN SERVER",         1, 0, 0, 0, 0},
    {"ADDRESS BOOK",        0, 1, 1, 0, 0},
    {"REFRESH SERVER LIST", 0, 1, 1, 0, 0},
    {"CONNECT TO...",       0, 0, 0, 0, 0},
    {"ADDRESS BOOK",        1, 0, 0, 0, 0},
    {"START SERVER",        1, 0, 0, 0, 0},
    {"DEATHMATCH FLAGS",    0, 1, 1, 0, 0},
    {"BEGIN",               0, 1, 1, 0, 0},
    {"DEATHMATCH FLAGS",    1, 0, 0, 0, 0},
    {"DOWNLOAD OPTIONS",    1, 0, 0, 0, 0},
    {"PLAYER SETUP",        1, 0, 0, 0, 0},
    {"DOWNLOAD OPTIONS",    0, 0, 1, 0, 1},
    {"VIDEO SETUP",         1, 0, 0, 0, 0},
    {"APPLY CHANGES",       0, 0, 1, 0, 1}
};

/* ---- dmflags rows, mirroring the engine DMOptions callback table ---- */

typedef struct {
    const char *label;
    int bit;                 /* single dmflags bit, 0 for the teamplay row */
    int invert;              /* drawn value is yes when the bit is clear */
    int tri;                 /* teamplay cycles none, skin teams, model teams */
} duke2DmFlag_t;

static duke2DmFlag_t dukeDmFlags[15] = {
    {"FALLING DAMAGE", DF_NO_FALLING,       1, 0},
    {"WEAPONS STAY",   DF_WEAPONS_STAY,     0, 0},
    {"INSTANT ITEMS",  DF_INSTANT_ITEMS,    0, 0},
    {"POWERUPS",       DF_NO_ITEMS,          1, 0},
    {"HEALTH",         DF_NO_HEALTH,         1, 0},
    {"ARMOR",          DF_NO_ARMOR,          1, 0},
    {"SPAWN FARTHEST", DF_SPAWN_FARTHEST,    0, 0},
    {"SAME MAP",       DF_SAME_LEVEL,        0, 0},
    {"FORCE RESPAWN",  DF_FORCE_RESPAWN,     0, 0},
    {"TEAMPLAY",       0,                    0, 1},
    {"ALLOW EXIT",     DF_ALLOW_EXIT,        0, 0},
    {"INFINITE AMMO",  DF_INFINITE_AMMO,     0, 0},
    {"FIXED FOV",      DF_FIXED_FOV,         0, 0},
    {"QUAD DROP",     DF_QUAD_DROP,          0, 0},
    {"FRIENDLY FIRE",  DF_NO_FRIENDLY_FIRE, 1, 0},
};

/* ---- caches filled from the engine imports ---- */

#define DUKE2_MAX_SLOTS   11
#define DUKE2_MAX_SERVERS 8
#define DUKE2_MAX_MODES   32
#define DUKE2_MAX_CREDITS 255

static char s_saveDesc[DUKE2_MAX_SLOTS][32];
static qboolean s_saveValid[DUKE2_MAX_SLOTS];

static char s_serverNames[DUKE2_MAX_SERVERS][80];
static int s_serverCount;

/* the map list, video modes and displays are static per run; refresh
   rates depend on the selected mode and are rebuilt on demand */
static char s_mapBuffer[8192];
static char s_mapShort[DUKE2_MAX_MODES][16];
static char s_mapLong[DUKE2_MAX_MODES][40];
static char *s_mapShortPtr[DUKE2_MAX_MODES];
static char *s_mapLongPtr[DUKE2_MAX_MODES];
static int s_mapCount;

static char s_modeLabels[DUKE2_MAX_MODES][32];
static char *s_modeLabelPtr[DUKE2_MAX_MODES];
static float s_modeValues[DUKE2_MAX_MODES];
static int s_modeCount;

static char s_displayNames[DUKE2_MAX_MODES][64];
static char *s_displayPtr[DUKE2_MAX_MODES];
static float s_displayValues[DUKE2_MAX_MODES];
static int s_displayCount;

static char s_rateLabels[DUKE2_MAX_MODES][32];
static char *s_rateLabelPtr[DUKE2_MAX_MODES];
static float s_rateValues[DUKE2_MAX_MODES];
static int s_rateCount;

/* credits text: engine file first, built-in fallback otherwise */
static char s_creditsBuffer[8192];
static const char *s_creditsLines[DUKE2_MAX_CREDITS];
static int s_creditsCount;
static int s_creditsTime;

static const char *s_creditsFallback[] = {
    "DUKE NUKEM 3D",
    "",
    "ID TECH 2 ENGINE PORT",
    "",
    "ORIGINAL GAME BY 3D REALMS",
    "",
    "PORT MAINTAINED BY GOATON128",
    "",
    "",
    ""
};


/* ---- helpers ---- */

static qboolean Duke2_LineFocused( int ownerDraw ) {
    dnMenu *menu = g_menus.GetFocused();
    dnItem *item = menu ? menu->GetFocusedItem() : NULL;

    return ( item != NULL && item->window.ownerDraw == ownerDraw ) ? qtrue : qfalse;
}

static void Duke2_SetMulti( dnItem *item, const char **labels, const float *values, int count ) {
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

/* string MULTI: the labels are shown, the strings are written to the cvar */
static void Duke2_SetMultiStr( dnItem *item, const char **labels, const char **strs, int count ) {
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
        multi->cvarStr[i] = strs[i];
    }
    multi->count = count;
    multi->strDef = qtrue;
    item->typeData = multi;
}

/* ---- engine data readers ---- */

static void Duke2_ReadSaveSlots( void ) {
    int i;

    for ( i = 0; i < DUKE2_MAX_SLOTS; i++ ) {
        s_saveDesc[i][0] = '\0';
        s_saveValid[i] = uii.SaveSlotInfo( i, s_saveDesc[i], sizeof( s_saveDesc[0] ) ) ? qtrue : qfalse;
        if ( !s_saveValid[i] ) {
            snprintf( s_saveDesc[i], sizeof( s_saveDesc[0] ), "EMPTY" );
        }
    }
}

static void Duke2_ReadMaps( void ) {
    const char *p;
    int i;

    s_mapCount = 0;
    if ( uii.ListMaps( s_mapBuffer, sizeof( s_mapBuffer ) ) <= 0 ) {
        return;
    }
    p = s_mapBuffer;
    for ( i = 0; i < DUKE2_MAX_MODES; i++ ) {
        if ( p[0] == '\0' ) {
            break;
        }
        Q_strncpyz( s_mapShort[i], p, sizeof( s_mapShort[0] ) );
        p += strlen( p ) + 1;
        if ( p[0] == '\0' ) {
            break;
        }
        Q_strncpyz( s_mapLong[i], p, sizeof( s_mapLong[0] ) );
        p += strlen( p ) + 1;
        s_mapShortPtr[i] = s_mapShort[i];
        s_mapLongPtr[i] = s_mapLong[i];
        s_mapCount++;
    }
}

static void Duke2_ReadVideoModes( void ) {
    int widths[DUKE2_MAX_MODES];
    int heights[DUKE2_MAX_MODES];
    int i;

    s_modeCount = uii.GetVideoModes( widths, heights, DUKE2_MAX_MODES );
    if ( s_modeCount > MAX_MULTI_CVARS ) {
        s_modeCount = MAX_MULTI_CVARS;
    }
    for ( i = 0; i < s_modeCount; i++ ) {
        snprintf( s_modeLabels[i], sizeof( s_modeLabels[0] ), "%d x %d", widths[i], heights[i] );
        s_modeLabelPtr[i] = s_modeLabels[i];
        s_modeValues[i] = ( float )i;
    }
}

static void Duke2_ReadDisplays( void ) {
    int i;

    s_displayCount = uii.GetDisplays( ( char * )s_displayNames, sizeof( s_displayNames[0] ), DUKE2_MAX_MODES );
    if ( s_displayCount > MAX_MULTI_CVARS ) {
        s_displayCount = MAX_MULTI_CVARS;
    }
    for ( i = 0; i < s_displayCount; i++ ) {
        s_displayPtr[i] = s_displayNames[i];
        s_displayValues[i] = ( float )i;
    }
}

/* fill the refresh rate list for one video mode */
static void Duke2_BuildRefreshRates( int mode ) {
    int rates[DUKE2_MAX_MODES];
    int i;

    if ( mode < 0 || mode >= s_modeCount ) {
        mode = 0;
    }
    s_rateCount = uii.GetRefreshRates( mode, rates, DUKE2_MAX_MODES );
    if ( s_rateCount > MAX_MULTI_CVARS - 1 ) {
        s_rateCount = MAX_MULTI_CVARS - 1;
    }
    /* entry zero means the mode list default refresh rate */
    snprintf( s_rateLabels[0], sizeof( s_rateLabels[0] ), "DEFAULT" );
    s_rateLabelPtr[0] = s_rateLabels[0];
    s_rateValues[0] = 0.0f;
    for ( i = 0; i < s_rateCount; i++ ) {
        snprintf( s_rateLabels[i + 1], sizeof( s_rateLabels[0] ), "%d HZ", rates[i] );
        s_rateLabelPtr[i + 1] = s_rateLabels[i + 1];
        s_rateValues[i + 1] = ( float )rates[i];
    }
    s_rateCount++;
}

/* rebuild the refresh rate list for the mode currently selected */
static void Duke2_ReadVideo( void ) {
    dnMenu *menu = g_menus.FindByName( "video" );
    int i;

    Duke2_BuildRefreshRates( ( int )uii.Cvar_Value( "gl_mode" ) );

    if ( menu == NULL ) {
        return;
    }
    for ( i = 0; i < menu->itemCount; i++ ) {
        dnItem *item = menu->items[i];
        if ( item->window.name && Q_strcasecmp( item->window.name, "vid_refresh" ) == 0 ) {
            Duke2_SetMulti( item, ( const char ** )s_rateLabelPtr, s_rateValues, s_rateCount );
            break;
        }
    }
}

static void Duke2_ReadCredits( void ) {
    char *p;
    int count;

    s_creditsCount = 0;
    count = uii.GetCredits( s_creditsBuffer, sizeof( s_creditsBuffer ) );
    if ( count <= 0 ) {
        /* no credits file shipped: fall back to the built-in list */
        int i;
        for ( i = 0; s_creditsFallback[i] && s_creditsCount < DUKE2_MAX_CREDITS; i++ ) {
            s_creditsLines[s_creditsCount++] = s_creditsFallback[i];
        }
        return;
    }
    p = s_creditsBuffer;
    while ( *p && s_creditsCount < count && s_creditsCount < DUKE2_MAX_CREDITS ) {
        char *line = p;
        while ( *p && *p != '\n' ) {
            p++;
        }
        if ( *p == '\n' ) {
            *p++ = '\0';
        }
        s_creditsLines[s_creditsCount++] = line;
    }
}

/* ---- ownerdraws ---- */

/* draws a slot row: number plus the description from the save header;
   the row sits on the rect's left edge and trims against its right edge */
static void Duke2_DrawSlot( int slot, const dukeRect_t *rect ) {
    char label[48];
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_SLOT0 + slot );
    int start;

    if ( slot == 0 ) {
        snprintf( label, sizeof( label ), "AUTO   %s", s_saveValid[0] ? "LEVEL START" : "EMPTY" );
    } else {
        snprintf( label, sizeof( label ), "%2d  %s", slot, s_saveDesc[slot] );
    }
    /* long level names would run off the 320 wide screen, so trim the
       row until it ends inside the rect */
    {
        int len = ( int )strlen( label );
        while ( len > 1 && rect->leftX + uii.MenuTextWidth( label ) > rect->rightX ) {
            label[--len] = '\0';
        }
    }
    start = uii.MenuText( rect->leftX, rect->topY, label, selected );
    if ( selected ) {
        uii.MenuCursor( start - 18, rect->topY - 7 );
    }
}

static void Duke2_DrawServer( int index, const dukeRect_t *rect ) {
    char label[80];
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_SERVER0 + index );
    int len, start;

    snprintf( label, sizeof( label ), "%s",
              ( index < s_serverCount ) ? s_serverNames[index] : "<no server>" );
    /* long server names would run off the 320 wide screen */
    len = ( int )strlen( label );
    while ( len > 1 && rect->leftX + uii.MenuTextWidth( label ) > rect->rightX ) {
        label[--len] = '\0';
    }
    start = uii.MenuText( rect->leftX, rect->topY, label, selected );

    if ( selected ) {
        uii.MenuCursor( start - 18, rect->topY - 7 );
    }
}

static void Duke2_DrawCredits( void ) {
    displayContextDef_t *dc = Display_GetContext();
    int y;
    int i;

    if ( s_creditsTime == 0 ) {
        s_creditsTime = dc->realTime;
    }
    y = 200 - ( dc->realTime - s_creditsTime ) / 50;
    for ( i = 0; i < s_creditsCount; i++ ) {
        int lineY = y + i * 12;
        if ( lineY < -12 ) {
            continue;
        }
        if ( lineY > 200 ) {
            break;
        }
        if ( s_creditsLines[i][0] ) {
            uii.MenuText( 160, lineY, s_creditsLines[i], 0 );
        }
    }
    /* loop the scroll once the last line clears the top */
    if ( s_creditsCount && y + s_creditsCount * 12 < 0 ) {
        s_creditsTime = dc->realTime;
    }
}

/* the yes or no state a dmflags row shows for the current dmflags value */
static const char *Duke2_DmFlagValue( const duke2DmFlag_t *f, int flags ) {
    if ( f->tri ) {
        if ( flags & DF_SKINTEAMS ) {
            return "SKIN";
        }
        if ( flags & DF_MODELTEAMS ) {
            return "MODEL";
        }
        return "NO";
    }
    if ( f->invert ) {
        return ( flags & f->bit ) ? "NO" : "YES";
    }
    return ( flags & f->bit ) ? "YES" : "NO";
}

static void Duke2_DrawDmFlag( int index, const dukeRect_t *rect ) {
    const duke2DmFlag_t *f = &dukeDmFlags[index];
    int flags = ( int )uii.Cvar_Value( "dmflags" );
    /* the setting right-aligns on the rect's right edge so the longest
       label never collides with it */
    const char *value = Duke2_DmFlagValue( f, flags );
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_DMFLAG0 + index );

    uii.MenuText( rect->leftX, rect->topY, f->label, selected );
    uii.MenuText( rect->rightX - uii.MenuTextWidth( value ), rect->topY, value, selected );
}

void Duke2_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color ) {
    int id = ownerDraw - DUKE2_OD_BASE;
    dukeLine_t *line;
    dukeRect_t rect;

    ( void )color;

    /* the rows draw through the engine Duke helpers anchored on the
       item rect mapped into Duke menu coordinates; the credits scroller
       is the one exception, it ignores the rect */
    rect = Duke_MapRect( x, y, w, h );

    if ( id >= DUKE2_OD_SLOT0 && id <= DUKE2_OD_SLOT0 + 10 ) {
        Duke2_DrawSlot( id - DUKE2_OD_SLOT0, &rect );
        return;
    }
    if ( id >= DUKE2_OD_SERVER0 && id < DUKE2_OD_SERVER0 + DUKE2_MAX_SERVERS ) {
        Duke2_DrawServer( id - DUKE2_OD_SERVER0, &rect );
        return;
    }
    if ( id == DUKE2_OD_CREDITS ) {
        Duke2_DrawCredits();
        return;
    }
    if ( id >= DUKE2_OD_DMFLAG0 && id < DUKE2_OD_DMFLAG0 + 15 ) {
        Duke2_DrawDmFlag( id - DUKE2_OD_DMFLAG0, &rect );
        return;
    }
    if ( id == DUKE2_OD_DMVALUE ) {
        /* the value readout centers on its rect like any other line */
        char label[32];
        dukeLine_t value;
        snprintf( label, sizeof( label ), "DMFLAGS = %d", ( int )uii.Cvar_Value( "dmflags" ) );
        memset( &value, 0, sizeof( value ) );
        value.label = label;
        value.bright = 1;
        Duke_PaintLine( &value, &rect, 1 );
        return;
    }
    if ( id >= DUKE2_OD_LINE0 && id - DUKE2_OD_LINE0 < DL2_COUNT ) {
        line = &duke2Lines[id - DUKE2_OD_LINE0];
        if ( line->label != NULL ) {
            Duke_PaintLine( line, &rect, line->bright || Duke2_LineFocused( ownerDraw ) );
        }
        return;
    }
}

/* ---- dmflags cycling ---- */

qboolean Duke_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key ) {
    int id = ownerDraw - DUKE2_OD_BASE;
    duke2DmFlag_t *f;
    int dmflags;

    ( void )flags;
    ( void )special;
    if ( id < DUKE2_OD_DMFLAG0 || id >= DUKE2_OD_DMFLAG0 + 15 ) {
        return qfalse;
    }
    if ( key != K_ENTER && key != K_KP_ENTER && key != K_MOUSE1 &&
         key != K_LEFTARROW && key != K_KP_LEFTARROW &&
         key != K_RIGHTARROW && key != K_KP_RIGHTARROW ) {
        return qfalse;
    }

    f = &dukeDmFlags[id - DUKE2_OD_DMFLAG0];
    dmflags = ( int )uii.Cvar_Value( "dmflags" );
    if ( f->tri ) {
        /* none, skin teams, model teams, back to none */
        if ( dmflags & DF_SKINTEAMS ) {
            dmflags &= ~DF_SKINTEAMS;
            dmflags |= DF_MODELTEAMS;
        } else if ( dmflags & DF_MODELTEAMS ) {
            dmflags &= ~DF_MODELTEAMS;
        } else {
            dmflags |= DF_SKINTEAMS;
        }
    } else {
        dmflags ^= f->bit;
    }
    uii.Cvar_SetValue( "dmflags", ( float )dmflags );
    return qtrue;
}

/* ---- custom script commands ("duke <subcommand> [arg]") ---- */

static void Duke2_Save( int slot ) {
    char command[128];
    char desc[48];

    if ( slot < 1 || slot > 10 ) {
        return;
    }
    /* the engine gates saving the same way the engine menu did */
    if ( !uii.CanSave() ) {
        uii.Print( "Save requires a living player in a local non-deathmatch game.\n" );
        return;
    }
    uii.GetLevelName( desc, sizeof( desc ) );
    if ( !desc[0] ) {
        snprintf( desc, sizeof( desc ), "DUKE" );
    }
    /* the save description is the level name; typed save names would need
       an edit field, which can come with the Quake 3 front-end fonts */
    snprintf( command, sizeof( command ), "save save%d \"%s\"\n", slot, desc );
    uii.ExecuteText( DN_EXEC_INSERT, command );
    uii.Cvar_SetValue( "dn_save_slot", ( float )slot );
    g_menus.CloseAll();
}

static void Duke2_Load( int slot ) {
    char command[64];

    if ( slot < 0 || slot > 10 || !s_saveValid[slot] ) {
        return;
    }
    snprintf( command, sizeof( command ), "load save%d\n", slot );
    uii.ExecuteText( DN_EXEC_INSERT, command );
    if ( slot > 0 ) {
        uii.Cvar_SetValue( "dn_save_slot", ( float )slot );
    }
    g_menus.CloseAll();
}

static void Duke2_Connect( int index ) {
    char command[128];
    char address[64];

    if ( index < 0 || index >= DUKE2_MAX_SERVERS ) {
        return;
    }
    if ( !uii.GetServerAddress( index, address, sizeof( address ) ) || !address[0] ) {
        return;
    }
    snprintf( command, sizeof( command ), "connect %s\n", address );
    uii.ExecuteText( DN_EXEC_APPEND, command );
    g_menus.CloseAll();
}

static void Duke2_StartServer( void ) {
    char command[128];
    char map[40];
    int rules;

    uii.Cvar_String( "dn_startmap", map, sizeof( map ) );
    if ( !map[0] ) {
        snprintf( map, sizeof( map ), "e1l1" );
    }
    rules = ( int )uii.Cvar_Value( "dn_rules" );
    /* the engine Q2 spot-override table for cooperative starts is
       Quake 2 level-set specific and stays dropped here */
    uii.Cvar_SetValue( "deathmatch", rules == 1 ? 0.0f : 1.0f );
    uii.Cvar_SetValue( "coop", rules == 1 ? 1.0f : 0.0f );
    uii.Cvar_SetValue( "gamerules", 0.0f );
    snprintf( command, sizeof( command ), "map %s\n", map );
    uii.ExecuteText( DN_EXEC_APPEND, command );
    g_menus.CloseAll();
}

/* reached through the display context runScript hook: RunScript
   passes the arguments of any command it does not know itself */
void Duke_RunScript( char **args ) {
    const char *cmd;
    const char *arg;

    if ( !String_Parse( args, &cmd ) ) {
        return;
    }
    if ( Q_strcasecmp( cmd, "readsave" ) == 0 ) {
        Duke2_ReadSaveSlots();
    } else if ( Q_strcasecmp( cmd, "readvideo" ) == 0 ) {
        Duke2_ReadVideo();
    } else if ( Q_strcasecmp( cmd, "refreshservers" ) == 0 ) {
        uii.PingServers();
    } else if ( Q_strcasecmp( cmd, "startserver" ) == 0 ) {
        Duke2_StartServer();
    } else if ( Q_strcasecmp( cmd, "save" ) == 0 ) {
        if ( String_Parse( args, &arg ) ) {
            Duke2_Save( atoi( arg ) );
        }
    } else if ( Q_strcasecmp( cmd, "load" ) == 0 ) {
        if ( String_Parse( args, &arg ) ) {
            Duke2_Load( atoi( arg ) );
        }
    } else if ( Q_strcasecmp( cmd, "connect" ) == 0 ) {
        if ( String_Parse( args, &arg ) ) {
            Duke2_Connect( atoi( arg ) );
        }
    } else if ( Q_strcasecmp( cmd, "connectselected" ) == 0 ) {
        /* the server list item names its row through the listbox cursor */
        dnMenu *menu = g_menus.FindByName( "joinserver" );
        dnItem *list = menu ? menu->FindItemByName( "join_serverlist" ) : NULL;
        if ( list && list->type == ITEM_TYPE_LISTBOX ) {
            Duke2_Connect( list->cursorPos );
        }
    } else if ( Q_strcasecmp( cmd, "resetcontrols" ) == 0 ) {
        /* the keys screen footer: every binding back to its default, then
           write the config out like the engine defaults button */
        Controls_SetDefaults();
        Controls_SetConfig( qfalse );
    } else {
        /* unknown subcommand: consume its argument so the remaining
           script tokens are not misread as more commands */
        String_Parse( args, &arg );
    }
}

/* ---- per-frame updates ---- */

static qboolean Duke2_MenuVisible( const char *name ) {
    dnMenu *menu = g_menus.FindByName( name );

    return ( menu && ( menu->window.flags & WINDOW_VISIBLE ) ) ? qtrue : qfalse;
}

void Duke2_Refresh( void ) {
    int i;

    if ( Duke2_MenuVisible( "savegame" ) || Duke2_MenuVisible( "loadgame" ) ) {
        Duke2_ReadSaveSlots();
    }
    if ( Duke2_MenuVisible( "joinserver" ) ) {
        s_serverCount = uii.GetServers( ( char * )s_serverNames, sizeof( s_serverNames[0] ),
                                        DUKE2_MAX_SERVERS );
        if ( s_serverCount > DUKE2_MAX_SERVERS ) {
            s_serverCount = DUKE2_MAX_SERVERS;
        }
        for ( i = 0; i < DUKE2_MAX_SERVERS; i++ ) {
            if ( !s_serverNames[i][0] ) {
                snprintf( s_serverNames[i], sizeof( s_serverNames[0] ), "<no server>" );
            }
        }
    }
    if ( !Duke2_MenuVisible( "credits" ) ) {
        s_creditsTime = 0;
    }
}


/* the .menu scripts declare the runtime-list MULTI items empty; the lists
   come from the engine and can only be read at runtime, so find the items
   by name and fill them once the menu files parsed */
void Duke_MenusAttachDynamic( void ) {
    dnMenu *menu;
    dnItem *item;

    /* rebuilds the refresh rate list for the current mode and attaches it
       to the named vid_refresh item */
    Duke2_ReadVideo();

    menu = g_menus.FindByName( "video" );
    if ( menu != NULL ) {
        item = menu->FindItemByName( "vid_mode" );
        Duke2_SetMulti( item, ( const char ** )s_modeLabelPtr, s_modeValues, s_modeCount );
        item = menu->FindItemByName( "vid_display" );
        Duke2_SetMulti( item, ( const char ** )s_displayPtr, s_displayValues, s_displayCount );
    }

    menu = g_menus.FindByName( "startserver" );
    if ( menu != NULL ) {
        item = menu->FindItemByName( "ss_map" );
        Duke2_SetMultiStr( item, ( const char ** )s_mapLongPtr, ( const char ** )s_mapShortPtr, s_mapCount );
    }
}

/* ---- module entry ---- */

void Duke_Menus2Init( void ) {
    int i;

    /* cvars the script menus read or write; DN_CVAR_ARCHIVE matches the
       engine CVAR_ARCHIVE so these persist in the config like the engine
       menus' variables */
    uii.Cvar_Register( "dn_save_slot", "1", DN_CVAR_ARCHIVE );
    uii.Cvar_Register( "dn_startmap", "e1l1", 0 );
    uii.Cvar_Register( "dn_rules", "0", 0 );
    for ( i = 0; i < 9; i++ ) {
        char cvarname[16];
        snprintf( cvarname, sizeof( cvarname ), "adr%d", i );
        uii.Cvar_Register( cvarname, "", DN_CVAR_ARCHIVE );
    }
    uii.Cvar_Register( "name", "unnamed", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "hand", "0", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "skin", "", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "rate", "2500", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "timelimit", "0", 0 );
    uii.Cvar_Register( "fraglimit", "0", 0 );
    uii.Cvar_Register( "maxclients", "8", 0 );
    uii.Cvar_Register( "hostname", "noname", 0 );

    /* the static engine data caches; the runtime lists reach the script
       items through Duke_MenusAttachDynamic after the files parse */
    Duke2_ReadMaps();
    Duke2_ReadVideoModes();
    Duke2_ReadDisplays();
    Duke2_ReadSaveSlots();
    Duke2_ReadCredits();
}
