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

//
// string allocation/managment

#include "ui_local.h"

#define SCROLL_TIME_START                   500
#define SCROLL_TIME_ADJUST              150
#define SCROLL_TIME_ADJUSTOFFSET    40
#define SCROLL_TIME_FLOOR                   20

typedef struct scrollInfo_s {
	int nextScrollTime;
	int nextAdjustTime;
	int adjustValue;
	int scrollKey;
	float xStart;
	float yStart;
	itemDef_t *item;
	qboolean scrollDir;
} scrollInfo_t;

static scrollInfo_t scrollInfo;

static void ( *captureFunc )( void *p ) = NULL;
static void *captureData = NULL;
static itemDef_t *itemCapture = NULL;   // item that has the mouse captured ( if any )

displayContextDef_t *DC = NULL;

qboolean g_waitingForKey = qfalse;
qboolean g_editingField = qfalse;

static itemDef_t *g_bindItem = NULL;
itemDef_t *g_editItem = NULL;

menuDef_t Menus[MAX_MENUS];      // defined menus
int menuCount = 0;               // how many

menuDef_t *menuStack[MAX_OPEN_MENUS];
int openMenuCount = 0;

static qboolean debugMode = qfalse;

#define DOUBLE_CLICK_DELAY 300
static int lastListBoxClickTime = 0;

void Item_RunScript( itemDef_t *item, const char *s );
void Item_SetupKeywordHash( void );
void Menu_SetupKeywordHash( void );
int BindingIDFromName( const char *name );
qboolean Item_Bind_HandleKey( itemDef_t *item, int key, qboolean down );
itemDef_t *Menu_SetPrevCursorItem( menuDef_t *menu );
itemDef_t *Menu_SetNextCursorItem( menuDef_t *menu );
static qboolean Menu_OverActiveItem( menuDef_t *menu, float x, float y );

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

	{"yes"},                 //
	{"no"},                      //
	{"keywait"},             //	"Waiting for new key... Press ESCAPE to cancel"
	{"keychange"},               //	"Press ENTER or CLICK to change, Press BACKSPACE to clear"
	{"pleasewait"},              //	"Please Wait..."
	{"dynamitetimer"},           //	"Dynamite timer set at"
	{"second"},                  //
	{"seconds"},             //
	{"minute"},                  //
	{"minutes"},             //
	{"hour"},                    //
	{"hours"},                   //
	{"day"},                 //
	{"days"},                    //
	{"month"},                   //
	{"months"},                  //
	{"year"},                    //
	{"years"},                   //
	{"or"}                       //
};

//----(SA)	added
/*
==============
UI_RoQDone
==============
*/
void UI_RoQDone( void ) {
	menuDef_t *menu = Menu_GetFocused();
	if ( menu->onROQDone ) {
		itemDef_t it;
		it.parent = menu;
		Item_RunScript( &it, menu->onROQDone );

	}
}
//----(SA)	end


/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size ) {
	char    *p;

	if ( allocPoint + size > MEM_POOL_SIZE ) {
		outOfMemory = qtrue;
		if ( DC->Print ) {
			DC->Print( "UI_Alloc: Failure. Out of memory!\n" );
		}
		//DC->UI_Printf(S_COLOR_YELLOW"WARNING: UI Out of Memory!\n");
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 15 ) & ~15;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
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


/*
=================
String_Init
=================
*/
void String_Init() {
	int i;
	for ( i = 0; i < HASH_TABLE_SIZE; i++ ) {
		strHandle[i] = 0;
	}
	strHandleCount = 0;
	strPoolIndex = 0;
	menuCount = 0;
	openMenuCount = 0;
	UI_InitMemory();
	Item_SetupKeywordHash();
	Menu_SetupKeywordHash();
	if ( DC && DC->getBindingBuf ) {
		Controls_GetConfig();
	}
}

/*
=================
PC_SourceWarning
=================
*/
void PC_SourceWarning( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	UI_PC_SourceFileAndLine( handle, filename, &line );

	UI_Printf( S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string );
}

/*
=================
PC_SourceError
=================
*/
void PC_SourceError( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	vsprintf( string, format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	UI_PC_SourceFileAndLine( handle, filename, &line );

	UI_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );
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

/*
=================
Float_Parse
=================
*/
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

/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse( int handle, float *f ) {
	pc_token_t token;
	int negative = qfalse;

	if ( !UI_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] == '-' ) {
		if ( !UI_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}
		negative = qtrue;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected float but found %s\n", token.string );
		return qfalse;
	}
	if ( negative ) {
		*f = -token.floatvalue;
	} else {
		*f = token.floatvalue;
	}
	return qtrue;
}

/*
=================
Color_Parse
=================
*/
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

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse( int handle, vec4_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return qfalse;
		}
		( *c )[i] = f;
	}
	return qtrue;
}

/*
=================
Int_Parse
=================
*/
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

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse( int handle, int *i ) {
	pc_token_t token;
	int negative = qfalse;

	if ( !UI_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] == '-' ) {
		if ( !UI_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}
		negative = qtrue;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected integer but found %s\n", token.string );
		return qfalse;
	}
	*i = token.intvalue;
	if ( negative ) {
		*i = -*i;
	}
	return qtrue;
}

/*
=================
Rect_Parse
=================
*/
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

/*
=================
PC_Rect_Parse
=================
*/
qboolean PC_Rect_Parse( int handle, rectDef_t *r ) {
	if ( PC_Float_Parse( handle, &r->x ) ) {
		if ( PC_Float_Parse( handle, &r->y ) ) {
			if ( PC_Float_Parse( handle, &r->w ) ) {
				if ( PC_Float_Parse( handle, &r->h ) ) {
					return qtrue;
				}
			}
		}
	}
	return qfalse;
}

