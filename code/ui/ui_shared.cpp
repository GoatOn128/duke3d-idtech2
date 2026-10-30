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

// the idTech 3 menu framework: string pool, items, menus and the runtime
// menu code; the .menu parser lives in qcommon/dnmenuparser.cpp now

#include "ui_local.h"

// placement new installs the item subclass vtables
#include <new>

#define SCROLL_TIME_START                   500
#define SCROLL_TIME_ADJUST              150
#define SCROLL_TIME_ADJUSTOFFSET    40
#define SCROLL_TIME_FLOOR                   20

/* the import table the engine hands the UI module; nearly every helper
   below draws or reads state through it */
displayContextDef_t *DC = NULL;

/* ---- mouse capture ---- */

/* while the user holds a listbox arrow or drags a listbox/slider thumb,
   the item keeps receiving updates: the capture stores the mode, the
   auto scroll timing and the item that owns the drag */
enum captureKind_t {
	CAPTURE_NONE,
	CAPTURE_LISTBOX_AUTO,
	CAPTURE_LISTBOX_THUMB,
	CAPTURE_SLIDER_THUMB
};

class dnMouseCapture {
public:
	qboolean Active( void ) const { return kind != CAPTURE_NONE; }
	dnItem *Item( void ) const { return item; }
	/* releases the drag; the captured item hears it through StopCapture */
	void Stop( void ) {
		if ( item ) {
			item->StopCapture();
		}
		item = NULL;
		kind = CAPTURE_NONE;
	}
	void StartListBoxAuto( dnItem *owner, int key, qboolean leftArrow );
	void StartListBoxThumb( dnItem *owner, int key );
	void StartSliderThumb( dnItem *owner, int key );
	/* runs the held scroll once per frame before the menus paint */
	void Update( void );

private:
	void AutoScroll( void );
	void ThumbScroll( void );
	void SliderThumb( void );

	captureKind_t kind;
	dnItem *item;
	int nextScrollTime;             // time the next auto scroll step fires
	int nextAdjustTime;             // time the auto scroll speed steps up
	int adjustValue;                // current delay between auto steps
	int scrollKey;                  // key that started the capture
	qboolean scrollDir;             // arrow direction of the auto scroll
	float xStart;                   // thumb drag start
	float yStart;
};

static dnMouseCapture g_capture;

/* ---- key binding capture ---- */

/* while the user picks a new key the menu swallows every key press; the
   waiting item can be a bindDef or a row of the bind list, the row
   command rides along for the second case */
class dnBindCapture {
public:
	qboolean Waiting( void ) const { return waiting; }
	dnItem *Item( void ) const { return item; }
	/* rowCmd can be NULL: a bindDef carries its command in the cvar */
	void Start( dnItem *owner, const char *rowCmd ) {
		if ( rowCmd ) {
			Q_strncpyz( rowCommand, rowCmd, sizeof( rowCommand ) );
		}
		waiting = qtrue;
		item = owner;
	}
	void Cancel( void ) { waiting = qfalse; }
	/* the former Item_Bind_HandleKey: eats the picked key, clears the
	   old bindings of the command and writes the new one */
	qboolean HandleKey( int key, qboolean down );

private:
	qboolean waiting;
	dnItem *item;
	char rowCommand[64];
};

static dnBindCapture g_bindCapture;

/* ---- edit field capture ---- */

/* the item whose text is being edited; its keys route through the
   item's HandleEditKey until the user clicks elsewhere */
class dnEditCapture {
public:
	qboolean IsActive( void ) const { return active; }
	dnItem *Item( void ) const { return item; }
	void Start( dnItem *owner ) { active = qtrue; item = owner; }
	void Stop( void ) { active = qfalse; item = NULL; }
	qboolean HandleKey( int key ) { return item->HandleEditKey( key ); }

private:
	qboolean active;
	dnItem *item;
};

static dnEditCapture g_editCapture;

static qboolean debugMode = qfalse;

#define DOUBLE_CLICK_DELAY 300
static int lastListBoxClickTime = 0;

int BindingIDFromName( const char *name );

/* ---- the item type subclasses ---- */

/* each class hooks its type behavior through the virtuals above; none
   of them adds fields, so every one keeps the plain dnItem layout and
   fits the one block size the allocator hands out. The per-type
   painters and key handlers live below as out-of-line methods. */
class dnTextItem : public dnItem {
public:
	void PaintType( void );
};

class dnOwnerDrawItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );
};

/* edit, numeric and filename fields paint through the shared text field
   painter; their key entry runs through the menu edit state */
class dnEditFieldItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleEditKey( int key );
};

class dnListBoxItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );
	/* the full list key logic, shared by the type hook, the feeder
	   scroll and the auto scroll capture */
	qboolean HandleListKey( int key, qboolean down, qboolean force );
	void MouseEnterRect( float x, float y );
	/* start a fresh list on the top row */
	void InitControls( void ) {
		listBoxDef_t *listPtr = ( listBoxDef_t * )typeData;

		cursorPos = 0;
		if ( listPtr ) {
			listPtr->cursorPos = 0;
			listPtr->startPos = 0;
			listPtr->endPos = 0;
		}
	}
	/* a held arrow or grabbed thumb starts a drag capture */
	void StartCapture( int key );
	/* feeder scroll from script code steps the list like a held arrow */
	qboolean ScrollFeed( qboolean down ) {
		return HandleListKey( ( down ) ? K_DOWNARROW : K_UPARROW, qtrue, qtrue );
	}
	int OverListBox( float x, float y );
	int MaxScroll( void );
	int ThumbPosition( void );
	int ThumbDrawPosition( void );

private:
	vec_t *RowColor( int row );
};

class dnModelItem : public dnItem {
public:
	void PaintType( void );
};

class dnYesNoItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );
};

class dnMultiItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );

private:
	int CountSettings( void );
	int FindCvarByValue( void );
	const char *Setting( void );
};

class dnBindItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );
};

class dnSliderItem : public dnItem {
public:
	void PaintType( void );
	qboolean HandleTypeKey( int key, qboolean down );
	void StartCapture( int key );

private:
	float ThumbPosition( void );
	int OverSlider( float x, float y );
};

/* the loaded menu set: menus, the open stack and the by-name lookups
   the whole module reaches through g_menus */
dnMenuSystem g_menus;


#ifdef CGAME
#define MEM_POOL_SIZE  128 * 1024
#else
#define MEM_POOL_SIZE  1024 * 1024
#endif

static char memoryPool[MEM_POOL_SIZE];
static int allocPoint, outOfMemory;


// these are expected to be translated by the strings.txt file
translateString_t translateStrings[] = {
	{"end_time"},                //	"Time"
	{"end_objectives"},          //	"Objectives"
	{"end_secrets"},         //	"Secret Areas"
	{"end_treasure"},            //	"Treasure"
	{"end_attempts"},            //	"Attempts"

	{"missionfail0"},            //	"Mission Failed"
	{"secretarea"},              //	"You found a secret area"
	{"objectivesnotcomplete"},   //	"Objectives not complete"
	{"drankwine"},               //	"You drank the wine"
	{"noitem"},                  //	"No item to use"
	{"gamesaved"},               //	"Game Saved"

	{"missionfail1"},            //	"Mission Failed\nYou Killed a Civilian"
	{"missionfail2"},            //	"Mission Failed\nYou Killed a Kreisau Agent"
	{"missionfail3"},            //	"Mission Failed\nYou Killed Kessler"
	{"missionfail4"},            //	"Mission Failed\nYou Killed Karl"
	{"missionfail5"},            //	"Mission Failed\nYou Have Been Detected"
	{"missionfail6"},            //	"Mission Failed\nRocket Launched"
	{"missionfail7"},            //	"Mission Failed\nThe Scientist Has Been Killed"
	{"missionfail8"},            //	"fail 8"
	{"missionfail9"},            //	"fail 9"
	{"missionfail10"},           //	"fail 10"
	{"missionfail11"},           //	"fail 11"
	{"missionfail12"},           //	"fail 12"
	{"missionfail13"},           //	"fail 13"
	{"missionfail14"},           //	"fail 14"
	{"missionfail15"},           //	"fail 15"
	{"missionfail16"},           //	"fail 16"
	{"end_title"},               //	"Mission Stats"
	{"end_exit"},                //	"Proceed forward to exit..."
	{"end_noexit"},              //	"Exit not yet available"

	{"yes"},
	{"no"},
	{"keywait"},             //	"Waiting for new key... Press ESCAPE to cancel"
	{"keychange"},               //	"Press ENTER or CLICK to change, Press BACKSPACE to clear"
	{"pleasewait"},              //	"Please Wait..."
	{"dynamitetimer"},           //	"Dynamite timer set at"
	{"second"},
	{"seconds"},
	{"minute"},
	{"minutes"},
	{"hour"},
	{"hours"},
	{"day"},
	{"days"},
	{"month"},
	{"months"},
	{"year"},
	{"years"},
	{"or"}
};

void UI_RoQDone( void ) {
	dnMenu *menu = g_menus.GetFocused();
	if ( menu->onROQDone ) {
		dnItem it;
		it.parent = menu;
		it.RunScript( menu->onROQDone );

	}
}


void *UI_Alloc( int size ) {
	char    *p;

	if ( allocPoint + size > MEM_POOL_SIZE ) {
		outOfMemory = qtrue;
		if ( DC->Print ) {
			DC->Print( "UI_Alloc: Failure. Out of memory!\n" );
		}
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

void UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = qfalse;
}

qboolean UI_OutOfMemory() {
	return outOfMemory;
}





#define HASH_TABLE_SIZE 2048
/*
================
return a hash value for the string
================
*/
static long hashForString( const char *str ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( str[i] != '\0' ) {
		letter = tolower( str[i] );
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( HASH_TABLE_SIZE - 1 );
	return hash;
}

typedef struct stringDef_s {
	struct stringDef_s *next;
	const char *str;
} stringDef_t;

static int strPoolIndex = 0;
static char strPool[STRING_POOL_SIZE];

static int strHandleCount = 0;
static stringDef_t *strHandle[HASH_TABLE_SIZE];


const char *String_Alloc( const char *p ) {
	int len;
	long hash;
	stringDef_t *str, *last;
	static const char *staticNULL = "";

	if ( p == NULL ) {
		return NULL;
	}

	if ( *p == 0 ) {
		return staticNULL;
	}

	hash = hashForString( p );

	str = strHandle[hash];
	while ( str ) {
		if ( strcmp( p, str->str ) == 0 ) {
			return str->str;
		}
		str = str->next;
	}

	len = strlen( p );
	if ( len + strPoolIndex + 1 < STRING_POOL_SIZE ) {
		int ph = strPoolIndex;
		strcpy( &strPool[strPoolIndex], p );
		strPoolIndex += len + 1;

		str = strHandle[hash];
		last = str;
		while ( str && str->next ) {
			last = str;
			str = str->next;
		}

		str  = ( stringDef_t * )UI_Alloc( sizeof( stringDef_t ) );
		str->next = NULL;
		str->str = &strPool[ph];
		if ( last ) {
			last->next = str;
		} else {
			strHandle[hash] = str;
		}
		return &strPool[ph];
	}
	return NULL;
}

void String_Report() {
	float f;
	UI_Printf( "Memory/String Pool Info\n" );
	UI_Printf( "----------------\n" );
	f = strPoolIndex;
	f /= STRING_POOL_SIZE;
	f *= 100;
	UI_Printf( "String Pool is %.1f%% full, %i bytes out of %i used.\n", f, strPoolIndex, STRING_POOL_SIZE );
	f = allocPoint;
	f /= MEM_POOL_SIZE;
	f *= 100;
	UI_Printf( "Memory Pool is %.1f%% full, %i bytes out of %i used.\n", f, allocPoint, MEM_POOL_SIZE );
}


void String_Init() {
	int i;
	for ( i = 0; i < HASH_TABLE_SIZE; i++ ) {
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	g_menus.Reset();
	UI_InitMemory();
	if ( DC && DC->getBindingBuf ) {
		Controls_GetConfig();
	}
}

/*
=================
LerpColor
	lerp and clamp each component of <a> and <b> into <c> by the fraction <t>
=================
*/
void LerpColor( vec4_t a, vec4_t b, vec4_t c, float t ) {
	int i;
	for ( i = 0; i < 4; i++ )
	{
		c[i] = a[i] + t * ( b[i] - a[i] );
		if ( c[i] < 0 ) {
			c[i] = 0;
		} else if ( c[i] > 1.0 ) {
			c[i] = 1.0;
		}
	}
}

qboolean Float_Parse( char **p, float *f ) {
	char    *token;
	token = COM_ParseExt( p, qfalse );
	if ( token && token[0] != 0 ) {
		*f = atof( token );
		return qtrue;
	} else {
		return qfalse;
	}
}

qboolean Color_Parse( char **p, vec4_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !Float_Parse( p, &f ) ) {
			return qfalse;
		}
		( *c )[i] = f;
	}
	return qtrue;
}

qboolean Int_Parse( char **p, int *i ) {
	char    *token;
	token = COM_ParseExt( p, qfalse );

	if ( token && token[0] != 0 ) {
		*i = atoi( token );
		return qtrue;
	} else {
		return qfalse;
	}
}

qboolean Rect_Parse( char **p, rectDef_t *r ) {
	if ( Float_Parse( p, &r->x ) ) {
		if ( Float_Parse( p, &r->y ) ) {
			if ( Float_Parse( p, &r->w ) ) {
				if ( Float_Parse( p, &r->h ) ) {
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

qboolean String_Parse( char **p, const char **out ) {
	char *token;

	token = COM_ParseExt( p, qfalse );
	if ( token && token[0] != 0 ) {
		*( out ) = String_Alloc( token );
		return qtrue;
	}
	return qfalse;
}

// display, window, menu, item code

/*
==================
Init_Display

Initializes the display with a structure to all the drawing routines
 ==================
*/
void Init_Display( displayContextDef_t *dc ) {
	DC = dc;
}



// type and style painting

void GradientBar_Paint( rectDef_t *rect, vec4_t color ) {
	// gradient bar takes two paints
	DC->setColor( color );
	DC->drawHandlePic( rect->x, rect->y, rect->w, rect->h, DC->Assets.gradientBar );
	DC->setColor( NULL );
}


/*
==================
Window_Init

Initializes a window structure ( dnWindow ) with defaults

==================
*/
void Window_Init( Window *w ) {
	memset( w, 0, sizeof( dnWindow ) );
	w->borderSize = 1;
	w->foreColor[0] = w->foreColor[1] = w->foreColor[2] = w->foreColor[3] = 1.0;
	w->cinematic = -1;
}

void Fade( int *flags, float *f, float clamp, int *nextTime, int offsetTime, qboolean bFlags, float fadeAmount ) {
	if ( *flags & ( WINDOW_FADINGOUT | WINDOW_FADINGIN ) ) {
		if ( DC->realTime > *nextTime ) {
			*nextTime = DC->realTime + offsetTime;
			if ( *flags & WINDOW_FADINGOUT ) {
				*f -= fadeAmount;
				if ( bFlags && *f <= 0.0 ) {
					*flags &= ~( WINDOW_FADINGOUT | WINDOW_VISIBLE );
				}
			} else {
				*f += fadeAmount;
				if ( *f >= clamp ) {
					*f = clamp;
					if ( bFlags ) {
						*flags &= ~WINDOW_FADINGIN;
					}
				}
			}
		}
	}
}



void Window_Paint( Window *w, float fadeAmount, float fadeClamp, float fadeCycle ) {
	vec4_t color;
	rectDef_t fillRect = w->rect;

	if ( debugMode ) {
		color[0] = color[1] = color[2] = color[3] = 1;
		DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, 1, color );
	}

	if ( w == NULL || ( w->style == 0 && w->border == 0 ) ) {
		return;
	}

	if ( w->border != 0 ) {
		fillRect.x += w->borderSize;
		fillRect.y += w->borderSize;
		fillRect.w -= w->borderSize + 1;
		fillRect.h -= w->borderSize + 1;
	}

	if ( w->style == WINDOW_STYLE_FILLED ) {
		// box, but possible a shader that needs filled
		if ( w->background ) {
			Fade( &w->flags, &w->backColor[3], fadeClamp, &w->nextTime, fadeCycle, qtrue, fadeAmount );
			DC->setColor( w->backColor );
			DC->drawHandlePic( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background );
			DC->setColor( NULL );
		} else {
			DC->fillRect( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->backColor );
		}
	} else if ( w->style == WINDOW_STYLE_GRADIENT ) {
		GradientBar_Paint( &fillRect, w->backColor );
	} else if ( w->style == WINDOW_STYLE_SHADER ) {
		if ( w->flags & WINDOW_FORECOLORSET ) {
			DC->setColor( w->foreColor );
		}
		DC->drawHandlePic( fillRect.x, fillRect.y, fillRect.w, fillRect.h, w->background );
		DC->setColor( NULL );
	} else if ( w->style == WINDOW_STYLE_TEAMCOLOR ) {
		if ( DC->getTeamColor ) {
			DC->getTeamColor( &color );
			DC->fillRect( fillRect.x, fillRect.y, fillRect.w, fillRect.h, color );
		}
	} else if ( w->style == WINDOW_STYLE_CINEMATIC ) {
		if ( w->cinematic == -1 ) {
			w->cinematic = DC->playCinematic( w->cinematicName, fillRect.x, fillRect.y, fillRect.w, fillRect.h );
			if ( w->cinematic == -1 ) {
				w->cinematic = -2;
			}
		}
		if ( w->cinematic >= 0 ) {
			DC->runCinematicFrame( w->cinematic );
			DC->drawCinematic( w->cinematic, fillRect.x, fillRect.y, fillRect.w, fillRect.h );
		}
	}

	if ( w->border == WINDOW_BORDER_FULL ) {
		// the team color border gets lightened so it reads on the fill
		if ( w->style == WINDOW_STYLE_TEAMCOLOR ) {
			if ( color[0] > 0 ) {
				// red
				color[0] = 1;
				color[1] = color[2] = .5;

			} else {
				color[2] = 1;
				color[0] = color[1] = .5;
			}
			color[3] = 1;
			DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, color );
		} else {
			DC->drawRect( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize, w->borderColor );
		}
	} else if ( w->border == WINDOW_BORDER_HORZ ) {
		// top/bottom
		DC->setColor( w->borderColor );
		DC->drawTopBottom( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize );
		DC->setColor( NULL );
	} else if ( w->border == WINDOW_BORDER_VERT ) {
		// left right
		DC->setColor( w->borderColor );
		DC->drawSides( w->rect.x, w->rect.y, w->rect.w, w->rect.h, w->borderSize );
		DC->setColor( NULL );
	} else if ( w->border == WINDOW_BORDER_KCGRADIENT ) {
		// this is just two gradient bars along each horz edge
		rectDef_t r = w->rect;
		r.h = w->borderSize;
		GradientBar_Paint( &r, w->borderColor );
		r.y = w->rect.y + w->rect.h - 1;
		GradientBar_Paint( &r, w->borderColor );
	}

}


void dnItem::SetScreenCoords( float x, float y ) {

	if ( window.border != 0 ) {
		x += window.borderSize;
		y += window.borderSize;
	}

	window.rect.x = x + window.rectClient.x;
	window.rect.y = y + window.rectClient.y;
	window.rect.w = window.rectClient.w;
	window.rect.h = window.rectClient.h;

	// force the text rects to recompute
	textRect.w = 0;
	textRect.h = 0;
}

void dnItem::UpdatePosition( void ) {
	float x, y;
	dnMenu *menu;

	if ( parent == NULL ) {
		return;
	}

	menu = (dnMenu *)parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	if ( menu->window.border != 0 ) {
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	SetScreenCoords( x, y );

}

// menus
void dnMenu::UpdatePosition( void ) {
	int i;
	float x, y;

	x = window.rect.x;
	y = window.rect.y;
	if ( window.border != 0 ) {
		x += window.borderSize;
		y += window.borderSize;
	}

	for ( i = 0; i < itemCount; i++ ) {
		items[i]->SetScreenCoords( x, y );
	}
}

void dnMenu::PostParse( void ) {
	if ( fullScreen ) {
		window.rect.x = 0;
		window.rect.y = 0;
		window.rect.w = 640;
		window.rect.h = 480;
	}
	UpdatePosition();
}

dnItem *dnMenu::ClearFocus( void ) {
	int i;
	dnItem *ret = NULL;

	for ( i = 0; i < itemCount; i++ ) {
		if ( items[i]->window.flags & WINDOW_HASFOCUS ) {
			ret = items[i];
		}
		items[i]->window.flags &= ~WINDOW_HASFOCUS;
		if ( items[i]->leaveFocus ) {
			items[i]->RunScript( items[i]->leaveFocus );
		}
	}

	return ret;
}

qboolean IsVisible( int flags ) {
	return ( flags & WINDOW_VISIBLE && !( flags & WINDOW_FADINGOUT ) );
}

qboolean Rect_ContainsPoint( rectDef_t *rect, float x, float y ) {
	if ( rect ) {
		if ( x > rect->x && x < rect->x + rect->w && y > rect->y && y < rect->y + rect->h ) {
			return qtrue;
		}
	}
	return qfalse;
}

int dnMenu::ItemsMatchingGroup( const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare


	pdest = ( char * )strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( items[i]->window.name && Q_strncmp( items[i]->window.name, name, wildcard ) == 0 || ( items[i]->window.group && Q_strncmp( items[i]->window.group, name, wildcard ) == 0 ) ) {
				count++;
			}
		} else {
			if ( items[i]->window.name && Q_strcasecmp( items[i]->window.name, name ) == 0 || ( items[i]->window.group && Q_strcasecmp( items[i]->window.group, name ) == 0 ) ) {
				count++;
			}
		}
	}

	return count;
}

dnItem *dnMenu::GetMatchingItemByNumber( int index, const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare

	pdest = ( char * )strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( items[i]->window.name && Q_strncmp( items[i]->window.name, name, wildcard ) == 0 || ( items[i]->window.group && Q_strncmp( items[i]->window.group, name, wildcard ) == 0 ) ) {
				if ( count == index ) {
					return items[i];
				}
				count++;
			}
		} else {
			if ( items[i]->window.name && Q_strcasecmp( items[i]->window.name, name ) == 0 || ( items[i]->window.group && Q_strcasecmp( items[i]->window.group, name ) == 0 ) ) {
				if ( count == index ) {
					return items[i];
				}
				count++;
			}
		}
	}
	return NULL;
}



void Script_SetColor( dnItem *item, char **args ) {
	const char *name;
	int i;
	float f;
	vec4_t *out;
	// expecting type of color to set and 4 args for the color
	if ( String_Parse( args, &name ) ) {
		out = NULL;
		if ( Q_strcasecmp( name, "backcolor" ) == 0 ) {
			out = &item->window.backColor;
			item->window.flags |= WINDOW_BACKCOLORSET;
		} else if ( Q_strcasecmp( name, "forecolor" ) == 0 ) {
			out = &item->window.foreColor;
			item->window.flags |= WINDOW_FORECOLORSET;
		} else if ( Q_strcasecmp( name, "bordercolor" ) == 0 ) {
			out = &item->window.borderColor;
		}

		if ( out ) {
			for ( i = 0; i < 4; i++ ) {
				if ( !Float_Parse( args, &f ) ) {
					return;
				}
				( *out )[i] = f;
			}
		}
	}
}

void Script_SetAsset( dnItem *item, char **args ) {
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		// check for a model
		if ( item->type == ITEM_TYPE_MODEL ) {
		}
	}
}

