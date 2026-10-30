/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __UI_SHARED_H
#define __UI_SHARED_H


#include "../game/q_shared.h"
#include "tr_types.h"
#include "keycodes.h"

#include "menudef.h"

#define MAX_MENUNAME 32
#define MAX_ITEMTEXT 64
#define MAX_ITEMACTION 64
#define MAX_MENUDEFFILE 4096
#define MAX_MENUFILE 32768
#define MAX_MENUS 64
#define MAX_MENUITEMS 256
#define MAX_COLOR_RANGES 10
#define MAX_OPEN_MENUS 16

/* list feeder ids the .menu listDefs reference through feeder */
#define DN_FEEDER_BINDS      1
#define DN_FEEDER_SERVERS    2

#define WINDOW_MOUSEOVER        0x00000001  // mouse is over it, non exclusive
#define WINDOW_HASFOCUS         0x00000002  // has cursor focus, exclusive
#define WINDOW_VISIBLE          0x00000004  // is visible
#define WINDOW_GREY             0x00000008  // is visible but grey ( non-active )
#define WINDOW_DECORATION       0x00000010  // for decoration only, no mouse, keyboard, etc..
#define WINDOW_FADINGOUT        0x00000020  // fading out, non-active
#define WINDOW_FADINGIN         0x00000040  // fading in
#define WINDOW_MOUSEOVERTEXT    0x00000080  // mouse is over it, non exclusive
#define WINDOW_INTRANSITION     0x00000100  // window is in transition
#define WINDOW_FORECOLORSET     0x00000200  // forecolor was explicitly set ( used to color alpha images or not )
#define WINDOW_HORIZONTAL       0x00000400  // for list boxes and sliders, vertical is default this is set of horizontal
#define WINDOW_LB_LEFTARROW     0x00000800  // mouse is over left/up arrow
#define WINDOW_LB_RIGHTARROW    0x00001000  // mouse is over right/down arrow
#define WINDOW_LB_THUMB         0x00002000  // mouse is over thumb
#define WINDOW_LB_PGUP          0x00004000  // mouse is over page up
#define WINDOW_LB_PGDN          0x00008000  // mouse is over page down
#define WINDOW_ORBITING         0x00010000  // item is in orbit
#define WINDOW_OOB_CLICK        0x00020000  // close on out of bounds click
#define WINDOW_WRAPPED          0x00040000  // manually wrap text
#define WINDOW_AUTOWRAPPED      0x00080000  // auto wrap text
#define WINDOW_FORCED           0x00100000  // forced open
#define WINDOW_POPUP            0x00200000  // popup
#define WINDOW_BACKCOLORSET     0x00400000  // backcolor was explicitly set
#define WINDOW_TIMEDVISIBLE     0x00800000  // visibility timing ( NOT implemented )
#define WINDOW_IGNORE_HUDALPHA  0x01000000  // window will apply cg_hudAlpha value to colors unless this flag is set
#define WINDOW_SCROLLABLE       0x02000000  // windowDef container whose children scroll under the mouse wheel

// CGAME cursor type bits
#define CURSOR_NONE             0x00000001
#define CURSOR_ARROW            0x00000002
#define CURSOR_SIZER            0x00000004

#ifdef CGAME
#define STRING_POOL_SIZE    128 * 1024
#else
#define STRING_POOL_SIZE    384 * 1024
#endif

#define MAX_STRING_HANDLES  4096
#define MAX_SCRIPT_ARGS     12
#define MAX_EDITFIELD       256

#define ART_FX_BASE         "menu/art/fx_base"
#define ART_FX_BLUE         "menu/art/fx_blue"
#define ART_FX_CYAN         "menu/art/fx_cyan"
#define ART_FX_GREEN        "menu/art/fx_grn"
#define ART_FX_RED          "menu/art/fx_red"
#define ART_FX_TEAL         "menu/art/fx_teal"
#define ART_FX_WHITE        "menu/art/fx_white"
#define ART_FX_YELLOW       "menu/art/fx_yel"