/*
=================
String_Parse
=================
*/
qboolean String_Parse( char **p, const char **out ) {
	char *token;

	token = COM_ParseExt( p, qfalse );
	if ( token && token[0] != 0 ) {
		*( out ) = String_Alloc( token );
		return qtrue;
	}
	return qfalse;
}

/*
=================
PC_String_Parse
=================
*/
qboolean PC_String_Parse( int handle, const char **out ) {
	pc_token_t token;

	if ( !UI_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}

	*( out ) = String_Alloc( token.string );
	return qtrue;
}

// NERVE - SMF
/*
=================
PC_Char_Parse
=================
*/
qboolean PC_Char_Parse( int handle, char *out ) {
	pc_token_t token;

	if ( !UI_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}

	*( out ) = token.string[0];
	return qtrue;
}
// -NERVE - SMF

/*
=================
PC_Script_Parse
=================
*/
qboolean PC_Script_Parse( int handle, const char **out ) {
	char script[1024];
	pc_token_t token;

	memset( script, 0, sizeof( script ) );
	// scripts start with { and have ; separated command lists.. commands are command, arg..
	// basically we want everything between the { } as it will be interpreted at run time

	if ( !UI_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( Q_strcasecmp( token.string, "{" ) != 0 ) {
		return qfalse;
	}

	while ( 1 ) {
		if ( !UI_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}

		if ( Q_strcasecmp( token.string, "}" ) == 0 ) {
			*out = String_Alloc( script );
			return qtrue;
		}

		if ( token.string[1] != '\0' ) {
			Q_strcat( script, 1024, va( "\"%s\"", token.string ) );
		} else {
			Q_strcat( script, 1024, token.string );
		}
		Q_strcat( script, 1024, " " );
	}
	//return qfalse;
}

// display, window, menu, item code
//

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

Initializes a window structure ( windowDef_t ) with defaults

==================
*/
void Window_Init( Window *w ) {
	memset( w, 0, sizeof( windowDef_t ) );
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
	//float bordersize = 0;
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
		// gradient bar
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
		// full
		// HACK HACK HACK
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


void Item_SetScreenCoords( itemDef_t *item, float x, float y ) {

	if ( item == NULL ) {
		return;
	}

	if ( item->window.border != 0 ) {
		x += item->window.borderSize;
		y += item->window.borderSize;
	}

	item->window.rect.x = x + item->window.rectClient.x;
	item->window.rect.y = y + item->window.rectClient.y;
	item->window.rect.w = item->window.rectClient.w;
	item->window.rect.h = item->window.rectClient.h;

	// force the text rects to recompute
	item->textRect.w = 0;
	item->textRect.h = 0;
}

// FIXME: consolidate this with nearby stuff
void Item_UpdatePosition( itemDef_t *item ) {
	float x, y;
	menuDef_t *menu;

	if ( item == NULL || item->parent == NULL ) {
		return;
	}

	menu = (menuDef_t *)item->parent;

	x = menu->window.rect.x;
	y = menu->window.rect.y;

	if ( menu->window.border != 0 ) {
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	Item_SetScreenCoords( item, x, y );

}

// menus
void Menu_UpdatePosition( menuDef_t *menu ) {
	int i;
	float x, y;

	if ( menu == NULL ) {
		return;
	}

	x = menu->window.rect.x;
	y = menu->window.rect.y;
	if ( menu->window.border != 0 ) {
		x += menu->window.borderSize;
		y += menu->window.borderSize;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		Item_SetScreenCoords( menu->items[i], x, y );
	}
}

void Menu_PostParse( menuDef_t *menu ) {
	if ( menu == NULL ) {
		return;
	}
	if ( menu->fullScreen ) {
		menu->window.rect.x = 0;
		menu->window.rect.y = 0;
		menu->window.rect.w = 640;
		menu->window.rect.h = 480;
	}
	Menu_UpdatePosition( menu );
}

itemDef_t *Menu_ClearFocus( menuDef_t *menu ) {
	int i;
	itemDef_t *ret = NULL;

	if ( menu == NULL ) {
		return NULL;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( menu->items[i]->window.flags & WINDOW_HASFOCUS ) {
			ret = menu->items[i];
		}
		menu->items[i]->window.flags &= ~WINDOW_HASFOCUS;
		if ( menu->items[i]->leaveFocus ) {
			Item_RunScript( menu->items[i], menu->items[i]->leaveFocus );
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

int Menu_ItemsMatchingGroup( menuDef_t *menu, const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare


	pdest = ( char * )strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( menu->items[i]->window.name && Q_strncmp( menu->items[i]->window.name, name, wildcard ) == 0 || ( menu->items[i]->window.group && Q_strncmp( menu->items[i]->window.group, name, wildcard ) == 0 ) ) {
				count++;
			}
		} else {
			if ( menu->items[i]->window.name && Q_strcasecmp( menu->items[i]->window.name, name ) == 0 || ( menu->items[i]->window.group && Q_strcasecmp( menu->items[i]->window.group, name ) == 0 ) ) {
				count++;
			}
		}
	}

	return count;
}

itemDef_t *Menu_GetMatchingItemByNumber( menuDef_t *menu, int index, const char *name ) {
	int i;
	int count = 0;
	char *pdest;
	int wildcard = -1;  // if wildcard is set, it's value is the number of characters to compare

	pdest = ( char * )strstr( name, "*" ); // allow wildcard strings (ex.  "hide nb_*" would translate to "hide nb_pg1; hide nb_extra" etc)
	if ( pdest ) {
		wildcard = pdest - name;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( wildcard != -1 ) {
			if ( menu->items[i]->window.name && Q_strncmp( menu->items[i]->window.name, name, wildcard ) == 0 || ( menu->items[i]->window.group && Q_strncmp( menu->items[i]->window.group, name, wildcard ) == 0 ) ) {
				if ( count == index ) {
					return menu->items[i];
				}
				count++;
			}
		} else {
			if ( menu->items[i]->window.name && Q_strcasecmp( menu->items[i]->window.name, name ) == 0 || ( menu->items[i]->window.group && Q_strcasecmp( menu->items[i]->window.group, name ) == 0 ) ) {
				if ( count == index ) {
					return menu->items[i];
				}
				count++;
			}
		}
	}
	return NULL;
}



void Script_SetColor( itemDef_t *item, char **args ) {
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

void Script_SetAsset( itemDef_t *item, char **args ) {
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		// check for a model
		if ( item->type == ITEM_TYPE_MODEL ) {
		}
	}
}

void Script_SetBackground( itemDef_t *item, char **args ) {
	const char *name;
	// expecting name to set asset to
	if ( String_Parse( args, &name ) ) {
		item->window.background = DC->registerShaderNoMip( name );
	}
}




itemDef_t *Menu_FindItemByName( menuDef_t *menu, const char *p ) {
	int i;

	if ( menu == NULL || p == NULL ) {
		return NULL;
	}

	for ( i = 0; i < menu->itemCount; i++ ) {
		/* code-built Duke menus attach decoration items that carry no name */
		if ( menu->items[i]->window.name && Q_strcasecmp( p, menu->items[i]->window.name ) == 0 ) {
			return menu->items[i];
		}
	}

	return NULL;
}

void Script_SetTeamColor( itemDef_t *item, char **args ) {
	if ( DC->getTeamColor ) {
		int i;
		vec4_t color;
		DC->getTeamColor( &color );
		for ( i = 0; i < 4; i++ ) {
			item->window.backColor[i] = color[i];
		}
	}
}

void Script_SetItemColor( itemDef_t *item, char **args ) {
	const char *itemname;
	const char *name;
	vec4_t color;
	int i;
	vec4_t *out;
	// expecting type of color to set and 4 args for the color
	if ( String_Parse( args, &itemname ) && String_Parse( args, &name ) ) {
		itemDef_t *item2;
		int j;
		int count = Menu_ItemsMatchingGroup( (menuDef_t *)item->parent, itemname );

		if ( !Color_Parse( args, &color ) ) {
			return;
		}

		for ( j = 0; j < count; j++ ) {
			item2 = Menu_GetMatchingItemByNumber( (menuDef_t *)item->parent, j, itemname );
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


void Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
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

void Menu_FadeItemByName( menuDef_t *menu, const char *p, qboolean fadeOut ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
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

menuDef_t *Menus_FindByName( const char *p ) {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		if ( Q_strcasecmp( Menus[i].window.name, p ) == 0 ) {
			return &Menus[i];
		}
	}
	return NULL;
}

void Menus_ShowByName( const char *p ) {
	menuDef_t *menu = Menus_FindByName( p );
	if ( menu ) {
		Menus_Activate( menu );
	}
}

void Menus_OpenByName( const char *p ) {
	Menus_ActivateByName( p );
}

static void Menu_RunCloseScript( menuDef_t *menu ) {
	if ( menu && menu->window.flags & WINDOW_VISIBLE && menu->onClose ) {
		itemDef_t item;
		item.parent = menu;
		Item_RunScript( &item, menu->onClose );
	}
}

void Menus_CloseByName( const char *p ) {
	menuDef_t *menu = Menus_FindByName( p );
	if ( menu != NULL ) {
		Menu_RunCloseScript( menu );
		menu->window.flags &= ~( WINDOW_VISIBLE | WINDOW_HASFOCUS );
	}
}

void Menus_CloseAll() {
	int i;
	for ( i = 0; i < menuCount; i++ ) {
		Menu_RunCloseScript( &Menus[i] );
		Menus[i].window.flags &= ~( WINDOW_HASFOCUS | WINDOW_VISIBLE );
	}
}


void Script_Show( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_ShowItemByName( (menuDef_t *)item->parent, name, qtrue );
	}
}

void Script_Hide( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_ShowItemByName( (menuDef_t *)item->parent, name, qfalse );
	}
}

void Script_FadeIn( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_FadeItemByName( (menuDef_t *)item->parent, name, qfalse );
	}
}

void Script_FadeOut( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menu_FadeItemByName( (menuDef_t *)item->parent, name, qtrue );
	}
}



void Script_Open( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menus_OpenByName( name );
	}
}

void Script_Close( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		Menus_CloseByName( name );
	}
}




/*
==============
Script_Clipboard
==============
*/
void Script_Clipboard( itemDef_t *item, char **args ) {
	char curscript[64];
	DC->getCVarString( "cg_clipboardName", curscript, sizeof( curscript ) ); // grab the string the client set
	Menu_ShowItemByName( (menuDef_t *)item->parent, curscript, qtrue );
}




#define NOTEBOOK_MAX_PAGES 6    // this will not be a define


/*
==============
Script_NotebookShowpage
	hide all notebook pages and show just the active one

	inc == 0	- show current page
	inc == val	- turn inc pages in the notebook (negative numbers are backwards)
	inc == 999	- key number.  +999 is jump to last page, -999 is jump to cover page
==============
*/
void Script_NotebookShowpage( itemDef_t *item, char **args ) {
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
//			newpage = NOTEBOOK_MAX_PAGES;	// = lastpage;
			curpage = 0;
			inc = -1;
		} else if ( inc == -999 ) {    // jump to start
			curpage = 0;
			inc = 0;
		} else if ( inc > 500 ) {
//			curpage = DEBRIEFING_BASE + (inc - 500);
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
//						if(dec == inc)
//							break;
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
//		Menu_ShowItemByName(item->parent, "page_*", qfalse);
		Menu_ShowItemByName( (menuDef_t *)item->parent, "cover", qfalse );
		for ( i = 1; i <= NOTEBOOK_MAX_PAGES; i++ ) {
			Menu_ShowItemByName( (menuDef_t *)item->parent, va( "page%d", i ), qfalse );
		}

		// show the visible one

		if ( curpage ) {
			Menu_ShowItemByName( (menuDef_t *)item->parent, va( "page%d", curpage ), qtrue );
		} else {
			Menu_ShowItemByName( (menuDef_t *)item->parent, "cover", qtrue );
		}

		DC->setCVar( "ui_notebookCurrentPage", va( "%d", curpage ) ); // store new current page

	}
}



