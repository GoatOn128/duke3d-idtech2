/* Front-end UI module entry plus the display context the ported idTech 3
   menu framework draws through. The framework calls every draw through the
   displayContextDef_t; its members map onto the engine import table where
   the engine already has the service, and onto module stubs for the
   services that wait for the Duke menus (ownerdraws, feeders, models). */
#include "ui_local.h"

dn_ui_import_t uii;

static dn_ui_export_t exports;

static displayContextDef_t dc;

/* ---- drawing adapters ---- */

static void DC_DrawHandlePic( float x, float y, float w, float h, qhandle_t asset ) {
    uii.DrawStretchPic( x, y, w, h, 0, 0, 1, 1, asset );
}

/* maps the 640x480 virtual menu space onto the Duke 320x200 menu grid,
   drawing through the engine MenuText ownerdraw; the selected heuristic
   reads the framework highlight color until the Duke menus drive it
   directly */
static void DC_DrawText( float x, float y, int font, float scale, vec4_t color,
                         const char *text, float adjust, int limit, int style ) {
    char buf[MAX_TOKENLENGTH];
    int len, selected, i;

    ( void )font;
    ( void )adjust;
    ( void )style;
    if ( !text ) {
        return;
    }

    len = strlen( text );
    if ( limit > 0 && limit < len ) {
        len = limit;
    }
    if ( len > (int)sizeof( buf ) - 1 ) {
        len = (int)sizeof( buf ) - 1;
    }
    for ( i = 0; i < len; i++ ) {
        buf[i] = text[i];
    }
    buf[len] = '\0';

    selected = ( color && ( color[0] >= 0.75f ) && ( color[1] >= 0.75f ) );
    uii.MenuText( (int)( x * 0.5f + 0.5f ), (int)( y * 0.5f + 0.5f ), buf, selected );
    ( void )scale;
}

/* Duke menu text is an 8x8 grid on the 320x200 screen, so a character cell
   is 16x16 in the 640x480 virtual space at scale 1 */
static int DC_TextWidth( const char *text, int font, float scale, int limit ) {
    int len;
    ( void )font;
    if ( !text ) {
        return 0;
    }
    len = strlen( text );
    if ( limit > 0 && limit < len ) {
        len = limit;
    }
    return (int)( len * 16.0f * scale );
}

static int DC_TextHeight( const char *text, int font, float scale, int limit ) {
    ( void )text;
    ( void )font;
    ( void )limit;
    return (int)( 16.0f * scale );
}

/* renderer color tint is not exposed yet: Duke tiles and text carry their
   own palette through the ownerdraws */
static void DC_SetColor( const vec4_t v ) {
    ( void )v;
}

static void DC_FillRect( float x, float y, float w, float h, const vec4_t color ) {
    ( void )x; ( void )y; ( void )w; ( void )h; ( void )color;
}

static void DC_DrawRect( float x, float y, float w, float h, float size, const vec4_t color ) {
    ( void )x; ( void )y; ( void )w; ( void )h; ( void )size; ( void )color;
}

static void DC_DrawSides( float x, float y, float w, float h, float size ) {
    ( void )x; ( void )y; ( void )w; ( void )h; ( void )size;
}

static void DC_DrawTopBottom( float x, float y, float w, float h, float size ) {
    ( void )x; ( void )y; ( void )w; ( void )h; ( void )size;
}

/* ---- engine service adapters ---- */

static void DC_GetCVarString( const char *cvar, char *buffer, int bufsize ) {
    uii.Cvar_String( cvar, buffer, (uint32_t)bufsize );
}

static void DC_SetOverstrikeMode( qboolean b ) {
    uii.SetOverstrikeMode( b ? 1 : 0 );
}

static qboolean DC_GetOverstrikeMode( void ) {
    return uii.GetOverstrikeMode() ? qtrue : qfalse;
}

static void DC_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
    uii.Key_KeynumToStringBuf( keynum, buf, (uint32_t)buflen );
}

static void DC_GetBindingBuf( int keynum, char *buf, int buflen ) {
    uii.Key_GetBindingBuf( keynum, buf, (uint32_t)buflen );
}

static void DC_Error( int level, const char *error, ... ) {
    va_list argptr;
    static char string[4096];

    ( void )level;
    va_start( argptr, error );
    vsnprintf( string, sizeof( string ), error, argptr );
    va_end( argptr );
    string[sizeof( string ) - 1] = '\0';
    uii.Error( "%s", string );
}

static void DC_Print( const char *msg, ... ) {
    va_list argptr;
    static char string[4096];

    va_start( argptr, msg );
    vsnprintf( string, sizeof( string ), msg, argptr );
    va_end( argptr );
    string[sizeof( string ) - 1] = '\0';
    uii.Print( "%s", string );
}

/* no localization layer: return the input unchanged like the unstrung
   idTech 3 front ends do */
static const char *DC_GetTranslatedString( const char *inString ) {
    return inString;
}

/* ---- stubs waiting for the Duke menu items ---- */

