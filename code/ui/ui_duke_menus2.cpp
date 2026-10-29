/* Second Duke menu set on the idTech 3 framework: save/load games, the
   multiplayer tree, the server browser, player setup, download options,
   deathmatch flags and the video setup, replacing the matching engine
   Q2 menus. Dynamic data (save slot text, LAN server names, video modes,
   the map list and the credits) comes through the engine import table.
   The port keeps several deliberate compromises, noted in the PR body:
   saves describe themselves with the level name instead of an editable
   string, there is no player model preview, and everything can be
   revisited once the Quake 2 renderer is replaced by the Quake 3 one. */

#include "ui_local.h"

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

typedef struct {
    const char *label;
    int x;                  /* Duke menu x, 160 means centered */
    int y;                  /* Duke menu y, the glyph top is y - 12 */
    int bright;             /* always drawn in the selected shade */
    int cursorFromStart;    /* cursor sits 18 pixels left of the text start */
    int cursorX;            /* fixed cursor x when cursorFromStart is 0 */
} duke2Line_t;

static duke2Line_t duke2Lines[DL2_COUNT] = {
    {"SAVE GAME",           160, 24,  1, 0, 0},
    {"LOAD GAME",           160, 24,  1, 0, 0},
    {"MULTIPLAYER",         160, 24,  1, 0, 0},
    {"JOIN NETWORK SERVER", 80,  64,  0, 0, 56},
    {"START NETWORK SERVER", 80,  80, 0, 0, 56},
    {"PLAYER SETUP",        80,  96,  0, 0, 56},
    {"JOIN SERVER",         160, 24,  1, 0, 0},
    {"ADDRESS BOOK",        40,  44,  0, 1, 0},
    {"REFRESH SERVER LIST", 40,  58,  0, 1, 0},
    {"CONNECT TO...",       160, 74,  0, 0, 0},
    {"ADDRESS BOOK",        160, 24,  1, 0, 0},
    {"START SERVER",        160, 24,  1, 0, 0},
    {"DEATHMATCH FLAGS",    40,  110, 0, 1, 0},
    {"BEGIN",               40,  124, 0, 1, 0},
    {"DEATHMATCH FLAGS",    160, 24,  1, 0, 0},
    {"DOWNLOAD OPTIONS",    160, 24,  1, 0, 0},
    {"PLAYER SETUP",        160, 24,  1, 0, 0},
    {"DOWNLOAD OPTIONS",    40,  100, 0, 1, 0},
    {"VIDEO SETUP",         160, 24,  1, 0, 0},
    {"APPLY CHANGES",       40,  140, 0, 1, 0}
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

/* action strings built once so the script pointers stay valid */
static char s_saveActions[DUKE2_MAX_SLOTS][96];
static char s_loadActions[DUKE2_MAX_SLOTS][96];
static char s_connectActions[DUKE2_MAX_SERVERS][48];

/* address book strings live for the whole run: item->text and item->cvar
   point straight at them */
static char s_adrNames[9][16];
static char s_adrLabels[9][16];
static char s_adrCvars[9][16];

/* ---- helper builders, same shapes as ui_duke_menus.cpp ---- */

static qboolean Duke2_LineFocused( int ownerDraw ) {
    dnMenu *menu = Menu_GetFocused();
    dnItem *item = menu ? menu->GetFocusedItem() : NULL;

    return ( item != NULL && item->window.ownerDraw == ownerDraw ) ? qtrue : qfalse;
}

static void Duke2_MenuSetup( dnMenu *menu ) {
    menu->fullScreen = 1;
    menu->focusColor[0] = 1.0f;
    menu->focusColor[1] = 1.0f;
    menu->focusColor[2] = 1.0f;
    menu->focusColor[3] = 1.0f;
}

static void Duke2_AddDeco( dnMenu *menu, int ownerDraw ) {
    dnItem *item = menu->AddItem( NULL, ITEM_TYPE_OWNERDRAW );

    if ( item == NULL ) {
        return;
    }
    item->window.ownerDraw = ownerDraw;
    item->window.flags |= WINDOW_VISIBLE | WINDOW_DECORATION;
}

static dnItem *Duke2_AddLine( dnMenu *menu, int ownerDraw, const char *name,
                                 float x, float y, float w, float h, const char *action ) {
    dnItem *item = menu->AddItem( name, ITEM_TYPE_OWNERDRAW );

    if ( item == NULL ) {
        return NULL;
    }
    item->window.ownerDraw = ownerDraw;
    item->window.rect.x = x;
    item->window.rect.y = y;
    item->window.rect.w = w;
    item->window.rect.h = h;
    item->window.flags |= WINDOW_VISIBLE;
    item->action = action;
    return item;
}

static dnItem *Duke2_AddSetting( dnMenu *menu, int type, const char *text,
                                    const char *cvar, const char *name,
                                    float x, float y, float w, float h ) {
    dnItem *item = menu->AddItem( name, type );

    if ( item == NULL ) {
        return NULL;
    }
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

static dnItem *Duke2_AddEditField( dnMenu *menu, const char *text,
                                     const char *cvar, const char *name,
                                     float x, float y, float w, float h,
                                     int maxChars, int maxPaintChars ) {
    dnItem *item = menu->AddItem( name, ITEM_TYPE_EDITFIELD );
    editFieldDef_t *edit;

    if ( item == NULL ) {
        return NULL;
    }
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
    edit = ( editFieldDef_t * )UI_Alloc( sizeof( editFieldDef_t ) );
    if ( edit == NULL ) {
        return NULL;
    }
    memset( edit, 0, sizeof( editFieldDef_t ) );
    edit->maxChars = maxChars;
    edit->maxPaintChars = maxPaintChars;
    item->typeData = edit;
    item->cursorPos = 0;
    return item;
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
    dnMenu *menu = Menus_FindByName( "video" );
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

/* draws a slot row: number plus the description from the save header */
static void Duke2_DrawSlot( int slot ) {
    char label[48];
    int y = 40 + slot * 14;
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_SLOT0 + slot );
    int start;

    if ( slot == 0 ) {
        snprintf( label, sizeof( label ), "AUTO   %s", s_saveValid[0] ? "LEVEL START" : "EMPTY" );
    } else {
        snprintf( label, sizeof( label ), "%2d  %s", slot, s_saveDesc[slot] );
    }
    start = uii.MenuText( 100, y, label, selected );
    if ( selected ) {
        uii.MenuCursor( start - 18, y - 7 );
    }
}

static void Duke2_DrawServer( int index ) {
    const char *label = ( index < s_serverCount ) ? s_serverNames[index] : "<no server>";
    int y = 86 + index * 13;
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_SERVER0 + index );
    int start = uii.MenuText( 100, y, label, selected );

    if ( selected ) {
        uii.MenuCursor( start - 18, y - 7 );
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

static void Duke2_DrawDmFlag( int index ) {
    const duke2DmFlag_t *f = &dukeDmFlags[index];
    int flags = ( int )uii.Cvar_Value( "dmflags" );
    /* two columns of rows, eight on the left, seven on the right */
    int x = ( index < 8 ) ? 8 : 168;
    int y = 44 + ( index % 8 ) * 16;
    int selected = Duke2_LineFocused( DUKE2_OD_BASE + DUKE2_OD_DMFLAG0 + index );

    uii.MenuText( x, y, f->label, selected );
    uii.MenuText( x + 112, y, Duke2_DmFlagValue( f, flags ), selected );
}

void Duke2_OwnerDraw( int ownerDraw, float x, float y, float w, float h, vec4_t color ) {
    int id = ownerDraw - DUKE2_OD_BASE;
    duke2Line_t *line;

    ( void )x;
    ( void )y;
    ( void )w;
    ( void )h;
    ( void )color;

    if ( id >= DUKE2_OD_SLOT0 && id <= DUKE2_OD_SLOT0 + 10 ) {
        Duke2_DrawSlot( id - DUKE2_OD_SLOT0 );
        return;
    }
    if ( id >= DUKE2_OD_SERVER0 && id < DUKE2_OD_SERVER0 + DUKE2_MAX_SERVERS ) {
        Duke2_DrawServer( id - DUKE2_OD_SERVER0 );
        return;
    }
    if ( id == DUKE2_OD_CREDITS ) {
        Duke2_DrawCredits();
        return;
    }
    if ( id >= DUKE2_OD_DMFLAG0 && id < DUKE2_OD_DMFLAG0 + 15 ) {
        Duke2_DrawDmFlag( id - DUKE2_OD_DMFLAG0 );
        return;
    }
    if ( id == DUKE2_OD_DMVALUE ) {
        char label[32];
        snprintf( label, sizeof( label ), "DMFLAGS = %d", ( int )uii.Cvar_Value( "dmflags" ) );
        uii.MenuText( 160, 176, label, 1 );
        return;
    }
    if ( id >= DUKE2_OD_LINE0 && id - DUKE2_OD_LINE0 < DL2_COUNT ) {
        line = &duke2Lines[id - DUKE2_OD_LINE0];
        if ( line->label == NULL ) {
            return;
        }
        {
            int selected = line->bright || Duke2_LineFocused( ownerDraw );
            int start = uii.MenuText( line->x, line->y, line->label, selected );
            if ( !line->bright && selected ) {
                if ( line->cursorFromStart ) {
                    uii.MenuCursor( start - 18, line->y - 7 );
                } else if ( line->cursorX ) {
                    uii.MenuCursor( line->cursorX, line->y - 7 );
                }
            }
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
    Menus_CloseAll();
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
    Menus_CloseAll();
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
    Menus_CloseAll();
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
    Menus_CloseAll();
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
    } else {
        /* unknown subcommand: consume its argument so the remaining
           script tokens are not misread as more commands */
        String_Parse( args, &arg );
    }
}

/* ---- per-frame updates ---- */

static qboolean Duke2_MenuVisible( const char *name ) {
    dnMenu *menu = Menus_FindByName( name );

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

/* ---- menus ---- */

static void Duke2_Menu_Save( void ) {
    dnMenu *menu = dnMenu::Alloc( "savegame" );
    /* item names live for the whole run: AddItem keeps the pointer */
    static char names[DUKE2_MAX_SLOTS][16];
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "duke readsave ; setfocus save_1";
    menu->onESC = "play item15.wav ; close savegame ; open main";

    Duke2_AddDeco( menu, 1 ); /* the episode header tile */
    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_SAVE_TITLE );
    for ( i = 1; i <= 10; i++ ) {
        snprintf( names[i], sizeof( names[0] ), "save_%d", i );
        Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_SLOT0 + i, names[i],
                       190, ( 40 + i * 14 - 12 ) * 2, 320, 28, s_saveActions[i] );
    }
    menu->PostParse();
}

static void Duke2_Menu_Load( void ) {
    dnMenu *menu = dnMenu::Alloc( "loadgame" );
    /* item names live for the whole run: AddItem keeps the pointer */
    static char names[DUKE2_MAX_SLOTS][16];
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "duke readsave ; setfocus load_0";
    menu->onESC = "play item15.wav ; close loadgame ; open main";

    Duke2_AddDeco( menu, 1 ); /* the episode header tile */
    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_LOAD_TITLE );
    for ( i = 0; i <= 10; i++ ) {
        snprintf( names[i], sizeof( names[0] ), "load_%d", i );
        Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_SLOT0 + i, names[i],
                       190, ( 40 + i * 14 - 12 ) * 2, 320, 28, s_loadActions[i] );
    }
    menu->PostParse();
}

static void Duke2_Menu_Multiplayer( void ) {
    dnMenu *menu = dnMenu::Alloc( "multiplayer" );

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus mp_join";
    menu->onESC = "play item15.wav ; close multiplayer ; open main";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_MP_TITLE );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_MP_JOIN, "mp_join",
                   144, 106, 416, 28,
                   "play kickhit.wav ; close multiplayer ; open joinserver" );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_MP_START, "mp_start",
                   144, 138, 416, 28,
                   "play kickhit.wav ; close multiplayer ; open startserver" );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_MP_SETUP, "mp_setup",
                   144, 170, 416, 28,
                   "play kickhit.wav ; close multiplayer ; open playerconfig" );
    menu->PostParse();
}