void Menu_TransitionItemByName( menuDef_t *menu, const char *p, rectDef_t rectFrom, rectDef_t rectTo, int time, float amt ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_INTRANSITION | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			memcpy( &item->window.rectClient, &rectFrom, sizeof( rectDef_t ) );
			memcpy( &item->window.rectEffects, &rectTo, sizeof( rectDef_t ) );
			item->window.rectEffects2.x = fabs( rectTo.x - rectFrom.x ) / amt;
			item->window.rectEffects2.y = fabs( rectTo.y - rectFrom.y ) / amt;
			item->window.rectEffects2.w = fabs( rectTo.w - rectFrom.w ) / amt;
			item->window.rectEffects2.h = fabs( rectTo.h - rectFrom.h ) / amt;
			Item_UpdatePosition( item );
		}
	}
}


void Script_Transition( itemDef_t *item, char **args ) {
	const char *name;
	rectDef_t rectFrom, rectTo;
	int time;
	float amt;

	if ( String_Parse( args, &name ) ) {
		if ( Rect_Parse( args, &rectFrom ) && Rect_Parse( args, &rectTo ) && Int_Parse( args, &time ) && Float_Parse( args, &amt ) ) {
			Menu_TransitionItemByName( (menuDef_t *)item->parent, name, rectFrom, rectTo, time, amt );
		}
	}
}