static qhandle_t DC_RegisterModel( const char *p ) {
    ( void )p;
    return NULL;
}

static void DC_ModelBounds( qhandle_t model, vec3_t mins, vec3_t maxs ) {
    ( void )model;
    VectorClear( mins );
    VectorClear( maxs );
}

static void DC_ClearScene( void ) {
}

static void DC_AddRefEntityToScene( const refEntity_t *re ) {
    ( void )re;
}

static void DC_RenderScene( const refdef_t *fd ) {
    ( void )fd;
}

static void DC_RegisterFont( const char *pFontname, int pointSize, fontInfo_t *font ) {
    ( void )pFontname;
    ( void )pointSize;
    memset( font, 0, sizeof( *font ) );
}

static void DC_OwnerDrawItem( float x, float y, float w, float h, float text_x, float text_y,
                              int ownerDraw, int ownerDrawFlags, int align, float special,
                              int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
    ( void )text_x; ( void )text_y;
    ( void )ownerDrawFlags; ( void )align; ( void )special;
    ( void )font; ( void )scale; ( void )shader; ( void )textStyle;
    Duke_OwnerDraw( ownerDraw, x, y, w, h, color );
}

static float DC_GetValue( int ownerDraw, int type ) {
    ( void )ownerDraw;
    ( void )type;
    return 0;
}

static qboolean DC_OwnerDrawVisible( int flags ) {
    ( void )flags;
    return qfalse;
}

static void DC_RunScript( char **p ) {
    ( void )p;
}

static void DC_GetTeamColor( vec4_t *color ) {
    ( void )color;
}

static void DC_DrawTextWithCursor( float x, float y, int font, float scale, vec4_t color,
                                   const char *text, int cursorPos, char cursor, int limit, int style ) {
    ( void )x; ( void )y; ( void )font; ( void )scale; ( void )color; ( void )text;
    ( void )cursorPos; ( void )cursor; ( void )limit; ( void )style;
}

static void DC_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
    ( void )channelNum;
    Duke_PlaySound( sfx );
}

static qboolean DC_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key ) {
    ( void )ownerDraw; ( void )flags; ( void )special; ( void )key;
    return qfalse;
}

static int DC_FeederCount( float feederID ) {
    ( void )feederID;
    return 0;
}

static const char *DC_FeederItemText( float feederID, int index, int column, qhandle_t *handle ) {
    ( void )feederID; ( void )index; ( void )column;
    if ( handle ) {
        *handle = NULL;
    }
    return NULL;
}

static const char *DC_FileText( char *fileName ) {
    ( void )fileName;
    return NULL;
}

static qhandle_t DC_FeederItemImage( float feederID, int index ) {
    ( void )feederID;
    ( void )index;
    return NULL;
}

static void DC_FeederSelection( float feederID, int index ) {
    ( void )feederID;
    ( void )index;
}

static void DC_FeederAddItem( float feederID, const char *name, int index ) {
    ( void )feederID; ( void )name; ( void )index;
}

static void DC_Pause( qboolean b ) {
    ( void )b;
}

static int DC_OwnerDrawWidth( int ownerDraw, int font, float scale ) {
    ( void )ownerDraw; ( void )font; ( void )scale;
    return 0;
}

static sfxHandle_t DC_RegisterSound( const char *name ) {
    return Duke_RegisterSound( name );
}

static void DC_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime ) {
    ( void )intro; ( void )loop; ( void )fadeupTime;
}

static void DC_StopBackgroundTrack( void ) {
}

static int DC_PlayCinematic( const char *name, float x, float y, float w, float h ) {
    ( void )name; ( void )x; ( void )y; ( void )w; ( void )h;
    return -1;
}

static void DC_StopCinematic( int handle ) {
    ( void )handle;
}

static void DC_DrawCinematic( int handle, float x, float y, float w, float h ) {
    ( void )handle; ( void )x; ( void )y; ( void )w; ( void )h;
}

static void DC_RunCinematicFrame( int handle ) {
    ( void )handle;
}

/* fills every display context member from the import table or the stubs
   above, then hands the context to the framework */