#define ASSET_GRADIENTBAR           "ui/assets/gradientbar2.tga"
#define ASSET_SCROLLBAR             "ui/assets/scrollbar.tga"
#define ASSET_SCROLLBAR_ARROWDOWN   "ui/assets/scrollbar_arrow_dwn_a.tga"
#define ASSET_SCROLLBAR_ARROWUP     "ui/assets/scrollbar_arrow_up_a.tga"
#define ASSET_SCROLLBAR_ARROWLEFT   "ui/assets/scrollbar_arrow_left.tga"
#define ASSET_SCROLLBAR_ARROWRIGHT  "ui/assets/scrollbar_arrow_right.tga"
#define ASSET_SCROLL_THUMB          "ui/assets/scrollbar_thumb.tga"
#define ASSET_SLIDER_BAR            "ui/assets/slider2.tga"
#define ASSET_SLIDER_THUMB          "ui/assets/sliderbutt_1.tga"

#define SCROLLBAR_SIZE      16.0
#define SLIDER_WIDTH        160.0
#define SLIDER_HEIGHT       16.0
#define SLIDER_THUMB_WIDTH  16.0
#define SLIDER_THUMB_HEIGHT 20.0
/* the Duke build draws sliders as glyph dash bars; this many segments */
#define DUKE_SLIDER_SEGMENTS 10
#define NUM_CROSSHAIRS      10

typedef struct {
	const char *command;
	const char *args[MAX_SCRIPT_ARGS];
} scriptDef_t;


typedef struct {
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
} rectDef_t;

typedef rectDef_t Rectangle;

class dnWindow {
public:
	Rectangle rect;                 // client coord rectangle
	Rectangle rectClient;           // screen coord rectangle
	const char *name;
	const char *model;
	const char *group;              // if it belongs to a group
	const char *cinematicName;      // cinematic name
	int cinematic;                  // cinematic handle
	int style;
	int border;
	int ownerDraw;                  // ownerDraw style
	int ownerDrawFlags;             // show flags for ownerdraw items
	float borderSize;
	int flags;                      // visible, focus, mouseover, cursor
	Rectangle rectEffects;          // for various effects
	Rectangle rectEffects2;         // for various effects
	int offsetTime;                 // time based value for various effects
	int nextTime;                   // time next effect should cycle
	vec4_t foreColor;               // text color
	vec4_t backColor;               // border color
	vec4_t borderColor;             // border color
	vec4_t outlineColor;            // border color
	qhandle_t background;           // background asset
};

typedef dnWindow Window;


typedef struct {
	vec4_t color;
	int type;
	float low;
	float high;
} colorRangeDef_t;

// the xxxDef_t structs below hang off an item through typeData, one per item
// type; several fields are shared between types, so the names do not always fit
#define MAX_LB_COLUMNS 16

typedef struct columnInfo_s {
	int pos;
	int width;
	int maxChars;
} columnInfo_t;

typedef struct listBoxDef_s {
	int startPos;
	int endPos;
	int drawPadding;
	int cursorPos;
	float elementWidth;
	float elementHeight;
	int elementStyle;
	int numColumns;
	columnInfo_t columnInfo[MAX_LB_COLUMNS];
	const char *doubleClick;
	qboolean notselectable;
} listBoxDef_t;

typedef struct editFieldDef_s {
	float minVal;                   //	edit field limits
	float maxVal;
	float defVal;
	float range;
	int maxChars;                   // for edit fields
	int maxPaintChars;              // for edit fields
	int paintOffset;
} editFieldDef_t;

#define MAX_MULTI_CVARS 32

typedef struct multiDef_s {
	const char *cvarList[MAX_MULTI_CVARS];
	const char *cvarStr[MAX_MULTI_CVARS];
	float cvarValue[MAX_MULTI_CVARS];
	int count;
	qboolean strDef;
} multiDef_t;