void Script_SetBackground( dnItem *item, char **args ) {
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		item->window.background = DC->registerShaderNoMip( name );
	}
}




dnItem *dnMenu::FindItemByName( const char *p ) {
	int i;

	if ( p == NULL ) {
		return NULL;
	}

	for ( i = 0; i < itemCount; i++ ) {
		/* script decoration items can carry no name */
		if ( items[i]->window.name && Q_strcasecmp( p, items[i]->window.name ) == 0 ) {
			return items[i];
		}
	}

	return NULL;
}

void Script_SetTeamColor( dnItem *item, char **args ) {
	if ( DC->getTeamColor ) {
		int i;
		vec4_t color;
		DC->getTeamColor( &color );
		for ( i = 0; i < 4; i++ ) {
			item->window.backColor[i] = color[i];
		}
	}
}

void Script_SetItemColor( dnItem *item, char **args ) {
	const char *itemname;
	const char *name;
	vec4_t color;
	int i;
	vec4_t *out;
	// expecting type of color to set and 4 args for the color
	if ( String_Parse( args, &itemname ) && String_Parse( args, &name ) ) {
		dnItem *item2;
		int j;
		int count = ( ( dnMenu * )item->parent )->ItemsMatchingGroup( itemname );

		if ( !Color_Parse( args, &color ) ) {
			return;
		}

		for ( j = 0; j < count; j++ ) {
			item2 = ( ( dnMenu * )item->parent )->GetMatchingItemByNumber( j, itemname );
			if ( item2 != NULL ) {
				out = NULL;
				if ( Q_strcasecmp( name, "backcolor" ) == 0 ) {
					out = &item2->window.backColor;
				} else if ( Q_strcasecmp( name, "forecolor" ) == 0 ) {
					out = &item2->window.foreColor;
					item2->window.flags |= WINDOW_FORECOLORSET;
				} else if ( Q_strcasecmp( name, "bordercolor" ) == 0 ) {
					out = &item2->window.borderColor;
				}

				if ( out ) {
					for ( i = 0; i < 4; i++ ) {
						( *out )[i] = color[i];
					}
				}
			}
		}
	}
}


void dnMenu::ShowItemByName( const char *p, qboolean bShow ) {
	dnItem *item;
	int i;
	int count = ItemsMatchingGroup( p );
	for ( i = 0; i < count; i++ ) {
		item = GetMatchingItemByNumber( i, p );
		if ( item != NULL ) {
			if ( bShow ) {
				item->window.flags |= WINDOW_VISIBLE;
			} else {
				item->window.flags &= ~WINDOW_VISIBLE;
				// stop cinematics playing in the window
				if ( item->window.cinematic >= 0 ) {
					DC->stopCinematic( item->window.cinematic );
					item->window.cinematic = -1;
				}
			}
		}
	}
}

void dnMenu::FadeItemByName( const char *p, qboolean fadeOut ) {
	dnItem *item;
	int i;
	int count = ItemsMatchingGroup( p );
	for ( i = 0; i < count; i++ ) {
		item = GetMatchingItemByNumber( i, p );
		if ( item != NULL ) {
			if ( fadeOut ) {
				item->window.flags |= ( WINDOW_FADINGOUT | WINDOW_VISIBLE );
				item->window.flags &= ~WINDOW_FADINGIN;
			} else {
				item->window.flags |= ( WINDOW_VISIBLE | WINDOW_FADINGIN );
				item->window.flags &= ~WINDOW_FADINGOUT;
			}
		}
	}
}

dnMenu *dnMenuSystem::FindByName( const char *p ) {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Q_strcasecmp( menus[i].window.name, p ) == 0 ) {
			return &menus[i];
		}
	}
	return NULL;
}

void dnMenuSystem::ShowByName( const char *p ) {
	dnMenu *menu = FindByName( p );
	if ( menu ) {
		menu->Activate();
	}
}

void dnMenuSystem::OpenByName( const char *p ) {
	ActivateByName( p );
}

void dnMenu::RunCloseScript( void ) {
	if ( window.flags & WINDOW_VISIBLE && onClose ) {
		dnItem item;
		item.parent = this;
		item.RunScript( onClose );
	}
}

void dnMenuSystem::CloseByName( const char *p ) {
	dnMenu *menu = FindByName( p );
	if ( menu != NULL ) {
		menu->RunCloseScript();
		menu->window.flags &= ~( WINDOW_VISIBLE | WINDOW_HASFOCUS );
	}
}

void dnMenuSystem::CloseAll() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		menus[i].RunCloseScript();
		menus[i].window.flags &= ~( WINDOW_HASFOCUS | WINDOW_VISIBLE );
	}
}

/* run every onNamedEvent handler bound to this name across the open
   menus and their items, like the BFG gui HandleNamedEvent sweep */
void dnMenuSystem::RunNamedEvent( const char *name ) {
	int i, j, k;
	dnItem it;

	for ( i = 0; i < menuCount; i++ ) {
		dnMenu *menu = &menus[i];

		if ( !( menu->window.flags & WINDOW_VISIBLE ) ) {
			continue;
		}

		/* the menu itself can own handlers, like the BFG top-level windowDef */
		for ( j = 0; j < menu->namedEventCount; j++ ) {
			if ( Q_strcasecmp( menu->namedEventNames[j], name ) == 0 && menu->namedEventScripts[j] ) {
				it.parent = menu;
				it.RunScript( menu->namedEventScripts[j] );
			}
		}

		for ( j = 0; j < menu->itemCount; j++ ) {
			dnItem *item = menu->items[j];

			for ( k = 0; k < item->namedEventCount; k++ ) {
				if ( Q_strcasecmp( item->namedEventNames[k], name ) == 0 && item->namedEventScripts[k] ) {
					item->RunScript( item->namedEventScripts[k] );
				}
			}
		}
	}
}


void Script_Show( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		( (dnMenu *)item->parent )->ShowItemByName( name, qtrue );
	}
}

void Script_Hide( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		( (dnMenu *)item->parent )->ShowItemByName( name, qfalse );
	}
}

void Script_FadeIn( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		( (dnMenu *)item->parent )->FadeItemByName( name, qfalse );
	}
}

void Script_FadeOut( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		( (dnMenu *)item->parent )->FadeItemByName( name, qtrue );
	}
}



void Script_Open( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		g_menus.OpenByName( name );
	}
}

void Script_Close( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		g_menus.CloseByName( name );
	}
}

/* run every handler bound to the name, the BFG gui namedEvent command */
void Script_NamedEvent( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		g_menus.RunNamedEvent( name );
	}
}




void Script_Clipboard( dnItem *item, char **args ) {
	char curscript[64];
	DC->getCVarString( "cg_clipboardName", curscript, sizeof( curscript ) ); // grab the string the client set
	( (dnMenu *)item->parent )->ShowItemByName( curscript, qtrue );
}




#define NOTEBOOK_MAX_PAGES 6


/*
==============
Script_NotebookShowpage
	hide all notebook pages and show just the active one

	inc == 0	- show current page
	inc == val	- turn inc pages in the notebook (negative numbers are backwards)
	inc == 999	- key number.  +999 is jump to last page, -999 is jump to cover page
==============
*/
void Script_NotebookShowpage( dnItem *item, char **args ) {
	int i, inc, curpage, newpage = 0, pages;

	pages = DC->getCVarValue( "cg_notebookpages" );

	if ( Int_Parse( args, &inc ) ) {

		curpage = DC->getCVarValue( "ui_notebookCurrentPage" );


		if ( inc == 0 ) {  // opening
			if ( pages && !curpage ) { // only open to cover if no pages exist
				inc = 1;    // otherwise, go to first available page
			}
		}

		if ( inc == 999 ) {            // jump to end
			curpage = 0;
			inc = -1;
		} else if ( inc == -999 ) {    // jump to start
			curpage = 0;
			inc = 0;
		} else if ( inc > 500 ) {
			curpage = inc;
			inc = 0;
		}

		if ( inc ) {
			int dec = 0;

			if ( inc > 0 ) {
				for ( i = 1; i < NOTEBOOK_MAX_PAGES; i++ ) {
					newpage = curpage + i;
					if ( newpage > NOTEBOOK_MAX_PAGES ) {
						newpage = newpage % NOTEBOOK_MAX_PAGES;
					}

					if ( newpage == 0 ) {
						continue;
					}

					if ( pages & ( 1 << ( newpage - 1 ) ) ) {
						dec++;
						break;
					}
				}
				if ( i < NOTEBOOK_MAX_PAGES ) {  // a valid page was found
					curpage = newpage;
				}

			} else {
				for ( i = 1; i < NOTEBOOK_MAX_PAGES; i++ ) {
					newpage = curpage - i;
					if ( newpage <= 0 ) {
						newpage = newpage + NOTEBOOK_MAX_PAGES;
					}

					if ( pages & ( 1 << ( newpage - 1 ) ) ) {
						break;
					}
				}
				if ( i < NOTEBOOK_MAX_PAGES ) {  // a valid page was found
					curpage = newpage;
				}

			}
		}


		// hide all the pages
		( (dnMenu *)item->parent )->ShowItemByName( "cover", qfalse );
		for ( i = 1; i <= NOTEBOOK_MAX_PAGES; i++ ) {
			( (dnMenu *)item->parent )->ShowItemByName( va( "page%d", i ), qfalse );
		}

		// show the visible one

		if ( curpage ) {
			( (dnMenu *)item->parent )->ShowItemByName( va( "page%d", curpage ), qtrue );
		} else {
			( (dnMenu *)item->parent )->ShowItemByName( "cover", qtrue );
		}

		DC->setCVar( "ui_notebookCurrentPage", va( "%d", curpage ) ); // store new current page

	}
}



void dnMenu::TransitionItemByName( const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt ) {
	dnItem *item;
	int i;
	int count = ItemsMatchingGroup( p );
	for ( i = 0; i < count; i++ ) {
		item = GetMatchingItemByNumber( i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_INTRANSITION | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			memcpy( &item->window.rectClient, &rectFrom, sizeof( rectDef_t ) );
			memcpy( &item->window.rectEffects, &rectTo, sizeof( rectDef_t ) );
			item->window.rectEffects2.x = fabs( rectTo.x - rectFrom.x ) / amt;
			item->window.rectEffects2.y = fabs( rectTo.y - rectFrom.y ) / amt;
			item->window.rectEffects2.w = fabs( rectTo.w - rectFrom.w ) / amt;
			item->window.rectEffects2.h = fabs( rectTo.h - rectFrom.h ) / amt;
			item->UpdatePosition();
		}
	}
}


void Script_Transition( dnItem *item, char **args ) {
	const char *name;
	rectDef_t rectFrom, rectTo;
	int time;
	float amt;

	if ( String_Parse( args, &name ) ) {
		if ( Rect_Parse( args, &rectFrom ) && Rect_Parse( args, &rectTo ) && Int_Parse( args, &time ) && Float_Parse( args, &amt ) ) {
			( (dnMenu *)item->parent )->TransitionItemByName( name, rectFrom, rectTo, time, amt );
		}
	}
}