static void Duke2_Menu_JoinServer( void ) {
    dnMenu *menu = dnMenu::Alloc( "joinserver" );
    /* item names live for the whole run: AddItem keeps the pointer */
    static char names[DUKE2_MAX_SERVERS][24];
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    /* the engine menu pings the local network when it opens */
    menu->onOpen = "duke refreshservers ; setfocus join_addressbook";
    menu->onESC = "play item15.wav ; close joinserver ; open multiplayer";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_JOIN_TITLE );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_JOIN_ADDRESSBOOK, "join_addressbook",
                   64, 68, 512, 24,
                   "play kickhit.wav ; close joinserver ; open addressbook" );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_JOIN_REFRESH, "join_refresh",
                   64, 96, 512, 24,
                   "play kickhit.wav ; duke refreshservers" );
    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_JOIN_CONNECTTO );
    for ( i = 0; i < DUKE2_MAX_SERVERS; i++ ) {
        snprintf( names[i], sizeof( names[0] ), "join_server_%d", i );
        Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_SERVER0 + i, names[i],
                       64, ( 86 + i * 13 - 10 ) * 2, 512, 26, s_connectActions[i] );
    }
    menu->PostParse();
}

static void Duke2_Menu_AddressBook( void ) {
    dnMenu *menu = dnMenu::Alloc( "addressbook" );
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus adr_0";
    menu->onESC = "play item15.wav ; close addressbook ; open joinserver";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_ADDR_TITLE );
    for ( i = 0; i < 9; i++ ) {
        /* edit fields write the adrN cvars on every keystroke, so the
           engine address book is saved without the engine close hook */
        Duke2_AddEditField( menu, s_adrLabels[i], s_adrCvars[i], s_adrNames[i],
                            80, ( 40 + i * 16 ) * 2, 480, 28, 60, 30 );
    }
    menu->PostParse();
}