void Menu_OrbitItemByName( menuDef_t *menu, const char *p, float x, float y, float cx, float cy, int time ) {
	itemDef_t *item;
	int i;
	int count = Menu_ItemsMatchingGroup( menu, p );
	for ( i = 0; i < count; i++ ) {
		item = Menu_GetMatchingItemByNumber( menu, i, p );
		if ( item != NULL ) {
			item->window.flags |= ( WINDOW_ORBITING | WINDOW_VISIBLE );
			item->window.offsetTime = time;
			item->window.rectEffects.x = cx;
			item->window.rectEffects.y = cy;
			item->window.rectClient.x = x;
			item->window.rectClient.y = y;
			Item_UpdatePosition( item );
		}
	}
}


void Script_Orbit( itemDef_t *item, char **args ) {
	const char *name;
	float cx, cy, x, y;
	int time;

	if ( String_Parse( args, &name ) ) {
		if ( Float_Parse( args, &x ) && Float_Parse( args, &y ) && Float_Parse( args, &cx ) && Float_Parse( args, &cy ) && Int_Parse( args, &time ) ) {
			Menu_OrbitItemByName( (menuDef_t *)item->parent, name, x, y, cx, cy, time );
		}
	}
}



void Script_SetFocus( itemDef_t *item, char **args ) {
	const char *name;
	itemDef_t *focusItem;

	if ( String_Parse( args, &name ) ) {
		focusItem = Menu_FindItemByName( (menuDef_t *)item->parent, name );
		if ( focusItem && !( focusItem->window.flags & WINDOW_DECORATION ) && !( focusItem->window.flags & WINDOW_HASFOCUS ) ) {
			Menu_ClearFocus( (menuDef_t *)item->parent );
			focusItem->window.flags |= WINDOW_HASFOCUS;
			if ( focusItem->onFocus ) {
				Item_RunScript( focusItem, focusItem->onFocus );
			}
			if ( DC->Assets.itemFocusSound ) {
				DC->startLocalSound( DC->Assets.itemFocusSound, CHAN_LOCAL_SOUND );
			}
		}
	}
}

void Script_SetPlayerModel( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_model", name );
	}
}

void Script_SetPlayerHead( itemDef_t *item, char **args ) {
	const char *name;
	if ( String_Parse( args, &name ) ) {
		DC->setCVar( "team_headmodel", name );
	}
}

void Script_SetCvar( itemDef_t *item, char **args ) {
	const char *cvar, *val;
	if ( String_Parse( args, &cvar ) && String_Parse( args, &val ) ) {
		DC->setCVar( cvar, val );
	}

}

void Script_Exec( itemDef_t *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->executeText( DN_EXEC_APPEND, va( "%s ; ", val ) );
	}
}

void Script_Play( itemDef_t *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		DC->startLocalSound( DC->registerSound( val ), CHAN_LOCAL_SOUND );      // all sounds are not 3d
	}
}

void Script_playLooped( itemDef_t *item, char **args ) {
	const char *val;
	if ( String_Parse( args, &val ) ) {
		// (SA) don't think this should happen...
//		DC->stopBackgroundTrack();
		DC->startBackgroundTrack( val, val, 0 );
	}
}

// NERVE - SMF
void Script_AddListItem( itemDef_t *item, char **args ) {
	const char *itemname, *val, *name;
	itemDef_t *t;

	if ( String_Parse( args, &itemname ) && String_Parse( args, &val ) && String_Parse( args, &name ) ) {
		t = Menu_FindItemByName( (menuDef_t *)item->parent, itemname );
		if ( t && t->special ) {
			DC->feederAddItem( t->special, name, atoi( val ) );
		}
	}
}
// -NERVE - SMF