void dnMenu::OrbitItemByName( const char *p, float x, float y, float cx, float cy, int time ) {
	dnItem *item;
	int i;
	int count = ItemsMatchingGroup( p );
	for ( i = 0; i < count; i++ ) {
		item = GetMatchingItemByNumber( i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_ORBITING | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			item->window.rectEffects.x = cx;
			item->window.rectEffects.y = cy;
			item->window.rectClient.x = x;
			item->window.rectClient.y = y;
			item->UpdatePosition();
		}
	}
}


void Script_Orbit( dnItem *item, char **args ) {
	const char *name;
	float cx, cy, x, y;
	int time;

	if ( String_Parse( args, &name ) ) {
		if ( Float_Parse( args, &x ) && Float_Parse( args, &y ) && Float_Parse( args, &cx ) && Float_Parse( args, &cy ) && Int_Parse( args, &time ) ) {
			( (dnMenu *)item->parent )->OrbitItemByName( name, x, y, cx, cy, time );
		}
	}
}



void Script_SetFocus( dnItem *item, char **args ) {
	const char *name;
	dnItem *focusItem;

	if ( String_Parse( args, &name ) ) {
		focusItem = ( ( dnMenu * )item->parent )->FindItemByName( name );
		if ( focusItem && !( focusItem->window.flags & WINDOW_DECORATION ) && !( focusItem->window.flags & WINDOW_HASFOCUS ) ) {
			( (dnMenu *)item->parent )->ClearFocus();
			focusItem->window.flags |= WINDOW_HASFOCUS;
			if ( focusItem->onFocus ) {
				focusItem->RunScript( focusItem->onFocus );
			}
			if ( DC->Assets.itemFocusSound ) {
				DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
			}
		}
	}
}

void Script_SetPlayerModel( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_model", name );
	}
}

void Script_SetPlayerHead( dnItem *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_headmodel", name );
	}
}

void Script_SetCvar( dnItem *item, char **args ) {
	const char *cvar, *val;
	if ( String_Parse( args, &cvar ) && String_Parse( args, &val ) ) {
		DC->setCVar( cvar, val );
	}

}

void Script_Exec( dnItem *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->executeText( DN_EXEC_APPEND, va( "%s ; ", val ) );
	}
}

void Script_Play( dnItem *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->startLocalSound( DC->registerSound( val ), CHAN_LOCAL_SOUND );      // all sounds are not 3d
	}
}

void Script_playLooped( dnItem *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->startBackgroundTrack( val, val, 0 );
	}
}

void Script_AddListItem( dnItem *item, char **args ) {
	const char *itemname, *val, *name;
	dnItem *t;

	if ( String_Parse( args, &itemname ) && String_Parse( args, &val ) && String_Parse( args, &name ) ) {
		t = ( ( dnMenu * )item->parent )->FindItemByName( itemname );
		if ( t && t->special ) {
			DC->feederAddItem( t->special, name, atoi( val ) );
		}
	}
}

commandDef_t commandList[] =
{
	{"fadein", &Script_FadeIn},                  // group/name
	{"fadeout", &Script_FadeOut},                // group/name
	{"show", &Script_Show},                      // group/name
	{"hide", &Script_Hide},                      // group/name
	{"setcolor", &Script_SetColor},              // works on this
	{"open", &Script_Open},                      // menu
	{"close", &Script_Close},                    // menu
	{"namedEvent", &Script_NamedEvent},          // run the handlers bound to the name
	{"clipboard", &Script_Clipboard},            // show the current clipboard group by name
	{"showpage", &Script_NotebookShowpage},
	{"setasset", &Script_SetAsset},              // works on this
	{"setbackground", &Script_SetBackground},    // works on this
	{"setitemcolor", &Script_SetItemColor},      // group/name
	{"setteamcolor", &Script_SetTeamColor}, // paint the background in the team color
	{"setfocus", &Script_SetFocus}, // focus an item by name
	{"setplayermodel", &Script_SetPlayerModel}, // set the player setup model item
	{"setplayerhead", &Script_SetPlayerHead}, // set the player setup head item
	{"transition", &Script_Transition},          // group/name
	{"setcvar", &Script_SetCvar},                // group/name
	{"exec", &Script_Exec}, // run a config file
	{"play", &Script_Play}, // play a sound by name
	{"playlooped", &Script_playLooped}, // loop a sound by name
	{"orbit", &Script_Orbit},                    // group/name
	{"addlistitem", &Script_AddListItem} // add text items to a list box
};

int scriptCommandCount = sizeof( commandList ) / sizeof( commandDef_t );


void dnItem::RunScript( const char *s ) {
	char script[1024], *p;
	int i;
	qboolean bRan;
	memset( script, 0, sizeof( script ) );
	if ( s && s[0] ) {
		Q_strcat( script, 1024, s );
		p = script;
		while ( 1 ) {
			const char *command;
			// expect command then arguments, ; ends command, NULL ends script
			if ( !String_Parse( &p, &command ) ) {
				return;
			}

			if ( command[0] == ';' && command[1] == '\0' ) {
				continue;
			}

			bRan = qfalse;
			for ( i = 0; i < scriptCommandCount; i++ ) {
				if ( Q_strcasecmp( command, commandList[i].name ) == 0 ) {
					( commandList[i].handler( this, &p ) );
					bRan = qtrue;
					break;
				}
			}
			// not in our auto list, pass to handler
			if ( !bRan ) {
				DC->runScript( &p );
			}
		}
	}
}


qboolean dnItem::EnableShowViaCvar( int flag ) {
	char script[1024], *p;
	memset( script, 0, sizeof( script ) );
	if ( enableCvar && *enableCvar && cvarTest && *cvarTest ) {
		char buff[1024];
		DC->getCVarString( cvarTest, buff, sizeof( buff ) );

		Q_strcat( script, 1024, enableCvar );
		p = script;
		while ( 1 ) {
			const char *val;
			// expect value then ; or NULL, NULL ends list
			if ( !String_Parse( &p, &val ) ) {
				return ( cvarFlags & flag ) ? qfalse : qtrue;
			}

			if ( val[0] == ';' && val[1] == '\0' ) {
				continue;
			}

			// enable it if any of the values are true
			if ( cvarFlags & flag ) {
				if ( Q_strcasecmp( buff, val ) == 0 ) {
					return qtrue;
				}
			} else {
				// disable it if any of the values are true
				if ( Q_strcasecmp( buff, val ) == 0 ) {
					return qfalse;
				}
			}

		}
		return ( cvarFlags & flag ) ? qfalse : qtrue;
	}
	return qtrue;
}


// optionally sets focus to this item
qboolean dnItem::SetFocus( float x, float y ) {
	int i;
	dnItem *oldFocus;
	sfxHandle_t *sfx = &DC->Assets.itemFocusSound;
	qboolean playSound = qfalse;
	dnMenu *parent = NULL;
	// sanity check, non-null, not a decoration and does not already have the focus
	if ( window.flags & WINDOW_DECORATION || window.flags & WINDOW_HASFOCUS || !( window.flags & WINDOW_VISIBLE ) || !TreeVisible() ) {
		return qfalse;
	}

	parent = ( dnMenu * )this->parent;

	// items can be enabled and disabled based on cvars
	if ( cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !EnableShowViaCvar( CVAR_ENABLE ) ) {
		return qfalse;
	}

	if ( cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !EnableShowViaCvar( CVAR_SHOW ) ) {
		return qfalse;
	}

	oldFocus = ( (dnMenu *)parent )->ClearFocus();

	if ( type == ITEM_TYPE_TEXT ) {
		rectDef_t r;
		r = textRect;
		r.y -= r.h;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			window.flags |= WINDOW_HASFOCUS;
			if ( focusSound ) {
				sfx = &focusSound;
			}
			playSound = qtrue;
		} else {
			if ( oldFocus ) {
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if ( oldFocus->onFocus ) {
					oldFocus->RunScript( oldFocus->onFocus );
				}
			}
		}
	} else {
		window.flags |= WINDOW_HASFOCUS;
		if ( onFocus ) {
			RunScript( onFocus );
		}
		if ( focusSound ) {
			sfx = &focusSound;
		}
		playSound = qtrue;
	}

	if ( playSound && sfx ) {
		DC->startLocalSound( *sfx, CHAN_LOCAL_SOUND );
	}

	for ( i = 0; i < parent->itemCount; i++ ) {
		if ( parent->items[i] == this ) {
			parent->cursorItem = i;
			break;
		}
	}

	return qtrue;
}

int dnListBoxItem::MaxScroll( void ) {
	listBoxDef_t *listPtr = (listBoxDef_t*)typeData;
	int count = DC->feederCount( special );
	int max;

	if ( window.flags & WINDOW_HORIZONTAL ) {
		max = count - ( window.rect.w / listPtr->elementWidth ) + 1;
	} else {
		max = count - ( window.rect.h / listPtr->elementHeight ) + 1;
	}
	if ( max < 0 ) {
		return 0;
	}
	return max;
}

int dnListBoxItem::ThumbPosition( void ) {
	float max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t*)typeData;

	max = MaxScroll();
	if ( window.flags & WINDOW_HORIZONTAL ) {
		size = window.rect.w - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	} else {
		size = window.rect.h - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

int dnListBoxItem::ThumbDrawPosition( void ) {
	int min, max;

	if ( g_capture.Item() == this ) {
		if ( window.flags & WINDOW_HORIZONTAL ) {
			min = window.rect.x + SCROLLBAR_SIZE + 1;
			max = window.rect.x + window.rect.w - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursorx >= min + SCROLLBAR_SIZE / 2 && DC->cursorx <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursorx - SCROLLBAR_SIZE / 2;
			} else {
				return ThumbPosition();
			}
		} else {
			min = window.rect.y + SCROLLBAR_SIZE + 1;
			max = window.rect.y + window.rect.h - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursory >= min + SCROLLBAR_SIZE / 2 && DC->cursory <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursory - SCROLLBAR_SIZE / 2;
			} else {
				return ThumbPosition();
			}
		}
	} else {
		return ThumbPosition();
	}
}

/* framework rows share one value column: sliders, yes/no toggles and
   choice lists all draw their value at the same offset from the row rect,
   so the values line up vertically like the engine menu screens */
#define DUKE_VALUE_COLUMN 320

float dnItem::ValueColumnX( void ) {
	return window.rect.x + DUKE_VALUE_COLUMN;
}

float dnSliderItem::ThumbPosition( void ) {
	float value, range, x;
	editFieldDef_t *editDef = ( editFieldDef_t * )typeData;

	if ( text ) {
		x = ValueColumnX();
	} else {
		x = window.rect.x;
	}

	if ( editDef == NULL && cvar ) {
		return x;
	}

	value = DC->getCVarValue( cvar );

	if ( value < editDef->minVal ) {
		value = editDef->minVal;
	} else if ( value > editDef->maxVal ) {
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	value *= SLIDER_WIDTH;
	x += value;
	return x;
}

int dnSliderItem::OverSlider( float x, float y ) {
	rectDef_t r;

	r.x = ThumbPosition() - ( SLIDER_THUMB_WIDTH / 2 );
	r.y = window.rect.y - 2;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if ( Rect_ContainsPoint( &r, x, y ) ) {
		return WINDOW_LB_THUMB;
	}
	return 0;
}

int dnListBoxItem::OverListBox( float x, float y ) {
	rectDef_t r;
	listBoxDef_t *listPtr;
	int thumbstart;
	int count;

	count = DC->feederCount( special );
	listPtr = (listBoxDef_t*)typeData;
	if ( window.flags & WINDOW_HORIZONTAL ) {
		// check if on left arrow
		r.x = window.rect.x;
		r.y = window.rect.y + window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = window.rect.x + window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		thumbstart = ThumbPosition();
		r.x = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.x = window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = window.rect.x + window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	} else {
		r.x = window.rect.x + window.rect.w - SCROLLBAR_SIZE;
		r.y = window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		r.y = window.rect.y + window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		thumbstart = ThumbPosition();
		r.y = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.y = window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = window.rect.y + window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}


void dnListBoxItem::MouseEnterRect( float x, float y ) {
	rectDef_t r;
	listBoxDef_t *listPtr = (listBoxDef_t*)typeData;

	window.flags &= ~( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN );
	window.flags |= OverListBox( x, y );

	if ( window.flags & WINDOW_HORIZONTAL ) {
		if ( !( window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
			// check for selection hit as we have exausted buttons and thumb
			if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
				r.x = window.rect.x;
				r.y = window.rect.y;
				r.h = window.rect.h - SCROLLBAR_SIZE;
				r.w = window.rect.w - listPtr->drawPadding;
				if ( Rect_ContainsPoint( &r, x, y ) ) {
					listPtr->cursorPos =  (int)( ( x - r.x ) / listPtr->elementWidth )  + listPtr->startPos;
					if ( listPtr->cursorPos >= listPtr->endPos ) {
						listPtr->cursorPos = listPtr->endPos;
					}
				}
			} else {
				// text hit..
			}
		}
	} else if ( !( window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
		r.x = window.rect.x;
		r.y = window.rect.y;
		r.w = window.rect.w - SCROLLBAR_SIZE;
		r.h = window.rect.h - listPtr->drawPadding;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			listPtr->cursorPos =  (int)( ( y - 2 - r.y ) / listPtr->elementHeight )  + listPtr->startPos;
			if ( listPtr->cursorPos > listPtr->endPos ) {
				listPtr->cursorPos = listPtr->endPos;
			}
		}
	}
}

void dnItem::MouseEnter( float x, float y ) {
	rectDef_t r;

	r = textRect;
	r.y -= r.h;
	// in the text rect?

	// items can be enabled and disabled based on cvars
	if ( cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !EnableShowViaCvar( CVAR_ENABLE ) ) {
		return;
	}

	if ( cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !EnableShowViaCvar( CVAR_SHOW ) ) {
		return;
	}

	if ( Rect_ContainsPoint( &r, x, y ) ) {
		if ( !( window.flags & WINDOW_MOUSEOVERTEXT ) ) {
			RunScript( mouseEnterText );
			window.flags |= WINDOW_MOUSEOVERTEXT;
		}
		if ( !( window.flags & WINDOW_MOUSEOVER ) ) {
			RunScript( mouseEnter );
			window.flags |= WINDOW_MOUSEOVER;
		}

	} else {
		// not in the text rect
		if ( window.flags & WINDOW_MOUSEOVERTEXT ) {
			RunScript( mouseExitText );
			window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}
		if ( !( window.flags & WINDOW_MOUSEOVER ) ) {
			RunScript( mouseEnter );
			window.flags |= WINDOW_MOUSEOVER;
		}

		MouseEnterRect( x, y );
	}
}

void dnItem::MouseLeave( void ) {

	if ( window.flags & WINDOW_MOUSEOVERTEXT ) {
		RunScript( mouseExitText );
		window.flags &= ~WINDOW_MOUSEOVERTEXT;
	}
	RunScript( mouseExit );
	window.flags &= ~( WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW );
}

dnItem *dnMenu::HitTest( float x, float y ) {
	int i;
	for ( i = 0; i < itemCount; i++ ) {
		if ( Rect_ContainsPoint( &items[i]->window.rect, x, y ) ) {
			return items[i];
		}
	}
	return NULL;
}

void dnItem::SetMouseOver( qboolean focus ) {

	if ( focus ) {
		window.flags |= WINDOW_MOUSEOVER;
	} else {
		window.flags &= ~WINDOW_MOUSEOVER;
	}
}


qboolean dnOwnerDrawItem::HandleTypeKey( int key, qboolean down ) {
	if ( DC->ownerDrawHandleKey ) {
		return DC->ownerDrawHandleKey( window.ownerDraw, window.ownerDrawFlags, &special, key );
	}
	return qfalse;
}

qboolean dnListBoxItem::HandleListKey( int key, qboolean down, qboolean force ) {
	listBoxDef_t *listPtr = (listBoxDef_t*)typeData;
	int count = DC->feederCount( special );
	int max, viewmax;

	if ( force || ( Rect_ContainsPoint( &window.rect, DC->cursorx, DC->cursory ) && window.flags & WINDOW_HASFOCUS ) ) {
		max = MaxScroll();
		if ( window.flags & WINDOW_HORIZONTAL ) {
			viewmax = ( window.rect.w / listPtr->elementWidth );
			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos--;
					if ( listPtr->cursorPos < 0 ) {
						listPtr->cursorPos = 0;
					}
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					cursorPos = listPtr->cursorPos;
					DC->feederSelection( special, cursorPos );
				} else {
					listPtr->startPos--;
					if ( listPtr->startPos < 0 ) {
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos++;
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= count ) {
						listPtr->cursorPos = count - 1;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					cursorPos = listPtr->cursorPos;
					DC->feederSelection( special, cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos >= count ) {
						listPtr->startPos = count - 1;
					}
				}
				return qtrue;
			}
		} else {
			viewmax = ( window.rect.h / listPtr->elementHeight );
			if ( key == K_UPARROW || key == K_KP_UPARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos--;
					if ( listPtr->cursorPos < 0 ) {
						listPtr->cursorPos = 0;
					}
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					cursorPos = listPtr->cursorPos;
					DC->feederSelection( special, cursorPos );
				} else {
					listPtr->startPos--;
					if ( listPtr->startPos < 0 ) {
						listPtr->startPos = 0;
					}
				}
				return qtrue;
			}
			if ( key == K_DOWNARROW || key == K_KP_DOWNARROW ) {
				if ( !listPtr->notselectable ) {
					listPtr->cursorPos++;
					if ( listPtr->cursorPos < listPtr->startPos ) {
						listPtr->startPos = listPtr->cursorPos;
					}
					if ( listPtr->cursorPos >= count ) {
						listPtr->cursorPos = count - 1;
					}
					if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
						listPtr->startPos = listPtr->cursorPos - viewmax + 1;
					}
					cursorPos = listPtr->cursorPos;
					DC->feederSelection( special, cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos > max ) {
						listPtr->startPos = max;
					}
				}
				return qtrue;
			}
		}
		/* the bind list starts the key capture on enter; the row command
		   comes from the feeder, not from an item cvar */
		if ( ( key == K_ENTER || key == K_KP_ENTER ) && !listPtr->notselectable ) {
			const char *cmd = DC->feederItemText( special, cursorPos, 0, NULL );
			if ( ( int )special == DN_FEEDER_BINDS && cmd ) {
				g_bindCapture.Start( this, cmd );
			}
			return qtrue;
		}
		// mouse hit
		if ( key == K_MOUSE1 || key == K_MOUSE2 ) {
			if ( window.flags & WINDOW_LB_LEFTARROW ) {
				listPtr->startPos--;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( window.flags & WINDOW_LB_RIGHTARROW ) {
				// one down
				listPtr->startPos++;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( window.flags & WINDOW_LB_PGUP ) {
				// page up
				listPtr->startPos -= viewmax;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( window.flags & WINDOW_LB_PGDN ) {
				// page down
				listPtr->startPos += viewmax;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( window.flags & WINDOW_LB_THUMB ) {
			} else {
				// select an item
				if ( DC->realTime < lastListBoxClickTime && listPtr->doubleClick ) {
					RunScript( listPtr->doubleClick );
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				if ( cursorPos != listPtr->cursorPos ) {
					if ( listPtr->cursorPos < DC->feederCount( special ) ) {
						cursorPos = listPtr->cursorPos;   // only set if it's valid
					}
					DC->feederSelection( special, cursorPos );
				}
			}
			return qtrue;
		}
		if ( key == K_HOME || key == K_KP_HOME ) {
			// home
			listPtr->startPos = 0;
			return qtrue;
		}
		if ( key == K_END || key == K_KP_END ) {
			// end
			listPtr->startPos = max;
			return qtrue;
		}
		if ( key == K_PGUP || key == K_KP_PGUP ) {
			// page up
			if ( !listPtr->notselectable ) {
				listPtr->cursorPos -= viewmax;
				if ( listPtr->cursorPos < 0 ) {
					listPtr->cursorPos = 0;
				}
				if ( listPtr->cursorPos < listPtr->startPos ) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				cursorPos = listPtr->cursorPos;
				DC->feederSelection( special, cursorPos );
			} else {
				listPtr->startPos -= viewmax;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			}
			return qtrue;
		}
		if ( key == K_PGDN || key == K_KP_PGDN ) {
			// page down
			if ( !listPtr->notselectable ) {
				listPtr->cursorPos += viewmax;
				if ( listPtr->cursorPos < listPtr->startPos ) {
					listPtr->startPos = listPtr->cursorPos;
				}
				if ( listPtr->cursorPos >= count ) {
					listPtr->cursorPos = count - 1;
				}
				if ( listPtr->cursorPos >= listPtr->startPos + viewmax ) {
					listPtr->startPos = listPtr->cursorPos - viewmax + 1;
				}
				cursorPos = listPtr->cursorPos;
				DC->feederSelection( special, cursorPos );
			} else {
				listPtr->startPos += viewmax;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			}
			return qtrue;
		}
	}
	return qfalse;
}

qboolean dnYesNoItem::HandleTypeKey( int key, qboolean down ) {

	if ( Rect_ContainsPoint( &window.rect, DC->cursorx, DC->cursory ) && window.flags & WINDOW_HASFOCUS && cvar ) {
		if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
			DC->setCVar( cvar, va( "%i", !DC->getCVarValue( cvar ) ) );
			return qtrue;
		}
	}

	return qfalse;

}

int dnMultiItem::CountSettings( void ) {
	multiDef_t *multiPtr = (multiDef_t*)typeData;
	if ( multiPtr == NULL ) {
		return 0;
	}
	return multiPtr->count;
}

int dnMultiItem::FindCvarByValue( void ) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( cvar );
		}
		for ( i = 0; i < multiPtr->count; i++ ) {
			if ( multiPtr->strDef ) {
				if ( Q_strcasecmp( buff, multiPtr->cvarStr[i] ) == 0 ) {
					return i;
				}
			} else {
				if ( multiPtr->cvarValue[i] == value ) {
					return i;
				}
			}
		}
	}
	return 0;
}