static void Duke2_Menu_StartServer( void ) {
    static const char *rulesLabels[2] = {"deathmatch", "cooperative"};
    static const float rulesValues[2] = {0.0f, 1.0f};
    dnMenu *menu = dnMenu::Alloc( "startserver" );
    dnItem *item;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus ss_map";
    menu->onESC = "play item15.wav ; close startserver ; open multiplayer";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_START_TITLE );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "initial map", "dn_startmap", "ss_map",
                             80, 56, 480, 24 );
    Duke2_SetMultiStr( item, ( const char ** )s_mapLongPtr, ( const char ** )s_mapShortPtr, s_mapCount );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "rules", "dn_rules", "ss_rules",
                             80, 84, 480, 24 );
    Duke2_SetMulti( item, rulesLabels, rulesValues, 2 );
    Duke2_AddSetting( menu, ITEM_TYPE_NUMERICFIELD, "time limit", "timelimit", "ss_timelimit",
                      80, 112, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_NUMERICFIELD, "frag limit", "fraglimit", "ss_fraglimit",
                      80, 140, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_NUMERICFIELD, "max players", "maxclients", "ss_maxclients",
                      80, 168, 480, 24 );
    Duke2_AddEditField( menu, "hostname", "hostname", "ss_hostname",
                        80, 196, 480, 24, 16, 16 );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_START_DMOPTIONS, "ss_dmflags",
                   64, 200, 512, 24,
                   "play kickhit.wav ; close startserver ; open dmoptions" );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_START_BEGIN, "ss_begin",
                   64, 228, 512, 24,
                   "play kickhit.wav ; duke startserver" );
    menu->PostParse();
}