commandDef_t commandList[] =
{
	{"fadein", &Script_FadeIn},                  // group/name
	{"fadeout", &Script_FadeOut},                // group/name
	{"show", &Script_Show},                      // group/name
	{"hide", &Script_Hide},                      // group/name
	{"setcolor", &Script_SetColor},              // works on this
	{"open", &Script_Open},                      // menu
	{"close", &Script_Close},                    // menu
	{"clipboard", &Script_Clipboard},            // show the current clipboard group by name
	{"showpage", &Script_NotebookShowpage},          //
	{"setasset", &Script_SetAsset},              // works on this
	{"setbackground", &Script_SetBackground},    // works on this
	{"setitemcolor", &Script_SetItemColor},      // group/name
	{"setteamcolor", &Script_SetTeamColor},      // sets this background color to team color
	{"setfocus", &Script_SetFocus},              // sets this background color to team color
	{"setplayermodel", &Script_SetPlayerModel},  // sets this background color to team color
	{"setplayerhead", &Script_SetPlayerHead},    // sets this background color to team color
	{"transition", &Script_Transition},          // group/name
	{"setcvar", &Script_SetCvar},                // group/name
	{"exec", &Script_Exec},                      // group/name
	{"play", &Script_Play},                      // group/name
	{"playlooped", &Script_playLooped},          // group/name
	{"orbit", &Script_Orbit},                    // group/name
	{"addlistitem", &Script_AddListItem}     // NERVE - SMF - special command to add text items to list box
};

int scriptCommandCount = sizeof( commandList ) / sizeof( commandDef_t );


void Item_RunScript( itemDef_t *item, const char *s ) {
	char script[1024], *p;
	int i;
	qboolean bRan;
	memset( script, 0, sizeof( script ) );
	if ( item && s && s[0] ) {
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
					( commandList[i].handler( item, &p ) );
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


qboolean Item_EnableShowViaCvar( itemDef_t *item, int flag ) {
	char script[1024], *p;
	memset( script, 0, sizeof( script ) );
	if ( item && item->enableCvar && *item->enableCvar && item->cvarTest && *item->cvarTest ) {
		char buff[1024];
		DC->getCVarString( item->cvarTest, buff, sizeof( buff ) );

		Q_strcat( script, 1024, item->enableCvar );
		p = script;
		while ( 1 ) {
			const char *val;
			// expect value then ; or NULL, NULL ends list
			if ( !String_Parse( &p, &val ) ) {
				return ( item->cvarFlags & flag ) ? qfalse : qtrue;
			}

			if ( val[0] == ';' && val[1] == '\0' ) {
				continue;
			}

			// enable it if any of the values are true
			if ( item->cvarFlags & flag ) {
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
		return ( item->cvarFlags & flag ) ? qfalse : qtrue;
	}
	return qtrue;
}


// will optionaly set focus to this item
qboolean Item_SetFocus( itemDef_t *item, float x, float y ) {
	int i;
	itemDef_t *oldFocus;
	sfxHandle_t *sfx = &DC->Assets.itemFocusSound;
	qboolean playSound = qfalse;
	menuDef_t *parent = NULL;
	// sanity check, non-null, not a decoration and does not already have the focus
	if ( item == NULL || item->window.flags & WINDOW_DECORATION || item->window.flags & WINDOW_HASFOCUS || !( item->window.flags & WINDOW_VISIBLE ) ) {
		return qfalse;
	}

	parent = (menuDef_t *)item->parent;

	// items can be enabled and disabled based on cvars
	if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
		return qfalse;
	}

	if ( item->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !Item_EnableShowViaCvar( item, CVAR_SHOW ) ) {
		return qfalse;
	}

	oldFocus = Menu_ClearFocus( (menuDef_t *)item->parent );

	if ( item->type == ITEM_TYPE_TEXT ) {
		rectDef_t r;
		r = item->textRect;
		r.y -= r.h;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			item->window.flags |= WINDOW_HASFOCUS;
			if ( item->focusSound ) {
				sfx = &item->focusSound;
			}
			playSound = qtrue;
		} else {
			if ( oldFocus ) {
				oldFocus->window.flags |= WINDOW_HASFOCUS;
				if ( oldFocus->onFocus ) {
					Item_RunScript( oldFocus, oldFocus->onFocus );
				}
			}
		}
	} else {
		item->window.flags |= WINDOW_HASFOCUS;
		if ( item->onFocus ) {
			Item_RunScript( item, item->onFocus );
		}
		if ( item->focusSound ) {
			sfx = &item->focusSound;
		}
		playSound = qtrue;
	}

	if ( playSound && sfx ) {
		DC->startLocalSound( *sfx, CHAN_LOCAL_SOUND );
	}

	for ( i = 0; i < parent->itemCount; i++ ) {
		if ( parent->items[i] == item ) {
			parent->cursorItem = i;
			break;
		}
	}

	return qtrue;
}

int Item_ListBox_MaxScroll( itemDef_t *item ) {
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount( item->special );
	int max;

	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		max = count - ( item->window.rect.w / listPtr->elementWidth ) + 1;
	} else {
		max = count - ( item->window.rect.h / listPtr->elementHeight ) + 1;
	}
	if ( max < 0 ) {
		return 0;
	}
	return max;
}

int Item_ListBox_ThumbPosition( itemDef_t *item ) {
	float max, pos, size;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	max = Item_ListBox_MaxScroll( item );
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		size = item->window.rect.w - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.x + 1 + SCROLLBAR_SIZE + pos;
	} else {
		size = item->window.rect.h - ( SCROLLBAR_SIZE * 2 ) - 2;
		if ( max > 0 ) {
			pos = ( size - SCROLLBAR_SIZE ) / (float) max;
		} else {
			pos = 0;
		}
		pos *= listPtr->startPos;
		return item->window.rect.y + 1 + SCROLLBAR_SIZE + pos;
	}
}

int Item_ListBox_ThumbDrawPosition( itemDef_t *item ) {
	int min, max;

	if ( itemCapture == item ) {
		if ( item->window.flags & WINDOW_HORIZONTAL ) {
			min = item->window.rect.x + SCROLLBAR_SIZE + 1;
			max = item->window.rect.x + item->window.rect.w - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursorx >= min + SCROLLBAR_SIZE / 2 && DC->cursorx <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursorx - SCROLLBAR_SIZE / 2;
			} else {
				return Item_ListBox_ThumbPosition( item );
			}
		} else {
			min = item->window.rect.y + SCROLLBAR_SIZE + 1;
			max = item->window.rect.y + item->window.rect.h - 2 * SCROLLBAR_SIZE - 1;
			if ( DC->cursory >= min + SCROLLBAR_SIZE / 2 && DC->cursory <= max + SCROLLBAR_SIZE / 2 ) {
				return DC->cursory - SCROLLBAR_SIZE / 2;
			} else {
				return Item_ListBox_ThumbPosition( item );
			}
		}
	} else {
		return Item_ListBox_ThumbPosition( item );
	}
}