const char *dnMultiItem::Setting( void ) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( cvar );
		}
		for ( i = 0; i < multiPtr->count; i++ ) {
			if ( multiPtr->strDef ) {
				if ( Q_strcasecmp( buff, multiPtr->cvarStr[i] ) == 0 ) {
					return multiPtr->cvarList[i];
				}
			} else {
				if ( multiPtr->cvarValue[i] == value ) {
					return multiPtr->cvarList[i];
				}
			}
		}
	}
	return "";
}

qboolean dnMultiItem::HandleTypeKey( int key, qboolean down ) {
	multiDef_t *multiPtr = (multiDef_t*)typeData;
	if ( multiPtr ) {
		if ( Rect_ContainsPoint( &window.rect, DC->cursorx, DC->cursory ) && window.flags & WINDOW_HASFOCUS && cvar ) {
			if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
				int current = FindCvarByValue() + 1;
				int max = CountSettings();
				if ( current < 0 || current >= max ) {
					current = 0;
				}
				if ( multiPtr->strDef ) {
					DC->setCVar( cvar, multiPtr->cvarStr[current] );
				} else {
					float value = multiPtr->cvarValue[current];
					if ( ( (float)( (int) value ) ) == value ) {
						DC->setCVar( cvar, va( "%i", (int) value ) );
					} else {
						DC->setCVar( cvar, va( "%f", value ) );
					}
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean dnEditFieldItem::HandleEditKey( int key ) {
	char buff[1024];
	int len;
	dnItem *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)typeData;

	if ( cvar ) {

		memset( buff, 0, sizeof( buff ) );
		DC->getCVarString( cvar, buff, sizeof( buff ) );
		len = strlen( buff );
		if ( editPtr->maxChars && len > editPtr->maxChars ) {
			len = editPtr->maxChars;
		}
		if ( key & K_CHAR_FLAG ) {
			key &= ~K_CHAR_FLAG;


			if ( key == 'h' - 'a' + 1 ) {      // ctrl-h is backspace
				if ( cursorPos > 0 ) {
					memmove( &buff[cursorPos - 1], &buff[cursorPos], len + 1 - cursorPos );
					cursorPos--;
					if ( cursorPos < editPtr->paintOffset ) {
						editPtr->paintOffset--;
					}
				}
				DC->setCVar( cvar, buff );
				return qtrue;
			}


			// filename fields only take characters that make valid save names
			if ( type == ITEM_TYPE_VALIDFILEFIELD ) {
				if ( !Q_isforfilename( key ) ) {
					return qtrue;
				}
			}

			// ignore any non printable chars
			if ( key < 32 || !cvar ) {
				return qtrue;
			}

			if ( type == ITEM_TYPE_NUMERICFIELD ) {
				if ( !Q_isnumeric( key ) ) {
					return qfalse;
				}
			}

			if ( !DC->getOverstrikeMode() ) {
				if ( ( len == MAX_EDITFIELD - 1 ) || ( editPtr->maxChars && len >= editPtr->maxChars ) ) {
					return qtrue;
				}
				memmove( &buff[cursorPos + 1], &buff[cursorPos], len + 1 - cursorPos );
			} else {
				if ( editPtr->maxChars && cursorPos >= editPtr->maxChars ) {
					return qtrue;
				}
			}

			buff[cursorPos] = key;

			DC->setCVar( cvar, buff );

			if ( cursorPos < len + 1 ) {
				cursorPos++;
				if ( editPtr->maxPaintChars && cursorPos > editPtr->maxPaintChars ) {
					editPtr->paintOffset++;
				}
			}

		} else {

			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( cursorPos < len ) {
					memmove( buff + cursorPos, buff + cursorPos + 1, len - cursorPos );
					DC->setCVar( cvar, buff );
				}
				return qtrue;
			}

			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
				if ( editPtr->maxPaintChars && cursorPos >= editPtr->maxPaintChars && cursorPos < len ) {
					cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if ( cursorPos < len ) {
					cursorPos++;
				}
				return qtrue;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
				if ( cursorPos > 0 ) {
					cursorPos--;
				}
				if ( cursorPos < editPtr->paintOffset ) {
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == K_HOME || key == K_KP_HOME ) {
				cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == K_END || key == K_KP_END ) {
				cursorPos = len;
				if ( cursorPos > editPtr->maxPaintChars ) {
					editPtr->paintOffset = len - editPtr->maxPaintChars;
				}
				return qtrue;
			}

			if ( key == K_INS || key == K_KP_INS ) {
				DC->setOverstrikeMode( !DC->getOverstrikeMode() );
				return qtrue;
			}
		}

		if ( key == K_TAB || key == K_DOWNARROW || key == K_KP_DOWNARROW ) {
			newItem = ( (dnMenu *)parent )->SetNextCursorItem();
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD || newItem->type == ITEM_TYPE_VALIDFILEFIELD ) ) {
				g_editCapture.Start( newItem );
			}
		}

		if ( key == K_UPARROW || key == K_KP_UPARROW ) {
			newItem = ( (dnMenu *)parent )->SetPrevCursorItem();
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD || newItem->type == ITEM_TYPE_VALIDFILEFIELD ) ) {
				g_editCapture.Start( newItem );
			}
		}

		if ( key == K_ENTER || key == K_KP_ENTER ) {
			if ( onAccept ) {
				RunScript( onAccept );
			}
		}

		if ( key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE ) {
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;

}

/* the capture starts: the item remembers the drag key, the mode keeps
   the auto scroll timing */
void dnMouseCapture::StartListBoxAuto( dnItem *owner, int key, qboolean leftArrow ) {
	kind = CAPTURE_LISTBOX_AUTO;
	item = owner;
	nextScrollTime = DC->realTime + SCROLL_TIME_START;
	nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
	adjustValue = SCROLL_TIME_START;
	scrollKey = key;
	scrollDir = leftArrow;
}

void dnMouseCapture::StartListBoxThumb( dnItem *owner, int key ) {
	kind = CAPTURE_LISTBOX_THUMB;
	item = owner;
	scrollKey = key;
	xStart = DC->cursorx;
	yStart = DC->cursory;
}

void dnMouseCapture::StartSliderThumb( dnItem *owner, int key ) {
	kind = CAPTURE_SLIDER_THUMB;
	item = owner;
	scrollKey = key;
	xStart = DC->cursorx;
	yStart = DC->cursory;
}

void dnMouseCapture::Update( void ) {
	switch ( kind ) {
	case CAPTURE_LISTBOX_AUTO:
		AutoScroll();
		break;
	case CAPTURE_LISTBOX_THUMB:
		ThumbScroll();
		break;
	case CAPTURE_SLIDER_THUMB:
		SliderThumb();
		break;
	default:
		break;
	}
}

/* the auto scroll repeats its key on a shrinking timer, exactly what a
   held listbox arrow needs; the mode guarantees the item is a listbox */
void dnMouseCapture::AutoScroll( void ) {
	if ( DC->realTime > nextScrollTime ) {
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		( ( dnListBoxItem * )item )->HandleListKey( scrollKey, qtrue, qfalse );
		nextScrollTime = DC->realTime + adjustValue;
	}

	if ( DC->realTime > nextAdjustTime ) {
		nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if ( adjustValue > SCROLL_TIME_FLOOR ) {
			adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/* the thumb drag maps the cursor onto the scroll range; the mode
   guarantees the item is a listbox */
void dnMouseCapture::ThumbScroll( void ) {
	rectDef_t r;
	int pos, max;

	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		if ( DC->cursorx == xStart ) {
			return;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE + 1;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE - 1;
		r.h = SCROLLBAR_SIZE;
		r.w = item->window.rect.w - ( SCROLLBAR_SIZE * 2 ) - 2;
		max = ( ( dnListBoxItem * )item )->MaxScroll();
		pos = ( DC->cursorx - r.x - SCROLLBAR_SIZE / 2 ) * max / ( r.w - SCROLLBAR_SIZE );
		if ( pos < 0 ) {
			pos = 0;
		} else if ( pos > max )     {
			pos = max;
		}
		listPtr->startPos = pos;
		xStart = DC->cursorx;
	} else if ( DC->cursory != yStart )     {

		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE - 1;
		r.y = item->window.rect.y + SCROLLBAR_SIZE + 1;
		r.h = item->window.rect.h - ( SCROLLBAR_SIZE * 2 ) - 2;
		r.w = SCROLLBAR_SIZE;
		max = ( ( dnListBoxItem * )item )->MaxScroll();
		pos = ( DC->cursory - r.y - SCROLLBAR_SIZE / 2 ) * max / ( r.h - SCROLLBAR_SIZE );
		if ( pos < 0 ) {
			pos = 0;
		} else if ( pos > max )     {
			pos = max;
		}
		listPtr->startPos = pos;
		yStart = DC->cursory;
	}

	if ( DC->realTime > nextScrollTime ) {
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		( ( dnListBoxItem * )item )->HandleListKey( scrollKey, qtrue, qfalse );
		nextScrollTime = DC->realTime + adjustValue;
	}

	if ( DC->realTime > nextAdjustTime ) {
		nextAdjustTime = DC->realTime + SCROLL_TIME_ADJUST;
		if ( adjustValue > SCROLL_TIME_FLOOR ) {
			adjustValue -= SCROLL_TIME_ADJUSTOFFSET;
		}
	}
}

/* the slider thumb drag maps the cursor onto the cvar range */
void dnMouseCapture::SliderThumb( void ) {
	float x, value, cursorx;
	editFieldDef_t *editDef = ( editFieldDef_t * )item->typeData;

	if ( item->text ) {
		x = item->ValueColumnX();
	} else {
		x = item->window.rect.x;
	}

	cursorx = DC->cursorx;

	if ( cursorx < x ) {
		cursorx = x;
	} else if ( cursorx > x + SLIDER_WIDTH ) {
		cursorx = x + SLIDER_WIDTH;
	}
	value = cursorx - x;
	value /= SLIDER_WIDTH;
	value *= ( editDef->maxVal - editDef->minVal );
	value += editDef->minVal;
	DC->setCVar( item->cvar, va( "%f", value ) );
}

/* plain items take no drag; the listbox and slider subclasses override */
void dnItem::StartCapture( int ) {
}

/* a held listbox arrow scrolls on a timer, a grabbed thumb follows the
   cursor */
void dnListBoxItem::StartCapture( int key ) {
	int flags = OverListBox( DC->cursorx, DC->cursory );

	if ( flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW ) ) {
		g_capture.StartListBoxAuto( this, key, ( flags & WINDOW_LB_LEFTARROW ) ? qtrue : qfalse );
	} else if ( flags & WINDOW_LB_THUMB ) {
		g_capture.StartListBoxThumb( this, key );
	}
}

/* a grabbed slider thumb follows the cursor */
void dnSliderItem::StartCapture( int key ) {
	if ( OverSlider( DC->cursorx, DC->cursory ) & WINDOW_LB_THUMB ) {
		g_capture.StartSliderThumb( this, key );
	}
}
void dnItem::StopCapture( void ) {

}

qboolean dnSliderItem::HandleTypeKey( int key, qboolean down ) {
	float x, value, width, work;
	editFieldDef_t *editDef = ( editFieldDef_t * )typeData;
	float step;

	/* keyboard adjust: one bar segment per press, matching the painted bar */
	if ( window.flags & WINDOW_HASFOCUS && cvar && editDef &&
		 editDef->maxVal > editDef->minVal &&
		 ( key == K_LEFTARROW || key == K_KP_LEFTARROW ||
		   key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) ) {
		step = ( editDef->maxVal - editDef->minVal ) / DUKE_SLIDER_SEGMENTS;
		value = DC->getCVarValue( cvar );
		value += ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) ? -step : step;
		if ( value < editDef->minVal ) {
			value = editDef->minVal;
		} else if ( value > editDef->maxVal ) {
			value = editDef->maxVal;
		}
		DC->setCVar( cvar, va( "%f", value ) );
		return qtrue;
	}

	if ( window.flags & WINDOW_HASFOCUS && cvar && Rect_ContainsPoint( &window.rect, DC->cursorx, DC->cursory ) ) {
		if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
			if ( editDef ) {
				rectDef_t testRect;
				width = SLIDER_WIDTH;
				if ( text ) {
					x = ValueColumnX();
				} else {
					x = window.rect.x;
				}

				testRect = window.rect;
				testRect.x = x;
				value = (float)SLIDER_THUMB_WIDTH / 2;
				testRect.x -= value;
				testRect.w = ( SLIDER_WIDTH + (float)SLIDER_THUMB_WIDTH / 2 );
				if ( Rect_ContainsPoint( &testRect, DC->cursorx, DC->cursory ) ) {
					work = DC->cursorx - x;
					value = work / width;
					value *= ( editDef->maxVal - editDef->minVal );
					value += editDef->minVal;
					DC->setCVar( cvar, va( "%f", value ) );
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}


qboolean dnItem::HandleKey( int key, qboolean down ) {

	if ( g_capture.Active() ) {
		g_capture.Stop();
	} else {
		// the down check must bind to all three mouse keys, a bare || chain fires on key-up too
		if ( down && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			StartCapture( key );
		}
	}

	if ( !down ) {
		return qfalse;
	}

	/* the per-type keys live in the type subclass; most types take none */
	return HandleTypeKey( key, down );
}

/* default per-type hooks: a type with nothing special falls through here */
void dnItem::PaintType( void ) {
}

qboolean dnItem::HandleTypeKey( int key, qboolean down ) {
	return qfalse;
}

/* edit keys only mean something to the edit field subclass */
qboolean dnItem::HandleEditKey( int key ) {
	return qfalse;
}

/* only listboxes feed from the script feeder scroll */
qboolean dnItem::ScrollFeed( qboolean down ) {
	return qfalse;
}

void dnItem::MouseEnterRect( float x, float y ) {
}

void dnItem::InitControls( void ) {
}

void dnItem::Action( void ) {

	RunScript( action );
}

dnItem *dnMenu::SetPrevCursorItem( void ) {
	qboolean wrapped = qfalse;
	int oldCursor = cursorItem;

	if ( cursorItem < 0 ) {
		cursorItem = itemCount - 1;
		wrapped = qtrue;
	}

	while ( cursorItem > -1 ) {

		cursorItem--;
		if ( cursorItem < 0 && !wrapped ) {
			wrapped = qtrue;
			cursorItem = itemCount - 1;
		}

		if ( items[cursorItem]->SetFocus( DC->cursorx, DC->cursory ) ) {
			HandleMouseMove( items[cursorItem]->window.rect.x + 1, items[cursorItem]->window.rect.y + 1 );
			return items[cursorItem];
		}
	}
	cursorItem = oldCursor;
	return NULL;

}

dnItem *dnMenu::SetNextCursorItem( void ) {

	qboolean wrapped = qfalse;
	int oldCursor = cursorItem;


	if ( cursorItem == -1 ) {
		cursorItem = 0;
		wrapped = qtrue;
	}

	while ( cursorItem < itemCount ) {

		cursorItem++;
		if ( cursorItem >= itemCount ) {  // stop wrapping once every item had its turn
			if ( !wrapped ) {
				wrapped = qtrue;
				cursorItem = 0;
			} else {
				return items[oldCursor];
			}
		}

		if ( items[cursorItem]->SetFocus( DC->cursorx, DC->cursory ) ) {
			HandleMouseMove( items[cursorItem]->window.rect.x + 1, items[cursorItem]->window.rect.y + 1 );
			return items[cursorItem];
		}
	}

	cursorItem = oldCursor;
	return NULL;
}


static void Window_CloseCinematic( dnWindow *window ) {
	if ( window->style == WINDOW_STYLE_CINEMATIC && window->cinematic >= 0 ) {
		DC->stopCinematic( window->cinematic );
		window->cinematic = -1;
	}
}

void dnMenu::CloseCinematics( void ) {

	int i;
	Window_CloseCinematic( &window );
	for ( i = 0; i < itemCount; i++ ) {
		Window_CloseCinematic( &items[i]->window );
		if ( items[i]->type == ITEM_TYPE_OWNERDRAW ) {
			DC->stopCinematic( 0 - items[i]->window.ownerDraw );
		}
	}
}

void dnMenuSystem::CloseCinematics() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		menus[i].CloseCinematics();
	}
}

void dnMenu::Activate( void ) {
	window.flags |= ( WINDOW_HASFOCUS | WINDOW_VISIBLE );
	if ( onOpen ) {
		dnItem item;
		item.parent = this;
		item.RunScript( onOpen );
	}

	if ( soundName && *soundName ) {
		// restarting the track would reset the raw sound buffer, so the old one keeps playing
		DC->startBackgroundTrack( soundName, soundName, 0 );
	}

	/* opening a menu stops the cinematics of every open menu, not just
	   this one's items */
	g_menus.CloseCinematics();

}

int dnMenuSystem::VisibleMenuCount() {
	int i, count;
	count = 0;
	for ( i = 0; i < menuCount; i++ ) {
		if ( menus[i].window.flags & ( WINDOW_FORCED | WINDOW_VISIBLE ) ) {
			count++;
		}
	}
	return count;
}

void dnMenuSystem::HandleOOBClick( dnMenu *menu, int key, qboolean down ) {
	if ( menu ) {
		int i;
		// basically the behaviour we are looking for is if there are windows in the stack.. see if
		// the cursor is within any of them.. if not close them otherwise activate them and pass the
		// key on.. force a mouse move to activate focus and script stuff
		if ( down && menu->window.flags & WINDOW_OOB_CLICK ) {
			menu->RunCloseScript();
			menu->window.flags &= ~( WINDOW_HASFOCUS | WINDOW_VISIBLE );
		}

		for ( i = 0; i < menuCount; i++ ) {
			if ( menus[i].OverActiveItem( DC->cursorx, DC->cursory ) ) {
				menu->window.flags &= ~( WINDOW_HASFOCUS );
				menus[i].Activate();
				menus[i].HandleMouseMove( DC->cursorx, DC->cursory );
				menus[i].HandleKey( key, down );
			}
		}

		if ( VisibleMenuCount() == 0 ) {
			if ( DC->Pause ) {
				DC->Pause( qfalse );
			}
		}
		CloseCinematics();
	}
}

rectDef_t *dnItem::CorrectedTextRect( void ) {
	static rectDef_t rect;
	memset( &rect, 0, sizeof( rectDef_t ) );

	rect = textRect;
	if ( rect.w ) {
		rect.y -= rect.h;
	}
	return &rect;
}

void dnMenu::HandleKey( int key, qboolean down ) {
	int i;
	dnItem *item = NULL;
	qboolean inHandler = qfalse;

	if ( inHandler ) {
		return;
	}

	inHandler = qtrue;
	if ( g_bindCapture.Waiting() && down ) {
		g_bindCapture.HandleKey( key, down );
		inHandler = qfalse;
		return;
	}

	if ( g_editCapture.IsActive() && down ) {
		if ( !g_editCapture.HandleKey( key ) ) {
			g_editCapture.Stop();
			inHandler = qfalse;
			return;
		} else if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) {
			g_editCapture.Stop();
			g_menus.MouseMove( NULL, DC->cursorx, DC->cursory );
		} else {
			return;
		}
	}

	// see if the mouse is within the window bounds and if so is this a mouse click
	if ( down && !( window.flags & WINDOW_POPUP ) && !Rect_ContainsPoint( &window.rect, DC->cursorx, DC->cursory ) ) {
		static qboolean inHandleKey = qfalse;
		// the guard must bind to all three mouse keys, a bare || chain re-enters the handler
		if ( !inHandleKey && ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) ) {
			inHandleKey = qtrue;
			g_menus.HandleOOBClick( this, key, down );
			inHandleKey = qfalse;
			inHandler = qfalse;
			return;
		}
	}

	// get the item with focus
	for ( i = 0; i < itemCount; i++ ) {
		if ( items[i]->window.flags & WINDOW_HASFOCUS ) {
			item = items[i];
		}
	}

	if ( item != NULL ) {
		if ( item->HandleKey( key, down ) ) {
			item->Action();
			inHandler = qfalse;
			return;
		}
	}

	if ( !down ) {
		inHandler = qfalse;
		return;
	}

	if ( key > 0 && key <= 255 && onKey[key] ) {
		dnItem it;
		it.parent = this;
		it.RunScript( onKey[key] );
		return;
	}

	// default handling
	switch ( key ) {

	case K_F11:
		if ( DC->getCVarValue( "developer" ) ) {
			debugMode ^= 1;
		}
		break;

	case K_F12:
		if ( DC->getCVarValue( "developer" ) ) {
			DC->executeText( DN_EXEC_APPEND, "screenshot\n" );
		}
		break;
	case K_KP_UPARROW:
	case K_UPARROW:
	case K_LEFTARROW:
	case K_MWHEELUP:
		SetPrevCursorItem();
		break;

	case K_ESCAPE:
		if ( !g_bindCapture.Waiting() && onESC ) {
			dnItem it;
			it.parent = this;
			it.RunScript( onESC );
		}
		break;

	case K_TAB:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_RIGHTARROW:
	case K_MWHEELDOWN:
		SetNextCursorItem();
		break;

	case K_MOUSE1:
	case K_MOUSE2:
		if ( item ) {
			if ( item->type == ITEM_TYPE_TEXT ) {
				if ( Rect_ContainsPoint( item->CorrectedTextRect(), DC->cursorx, DC->cursory ) ) {
					item->Action();
				}
			} else if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_VALIDFILEFIELD ) {
				if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) ) {
					item->cursorPos = 0;
					g_editCapture.Start( item );
					DC->setOverstrikeMode( qtrue );
				}
			} else {
				if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) ) {
					item->Action();
				}
			}
		}
		break;

	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
		break;
	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE3:
		if ( item ) {
			if ( item->type == ITEM_TYPE_EDITFIELD || item->type == ITEM_TYPE_NUMERICFIELD || item->type == ITEM_TYPE_VALIDFILEFIELD ) {
				item->cursorPos = 0;

				g_editCapture.Start( item );
				DC->setOverstrikeMode( qtrue );
			} else {
				item->Action();
			}
		}
		break;
	}
	inHandler = qfalse;
}