typedef struct modelDef_s {
	int angle;
	vec3_t origin;
	float fov_x;
	float fov_y;
	int rotationSpeed;

	int animated;
	int startframe;
	int numframes;
	int loopframes;
	int fps;

	int frame;
	int oldframe;
	float backlerp;
	int frameTime;
} modelDef_t;

#define CVAR_ENABLE     0x00000001
#define CVAR_DISABLE    0x00000002
#define CVAR_SHOW       0x00000004
#define CVAR_HIDE       0x00000008

#define UI_MAX_TEXT_LINES 64

/* BFG gui style named events: handlers can hang off both menus and
   items, the namedEvent script command runs them by name */
#define MAX_NAMED_EVENTS        8
#define MAX_NAMED_EVENT_LENGTH  64

class dnItem {
public:
	Window window;                  // common positional, border, style, layout info
	Rectangle textRect;             // rectangle the text ( if any ) consumes
	int type;                       // text, button, radiobutton, checkbox, textfield, listbox, combo
	int alignment;                  // left center right
	int font;
	int textalignment;              // ( optional ) alignment for text within rect based on text width
	float textalignx;               // ( optional ) text alignment x coord
	float textaligny; // ( optional ) text alignment y coord
	float textscale;                // scale percentage from 72pts
	int textStyle;                  // ( optional ) style, normal and shadowed are it for now
	const char *text;               // display text
	qboolean textSavegameInfo; // text comes from the ui_savegameInfo cvar
	void *parent;                   // menu owner
	dnItem *containerItem;          // nested defs: the enclosing windowDef, NULL at menu scope
	float scrollOffset;            // scrollable containers: how far the children scrolled down
	float scrollContent;           // scrollable containers: content height measured at offset 0
	qhandle_t asset;                // handle to asset
	const char *mouseEnterText;     // mouse enter script
	const char *mouseExitText;      // mouse exit script
	const char *mouseEnter;         // mouse enter script
	const char *mouseExit;          // mouse exit script
	const char *action;             // select script
	const char *onAccept;           // run when the user presses the enter key
	const char *onFocus;            // select script
	const char *leaveFocus;         // select script
	const char *cvar;               // associated cvar
	const char *cvarTest;           // associated cvar for enable actions
	const char *enableCvar;         // enable, disable, show, or hide based on value, this can contain a list
	int cvarFlags;                  //	what type of action to take on cvarenables
	sfxHandle_t focusSound;
	int numColors;                  // number of color ranges
	colorRangeDef_t colorRanges[MAX_COLOR_RANGES];
	int colorRangeType; // whether the color ranges are absolute or cvar relative
	float special;                  // used for feeder id's etc.. diff per type
	int cursorPos;                  // cursor position in characters
	void *typeData;                 // type specific data ptr's
	char namedEventNames[MAX_NAMED_EVENTS][MAX_NAMED_EVENT_LENGTH];
	const char *namedEventScripts[MAX_NAMED_EVENTS];
	int namedEventCount;

	/* resets the item to its defaults; the .menu parser calls it on every
	   fresh itemDef block */
	void Init( void );

	/* the former Item_* framework helpers as methods */
	void RunScript( const char *s, int depth = 0 );
	void SetScreenCoords( float x, float y );
	void UpdatePosition( void );
	qboolean EnableShowViaCvar( int flag );
	qboolean SetFocus( float x, float y );
	void MouseEnter( float x, float y );
	void MouseLeave( void );
	void SetMouseOver( qboolean focus );
	/* drag capture: listboxes and sliders grab arrows and thumbs */
	virtual void StartCapture( int key );
	void StopCapture( void );
	qboolean HandleKey( int key, qboolean down );
	/* edit fields take their keys through the menu edit state */
	virtual qboolean HandleEditKey( int key );
	/* feeder scroll from script code steps the owning list */
	virtual qboolean ScrollFeed( qboolean down );