static void Duke2_Menu_DmOptions( void ) {
    dnMenu *menu = dnMenu::Alloc( "dmoptions" );
    /* item names live for the whole run: AddItem keeps the pointer */
    static char names[15][16];
    int i;

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus dm_0";
    menu->onESC = "play item15.wav ; close dmoptions ; open startserver";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_DM_TITLE );
    for ( i = 0; i < 15; i++ ) {
        int x = ( i < 8 ) ? 8 : 168;
        int y = 44 + ( i % 8 ) * 16;
        snprintf( names[i], sizeof( names[0] ), "dm_%d", i );
        Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_DMFLAG0 + i, names[i],
                       x * 2, ( y - 10 ) * 2, 260, 28, "play bulithit.wav" );
    }
    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_DMVALUE );
    menu->PostParse();
}

static void Duke2_Menu_DownloadOptions( void ) {
    dnMenu *menu = dnMenu::Alloc( "downloadoptions" );

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus dl_allow";
    menu->onESC = "play item15.wav ; close downloadoptions ; open playerconfig";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_DL_TITLE );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "allow downloading", "allow_download", "dl_allow",
                      80, 64, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "maps", "allow_download_maps", "dl_maps",
                      80, 92, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "player models/skins", "allow_download_players", "dl_players",
                      80, 120, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "models", "allow_download_models", "dl_models",
                      80, 148, 480, 24 );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "sounds", "allow_download_sounds", "dl_sounds",
                      80, 176, 480, 24 );
    menu->PostParse();
}