static void SetupDisplayContext( void ) {
    int w, h;

    dc.registerShaderNoMip = uii.RegisterPic;
    dc.setColor = DC_SetColor;
    dc.drawHandlePic = DC_DrawHandlePic;
    dc.drawStretchPic = uii.DrawStretchPic;
    dc.drawText = DC_DrawText;
    dc.textWidth = DC_TextWidth;
    dc.textHeight = DC_TextHeight;
    dc.registerModel = DC_RegisterModel;
    dc.modelBounds = DC_ModelBounds;
    dc.fillRect = DC_FillRect;
    dc.drawRect = DC_DrawRect;
    dc.drawSides = DC_DrawSides;
    dc.drawTopBottom = DC_DrawTopBottom;
    dc.clearScene = DC_ClearScene;
    dc.addRefEntityToScene = DC_AddRefEntityToScene;
    dc.renderScene = DC_RenderScene;
    dc.registerFont = DC_RegisterFont;
    dc.ownerDrawItem = DC_OwnerDrawItem;
    dc.getValue = DC_GetValue;
    dc.ownerDrawVisible = DC_OwnerDrawVisible;
    dc.runScript = DC_RunScript;
    dc.getTeamColor = DC_GetTeamColor;
    dc.getCVarString = DC_GetCVarString;
    dc.getCVarValue = uii.Cvar_Value;
    dc.setCVar = uii.Cvar_Set;
    dc.drawTextWithCursor = DC_DrawTextWithCursor;
    dc.setOverstrikeMode = DC_SetOverstrikeMode;
    dc.getOverstrikeMode = DC_GetOverstrikeMode;
    dc.startLocalSound = DC_StartLocalSound;
    dc.ownerDrawHandleKey = DC_OwnerDrawHandleKey;
    dc.feederCount = DC_FeederCount;
    dc.feederItemText = DC_FeederItemText;
    dc.fileText = DC_FileText;
    dc.getTranslatedString = DC_GetTranslatedString;
    dc.feederItemImage = DC_FeederItemImage;
    dc.feederSelection = DC_FeederSelection;
    dc.feederAddItem = DC_FeederAddItem;
    dc.keynumToStringBuf = DC_KeynumToStringBuf;
    dc.getBindingBuf = DC_GetBindingBuf;
    dc.setBinding = uii.Key_SetBinding;
    dc.executeText = uii.ExecuteText;
    dc.Error = DC_Error;
    dc.Print = DC_Print;
    dc.Pause = DC_Pause;
    dc.ownerDrawWidth = DC_OwnerDrawWidth;
    dc.registerSound = DC_RegisterSound;
    dc.startBackgroundTrack = DC_StartBackgroundTrack;
    dc.stopBackgroundTrack = DC_StopBackgroundTrack;
    dc.playCinematic = DC_PlayCinematic;
    dc.stopCinematic = DC_StopCinematic;
    dc.drawCinematic = DC_DrawCinematic;
    dc.runCinematicFrame = DC_RunCinematicFrame;

    uii.GetVideoMode( &w, &h );
    dc.xscale = w / 640.0f;
    dc.yscale = h / 480.0f;
    dc.bias = 0;
    dc.realTime = 0;
    dc.frameTime = 0;
    dc.cursorx = SCREEN_WIDTH / 2;
    dc.cursory = SCREEN_HEIGHT / 2;
    dc.debug = qfalse;

    memset( &dc.glconfig, 0, sizeof( dc.glconfig ) );
    dc.glconfig.vidWidth = w;
    dc.glconfig.vidHeight = h;
    dc.glconfig.windowAspect = ( h != 0 ) ? ( (float)w / (float)h ) : ( 4.0f / 3.0f );

    memset( &dc.Assets, 0, sizeof( dc.Assets ) );
    /* the Duke menus move focus with the engine menu move sound */
    dc.Assets.itemFocusSound = Duke_RegisterSound( "bulithit.wav" );
    dc.Assets.fadeClamp = 1.0f;
    dc.Assets.fadeCycle = 5;
    dc.Assets.fadeAmount = 0.05f;
    dc.whiteShader = NULL;
    dc.gradientImage = NULL;
    dc.cursor = NULL;
    dc.FPS = 0;
}

/* ---- module entry points ---- */

static int Init( void ) {
    SetupDisplayContext();
    Init_Display( &dc );
    UI_InitMemory();
    String_Init();
    Duke_MenusInit();
    uii.Print( "Duke UI module loaded\n" );
    return 1;
}

static void Shutdown( void ) {
}

static void KeyEvent( int key, int down ) {
    Duke_KeyEvent( key, down );
}

static void MouseEvent( int dx, int dy ) {
    Duke_MouseEvent( dx, dy );
}

static void Refresh( int realtime ) {
    static int lastRealTime;

    // the framework reads the clock from the display context
    dc.frameTime = realtime - lastRealTime;
    lastRealTime = realtime;
    dc.realTime = realtime;
    Duke_Refresh();
}

static int IsFullscreen( void ) {
    return Duke_IsFullscreen();
}

static void SetActiveMenu( dn_uimenu_t menu ) {
    Duke_SetActiveMenu( menu );
}

static int ConsoleCommand( int ) {
    return 0;
}

extern "C" __declspec( dllexport ) dn_ui_export_t *GetUIAPI( const dn_ui_import_t *imports ) {
    if ( !imports || imports->version != DN_UI_API_VERSION || imports->size != sizeof( *imports ) ||
        !imports->Print || !imports->Error ) {
        return NULL;
    }
    uii = *imports;
    exports.version = DN_UI_API_VERSION;
    exports.size = sizeof( exports );
    exports.Init = Init;
    exports.Shutdown = Shutdown;
    exports.KeyEvent = KeyEvent;
    exports.MouseEvent = MouseEvent;
    exports.Refresh = Refresh;
    exports.IsFullscreen = IsFullscreen;
    exports.SetActiveMenu = SetActiveMenu;
    exports.ConsoleCommand = ConsoleCommand;
    return &exports;
}