	/* per-type dispatch hooks; the item type subclasses override the
	   ones their type needs, the defaults cover the rest */
	virtual void PaintType( void );
	virtual qboolean HandleTypeKey( int key, qboolean down );
	virtual void MouseEnterRect( float x, float y );
	virtual void InitControls( void );

	void Action( void );
	rectDef_t *CorrectedTextRect( void );
	void SetTextExtents( int *width, int *height, const char *text );
	void TextColor( vec4_t *newColor );
	void Paint( void );

	/* shared row helpers the type subclasses draw through: the value
	   column lines every value up vertically, the tree walk gates the
	   nested defs */
	float ValueColumnX( void );
	qboolean TreeVisible( void );
	void PaintText( void );
	void PaintWrappedText( void );
	void PaintAutoWrappedText( void );
	/* kept from the port: image rows have no subclass wired to a type */
	void PaintImage( void );

	/* script items learn their type while the itemDef block parses, so
	   the subclass vtable is installed once the type is known */
	void FinalizeType( void );

	void ValidateTypeData( void );
	void CacheContents( void );
};

class dnMenu {
public:
	Window window;
	const char  *font;              // font
	qboolean fullScreen;            // covers entire screen
	int itemCount;                  // number of items;
	int fontIndex;
	int cursorItem;                 // item the cursor is on
	int fadeCycle;
	float fadeClamp;
	float fadeAmount;
	const char *onOpen;             // run when the menu is first opened
	const char *onClose;            // run when the menu is closed
	const char *onESC;              // run when the menu is closed
	const char *onKey[255];         // scripts run when the matching key is pressed
	const char *soundName;          // background loop sound for menu
	const char *onROQDone;          // run when a menu cinematic (roq) finishes
	char namedEventNames[MAX_NAMED_EVENTS][MAX_NAMED_EVENT_LENGTH];
	const char *namedEventScripts[MAX_NAMED_EVENTS];
	int namedEventCount;

	vec4_t focusColor;              // focus color for items
	vec4_t disableColor; // color used while the item is disabled
	dnItem *items[MAX_MENUITEMS]; // items this menu contains

	/* framework defaults and the by-name item lookups as methods */
	void Init( void );
	dnItem *FindItemByName( const char *p );
	int ItemsMatchingGroup( const char *name );
	dnItem *GetMatchingItemByNumber( int index, const char *name );

	/* the former Menu_* framework helpers as methods */
	dnItem *AddItem( const char *name, int type );
	void PostParse( void );
	void UpdatePosition( void );
	dnItem *ClearFocus( void );
	void ShowItemByName( const char *p, qboolean bShow );
	void FadeItemByName( const char *p, qboolean fadeOut );
	void RunCloseScript( void );
	void TransitionItemByName( const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt );
	void OrbitItemByName( const char *p, float x, float y, float cx, float cy, int time );
	dnItem *HitTest( float x, float y );
	void CloseCinematics( void );
	void Activate( void );
	void HandleKey( int key, qboolean down );
	void HandleMouseMove( float x, float y );
	void Paint( qboolean forcePaint );
	void ScrollFeeder( int feeder, qboolean down );
	void CacheContents( void );
	qboolean OverActiveItem( float x, float y );
	dnItem *GetFocusedItem( void );
	dnItem *SetPrevCursorItem( void );
	dnItem *SetNextCursorItem( void );

	/* scrollable container support: gating, wheel scroll and the lookup */
	qboolean ItemInView( dnItem *item );
	void ScrollContainerBy( dnItem *container, float delta );
	dnItem *FindScrollContainer( void );
};

/* the loaded menu set: the menu array, the open menu stack and every
   lookup the module runs by menu name. One global instance, g_menus,
   owns it all; the .menu parser fills it through AllocMenu while a
   menu block parses */