static void Duke2_Menu_PlayerConfig( void ) {
    static const char *handLabels[3] = {"right", "left", "center"};
    static const float handValues[3] = {0.0f, 1.0f, 2.0f};
    static const char *rateLabels[5] = {"28.8 MODEM", "33.6 MODEM", "SINGLE ISDN",
                                        "DUAL ISDN/CABLE", "T1/LAN"};
    static const float rateValues[5] = {2500.0f, 3200.0f, 5000.0f, 10000.0f, 25000.0f};
    dnMenu *menu = dnMenu::Alloc( "playerconfig" );

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "setfocus pc_name";
    menu->onESC = "play item15.wav ; close playerconfig ; open multiplayer";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_PC_TITLE );
    /* the engine menu rendered a rotating player model here; the module
       keeps the settings until a Quake 3 renderer brings renderScene */
    Duke2_AddEditField( menu, "name", "name", "pc_name", 80, 64, 480, 24, 16, 16 );
    {
        dnItem *item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "handedness", "hand", "pc_hand",
                                            80, 100, 480, 24 );
        Duke2_SetMulti( item, handLabels, handValues, 3 );
    }
    Duke2_AddEditField( menu, "skin", "skin", "pc_skin", 80, 136, 480, 24, 32, 24 );
    {
        dnItem *item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "connection", "rate", "pc_rate",
                                            80, 172, 480, 24 );
        Duke2_SetMulti( item, rateLabels, rateValues, 5 );
    }
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_PC_DOWNLOADS, "pc_downloads",
                   64, 180, 512, 24,
                   "play kickhit.wav ; close playerconfig ; open downloadoptions" );
    menu->PostParse();
}

static void Duke2_Menu_Video( void ) {
    static char sizeLabels[10][8];
    static float sizeValues[10];
    static char gammaLabels[9][8];
    static float gammaValues[9];
    static const char *tqLabels[4] = {"0", "1", "2", "3"};
    /* engine order: texture quality 0..3 maps to gl_picmip 3..0 */
    static const float tqValues[4] = {3.0f, 2.0f, 1.0f, 0.0f};
    dnMenu *menu = dnMenu::Alloc( "video" );
    dnItem *item;
    int i;

    if ( menu == NULL ) {
        return;
    }

    for ( i = 0; i < 10; i++ ) {
        snprintf( sizeLabels[i], sizeof( sizeLabels[0] ), "%d", 30 + i * 10 );
        sizeValues[i] = ( float )( 30 + i * 10 );
    }
    /* build labels and values from the same double math so the cvar
       written from a label always matches the value the framework
       compares against after the engine parses it back */
    for ( i = 0; i < 9; i++ ) {
        snprintf( gammaLabels[i], sizeof( gammaLabels[0] ), "%.1f", 0.5 + 0.1 * i );
        gammaValues[i] = ( float )( 0.5 + 0.1 * i );
    }

    Duke2_MenuSetup( menu );
    menu->onOpen = "duke readvideo ; setfocus vid_mode";
    menu->onESC = "play item15.wav ; close video ; open main";

    Duke2_AddDeco( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_VIDEO_TITLE );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "video mode", "gl_mode", "vid_mode",
                             80, 56, 480, 24 );
    Duke2_SetMulti( item, ( const char ** )s_modeLabelPtr, s_modeValues, s_modeCount );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "refresh rate", "vid_displayrefresh", "vid_refresh",
                             80, 84, 480, 24 );
    Duke2_SetMulti( item, ( const char ** )s_rateLabelPtr, s_rateValues, s_rateCount );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "fullscreen", "vid_fullscreen", "vid_fullscreen",
                      80, 112, 480, 24 );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "display", "vid_display", "vid_display",
                             80, 140, 480, 24 );
    Duke2_SetMulti( item, ( const char ** )s_displayPtr, s_displayValues, s_displayCount );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "screen size", "viewsize", "vid_size",
                             80, 168, 480, 24 );
    Duke2_SetMulti( item, ( const char ** )sizeLabels, sizeValues, 10 );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "brightness", "vid_gamma", "vid_gamma",
                             80, 196, 480, 24 );
    Duke2_SetMulti( item, ( const char ** )gammaLabels, gammaValues, 9 );
    item = Duke2_AddSetting( menu, ITEM_TYPE_MULTI, "texture quality", "gl_picmip", "vid_picmip",
                             80, 224, 480, 24 );
    Duke2_SetMulti( item, tqLabels, tqValues, 4 );
    Duke2_AddSetting( menu, ITEM_TYPE_YESNO, "sync every frame", "gl_finish", "vid_finish",
                      80, 252, 480, 24 );
    Duke2_AddLine( menu, DUKE2_OD_BASE + DUKE2_OD_LINE0 + DL2_VIDEO_APPLY, "vid_apply",
                   64, 260, 512, 24,
                   /* applying always restarts the video subsystem; the
                      engine menu skipped the restart when nothing needed
                      it, which a future pass can restore */
                   "play kickhit.wav ; exec vid_restart" );
    menu->PostParse();
}