void ToWindowCoords( float *x, float *y, dnWindow *window ) {
	if ( window->border != 0 ) {
		*x += window->borderSize;
		*y += window->borderSize;
	}
	*x += window->rect.x;
	*y += window->rect.y;
}

void Rect_ToWindowCoords( rectDef_t *rect, dnWindow *window ) {
	ToWindowCoords( &rect->x, &rect->y, window );
}

void dnItem::SetTextExtents( int *width, int *height, const char *text ) {
	const char *textPtr = ( text ) ? text : text;

	if ( textPtr == NULL ) {
		return;
	}

	*width = textRect.w;
	*height = textRect.h;

	// keeps us from computing the widths and heights more than once
	if ( *width == 0 || ( type == ITEM_TYPE_OWNERDRAW && textalignment == ITEM_ALIGN_CENTER ) ) {
		int originalWidth = DC->textWidth( text, font, textscale, 0 );

		if ( type == ITEM_TYPE_OWNERDRAW && ( textalignment == ITEM_ALIGN_CENTER || textalignment == ITEM_ALIGN_RIGHT ) ) {
			originalWidth += DC->ownerDrawWidth( window.ownerDraw, font, textscale );
		} else if ( ( type == ITEM_TYPE_EDITFIELD || type == ITEM_TYPE_VALIDFILEFIELD ) && textalignment == ITEM_ALIGN_CENTER && cvar ) {
			char buff[256];
			DC->getCVarString( cvar, buff, 256 );
			originalWidth += DC->textWidth( buff, font, textscale, 0 );
		}

		*width = DC->textWidth( textPtr, font, textscale, 0 );
		*height = DC->textHeight( textPtr, font, textscale, 0 );
		textRect.w = *width;
		textRect.h = *height;
		textRect.x = textalignx;
		textRect.y = textaligny;
		if ( textalignment == ITEM_ALIGN_RIGHT ) {
			textRect.x = textalignx - originalWidth;
		} else if ( textalignment == ITEM_ALIGN_CENTER ) {
			textRect.x = textalignx - originalWidth / 2;
		}

		ToWindowCoords( &textRect.x, &textRect.y, &window );
	}
}