class dnMenuSystem {
public:
	void Reset( void );
	dnMenu *AllocMenu( const char *name );
	dnMenu *FindByName( const char *p );
	dnMenu *GetFocused( void );
	dnMenu *CaptureItem( int x, int y );
	qboolean AnyFullScreenVisible( void );
	int VisibleMenuCount( void );
	int Count( void );
	dnMenu *ActivateByName( const char *p );
	void OpenByName( const char *p );
	void ShowByName( const char *p );
	void CloseByName( const char *p );
	void CloseAll( void );
	void HandleOOBClick( dnMenu *menu, int key, qboolean down );
	void SetFeederSelection( dnMenu *menu, int feeder, int index, const char *name );
	void HandleKey( int key, qboolean down, int x, int y );
	qboolean MouseMove( dnMenu *menu, int x, int y );
	int CursorType( int x, int y );
	void RunNamedEvent( const char *name );
	void CloseCinematics( void );
	void PaintAll( void );
	void CacheAll( void );

private:
	dnMenu menus[MAX_MENUS];
	int menuCount;
	dnMenu *menuStack[MAX_OPEN_MENUS];
	int openMenuCount;
};

extern dnMenuSystem g_menus;

typedef struct {
	const char  *fontStr;
	const char  *cursorStr;
	const char  *gradientStr;
	fontInfo_t textFont;
	fontInfo_t smallFont;
	fontInfo_t bigFont;
	fontInfo_t handwritingFont;
	qhandle_t cursor;
	qhandle_t gradientBar;
	qhandle_t scrollBarArrowUp;
	qhandle_t scrollBarArrowDown;
	qhandle_t scrollBarArrowLeft;
	qhandle_t scrollBarArrowRight;
	qhandle_t scrollBar;
	qhandle_t scrollBarThumb;
	qhandle_t buttonMiddle;
	qhandle_t buttonInside;
	qhandle_t solidBox;
	qhandle_t sliderBar;
	qhandle_t sliderThumb;
	sfxHandle_t menuEnterSound;
	sfxHandle_t menuExitSound;
	sfxHandle_t menuBuzzSound;
	sfxHandle_t itemFocusSound;
	float fadeClamp;
	int fadeCycle;
	float fadeAmount;
	float shadowX;
	float shadowY;
	vec4_t shadowColor;
	float shadowFadeClamp;
	qboolean fontRegistered;

	// player settings
	qhandle_t fxBasePic;
	qhandle_t fxPic[7];
	qhandle_t crosshairShader[NUM_CROSSHAIRS];

} cachedAssets_t;

typedef struct {
	const char *name;
	void ( *handler )( dnItem *item, char** args );
} commandDef_t;