static void Duke2_Menu_Credits( void ) {
    dnMenu *menu = dnMenu::Alloc( "credits" );

    if ( menu == NULL ) {
        return;
    }

    Duke2_MenuSetup( menu );
    menu->onESC = "play item15.wav ; close credits ; open main";

    {
        dnItem *item = menu->AddItem( "credits_scroll", ITEM_TYPE_OWNERDRAW );
        if ( item ) {
            item->window.ownerDraw = DUKE2_OD_BASE + DUKE2_OD_CREDITS;
            item->window.rect.x = 0;
            item->window.rect.y = 0;
            item->window.rect.w = SCREEN_WIDTH;
            item->window.rect.h = SCREEN_HEIGHT;
            item->window.flags |= WINDOW_VISIBLE | WINDOW_DECORATION;
        }
    }
    menu->PostParse();
}

/* ---- module entry ---- */

void Duke_Menus2Init( void ) {
    int i;

    /* cvars the new menus read or write; DN_CVAR_ARCHIVE matches the
       engine CVAR_ARCHIVE so these persist in the config like the
       engine menus' variables */
    uii.Cvar_Register( "dn_save_slot", "1", DN_CVAR_ARCHIVE );
    uii.Cvar_Register( "dn_startmap", "e1l1", 0 );
    uii.Cvar_Register( "dn_rules", "0", 0 );
    for ( i = 0; i < 9; i++ ) {
        snprintf( s_adrNames[i], sizeof( s_adrNames[0] ), "adr_%d", i );
        snprintf( s_adrLabels[i], sizeof( s_adrLabels[0] ), "ADR %d", i );
        snprintf( s_adrCvars[i], sizeof( s_adrCvars[0] ), "adr%d", i );
        uii.Cvar_Register( s_adrCvars[i], "", DN_CVAR_ARCHIVE );
    }
    uii.Cvar_Register( "name", "unnamed", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "hand", "0", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "skin", "", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "rate", "2500", DN_CVAR_ARCHIVE | 2 );
    uii.Cvar_Register( "timelimit", "0", 0 );
    uii.Cvar_Register( "fraglimit", "0", 0 );
    uii.Cvar_Register( "maxclients", "8", 0 );
    uii.Cvar_Register( "hostname", "noname", 0 );

    /* action strings built once so the script pointers stay valid */
    for ( i = 0; i <= 10; i++ ) {
        snprintf( s_loadActions[i], sizeof( s_loadActions[0] ),
                  "play kickhit.wav ; duke load %d", i );
    }
    for ( i = 1; i <= 10; i++ ) {
        snprintf( s_saveActions[i], sizeof( s_saveActions[0] ),
                  "play kickhit.wav ; duke save %d", i );
    }
    for ( i = 0; i < DUKE2_MAX_SERVERS; i++ ) {
        snprintf( s_connectActions[i], sizeof( s_connectActions[0] ),
                  "duke connect %d", i );
    }

    Duke2_ReadMaps();
    Duke2_ReadVideoModes();
    Duke2_ReadDisplays();
    Duke2_ReadVideo();
    Duke2_ReadSaveSlots();
    Duke2_ReadCredits();

    Duke2_Menu_Save();
    Duke2_Menu_Load();
    Duke2_Menu_Multiplayer();
    Duke2_Menu_JoinServer();
    Duke2_Menu_AddressBook();
    Duke2_Menu_StartServer();
    Duke2_Menu_DmOptions();
    Duke2_Menu_DownloadOptions();
    Duke2_Menu_PlayerConfig();
    Duke2_Menu_Video();
    Duke2_Menu_Credits();
}