void dnItem::TextColor( vec4_t *newColor ) {
	vec4_t lowLight;
	dnMenu *parent = ( dnMenu * )this->parent;

	Fade( &window.flags, &window.foreColor[3], parent->fadeClamp, &window.nextTime, parent->fadeCycle, qtrue, parent->fadeAmount );

	if ( window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * parent->focusColor[0];
		lowLight[1] = 0.8 * parent->focusColor[1];
		lowLight[2] = 0.8 * parent->focusColor[2];
		lowLight[3] = 0.8 * parent->focusColor[3];
		LerpColor( parent->focusColor,lowLight,*newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else if ( textStyle == ITEM_TEXTSTYLE_BLINK && !( ( DC->realTime / BLINK_DIVISOR ) & 1 ) ) {
		lowLight[0] = 0.8 * window.foreColor[0];
		lowLight[1] = 0.8 * window.foreColor[1];
		lowLight[2] = 0.8 * window.foreColor[2];
		lowLight[3] = 0.8 * window.foreColor[3];
		LerpColor( window.foreColor,lowLight,*newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( newColor, &window.foreColor, sizeof( vec4_t ) );
		// items can be enabled and disabled based on cvars
	}

	if ( enableCvar && *enableCvar && cvarTest && *cvarTest ) {
		if ( cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !EnableShowViaCvar( CVAR_ENABLE ) ) {
			memcpy( newColor, &parent->disableColor, sizeof( vec4_t ) );
		}
	}
}

void dnItem::PaintAutoWrappedText( void ) {
	char buf[1024];
	const char *p, *textPtr, *newLinePtr;
	char buff[1024];
	int width, height, len, textWidth, newLine, newLineWidth;
	float y;
	vec4_t color;

	textWidth = 0;
	newLinePtr = NULL;

	if ( text == NULL ) {
		if ( cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( cvar, buf, sizeof( buf ) );
			textPtr = buf;
		}
	} else {
		textPtr = text;
	}
	if ( *textPtr == '\0' ) {
		return;
	}
	TextColor( &color );
	SetTextExtents( &width, &height, textPtr );

	y = textaligny;
	len = 0;
	buff[0] = '\0';
	newLine = 0;
	newLineWidth = 0;
	p = textPtr;
	while ( p ) {
		if ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\0' ) {
			newLine = len;
			newLinePtr = p + 1;
			newLineWidth = textWidth;
		}
		textWidth = DC->textWidth( buff, font, textscale, 0 );
		if ( ( newLine && textWidth > window.rect.w ) || *p == '\n' || *p == '\0' ) {
			if ( len ) {
				if ( textalignment == ITEM_ALIGN_LEFT ) {
					textRect.x = textalignx;
				} else if ( textalignment == ITEM_ALIGN_RIGHT ) {
					textRect.x = textalignx - newLineWidth;
				} else if ( textalignment == ITEM_ALIGN_CENTER ) {
					textRect.x = textalignx - newLineWidth / 2;
				}
				textRect.y = y;
				ToWindowCoords( &textRect.x, &textRect.y, &window );
				buff[newLine] = '\0';
				DC->drawText( textRect.x, textRect.y, font, textscale, color, buff, 0, 0, textStyle );
			}
			if ( *p == '\0' ) {
				break;
			}
			y += height + 5;
			p = newLinePtr;
			len = 0;
			newLine = 0;
			newLineWidth = 0;
			continue;
		}
		buff[len++] = *p++;

		if ( buff[len - 1] == 13 ) {
			buff[len - 1] = ' ';
		}

		buff[len] = '\0';
	}
}

void dnItem::PaintWrappedText( void ) {
	char buf[1024];
	const char *p, *start, *textPtr;
	char buff[1024];
	int width, height;
	float x, y;
	vec4_t color;

	// now paint the text and/or any optional images
	// default to left

	if ( text == NULL ) {
		if ( cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( cvar, buf, sizeof( buf ) );
			textPtr = buf;
		}
	} else {
		textPtr = text;
	}
	if ( *textPtr == '\0' ) {
		return;
	}

	TextColor( &color );
	SetTextExtents( &width, &height, textPtr );

	x = textRect.x;
	y = textRect.y;
	start = textPtr;
	p = strchr( textPtr, '\r' );
	while ( p && *p ) {
		strncpy( buff, start, p - start + 1 );
		buff[p - start] = '\0';
		DC->drawText( x, y, font, textscale, color, buff, 0, 0, textStyle );
		y += height + 5;
		start += p - start + 1;
		p = strchr( p + 1, '\r' );
	}
	DC->drawText( x, y, font, textscale, color, start, 0, 0, textStyle );
}

void dnItem::PaintText( void ) {
	/* the local holds the cvar text; the member keeps the name text, so
	   the buffer is named differently here */
	char buf[1024];
	char infostring[SAVE_INFOSTRING_LENGTH];
	const char *textPtr;
	int height, width;
	vec4_t color;


	if ( textSavegameInfo ) {
		// the client fills this cvar right before the save info text paints
		DC->getCVarString( "ui_savegameInfo", infostring, sizeof( infostring ) );
		text = &infostring[0];
	}

	if ( window.flags & WINDOW_WRAPPED ) {
		PaintWrappedText();
		return;
	}
	if ( window.flags & WINDOW_AUTOWRAPPED ) {
		PaintAutoWrappedText();
		return;
	}

	if ( text == NULL ) {
		if ( cvar == NULL ) {
			return;
		} else {
			DC->getCVarString( cvar, buf, sizeof( buf ) );
			if ( window.flags & CG_SHOW_TEXTASINT ) {
				COM_StripExtension( buf, buf );
			}
			textPtr = buf;
		}
	} else {
		textPtr = text;
	}

	// this needs to go here as it sets extents for cvar types as well
	SetTextExtents( &width, &height, textPtr );

	if ( *textPtr == '\0' ) {
		return;
	}


	TextColor( &color );

	// text shadow and outline styles are not ported yet

	DC->drawText( textRect.x, textRect.y, font, textscale, color, textPtr, 0, 0, textStyle );
}




void dnEditFieldItem::PaintType( void ) {
	char buff[1024];
	vec4_t newColor, lowLight;
	int offset;
	dnMenu *owner = (dnMenu*)parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)typeData;

	PaintText();

	buff[0] = '\0';

	if ( cvar ) {
		DC->getCVarString( cvar, buff, sizeof( buff ) );
	}

	owner = (dnMenu*)parent;

	if ( window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * owner->focusColor[0];
		lowLight[1] = 0.8 * owner->focusColor[1];
		lowLight[2] = 0.8 * owner->focusColor[2];
		lowLight[3] = 0.8 * owner->focusColor[3];
		LerpColor( owner->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &window.foreColor, sizeof( vec4_t ) );
	}

	offset = ( text && *text ) ? 8 : 0;
	if ( window.flags & WINDOW_HASFOCUS && g_editCapture.IsActive() ) {
		char cursor = DC->getOverstrikeMode() ? '_' : '|';
		DC->drawTextWithCursor( textRect.x + textRect.w + offset, textRect.y, font, textscale, newColor, buff + editPtr->paintOffset, cursorPos - editPtr->paintOffset, cursor, editPtr->maxPaintChars, textStyle );
	} else {
		DC->drawText( textRect.x + textRect.w + offset, textRect.y, font, textscale, newColor, buff + editPtr->paintOffset, 0, editPtr->maxPaintChars, textStyle );
	}

}

void dnYesNoItem::PaintType( void ) {
	vec4_t newColor, lowLight;
	float value;
	dnMenu *owner = (dnMenu*)parent;
	const char *yes_str = "yes", *no_str = "no";

	value = ( cvar ) ? DC->getCVarValue( cvar ) : 0;

	if ( window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * owner->focusColor[0];
		lowLight[1] = 0.8 * owner->focusColor[1];
		lowLight[2] = 0.8 * owner->focusColor[2];
		lowLight[3] = 0.8 * owner->focusColor[3];
		LerpColor( owner->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &window.foreColor, sizeof( vec4_t ) );
	}

	yes_str = DC->getTranslatedString( yes_str );
	no_str = DC->getTranslatedString( no_str );

	if ( text ) {
		PaintText();
		DC->drawText( ValueColumnX(), textRect.y, font, textscale, newColor, ( value != 0 ) ? yes_str : no_str, 0, 0, textStyle );
	} else {
		DC->drawText( textRect.x, textRect.y, font, textscale, newColor, ( value != 0 ) ? yes_str : no_str, 0, 0, textStyle );
	}
}

void dnMultiItem::PaintType( void ) {
	vec4_t newColor, lowLight;
	/* the member keeps the name text, so the drawn setting is named
	   value here */
	const char *value = "";
	dnMenu *owner = (dnMenu*)parent;

	if ( window.flags & WINDOW_HASFOCUS ) {
		lowLight[0] = 0.8 * owner->focusColor[0];
		lowLight[1] = 0.8 * owner->focusColor[1];
		lowLight[2] = 0.8 * owner->focusColor[2];
		lowLight[3] = 0.8 * owner->focusColor[3];
		LerpColor( owner->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &window.foreColor, sizeof( vec4_t ) );
	}

	value = Setting();

	if ( text ) {
		PaintText();
		DC->drawText( ValueColumnX(), textRect.y, font, textscale, newColor, value, 0, 0, textStyle );
	} else {
		DC->drawText( textRect.x, textRect.y, font, textscale, newColor, value, 0, 0, textStyle );
	}
}


typedef struct {
	char    *command;
	int id;
	int defaultbind1;
	int defaultbind2;
	int bind1;
	int bind2;
} bind_t;

typedef struct
{
	char*   name;
	float defaultvalue;
	float value;
} configcvar_t;


static bind_t g_bindings[] =
{
	{"+scores",      -1,             -1, -1, -1},
	{"+speed",           K_SHIFT,        -1, -1, -1},
	{"+forward",     K_UPARROW,      -1, -1, -1},
	{"+back",            K_DOWNARROW,    -1, -1, -1},
	{"+moveleft",        ',',         -1, -1, -1},
	{"+moveright",       '.',         -1, -1, -1},
	{"+moveup",      K_SPACE,        -1, -1, -1},
	{"+movedown",        'c',         -1, -1, -1},
	{"+left",            K_LEFTARROW,    -1, -1, -1},
	{"+right",           K_RIGHTARROW,   -1, -1, -1},
	{"+strafe",      K_ALT,          -1, -1, -1},
	{"+lookup",      K_PGDN,         -1, -1, -1},
	{"+lookdown",        K_DEL,          -1, -1, -1},
	{"+mlook",           '/',         -1, -1, -1},
	{"centerview",       K_END,          -1, -1, -1},
	{"+zoom",            'z',             -1, -1, -1},
	{"weaponbank 1", '1',         -1, -1, -1},
	{"weaponbank 2", '2',         -1, -1, -1},
	{"weaponbank 3", '3',         -1, -1, -1},
	{"weaponbank 4", '4',         -1, -1, -1},
	{"weaponbank 5", '5',         -1, -1, -1},
	{"weaponbank 6", '6',         -1, -1, -1},
	{"weaponbank 7", '7',         -1, -1, -1},
	{"weaponbank 8", '8',         -1, -1, -1},
	{"weaponbank 9", '9',         -1, -1, -1},
	{"weaponbank 10",    '0',         -1, -1, -1},
	{"weapon 1",     -1,             -1, -1, -1},
	{"weapon 2",     -1,             -1, -1, -1},
	{"weapon 3",     -1,             -1, -1, -1},
	{"weapon 4",     -1,             -1, -1, -1},
	{"weapon 5",     -1,             -1, -1, -1},
	{"weapon 6",     -1,             -1, -1, -1},
	{"weapon 7",     -1,             -1, -1, -1},
	{"weapon 8",     -1,             -1, -1, -1},
	{"weapon 9",     -1,             -1, -1, -1},
	{"weapon 10",        -1,             -1, -1, -1},
	{"weapon 11",        -1,             -1, -1, -1},
	{"weapon 12",        -1,             -1, -1, -1},
	{"weapon 13",        -1,             -1, -1, -1},
	{"weapon 14",        -1,             -1, -1, -1},
	{"weapon 15",        -1,             -1, -1, -1},
	{"weapon 16",        -1,             -1, -1, -1},
	{"weapon 17",        -1,             -1, -1, -1},
	{"weapon 18",        -1,             -1, -1, -1},
	{"weapon 19",        -1,             -1, -1, -1},
	{"weapon 20",        -1,             -1, -1, -1},
	{"weapon 21",        -1,             -1, -1, -1},
	{"weapon 22",        -1,             -1, -1, -1},
	{"weapon 23",        -1,             -1, -1, -1},
	{"weapon 24",        -1,             -1, -1, -1},
	{"weapon 25",        -1,             -1, -1, -1},
	{"weapon 26",        -1,             -1, -1, -1},
	{"weapon 27",        -1,             -1, -1, -1},
	{"weapon 28",        -1,             -1, -1, -1},
	{"weapon 29",        -1,             -1, -1, -1},
	{"weapon 30",        -1,             -1, -1, -1},
	{"weapon 31",        -1,             -1, -1, -1},
	{"weapon 32",        -1,             -1, -1, -1},
	{"+attack",      K_CTRL,         -1, -1, -1},
	{"weapprev",     K_MWHEELDOWN,   -1, -1, -1},
	{"weapnext",     K_MWHEELUP,     -1, -1, -1},
	{"weapalt",          -1,             -1, -1, -1},
	{"weaplastused", -1,             -1, -1, -1},
	{"weapnextinbank",   -1,             -1, -1, -1},
	{"weapprevinbank",   -1,             -1, -1, -1},
	{"+useitem",     K_ENTER,        -1, -1, -1},
	{"itemprev",     '[',         -1, -1, -1},
	{"itemnext",     ']',         -1, -1, -1},
	{"+button3",     K_MOUSE3,       -1, -1, -1},
	{"prevTeamMember",   -1,             -1, -1, -1},
	{"nextTeamMember",   -1,             -1, -1, -1},
	{"nextOrder",        -1,             -1, -1, -1},
	{"confirmOrder", -1,             -1, -1, -1},
	{"denyOrder",        -1,             -1, -1, -1},
	{"taskOffense",  -1,             -1, -1, -1},
	{"taskDefense",  -1,             -1, -1, -1},
	{"taskPatrol",       -1,             -1, -1, -1},
	{"taskCamp",     -1,             -1, -1, -1},
	{"taskFollow",       -1,             -1, -1, -1},
	{"taskRetrieve", -1,             -1, -1, -1},
	{"taskEscort",       -1,             -1, -1, -1},
	{"taskOwnFlag",  -1,             -1, -1, -1},
	{"taskSuicide",  -1,             -1, -1, -1},
	{"tauntKillInsult", -1,              -1, -1, -1},
	{"tauntPraise",  -1,             -1, -1, -1},
	{"tauntTaunt",       -1,             -1, -1, -1},
	{"tauntDeathInsult",-1,              -1, -1, -1},
	{"tauntGauntlet",    -1,             -1, -1, -1},
	{"scoresUp",     -1,             -1, -1, -1},
	{"scoresDown",       -1,             -1, -1, -1},
	{"messagemode",  -1,             -1, -1, -1},
	{"messagemode2", -1,             -1, -1, -1},
	{"messagemode3", -1,             -1, -1, -1},
	{"messagemode4", -1,             -1, -1, -1},

	{"savegame quicksave",   -1,         -1, -1, -1},
	{"loadgame quicksave",   -1,         -1, -1, -1},

	{"+activate",        -1,             -1, -1, -1},
	{"zoomin",           -1,             -1, -1, -1},
	{"zoomout",          -1,             -1, -1, -1},
	{"+kick",            -1,             -1, -1, -1},
	{"+reload",      -1,             -1, -1, -1},
	{"+sprint",      -1,             -1, -1, -1},
	{"notebook",     K_TAB,          -1, -1, -1},
	{"+leanleft",        -1,             -1, -1, -1},
	{"+leanright",       -1,             -1, -1, -1},
	{"kill",         -1,             -1, -1, -1},
	// Q2 engine commands shown in the Duke customize controls list
	{"+klook",       -1,             -1, -1, -1},
	{"inven",        -1,             -1, -1, -1},
	{"invuse",       -1,             -1, -1, -1},
	{"invdrop",      -1,             -1, -1, -1},
	{"invprev",      -1,             -1, -1, -1},
	{"invnext",      -1,             -1, -1, -1},
	{"cmd help",     -1,             -1, -1, -1}
};


static const int g_bindCount = sizeof( g_bindings ) / sizeof( bind_t );

static void Controls_GetKeyAssignment( char *command, int *twokeys ) {
	int count;
	int j;
	char b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		DC->getBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_strcasecmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if ( count == 2 ) {
				break;
			}
		}
	}
}

void Controls_GetConfig( void ) {
	int i;
	int twokeys[2];

	// iterate each command, get its numeric binding
	for ( i = 0; i < g_bindCount; i++ )
	{

		Controls_GetKeyAssignment( g_bindings[i].command, twokeys );

		g_bindings[i].bind1 = twokeys[0];
		g_bindings[i].bind2 = twokeys[1];
	}

}

void Controls_SetConfig( qboolean restart ) {
	int i;

	// iterate each command, get its numeric binding
	for ( i = 0; i < g_bindCount; i++ )
	{

		if ( g_bindings[i].bind1 != -1 ) {
			DC->setBinding( g_bindings[i].bind1, g_bindings[i].command );

			if ( g_bindings[i].bind2 != -1 ) {
				DC->setBinding( g_bindings[i].bind2, g_bindings[i].command );
			}
		}
	}

#if !defined( __MACOS__ )
	DC->executeText( DN_EXEC_APPEND, "in_restart\n" );
#endif
}

void Controls_SetDefaults( void ) {
	int i;

	// iterate each command, set its default binding
	for ( i = 0; i < g_bindCount; i++ )
	{
		g_bindings[i].bind1 = g_bindings[i].defaultbind1;
		g_bindings[i].bind2 = g_bindings[i].defaultbind2;
	}

}

int BindingIDFromName( const char *name ) {
	int i;
	for ( i = 0; i < g_bindCount; i++ )
	{
		if ( Q_strcasecmp( name, g_bindings[i].command ) == 0 ) {
			return i;
		}
	}
	return -1;
}

char g_nameBind1[32];
char g_nameBind2[32];

void BindingFromName( const char *cvar ) {
	int i, b1, b2;

	// iterate each command, set its default binding
	for ( i = 0; i < g_bindCount; i++ )
	{
		if ( Q_strcasecmp( cvar, g_bindings[i].command ) == 0 ) {
			b1 = g_bindings[i].bind1;
			if ( b1 == -1 ) {
				break;
			}
			DC->keynumToStringBuf( b1, g_nameBind1, 32 );
			Q_strupr( g_nameBind1 );

			b2 = g_bindings[i].bind2;
			if ( b2 != -1 ) {
				DC->keynumToStringBuf( b2, g_nameBind2, 32 );
				Q_strupr( g_nameBind2 );
				strcat( g_nameBind1, va( " %s ", DC->getTranslatedString( "or" ) ) );
				strcat( g_nameBind1, g_nameBind2 );
			}
			return;
		}
	}
	strcpy( g_nameBind1, "???" );
}

/* list feeders read the binding table directly; the command string doubles
   as the row label so the list can never drift from the key setup */
int UI_BindListCount( void ) {
	return g_bindCount;
}

const char *UI_BindListLabel( int index ) {
	if ( index < 0 || index >= g_bindCount ) {
		return NULL;
	}
	return g_bindings[index].command;
}

const char *UI_BindListKeys( int index ) {
	if ( index < 0 || index >= g_bindCount ) {
		return NULL;
	}
	BindingFromName( g_bindings[index].command );
	return g_nameBind1;
}

void dnSliderItem::PaintType( void ) {
	/* the Duke build has no shader slider assets, so the bar is a row of
	   glyph dashes: the dim bar first, then the filled part drawn bright
	   over the same glyphs so both halves line up exactly */
	static const char barFull[DUKE_SLIDER_SEGMENTS + 1] = "----------";
	vec4_t barDim = { 0.45f, 0.45f, 0.45f, 1.0f };
	vec4_t barLit = { 1.0f, 1.0f, 1.0f, 1.0f };
	editFieldDef_t *editDef = ( editFieldDef_t * )typeData;
	char filled[DUKE_SLIDER_SEGMENTS + 1];
	float x, y, value, frac;
	int filledCount, i;

	y = window.rect.y;
	if ( text ) {
		PaintText();
		x = ValueColumnX();
	} else {
		x = window.rect.x;
	}

	value = ( cvar ) ? DC->getCVarValue( cvar ) : 0;
	frac = 0.0f;
	if ( editDef && editDef->maxVal > editDef->minVal ) {
		if ( value < editDef->minVal ) {
			value = editDef->minVal;
		} else if ( value > editDef->maxVal ) {
			value = editDef->maxVal;
		}
		frac = ( value - editDef->minVal ) / ( editDef->maxVal - editDef->minVal );
	}
	filledCount = ( int )( frac * DUKE_SLIDER_SEGMENTS + 0.5f );
	for ( i = 0; i < filledCount; i++ ) {
		filled[i] = '-';
	}
	filled[filledCount] = '\0';

	DC->drawText( x, y, font, textscale, barDim, barFull, 0, 0, textStyle );
	if ( filledCount > 0 ) {
		DC->drawText( x, y, font, textscale, barLit, filled, 0, 0, textStyle );
	}
}