typedef struct {
	qhandle_t ( *registerShaderNoMip )( const char *p );
	void ( *setColor )( const vec4_t v );
	void ( *drawHandlePic )( float x, float y, float w, float h, qhandle_t asset );
	void ( *drawStretchPic )( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
	void ( *drawText )( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style );
	int ( *textWidth )( const char *text, int font, float scale, int limit );
	int ( *textHeight )( const char *text, int font, float scale, int limit );
	qhandle_t ( *registerModel )( const char *p );
	void ( *modelBounds )( qhandle_t model, vec3_t min, vec3_t max );
	void ( *fillRect )( float x, float y, float w, float h, const vec4_t color );
	void ( *drawRect )( float x, float y, float w, float h, float size, const vec4_t color );
	void ( *drawSides )( float x, float y, float w, float h, float size );
	void ( *drawTopBottom )( float x, float y, float w, float h, float size );
	void ( *clearScene )();
	void ( *addRefEntityToScene )( const refEntity_t *re );
	void ( *renderScene )( const refdef_t *fd );
	void ( *registerFont )( const char *pFontname, int pointSize, fontInfo_t *font );
	void ( *ownerDrawItem )( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle );
	float ( *getValue )( int ownerDraw, int type );
	qboolean ( *ownerDrawVisible )( int flags );
	void ( *runScript )( char **p );
	void ( *getTeamColor )( vec4_t *color );
	void ( *getCVarString )( const char *cvar, char *buffer, int bufsize );
	float ( *getCVarValue )( const char *cvar );
	void ( *setCVar )( const char *cvar, const char *value );
	void ( *drawTextWithCursor )( float x, float y, int font, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style );
	/* the Duke nuke symbol beside the focused row */
	void ( *drawCursor )( float x, float y );
	void ( *setOverstrikeMode )( qboolean b );
	qboolean ( *getOverstrikeMode )();
	void ( *startLocalSound )( sfxHandle_t sfx, int channelNum );
	qboolean ( *ownerDrawHandleKey )( int ownerDraw, int flags, float *special, int key );
	int ( *feederCount )( float feederID );
	const char *( *feederItemText )( float feederID, int index, int column, qhandle_t * handle );

	const char *( *fileText )( char *flieName );
	const char *( *getTranslatedString )( const char *inString );

	qhandle_t ( *feederItemImage )( float feederID, int index );
	void ( *feederSelection )( float feederID, int index );
	void ( *feederAddItem )( float feederID, const char *name, int index );

	void ( *keynumToStringBuf )( int keynum, char *buf, int buflen );
	void ( *getBindingBuf )( int keynum, char *buf, int buflen );
	void ( *setBinding )( int keynum, const char *binding );
	void ( *executeText )( int exec_when, const char *text );
	void ( *Error )( int level, const char *error, ... );
	void ( *Print )( const char *msg, ... );
	void ( *Pause )( qboolean b );
	int ( *ownerDrawWidth )( int ownerDraw, int font, float scale );
	sfxHandle_t ( *registerSound )( const char *name );
	void ( *startBackgroundTrack )( const char *intro, const char *loop, int fadeupTime );
	void ( *stopBackgroundTrack )();
	int ( *playCinematic )( const char *name, float x, float y, float w, float h );
	void ( *stopCinematic )( int handle );
	void ( *drawCinematic )( int handle, float x, float y, float w, float h );
	void ( *runCinematicFrame )( int handle );

	float yscale;
	float xscale;
	float bias;
	int realTime;
	int frameTime;
	int cursorx;
	int cursory;
	qboolean debug;

	cachedAssets_t Assets;

	glconfig_t glconfig;
	qhandle_t whiteShader;
	qhandle_t gradientImage;
	qhandle_t cursor;
	float FPS;

} displayContextDef_t;



typedef struct {
	char *name;
	char *localname;
} translateString_t;

#define MAX_TRANSLATESTRINGS 64
extern translateString_t translateStrings[MAX_TRANSLATESTRINGS];



const char *String_Alloc( const char *p );
void String_Init();
void String_Report();
void Init_Display( displayContextDef_t *dc );
void Display_ExpandMacros( char * buff );
/* menus come from the .menu script files: g_menus.AllocMenu takes a menu
   slot (the qcommon parser calls it while a menu block parses) */
qboolean Float_Parse( char **p, float *f );
qboolean Color_Parse( char **p, vec4_t *c );
qboolean Int_Parse( char **p, int *i );
qboolean Rect_Parse( char **p, rectDef_t *r );
qboolean String_Parse( char **p, const char **out );
qboolean Script_Parse( char **p, const char **out );

displayContextDef_t *Display_GetContext();
qboolean    Display_KeyBindPending();
void        LerpColor( vec4_t a, vec4_t b, vec4_t c, float t );

void        *UI_Alloc( int size );
void        UI_InitMemory( void );
qboolean    UI_OutOfMemory();
/* allocates an item block from the UI memory pool and builds the subclass
   matching the type; the .menu parser in qcommon/dnmenuparser.cpp builds
   its itemDefs through it */
dnItem       *UI_ConstructItem( int type );
void        UI_RoQDone( void ); // ui callback.  roq is done, allow menu to handle if it wants

void        Controls_GetConfig( void );
void        Controls_SetConfig( qboolean restart );
void        Controls_SetDefaults( void );

/* binding list feeder accessors; the labels come from ui/binds.lst */
int         UI_BindListCount( void );
const char  *UI_BindListLabel( int index );
const char  *UI_BindListKeys( int index );


#endif