float Item_Slider_ThumbPosition( itemDef_t *item ) {
	float value, range, x;
	editFieldDef_t *editDef = ( editFieldDef_t * )item->typeData;

	if ( item->text ) {
		x = item->textRect.x + item->textRect.w + 8;
	} else {
		x = item->window.rect.x;
	}

	if ( editDef == NULL && item->cvar ) {
		return x;
	}

	value = DC->getCVarValue( item->cvar );

	if ( value < editDef->minVal ) {
		value = editDef->minVal;
	} else if ( value > editDef->maxVal ) {
		value = editDef->maxVal;
	}

	range = editDef->maxVal - editDef->minVal;
	value -= editDef->minVal;
	value /= range;
	//value /= (editDef->maxVal - editDef->minVal);
	value *= SLIDER_WIDTH;
	x += value;
	// vm fuckage
	//x = x + (((float)value / editDef->maxVal) * SLIDER_WIDTH);
	return x;
}

int Item_Slider_OverSlider( itemDef_t *item, float x, float y ) {
	rectDef_t r;

	r.x = Item_Slider_ThumbPosition( item ) - ( SLIDER_THUMB_WIDTH / 2 );
	r.y = item->window.rect.y - 2;
	r.w = SLIDER_THUMB_WIDTH;
	r.h = SLIDER_THUMB_HEIGHT;

	if ( Rect_ContainsPoint( &r, x, y ) ) {
		return WINDOW_LB_THUMB;
	}
	return 0;
}

int Item_ListBox_OverLB( itemDef_t *item, float x, float y ) {
	rectDef_t r;
	listBoxDef_t *listPtr;
	int thumbstart;
	int count;

	count = DC->feederCount( item->special );
	listPtr = (listBoxDef_t*)item->typeData;
	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		// check if on left arrow
		r.x = item->window.rect.x;
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		// check if on right arrow
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		// check if on thumb
		thumbstart = Item_ListBox_ThumbPosition( item );
		r.x = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.x = item->window.rect.x + SCROLLBAR_SIZE;
		r.w = thumbstart - r.x;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.x = thumbstart + SCROLLBAR_SIZE;
		r.w = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	} else {
		r.x = item->window.rect.x + item->window.rect.w - SCROLLBAR_SIZE;
		r.y = item->window.rect.y;
		r.h = r.w = SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_LEFTARROW;
		}
		r.y = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_RIGHTARROW;
		}
		thumbstart = Item_ListBox_ThumbPosition( item );
		r.y = thumbstart;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_THUMB;
		}
		r.y = item->window.rect.y + SCROLLBAR_SIZE;
		r.h = thumbstart - r.y;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGUP;
		}
		r.y = thumbstart + SCROLLBAR_SIZE;
		r.h = item->window.rect.y + item->window.rect.h - SCROLLBAR_SIZE;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			return WINDOW_LB_PGDN;
		}
	}
	return 0;
}