void dnBindItem::PaintType( void ) {
	vec4_t newColor, lowLight;
	float value;
	int maxChars = 0;
	dnMenu *owner = (dnMenu*)parent;
	editFieldDef_t *editPtr = (editFieldDef_t*)typeData;
	if ( editPtr ) {
		maxChars = editPtr->maxPaintChars;
	}

	value = ( cvar ) ? DC->getCVarValue( cvar ) : 0;

	if ( window.flags & WINDOW_HASFOCUS ) {
		if ( g_bindCapture.Item() == this ) {
			lowLight[0] = 0.8f * 1.0f;
			lowLight[1] = 0.8f * 0.0f;
			lowLight[2] = 0.8f * 0.0f;
			lowLight[3] = 0.8f * 1.0f;
		} else {
			lowLight[0] = 0.8f * owner->focusColor[0];
			lowLight[1] = 0.8f * owner->focusColor[1];
			lowLight[2] = 0.8f * owner->focusColor[2];
			lowLight[3] = 0.8f * owner->focusColor[3];
		}
		LerpColor( owner->focusColor,lowLight,newColor,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
	} else {
		memcpy( &newColor, &window.foreColor, sizeof( vec4_t ) );
	}

	if ( text ) {
		PaintText();
		BindingFromName( cvar );
		DC->drawText( textRect.x + textRect.w + 8, textRect.y, font, textscale, newColor, g_nameBind1, 0, maxChars, textStyle );
	} else {
		DC->drawText( textRect.x, textRect.y, font, textscale, newColor, ( value != 0 ) ? "FIXME" : "FIXME", 0, maxChars, textStyle );
	}
}

qboolean Display_KeyBindPending() {
	return g_bindCapture.Waiting();
}

/* nested defs live flat in the menu item list with baked absolute rects;
   reachability walks the container back links, so a hidden windowDef hides
   its whole subtree */
qboolean dnItem::TreeVisible( void ) {
	dnItem *item = this;
	while ( item ) {
		if ( !( item->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
			return qfalse;
		}
		item = item->containerItem;
	}
	return qtrue;
}

qboolean dnBindCapture::HandleKey( int key, qboolean down ) {
	int id;
	int i;
	/* a bindDef carries the command in its cvar; a listbox row stores it
	   in the capture buffer instead */
	const char *bindCommand = ( item->type == ITEM_TYPE_BIND && item->cvar ) ? item->cvar : rowCommand;

	if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && !waiting ) {
		if ( down && ( key == K_MOUSE1 || key == K_ENTER ) ) {

			Start( item, NULL );
		}
		return qtrue;
	} else
	{
		if ( !waiting || item == NULL ) {
			return qtrue;
		}

		if ( key & K_CHAR_FLAG ) {
			return qtrue;
		}

		switch ( key )
		{
		case K_ESCAPE:
			Cancel();
			return qtrue;

		case K_BACKSPACE:
			id = BindingIDFromName( bindCommand );
			if ( id != -1 ) {
				// a -1 key clears every binding of the command on the way down;
				// returning early would leave the old second binding active
				key = -1;
			}
			break;

		case '`':
			return qtrue;
		}
	}

	if ( key != -1 ) {

		for ( i = 0; i < g_bindCount; i++ )
		{

			if ( g_bindings[i].bind2 == key ) {
				g_bindings[i].bind2 = -1;
			}

			if ( g_bindings[i].bind1 == key ) {
				g_bindings[i].bind1 = g_bindings[i].bind2;
				g_bindings[i].bind2 = -1;
			}
		}
	}


	id = BindingIDFromName( bindCommand );

	if ( id != -1 ) {
		if ( key == -1 ) {
			if ( g_bindings[id].bind1 != -1 ) {
				DC->setBinding( g_bindings[id].bind1, "" );
				g_bindings[id].bind1 = -1;
			}
			if ( g_bindings[id].bind2 != -1 ) {
				DC->setBinding( g_bindings[id].bind2, "" );
				g_bindings[id].bind2 = -1;
			}
		} else if ( g_bindings[id].bind1 == -1 )     {
			g_bindings[id].bind1 = key;
		} else if ( g_bindings[id].bind1 != key && g_bindings[id].bind2 == -1 )     {
			g_bindings[id].bind2 = key;
		} else {
			DC->setBinding( g_bindings[id].bind1, "" );
			DC->setBinding( g_bindings[id].bind2, "" );
			g_bindings[id].bind1 = key;
			g_bindings[id].bind2 = -1;
		}
	}

	Controls_SetConfig( qtrue );
	Cancel();

	return qtrue;
}



void AdjustFrom640( float *x, float *y, float *w, float *h ) {
	*x *= DC->xscale;
	*y *= DC->yscale;
	*w *= DC->xscale;
	*h *= DC->yscale;
}

void dnModelItem::PaintType( void ) {
	float x, y, w, h;
	refdef_t refdef;
	qhandle_t hModel;
	refEntity_t ent;
	vec3_t mins, maxs, origin;
	vec3_t angles;
	modelDef_t *modelPtr = (modelDef_t*)typeData;
	int backLerpWhole;

	if ( modelPtr == NULL ) {
		return;
	}

	if ( !asset ) {
		return;
	}

	hModel = asset;

	// setup the refdef
	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );
	x = window.rect.x + 1;
	y = window.rect.y + 1;
	w = window.rect.w - 2;
	h = window.rect.h - 2;

	AdjustFrom640( &x, &y, &w, &h );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	DC->modelBounds( hModel, mins, maxs );

	origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );

	// calculate distance so the model nearly fills the box
	if ( qtrue ) {
		float len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;    // len / tan( fov/2 )
	} else {
		origin[0] = textscale;
	}

#define NEWWAY
#ifdef NEWWAY
	refdef.fov_x = ( modelPtr->fov_x ) ? modelPtr->fov_x : w;
	refdef.fov_y = ( modelPtr->fov_y ) ? modelPtr->fov_y : h;
#else
	refdef.fov_x = (int)( (float)refdef.width / 640.0f * 90.0f );
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );
#endif
	DC->clearScene();

	refdef.time = DC->realTime;

	// add the model

	memset( &ent, 0, sizeof( ent ) );


	// use item storage to track
	if ( modelPtr->rotationSpeed ) {
		if ( DC->realTime > window.nextTime ) {
			window.nextTime = DC->realTime + modelPtr->rotationSpeed;
			modelPtr->angle = (int)( modelPtr->angle + 1 ) % 360;
		}
	}
	VectorSet( angles, 0, modelPtr->angle, 0 );
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;


	if ( modelPtr->frameTime ) { // don't advance on the first frame
		modelPtr->backlerp += ( ( ( DC->realTime - modelPtr->frameTime ) / 1000.0f ) * (float)modelPtr->fps );
	}

	if ( modelPtr->backlerp > 1 ) {
		backLerpWhole = ( int )floor( modelPtr->backlerp );

		modelPtr->frame += ( backLerpWhole );
		if ( ( modelPtr->frame - modelPtr->startframe ) > modelPtr->numframes ) {
			modelPtr->frame = modelPtr->startframe + modelPtr->frame % modelPtr->numframes; // frame wrapping ignores loopframes

		}
		modelPtr->oldframe += ( backLerpWhole );
		if ( ( modelPtr->oldframe - modelPtr->startframe ) > modelPtr->numframes ) {
			modelPtr->oldframe = modelPtr->startframe + modelPtr->oldframe % modelPtr->numframes;   // frame wrapping ignores loopframes

		}
		modelPtr->backlerp = modelPtr->backlerp - backLerpWhole;
	}

	modelPtr->frameTime = DC->realTime;

	ent.frame       = modelPtr->frame;
	ent.oldframe    = modelPtr->oldframe;
	ent.backlerp    = 1.0f - modelPtr->backlerp;

	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	DC->addRefEntityToScene( &ent );
	DC->renderScene( &refdef );

}


void dnItem::PaintImage( void ) {
	DC->drawHandlePic( window.rect.x + 1, window.rect.y + 1, window.rect.w - 2, window.rect.h - 2, asset );
}

/* Duke menus highlight the selected row by drawing it bright; the glyph
   renderer only has the dim and bright palette variants, so the row color
   is picked by cursor position rather than shaded */
vec_t *dnListBoxItem::RowColor( int row ) {
	static vec4_t rowDim = { 0.45f, 0.45f, 0.45f, 1.0f };
	static vec4_t rowLit = { 1.0f, 1.0f, 1.0f, 1.0f };

	return ( row == cursorPos ) ? rowLit : rowDim;
}

void dnListBoxItem::PaintType( void ) {
	float x, y, size, count, i, thumb;
	qhandle_t image;
	qhandle_t optionalImage;
	listBoxDef_t *listPtr = (listBoxDef_t*)typeData;

	// the listbox is horizontal or vertical and has a fixed size scroll bar going either direction
	// elements are enumerated from the DC and either text or image handles are acquired from the DC as well
	// textscale is used to size the text, textalignx and textaligny are used to size image elements
	// there is no clipping available so only the last completely visible item is painted
	count = DC->feederCount( special );
	// default is vertical if horizontal flag is not here
	if ( window.flags & WINDOW_HORIZONTAL ) {
		// draw scrollbar in bottom of the window
		// bar
		x = window.rect.x + 1;
		y = window.rect.y + window.rect.h - SCROLLBAR_SIZE - 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowLeft );
		x += SCROLLBAR_SIZE - 1;
		size = window.rect.w - ( SCROLLBAR_SIZE * 2 );
		DC->drawHandlePic( x, y, size + 1, SCROLLBAR_SIZE, DC->Assets.scrollBar );
		x += size - 1;
		DC->drawHandlePic( x, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarArrowRight );
		// thumb
		thumb = ThumbDrawPosition(); //Item_ListBox_ThumbPosition(item);
		if ( thumb > x - SCROLLBAR_SIZE - 1 ) {
			thumb = x - SCROLLBAR_SIZE - 1;
		}
		DC->drawHandlePic( thumb, y, SCROLLBAR_SIZE, SCROLLBAR_SIZE, DC->Assets.scrollBarThumb );
		listPtr->endPos = listPtr->startPos;
		size = window.rect.w - 2;
		// items
		// size contains max available space
		if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
			x = window.rect.x + 1;
			y = window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage( special, i );
				if ( image ) {
					DC->drawHandlePic( x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image );
				}

				if ( i == cursorPos ) {
					DC->drawRect( x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, window.borderSize, window.borderColor );
				}

				size -= listPtr->elementWidth;
				if ( size < listPtr->elementWidth ) {
					listPtr->drawPadding = size;
					break;
				}
				x += listPtr->elementWidth;
				listPtr->endPos++;
			}
		} else {
		}
	} else {
		// draw scrollbar to right side of the window
		// the Duke build registers no scrollbar shader assets, so the bar is
		// a column of glyph dashes: dim track down the whole height, bright
		// dash over the thumb position
		x = window.rect.x + window.rect.w - SCROLLBAR_SIZE - 1;
		y = window.rect.y + 1;

		listPtr->endPos = listPtr->startPos;
		size = window.rect.h - ( SCROLLBAR_SIZE * 2 );
		thumb = ThumbDrawPosition();
		if ( thumb > y + size - listPtr->elementHeight ) {
			thumb = y + size - listPtr->elementHeight;
		}
		{
			vec4_t trackDim = { 0.45f, 0.45f, 0.45f, 1.0f };
			vec4_t thumbLit = { 1.0f, 1.0f, 1.0f, 1.0f };
			float dashY, dashStep;

			// a missing elementHeight would spin the loop forever
			dashStep = ( listPtr->elementHeight > 0 ) ? listPtr->elementHeight : 16.0f;
			for ( dashY = y; dashY < y + size; dashY += dashStep ) {
				DC->drawText( x, dashY, font, textscale, trackDim, "-", 0, 0, textStyle );
			}
			DC->drawText( x, thumb, font, textscale, thumbLit, "-", 0, 0, textStyle );
		}

		// adjust size for item painting
		size = window.rect.h - 2;
		if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
			x = window.rect.x + 1;
			y = window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				// always draw at least one
				// which may overdraw the box if it is too small for the element
				image = DC->feederItemImage( special, i );
				if ( image ) {
					DC->drawHandlePic( x + 1, y + 1, listPtr->elementWidth - 2, listPtr->elementHeight - 2, image );
				}

				if ( i == cursorPos ) {
					DC->drawRect( x, y, listPtr->elementWidth - 1, listPtr->elementHeight - 1, window.borderSize, window.borderColor );
				}

				listPtr->endPos++;
				size -= listPtr->elementWidth;
				if ( size < listPtr->elementHeight ) {
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				y += listPtr->elementHeight;
			}
		} else {
			x = window.rect.x + 1;
			y = window.rect.y + 1;
			for ( i = listPtr->startPos; i < count; i++ ) {
				const char *text;
				// always draw at least one
				// which may overdraw the box if it is too small for the element

				if ( listPtr->numColumns > 0 ) {
					int j;
					for ( j = 0; j < listPtr->numColumns; j++ ) {
						text = DC->feederItemText( special, i, j, &optionalImage );
						if ( optionalImage != NULL ) {
							DC->drawHandlePic( x + 4 + listPtr->columnInfo[j].pos, y - 1 + listPtr->elementHeight / 2, listPtr->columnInfo[j].width, listPtr->columnInfo[j].width, optionalImage );
						} else if ( text ) {
							DC->drawText( x + 4 + listPtr->columnInfo[j].pos, y, font, textscale, RowColor( i ), text, 0, listPtr->columnInfo[j].maxChars, textStyle );
						}
					}
				} else {
					text = DC->feederItemText( special, i, 0, &optionalImage );
					if ( optionalImage != NULL ) {
						// image columns from the feeder are not drawn yet
					} else if ( text ) {
						DC->drawText( x + 4, y, font, textscale, RowColor( i ), text, 0, 0, textStyle );
					}
				}

				size -= listPtr->elementHeight;
				if ( size < listPtr->elementHeight ) {
					listPtr->drawPadding = listPtr->elementHeight - size;
					break;
				}
				listPtr->endPos++;
				y += listPtr->elementHeight;
			}
		}
	}
}


void dnOwnerDrawItem::PaintType( void ) {
	dnMenu *owner = (dnMenu*)parent;

	if ( DC->ownerDrawItem ) {
		vec4_t color, lowLight;
		dnMenu *owner = (dnMenu*)parent;
		Fade( &window.flags, &window.foreColor[3], owner->fadeClamp, &window.nextTime, owner->fadeCycle, qtrue, owner->fadeAmount );
		memcpy( &color, &window.foreColor, sizeof( color ) );
		if ( numColors > 0 && DC->getValue ) {
			// if the value is within one of the ranges then set color to that, otherwise leave at default
			int i;
			float f = DC->getValue( window.ownerDraw, colorRangeType );
			for ( i = 0; i < numColors; i++ ) {
				if ( f >= colorRanges[i].low && f <= colorRanges[i].high ) {
					memcpy( &color, &colorRanges[i].color, sizeof( color ) );
					break;
				}
			}
		}

		// take hudalpha into account unless explicitly ignoring
		if ( !( window.flags & WINDOW_IGNORE_HUDALPHA ) ) {
			color[3] *= DC->getCVarValue( "cg_hudAlpha" );;
		}


		if ( window.flags & WINDOW_HASFOCUS ) {
			lowLight[0] = 0.8 * owner->focusColor[0];
			lowLight[1] = 0.8 * owner->focusColor[1];
			lowLight[2] = 0.8 * owner->focusColor[2];
			lowLight[3] = 0.8 * owner->focusColor[3];
			LerpColor( owner->focusColor,lowLight,color,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
		} else if ( textStyle == ITEM_TEXTSTYLE_BLINK && !( ( DC->realTime / BLINK_DIVISOR ) & 1 ) ) {
			lowLight[0] = 0.8 * window.foreColor[0];
			lowLight[1] = 0.8 * window.foreColor[1];
			lowLight[2] = 0.8 * window.foreColor[2];
			lowLight[3] = 0.8 * window.foreColor[3];
			LerpColor( window.foreColor,lowLight,color,0.5 + 0.5 * sin( DC->realTime / PULSE_DIVISOR ) );
		}

		if ( cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !EnableShowViaCvar( CVAR_ENABLE ) ) {
			memcpy( color, owner->disableColor, sizeof( vec4_t ) );
		}

		if ( text ) {
			PaintText();
			if ( text[0] ) {
				// +8 shifts the bar past the label of owner draw items with text
				DC->ownerDrawItem( textRect.x + textRect.w + 8, window.rect.y, window.rect.w, window.rect.h, 0, textaligny, window.ownerDraw, window.ownerDrawFlags, alignment, special, font, textscale, color, window.background, textStyle );
			} else {
				DC->ownerDrawItem( textRect.x + textRect.w, window.rect.y, window.rect.w, window.rect.h, 0, textaligny, window.ownerDraw, window.ownerDrawFlags, alignment, special, font, textscale, color, window.background, textStyle );
			}
		} else {
			DC->ownerDrawItem( window.rect.x, window.rect.y, window.rect.w, window.rect.h, textalignx, textaligny, window.ownerDraw, window.ownerDrawFlags, alignment, special, font, textscale, color, window.background, textStyle );
		}
	}
}


void dnItem::Paint( void ) {
	vec4_t red;
	dnMenu *parent = ( dnMenu * )this->parent;
	red[0] = red[3] = 1;
	red[1] = red[2] = 0;

	if ( window.flags & WINDOW_ORBITING ) {
		if ( DC->realTime > window.nextTime ) {
			float rx, ry, a, c, s, w, h;

			window.nextTime = DC->realTime + window.offsetTime;
			// translate
			w = window.rectClient.w / 2;
			h = window.rectClient.h / 2;
			rx = window.rectClient.x + w - window.rectEffects.x;
			ry = window.rectClient.y + h - window.rectEffects.y;
			a = 3 * M_PI / 180;
			c = cos( a );
			s = sin( a );
			window.rectClient.x = ( rx * c - ry * s ) + window.rectEffects.x - w;
			window.rectClient.y = ( rx * s + ry * c ) + window.rectEffects.y - h;
			UpdatePosition();

		}
	}


	if ( window.flags & WINDOW_INTRANSITION ) {
		if ( DC->realTime > window.nextTime ) {
			int done = 0;
			window.nextTime = DC->realTime + window.offsetTime;
			// transition the x,y
			if ( window.rectClient.x == window.rectEffects.x ) {
				done++;
			} else {
				if ( window.rectClient.x < window.rectEffects.x ) {
					window.rectClient.x += window.rectEffects2.x;
					if ( window.rectClient.x > window.rectEffects.x ) {
						window.rectClient.x = window.rectEffects.x;
						done++;
					}
				} else {
					window.rectClient.x -= window.rectEffects2.x;
					if ( window.rectClient.x < window.rectEffects.x ) {
						window.rectClient.x = window.rectEffects.x;
						done++;
					}
				}
			}
			if ( window.rectClient.y == window.rectEffects.y ) {
				done++;
			} else {
				if ( window.rectClient.y < window.rectEffects.y ) {
					window.rectClient.y += window.rectEffects2.y;
					if ( window.rectClient.y > window.rectEffects.y ) {
						window.rectClient.y = window.rectEffects.y;
						done++;
					}
				} else {
					window.rectClient.y -= window.rectEffects2.y;
					if ( window.rectClient.y < window.rectEffects.y ) {
						window.rectClient.y = window.rectEffects.y;
						done++;
					}
				}
			}
			if ( window.rectClient.w == window.rectEffects.w ) {
				done++;
			} else {
				if ( window.rectClient.w < window.rectEffects.w ) {
					window.rectClient.w += window.rectEffects2.w;
					if ( window.rectClient.w > window.rectEffects.w ) {
						window.rectClient.w = window.rectEffects.w;
						done++;
					}
				} else {
					window.rectClient.w -= window.rectEffects2.w;
					if ( window.rectClient.w < window.rectEffects.w ) {
						window.rectClient.w = window.rectEffects.w;
						done++;
					}
				}
			}
			if ( window.rectClient.h == window.rectEffects.h ) {
				done++;
			} else {
				if ( window.rectClient.h < window.rectEffects.h ) {
					window.rectClient.h += window.rectEffects2.h;
					if ( window.rectClient.h > window.rectEffects.h ) {
						window.rectClient.h = window.rectEffects.h;
						done++;
					}
				} else {
					window.rectClient.h -= window.rectEffects2.h;
					if ( window.rectClient.h < window.rectEffects.h ) {
						window.rectClient.h = window.rectEffects.h;
						done++;
					}
				}
			}

			UpdatePosition();

			if ( done == 4 ) {
				window.flags &= ~WINDOW_INTRANSITION;
			}

		}
	}

	if ( window.ownerDrawFlags && DC->ownerDrawVisible ) {
		if ( !DC->ownerDrawVisible( window.ownerDrawFlags ) ) {
			window.flags &= ~WINDOW_VISIBLE;
		} else {
			window.flags |= WINDOW_VISIBLE;
		}
	}

	if ( cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) ) {
		if ( !EnableShowViaCvar( CVAR_SHOW ) ) {
			return;
		}
	}

	if ( window.flags & WINDOW_TIMEDVISIBLE ) {
	}

	if ( !( window.flags & WINDOW_VISIBLE ) ) {
		return;
	}

	// a nested def paints only while its container chain is visible
	if ( !TreeVisible() ) {
		return;
	}

	// paint the rect first..
	Window_Paint( &window, parent->fadeAmount, parent->fadeClamp, parent->fadeCycle );

	if ( debugMode ) {
		vec4_t color;
		rectDef_t *r = CorrectedTextRect();
		color[1] = color[3] = 1;
		color[0] = color[2] = 0;
		DC->drawRect( r->x, r->y, r->w, r->h, 1, color );
	}


	PaintType();
}

/* installs the subclass vtable for the type; the subclasses carry no
   fields of their own, so nothing else in the block changes */
static void UI_PlaceItem( dnItem *item, int type ) {
	switch ( type ) {
	case ITEM_TYPE_OWNERDRAW:
		new ( item ) dnOwnerDrawItem;
		break;
	case ITEM_TYPE_TEXT:
	case ITEM_TYPE_BUTTON:
		new ( item ) dnTextItem;
		break;
	case ITEM_TYPE_EDITFIELD:
	case ITEM_TYPE_NUMERICFIELD:
	case ITEM_TYPE_VALIDFILEFIELD:
		new ( item ) dnEditFieldItem;
		break;
	case ITEM_TYPE_LISTBOX:
		new ( item ) dnListBoxItem;
		break;
	case ITEM_TYPE_MODEL:
	case ITEM_TYPE_MENUMODEL:
		new ( item ) dnModelItem;
		break;
	case ITEM_TYPE_YESNO:
		new ( item ) dnYesNoItem;
		break;
	case ITEM_TYPE_MULTI:
		new ( item ) dnMultiItem;
		break;
	case ITEM_TYPE_BIND:
		new ( item ) dnBindItem;
		break;
	case ITEM_TYPE_SLIDER:
		new ( item ) dnSliderItem;
		break;
	default:
		new ( item ) dnItem;
		break;
	}
}

/*
===============
dnItem::FinalizeType

a script item learns its type from the menu file while the itemDef block
parses; this installs the matching subclass vtable once the block is
done, without touching the parsed fields
===============
*/
void dnItem::FinalizeType( void ) {
	UI_PlaceItem( this, type );
}

/*
===============
UI_ConstructItem

allocates an item block from the UI memory pool and builds the subclass
matching the type; zeroing the fields is left to Init
===============
*/
dnItem *UI_ConstructItem( int type ) {
	dnItem *item = ( dnItem * )UI_Alloc( sizeof( dnItem ) );

	if ( item == NULL ) {
		return NULL;
	}

	UI_PlaceItem( item, type );
	return item;
}

void dnMenu::Init( void ) {
	memset( this, 0, sizeof( dnMenu ) );
	cursorItem = -1;
	fadeAmount = DC->Assets.fadeAmount;
	fadeClamp = DC->Assets.fadeClamp;
	fadeCycle = DC->Assets.fadeCycle;
	Window_Init( &window );
}

dnItem *dnMenu::GetFocusedItem( void ) {
	int i;

	for ( i = 0; i < itemCount; i++ ) {
		if ( items[i]->window.flags & WINDOW_HASFOCUS ) {
			return items[i];
		}
	}
	return NULL;
}

dnMenu *dnMenuSystem::GetFocused() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( menus[i].window.flags & WINDOW_HASFOCUS && menus[i].window.flags & WINDOW_VISIBLE ) {
			return &menus[i];
		}
	}
	return NULL;
}

void dnMenu::ScrollFeeder( int feeder, qboolean down ) {

	int i;
	for ( i = 0; i < itemCount; i++ ) {
		if ( items[i]->special == feeder ) {
			items[i]->ScrollFeed( down );
			return;
		}
	}
}



void dnMenuSystem::SetFeederSelection( dnMenu *menu, int feeder, int index, const char *name ) {
	if ( menu == NULL ) {
		if ( name == NULL ) {
			menu = GetFocused();
		} else {
			menu = FindByName( name );
		}
	}

	if ( menu ) {
		int i;
		for ( i = 0; i < menu->itemCount; i++ ) {
			if ( menu->items[i]->special == feeder ) {
				if ( index == 0 ) {
					listBoxDef_t *listPtr = (listBoxDef_t*)menu->items[i]->typeData;
					listPtr->cursorPos = 0;
					listPtr->startPos = 0;
				}
				menu->items[i]->cursorPos = index;
				DC->feederSelection( menu->items[i]->special, menu->items[i]->cursorPos );
				return;
			}
		}
	}
}

qboolean dnMenuSystem::AnyFullScreenVisible() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( menus[i].window.flags & WINDOW_VISIBLE && menus[i].fullScreen ) {
			return qtrue;
		}
	}
	return qfalse;
}

dnMenu *dnMenuSystem::ActivateByName( const char *p ) {
	int i;
	dnMenu *m = NULL;
	dnMenu *focus = GetFocused();
	for ( i = 0; i < menuCount; i++ ) {
		if ( Q_strcasecmp( menus[i].window.name, p ) == 0 ) {
			m = &menus[i];
			m->Activate();
			if ( openMenuCount < MAX_OPEN_MENUS && focus != NULL ) {
				menuStack[openMenuCount++] = focus;
			}
		} else {
			menus[i].window.flags &= ~WINDOW_HASFOCUS;
		}
	}
	CloseCinematics();
	return m;
}


void dnItem::Init( void ) {
	/* keep the vtable: the memset covers the whole item, subclass
	   included, so the pointer is saved and restored */
	void *vtable = *( void ** )this;

	memset( ( void * )this, 0, sizeof( dnItem ) );
	*( void ** )this = vtable;
	textscale = 0.55f;
	Window_Init( &window );
}

void dnMenu::HandleMouseMove( float x, float y ) {
	int i, pass;
	qboolean focusSet = qfalse;

	dnItem *overItem;
	if ( !( window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
		return;
	}

	if ( g_capture.Active() ) {
		return;
	}

	if ( g_bindCapture.Waiting() || g_editCapture.IsActive() ) {
		return;
	}

	// two passes: the first sets mouse-over, the second hands out focus
	for ( pass = 0; pass < 2; pass++ ) {
		for ( i = 0; i < itemCount; i++ ) {

			if ( !( items[i]->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
				continue;
			}

			// a nested def only responds when its container chain is visible
			if ( !items[i]->TreeVisible() ) {
				continue;
			}

			// items can be enabled and disabled based on cvars
			if ( items[i]->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !items[i]->EnableShowViaCvar( CVAR_ENABLE ) ) {
				continue;
			}

			if ( items[i]->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !items[i]->EnableShowViaCvar( CVAR_SHOW ) ) {
				continue;
			}



			if ( Rect_ContainsPoint( &items[i]->window.rect, x, y ) ) {
				if ( pass == 1 ) {
					overItem = items[i];
					if ( overItem->type == ITEM_TYPE_TEXT && overItem->text ) {
						if ( !Rect_ContainsPoint( overItem->CorrectedTextRect(), x, y ) ) {
							continue;
						}
					}
					// if we are over an item
					if ( IsVisible( overItem->window.flags ) ) {
						// different one
						overItem->MouseEnter( x, y );

						// if item is not a decoration see if it can take focus
						if ( !focusSet ) {
							focusSet = overItem->SetFocus( x, y );
						}
					}
				}
			} else if ( items[i]->window.flags & WINDOW_MOUSEOVER ) {
				items[i]->MouseLeave();
				items[i]->SetMouseOver( qfalse );
			}
		}
	}

}

void dnMenu::Paint( qboolean forcePaint ) {
	int i;

	if ( !( window.flags & WINDOW_VISIBLE ) &&  !forcePaint ) {
		return;
	}

	if ( window.ownerDrawFlags && DC->ownerDrawVisible && !DC->ownerDrawVisible( window.ownerDrawFlags ) ) {
		return;
	}

	if ( forcePaint ) {
		window.flags |= WINDOW_FORCED;
	}

	// draw the background if necessary
	if ( fullScreen ) {
		// FIXME: fullscreen menus without a background shader draw nothing
		DC->drawHandlePic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, window.background );
	} else if ( window.background ) {
		// a background without fullscreen is painted by Window_Paint below
	}

	// paint the background and or border
	Window_Paint( &window, fadeAmount, fadeClamp, fadeCycle );

	for ( i = 0; i < itemCount; i++ ) {
		items[i]->Paint();
	}

	if ( debugMode ) {
		vec4_t color;
		color[0] = color[2] = color[3] = 1;
		color[1] = 0;
		DC->drawRect( window.rect.x, window.rect.y, window.rect.w, window.rect.h, 1, color );
	}
}

void dnItem::ValidateTypeData( void ) {
	if ( typeData ) {
		return;
	}

	if ( type == ITEM_TYPE_LISTBOX ) {
		typeData = UI_Alloc( sizeof( listBoxDef_t ) );
		memset( typeData, 0, sizeof( listBoxDef_t ) );
	} else if ( type == ITEM_TYPE_EDITFIELD || type == ITEM_TYPE_NUMERICFIELD || type == ITEM_TYPE_VALIDFILEFIELD || type == ITEM_TYPE_YESNO || type == ITEM_TYPE_BIND || type == ITEM_TYPE_SLIDER || type == ITEM_TYPE_TEXT ) {
		typeData = UI_Alloc( sizeof( editFieldDef_t ) );
		memset( typeData, 0, sizeof( editFieldDef_t ) );
		if ( type == ITEM_TYPE_EDITFIELD || type == ITEM_TYPE_VALIDFILEFIELD ) {
			if ( !( (editFieldDef_t *) typeData )->maxPaintChars ) {
				( (editFieldDef_t *) typeData )->maxPaintChars = MAX_EDITFIELD;
			}
		}
	} else if ( type == ITEM_TYPE_MULTI ) {
		typeData = UI_Alloc( sizeof( multiDef_t ) );
	} else if ( type == ITEM_TYPE_MODEL ) {
		typeData = UI_Alloc( sizeof( modelDef_t ) );
	} else if ( type == ITEM_TYPE_MENUMODEL ) {
		typeData = UI_Alloc( sizeof( modelDef_t ) );
	}
}

int dnMenuSystem::Count() {
	return menuCount;
}

/*
===============
dnMenuSystem::AllocMenu

takes a menu slot and counts it right away; the qcommon .menu parser
builds its menus through it while the menu block parses, and the menu
name can be empty until the name keyword fills it in
===============
*/
dnMenu *dnMenuSystem::AllocMenu( const char *name ) {
	dnMenu *menu;

	if ( menuCount >= MAX_MENUS ) {
		return NULL;
	}

	menu = &menus[menuCount];
	menu->Init();
	menu->window.name = name;
	menuCount++;
	return menu;
}

/*
===============
dnMenu::AddItem

allocate an item of the given type from the UI memory pool and attach
it to a menu; the caller fills in the type specific fields afterwards
===============
*/
dnItem *dnMenu::AddItem( const char *name, int type ) {
	dnItem *item;

	if ( itemCount >= MAX_MENUITEMS ) {
		return NULL;
	}

	item = UI_ConstructItem( type );
	if ( item == NULL ) {
		return NULL;
	}

	item->Init();
	item->type = type;
	item->parent = this;
	if ( name ) {
		item->window.name = name;
	}
	items[itemCount] = item;
	itemCount++;
	return item;
}

void dnMenuSystem::PaintAll() {
	int i;
	g_capture.Update();

	for ( i = 0; i < Count(); i++ ) {
		menus[i].Paint( qfalse );
	}

	if ( debugMode ) {
		vec4_t v = {1, 1, 1, 1};
		DC->drawText( 5, 25, 0, .5, v, va( "fps: %f", DC->FPS ), 0, 0, 0 );
	}
}

void dnMenuSystem::Reset() {
	menuCount = 0;
	openMenuCount = 0;
}

displayContextDef_t *Display_GetContext() {
	return DC;
}


dnMenu *dnMenuSystem::CaptureItem( int x, int y ) {
	int i;

	for ( i = 0; i < menuCount; i++ ) {
		if ( Rect_ContainsPoint( &menus[i].window.rect, x, y ) ) {
			return &menus[i];
		}
	}
	return NULL;
}


qboolean dnMenuSystem::MouseMove( dnMenu *menu, int x, int y ) {
	int i;


	if ( menu == NULL ) {
		menu = GetFocused();
		if ( menu ) {
			if ( menu->window.flags & WINDOW_POPUP ) {
				menu->HandleMouseMove( x, y );
				return qtrue;
			}
		}
		for ( i = 0; i < menuCount; i++ ) {
			menus[i].HandleMouseMove( x, y );
		}
	} else {
		menu->window.rect.x += x;
		menu->window.rect.y += y;
		menu->UpdatePosition();
	}
	return qtrue;

}

int dnMenuSystem::CursorType( int x, int y ) {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		rectDef_t r2;
		r2.x = menus[i].window.rect.x - 3;
		r2.y = menus[i].window.rect.y - 3;
		r2.w = r2.h = 7;
		if ( Rect_ContainsPoint( &r2, x, y ) ) {
			return CURSOR_SIZER;
		}
	}
	return CURSOR_ARROW;
}


void dnMenuSystem::HandleKey( int key, qboolean down, int x, int y ) {
	/* every fullscreen menu covers the whole screen rect, so a point
	   capture cannot tell them apart: the focused menu owns the key,
	   and the capture only serves menus without focus */
	dnMenu *menu = GetFocused();
	if ( menu == NULL ) {
		menu = CaptureItem( x, y );
	}
	if ( menu ) {
		menu->HandleKey( key, down );
	}
}

static void Window_CacheContents( dnWindow *window ) {
	if ( window ) {
		if ( window->cinematicName ) {
			int cin = DC->playCinematic( window->cinematicName, 0, 0, 0, 0 );
			DC->stopCinematic( cin );
		}
	}
}


void dnItem::CacheContents( void ) {

	Window_CacheContents( &window );

}

void dnMenu::CacheContents( void ) {

	int i;
	Window_CacheContents( &window );
	for ( i = 0; i < itemCount; i++ ) {
		items[i]->CacheContents();
	}

	if ( soundName && *soundName ) {
		DC->registerSound( soundName );
	}

}

void dnMenuSystem::CacheAll() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		menus[i].CacheContents();
	}
}


qboolean dnMenu::OverActiveItem( float x, float y ) {
	if ( window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) {
		if ( Rect_ContainsPoint( &window.rect, x, y ) ) {
			int i;
			for ( i = 0; i < itemCount; i++ ) {

				if ( !( items[i]->window.flags & ( WINDOW_VISIBLE | WINDOW_FORCED ) ) ) {
					continue;
				}

				// a nested def only counts as active with its container chain visible
				if ( !items[i]->TreeVisible() ) {
					continue;
				}

				if ( items[i]->window.flags & WINDOW_DECORATION ) {
					continue;
				}

				if ( Rect_ContainsPoint( &items[i]->window.rect, x, y ) ) {
					dnItem *overItem = items[i];
					if ( overItem->type == ITEM_TYPE_TEXT && overItem->text ) {
						if ( Rect_ContainsPoint( overItem->CorrectedTextRect(), x, y ) ) {
							return qtrue;
						} else {
							continue;
						}
					} else {
						return qtrue;
					}
				}
			}

		}
	}
	return qfalse;
}