void Item_ListBox_MouseEnter( itemDef_t *item, float x, float y ) {
	rectDef_t r;
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;

	item->window.flags &= ~( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN );
	item->window.flags |= Item_ListBox_OverLB( item, x, y );

	if ( item->window.flags & WINDOW_HORIZONTAL ) {
		if ( !( item->window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
			// check for selection hit as we have exausted buttons and thumb
			if ( listPtr->elementStyle == LISTBOX_IMAGE ) {
				r.x = item->window.rect.x;
				r.y = item->window.rect.y;
				r.h = item->window.rect.h - SCROLLBAR_SIZE;
				r.w = item->window.rect.w - listPtr->drawPadding;
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
	} else if ( !( item->window.flags & ( WINDOW_LB_LEFTARROW | WINDOW_LB_RIGHTARROW | WINDOW_LB_THUMB | WINDOW_LB_PGUP | WINDOW_LB_PGDN ) ) ) {
		r.x = item->window.rect.x;
		r.y = item->window.rect.y;
		r.w = item->window.rect.w - SCROLLBAR_SIZE;
		r.h = item->window.rect.h - listPtr->drawPadding;
		if ( Rect_ContainsPoint( &r, x, y ) ) {
			listPtr->cursorPos =  (int)( ( y - 2 - r.y ) / listPtr->elementHeight )  + listPtr->startPos;
			if ( listPtr->cursorPos > listPtr->endPos ) {
				listPtr->cursorPos = listPtr->endPos;
			}
		}
	}
}

void Item_MouseEnter( itemDef_t *item, float x, float y ) {
	rectDef_t r;
	if ( item ) {
		r = item->textRect;
		r.y -= r.h;
		// in the text rect?

		// items can be enabled and disabled based on cvars
		if ( item->cvarFlags & ( CVAR_ENABLE | CVAR_DISABLE ) && !Item_EnableShowViaCvar( item, CVAR_ENABLE ) ) {
			return;
		}

		if ( item->cvarFlags & ( CVAR_SHOW | CVAR_HIDE ) && !Item_EnableShowViaCvar( item, CVAR_SHOW ) ) {
			return;
		}

		if ( Rect_ContainsPoint( &r, x, y ) ) {
			if ( !( item->window.flags & WINDOW_MOUSEOVERTEXT ) ) {
				Item_RunScript( item, item->mouseEnterText );
				item->window.flags |= WINDOW_MOUSEOVERTEXT;
			}
			if ( !( item->window.flags & WINDOW_MOUSEOVER ) ) {
				Item_RunScript( item, item->mouseEnter );
				item->window.flags |= WINDOW_MOUSEOVER;
			}

		} else {
			// not in the text rect
			if ( item->window.flags & WINDOW_MOUSEOVERTEXT ) {
				// if we were
				Item_RunScript( item, item->mouseExitText );
				item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
			}
			if ( !( item->window.flags & WINDOW_MOUSEOVER ) ) {
				Item_RunScript( item, item->mouseEnter );
				item->window.flags |= WINDOW_MOUSEOVER;
			}

			if ( item->type == ITEM_TYPE_LISTBOX ) {
				Item_ListBox_MouseEnter( item, x, y );
			}
		}
	}
}

void Item_MouseLeave( itemDef_t *item ) {
	if ( item ) {
		if ( item->window.flags & WINDOW_MOUSEOVERTEXT ) {
			Item_RunScript( item, item->mouseExitText );
			item->window.flags &= ~WINDOW_MOUSEOVERTEXT;
		}
		Item_RunScript( item, item->mouseExit );
		item->window.flags &= ~( WINDOW_LB_RIGHTARROW | WINDOW_LB_LEFTARROW );
	}
}

itemDef_t *Menu_HitTest( menuDef_t *menu, float x, float y ) {
	int i;
	for ( i = 0; i < menu->itemCount; i++ ) {
		if ( Rect_ContainsPoint( &menu->items[i]->window.rect, x, y ) ) {
			return menu->items[i];
		}
	}
	return NULL;
}

void Item_SetMouseOver( itemDef_t *item, qboolean focus ) {
	if ( item ) {
		if ( focus ) {
			item->window.flags |= WINDOW_MOUSEOVER;
		} else {
			item->window.flags &= ~WINDOW_MOUSEOVER;
		}
	}
}


qboolean Item_OwnerDraw_HandleKey( itemDef_t *item, int key ) {
	if ( item && DC->ownerDrawHandleKey ) {
		return DC->ownerDrawHandleKey( item->window.ownerDraw, item->window.ownerDrawFlags, &item->special, key );
	}
	return qfalse;
}

qboolean Item_ListBox_HandleKey( itemDef_t *item, int key, qboolean down, qboolean force ) {
	listBoxDef_t *listPtr = (listBoxDef_t*)item->typeData;
	int count = DC->feederCount( item->special );
	int max, viewmax;

	if ( force || ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS ) ) {
		max = Item_ListBox_MaxScroll( item );
		if ( item->window.flags & WINDOW_HORIZONTAL ) {
			viewmax = ( item->window.rect.w / listPtr->elementWidth );
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
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection( item->special, item->cursorPos );
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
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection( item->special, item->cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos >= count ) {
						listPtr->startPos = count - 1;
					}
				}
				return qtrue;
			}
		} else {
			viewmax = ( item->window.rect.h / listPtr->elementHeight );
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
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection( item->special, item->cursorPos );
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
					item->cursorPos = listPtr->cursorPos;
					DC->feederSelection( item->special, item->cursorPos );
				} else {
					listPtr->startPos++;
					if ( listPtr->startPos > max ) {
						listPtr->startPos = max;
					}
				}
				return qtrue;
			}
		}
		// mouse hit
		if ( key == K_MOUSE1 || key == K_MOUSE2 ) {
			if ( item->window.flags & WINDOW_LB_LEFTARROW ) {
				listPtr->startPos--;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( item->window.flags & WINDOW_LB_RIGHTARROW ) {
				// one down
				listPtr->startPos++;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( item->window.flags & WINDOW_LB_PGUP ) {
				// page up
				listPtr->startPos -= viewmax;
				if ( listPtr->startPos < 0 ) {
					listPtr->startPos = 0;
				}
			} else if ( item->window.flags & WINDOW_LB_PGDN ) {
				// page down
				listPtr->startPos += viewmax;
				if ( listPtr->startPos > max ) {
					listPtr->startPos = max;
				}
			} else if ( item->window.flags & WINDOW_LB_THUMB ) {
				// Display_SetCaptureItem(item);
			} else {
				// select an item
				if ( DC->realTime < lastListBoxClickTime && listPtr->doubleClick ) {
					Item_RunScript( item, listPtr->doubleClick );
				}
				lastListBoxClickTime = DC->realTime + DOUBLE_CLICK_DELAY;
				if ( item->cursorPos != listPtr->cursorPos ) {
					if ( listPtr->cursorPos < DC->feederCount( item->special ) ) {
						item->cursorPos = listPtr->cursorPos;   // only set if it's valid
					}
					DC->feederSelection( item->special, item->cursorPos );
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
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection( item->special, item->cursorPos );
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
				item->cursorPos = listPtr->cursorPos;
				DC->feederSelection( item->special, item->cursorPos );
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

qboolean Item_YesNo_HandleKey( itemDef_t *item, int key ) {

	if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS && item->cvar ) {
		if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
			DC->setCVar( item->cvar, va( "%i", !DC->getCVarValue( item->cvar ) ) );
			return qtrue;
		}
	}

	return qfalse;

}

int Item_Multi_CountSettings( itemDef_t *item ) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr == NULL ) {
		return 0;
	}
	return multiPtr->count;
}

int Item_Multi_FindCvarByValue( itemDef_t *item ) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( item->cvar );
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

const char *Item_Multi_Setting( itemDef_t *item ) {
	char buff[1024];
	float value = 0;
	int i;
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( multiPtr->strDef ) {
			DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		} else {
			value = DC->getCVarValue( item->cvar );
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

qboolean Item_Multi_HandleKey( itemDef_t *item, int key ) {
	multiDef_t *multiPtr = (multiDef_t*)item->typeData;
	if ( multiPtr ) {
		if ( Rect_ContainsPoint( &item->window.rect, DC->cursorx, DC->cursory ) && item->window.flags & WINDOW_HASFOCUS && item->cvar ) {
			if ( key == K_MOUSE1 || key == K_ENTER || key == K_MOUSE2 || key == K_MOUSE3 ) {
				int current = Item_Multi_FindCvarByValue( item ) + 1;
				int max = Item_Multi_CountSettings( item );
				if ( current < 0 || current >= max ) {
					current = 0;
				}
				if ( multiPtr->strDef ) {
					DC->setCVar( item->cvar, multiPtr->cvarStr[current] );
				} else {
					float value = multiPtr->cvarValue[current];
					if ( ( (float)( (int) value ) ) == value ) {
						DC->setCVar( item->cvar, va( "%i", (int) value ) );
					} else {
						DC->setCVar( item->cvar, va( "%f", value ) );
					}
				}
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean Item_TextField_HandleKey( itemDef_t *item, int key ) {
	char buff[1024];
	int len;
	itemDef_t *newItem = NULL;
	editFieldDef_t *editPtr = (editFieldDef_t*)item->typeData;

	if ( item->cvar ) {

		memset( buff, 0, sizeof( buff ) );
		DC->getCVarString( item->cvar, buff, sizeof( buff ) );
		len = strlen( buff );
		if ( editPtr->maxChars && len > editPtr->maxChars ) {
			len = editPtr->maxChars;
		}
		if ( key & K_CHAR_FLAG ) {
			key &= ~K_CHAR_FLAG;


			if ( key == 'h' - 'a' + 1 ) {      // ctrl-h is backspace
				if ( item->cursorPos > 0 ) {
					memmove( &buff[item->cursorPos - 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
					item->cursorPos--;
					if ( item->cursorPos < editPtr->paintOffset ) {
						editPtr->paintOffset--;
					}
				}
				DC->setCVar( item->cvar, buff );
				return qtrue;
			}


			// 'valid filename' text entry.  so users can't try to save games like '!**$er' and stuff (from the menu)
			if ( item->type == ITEM_TYPE_VALIDFILEFIELD ) {
				if ( !Q_isforfilename( key ) ) {
					return qtrue;
				}
			}

			//
			// ignore any non printable chars
			//
			if ( key < 32 || !item->cvar ) {
				return qtrue;
			}

			if ( item->type == ITEM_TYPE_NUMERICFIELD ) {
				if ( !Q_isnumeric( key ) ) {
					return qfalse;
				}
			}

			if ( !DC->getOverstrikeMode() ) {
				if ( ( len == MAX_EDITFIELD - 1 ) || ( editPtr->maxChars && len >= editPtr->maxChars ) ) {
					return qtrue;
				}
				memmove( &buff[item->cursorPos + 1], &buff[item->cursorPos], len + 1 - item->cursorPos );
			} else {
				if ( editPtr->maxChars && item->cursorPos >= editPtr->maxChars ) {
					return qtrue;
				}
			}

			buff[item->cursorPos] = key;

			DC->setCVar( item->cvar, buff );

			if ( item->cursorPos < len + 1 ) {
				item->cursorPos++;
				if ( editPtr->maxPaintChars && item->cursorPos > editPtr->maxPaintChars ) {
					editPtr->paintOffset++;
				}
			}

		} else {

			if ( key == K_DEL || key == K_KP_DEL ) {
				if ( item->cursorPos < len ) {
					memmove( buff + item->cursorPos, buff + item->cursorPos + 1, len - item->cursorPos );
					DC->setCVar( item->cvar, buff );
				}
				return qtrue;
			}

			if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
				if ( editPtr->maxPaintChars && item->cursorPos >= editPtr->maxPaintChars && item->cursorPos < len ) {
					item->cursorPos++;
					editPtr->paintOffset++;
					return qtrue;
				}
				if ( item->cursorPos < len ) {
					item->cursorPos++;
				}
				return qtrue;
			}

			if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
				if ( item->cursorPos > 0 ) {
					item->cursorPos--;
				}
				if ( item->cursorPos < editPtr->paintOffset ) {
					editPtr->paintOffset--;
				}
				return qtrue;
			}

			if ( key == K_HOME || key == K_KP_HOME ) { // || ( tolower(key) == 'a' && uii.Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = 0;
				editPtr->paintOffset = 0;
				return qtrue;
			}

			if ( key == K_END || key == K_KP_END ) { // ( tolower(key) == 'e' && uii.Key_IsDown( K_CTRL ) ) ) {
				item->cursorPos = len;
				if ( item->cursorPos > editPtr->maxPaintChars ) {
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
			newItem = Menu_SetNextCursorItem( (menuDef_t *)item->parent );
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD || newItem->type == ITEM_TYPE_VALIDFILEFIELD ) ) {
				g_editItem = newItem;
			}
		}

		if ( key == K_UPARROW || key == K_KP_UPARROW ) {
			newItem = Menu_SetPrevCursorItem( (menuDef_t *)item->parent );
			if ( newItem && ( newItem->type == ITEM_TYPE_EDITFIELD || newItem->type == ITEM_TYPE_NUMERICFIELD || newItem->type == ITEM_TYPE_VALIDFILEFIELD ) ) {
				g_editItem = newItem;
			}
		}

		// NERVE - SMF
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			if ( item->onAccept ) {
				Item_RunScript( item, item->onAccept );
			}
		}
		// -NERVE - SMF

		if ( key == K_ENTER || key == K_KP_ENTER || key == K_ESCAPE ) {
			return qfalse;
		}

		return qtrue;
	}
	return qfalse;

}

static void Scroll_ListBox_AutoFunc( void *p ) {
	scrollInfo_t *si = (scrollInfo_t*)p;
	if ( DC->realTime > si->nextScrollTime ) {
		// need to scroll which is done by simulating a click to the item
		// this is done a bit sideways as the autoscroll "knows" that the item is a listbox
		// so it calls it directly
		Item_ListBox_HandleKey( si->item, si->scrollKey, qtrue, qfalse );
		si->nextScrollTime = DC->realTime + si->adjustValue;
	}

