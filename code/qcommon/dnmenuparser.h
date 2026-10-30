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


/* Menu script parser for the ported idTech 3 menu framework: reads the
   .menu files named by a menu list and fills the dnMenu/dnItem objects
   the framework in code/ui/ui_shared.cpp owns. The parse half (keyword
   handlers, hash tables, file loading) moved out of the UI folder as
   its own module; it compiles into the UI module and reads the files
   through the engine import table. */

#ifndef DN_MENU_PARSER_H
#define DN_MENU_PARSER_H

#include "../ui/ui_shared.h"

/* open .menu sources at once, handles are 1..MAX_MENU_SOURCES */
#define MAX_MENU_SOURCES 8

/* keyword table entry: a .menu keyword with its parse handler */
typedef struct keywordHash_s
{
    char *keyword;
    qboolean ( *func )( dnItem *item, int handle );
    struct keywordHash_s *next;
} keywordHash_t;

enum { KEYWORDHASH_SIZE = 512 };

/* one .menu file handed over by the engine as a single malloc'd block;
   the cursor walks it token by token until the source closes */
class dnMenuSource
{
public:
    dnMenuSource( void );

    qboolean IsOpen( void ) const { return buffer != NULL; }
    qboolean Open( const char *file );
    void Close( void );
    int ReadToken( pc_token_t *token );
    qboolean FileAndLine( char *out, int *line ) const;

private:
    char *buffer;               /* whole file text, nul-terminated */
    char *cursor;               /* next character to read */
    int line;                   /* line the last token started on */
    char filename[MAX_QPATH];
};

/* static-only class: parses the .menu script files into the framework
   menus, no instances are created */
class dnMenuParser
{
public:
    /* loads every .menu file named by the list file (ui/menus.txt) */
    static void LoadMenus( const char *listFile );

private:
    dnMenuParser( void );        /* static methods only */

    /* ---- open source handles ---- */
    static dnMenuSource sources[MAX_MENU_SOURCES];
    static int OpenSource( const char *file );
    static int CloseSource( int handle );
    static int ReadToken( int handle, pc_token_t *token );
    static qboolean SourceFileAndLine( int handle, char *filename, int *line );

    /* ---- parse helpers shared by the keyword handlers ---- */
    static void PC_SourceWarning( int handle, char *format, ... );
    static void PC_SourceError( int handle, char *format, ... );
    static qboolean PC_Float_Parse( int handle, float *f );
    static qboolean PC_Color_Parse( int handle, vec4_t *c );
    static qboolean PC_Int_Parse( int handle, int *i );
    static qboolean PC_Rect_Parse( int handle, rectDef_t *r );
    static qboolean PC_String_Parse( int handle, const char **out );
    static qboolean PC_Char_Parse( int handle, char *out );
    static qboolean PC_Script_Parse( int handle, const char **out );
    static qboolean ParseColorRange( dnItem *item, int handle, int type );

    /* ---- keyword hash ---- */
    static int KeywordHash_Key( char *keyword );
    static void KeywordHash_Add( keywordHash_t *table[], keywordHash_t *key );
    static keywordHash_t *KeywordHash_Find( keywordHash_t *table[], char *keyword );

    /* ---- itemDef keyword handlers ---- */
    static qboolean ItemParse_accept( dnItem *item, int handle );
    static qboolean ItemParse_action( dnItem *item, int handle );
    static qboolean ItemParse_addColorRange( dnItem *item, int handle );
    static qboolean ItemParse_addColorRangeRel( dnItem *item, int handle );
    static qboolean ItemParse_align( dnItem *item, int handle );
    static qboolean ItemParse_asset_model( dnItem *item, int handle );
    static qboolean ItemParse_asset_shader( dnItem *item, int handle );
    static qboolean ItemParse_autowrapped( dnItem *item, int handle );
    static qboolean ItemParse_backcolor( dnItem *item, int handle );
    static qboolean ItemParse_background( dnItem *item, int handle );
    static qboolean ItemParse_border( dnItem *item, int handle );
    static qboolean ItemParse_bordercolor( dnItem *item, int handle );
    static qboolean ItemParse_bordersize( dnItem *item, int handle );
    static qboolean ItemParse_cinematic( dnItem *item, int handle );
    static qboolean ItemParse_columns( dnItem *item, int handle );
    static qboolean ItemParse_cvar( dnItem *item, int handle );
    static qboolean ItemParse_cvarFloat( dnItem *item, int handle );
    static qboolean ItemParse_cvarFloatList( dnItem *item, int handle );
    static qboolean ItemParse_cvarStrList( dnItem *item, int handle );
    static qboolean ItemParse_cvarTest( dnItem *item, int handle );
    static qboolean ItemParse_decoration( dnItem *item, int handle );
    static qboolean ItemParse_scrollable( dnItem *item, int handle );
    static qboolean ItemParse_disableCvar( dnItem *item, int handle );
    static qboolean ItemParse_doubleClick( dnItem *item, int handle );
    static qboolean ItemParse_elementheight( dnItem *item, int handle );
    static qboolean ItemParse_elementtype( dnItem *item, int handle );
    static qboolean ItemParse_elementwidth( dnItem *item, int handle );
    static qboolean ItemParse_enableCvar( dnItem *item, int handle );
    static qboolean ItemParse_feeder( dnItem *item, int handle );
    static qboolean ItemParse_focusSound( dnItem *item, int handle );
    static qboolean ItemParse_forecolor( dnItem *item, int handle );
    static qboolean ItemParse_group( dnItem *item, int handle );
    static qboolean ItemParse_hideCvar( dnItem *item, int handle );
    static qboolean ItemParse_horizontalscroll( dnItem *item, int handle );
    static qboolean ItemParse_leaveFocus( dnItem *item, int handle );
    static qboolean ItemParse_maxChars( dnItem *item, int handle );
    static qboolean ItemParse_maxPaintChars( dnItem *item, int handle );
    static qboolean ItemParse_model_angle( dnItem *item, int handle );
    static qboolean ItemParse_model_animplay( dnItem *item, int handle );
    static qboolean ItemParse_model_fovx( dnItem *item, int handle );
    static qboolean ItemParse_model_fovy( dnItem *item, int handle );
    static qboolean ItemParse_model_origin( dnItem *item, int handle );
    static qboolean ItemParse_model_rotation( dnItem *item, int handle );
    static qboolean ItemParse_mouseEnter( dnItem *item, int handle );
    static qboolean ItemParse_mouseEnterText( dnItem *item, int handle );
    static qboolean ItemParse_mouseExit( dnItem *item, int handle );
    static qboolean ItemParse_mouseExitText( dnItem *item, int handle );
    static qboolean ItemParse_name( dnItem *item, int handle );
    static qboolean ItemParse_notselectable( dnItem *item, int handle );
    static qboolean ItemParse_onNamedEvent( dnItem *item, int handle );
    static qboolean ItemParse_onFocus( dnItem *item, int handle );
    static qboolean ItemParse_outlinecolor( dnItem *item, int handle );
    static qboolean ItemParse_ownerdraw( dnItem *item, int handle );
    static qboolean ItemParse_ownerdrawFlag( dnItem *item, int handle );
    static qboolean ItemParse_rect( dnItem *item, int handle );
    static qboolean ItemParse_showCvar( dnItem *item, int handle );
    static qboolean ItemParse_special( dnItem *item, int handle );
    static qboolean ItemParse_style( dnItem *item, int handle );
    static qboolean ItemParse_text( dnItem *item, int handle );
    static qboolean ItemParse_textalign( dnItem *item, int handle );
    static qboolean ItemParse_textalignx( dnItem *item, int handle );
    static qboolean ItemParse_textaligny( dnItem *item, int handle );
    static qboolean ItemParse_textfile( dnItem *item, int handle );
    static qboolean ItemParse_textfont( dnItem *item, int handle );
    static qboolean ItemParse_textsavegame( dnItem *item, int handle );
    static qboolean ItemParse_textscale( dnItem *item, int handle );
    static qboolean ItemParse_textstyle( dnItem *item, int handle );
    static qboolean ItemParse_type( dnItem *item, int handle );
    static qboolean ItemParse_visible( dnItem *item, int handle );
    static qboolean ItemParse_wrapped( dnItem *item, int handle );

    static keywordHash_t itemParseKeywords[];
    static keywordHash_t *itemParseKeywordHash[KEYWORDHASH_SIZE];
    static void Item_SetupKeywordHash( void );
    static qboolean Item_Parse( int handle, dnItem *item );

    /* ---- menuDef keyword handlers ---- */
    static qboolean MenuParse_backcolor( dnItem *item, int handle );
    static qboolean MenuParse_background( dnItem *item, int handle );
    static qboolean MenuParse_border( dnItem *item, int handle );
    static qboolean MenuParse_bordercolor( dnItem *item, int handle );
    static qboolean MenuParse_borderSize( dnItem *item, int handle );
    static qboolean MenuParse_cinematic( dnItem *item, int handle );
    static qboolean MenuParse_disablecolor( dnItem *item, int handle );
    static qboolean MenuParse_execKey( dnItem *item, int handle );
    static qboolean MenuParse_execKeyInt( dnItem *item, int handle );
    static qboolean MenuParse_fadeAmount( dnItem *item, int handle );
    static qboolean MenuParse_fadeClamp( dnItem *item, int handle );
    static qboolean MenuParse_fadeCycle( dnItem *item, int handle );
    static qboolean MenuParse_focuscolor( dnItem *item, int handle );
    static qboolean MenuParse_font( dnItem *item, int handle );
    static qboolean MenuParse_forecolor( dnItem *item, int handle );
    static qboolean MenuParse_fullscreen( dnItem *item, int handle );
    static qboolean MenuParse_itemDef( dnItem *item, int handle );
    static qboolean MenuParse_DefItem( dnItem *item, int handle, int defType );
    static qboolean MenuParse_windowDef( dnItem *item, int handle );
    static qboolean MenuParse_buttonDef( dnItem *item, int handle );
    static qboolean MenuParse_editDef( dnItem *item, int handle );
    static qboolean MenuParse_choiceDef( dnItem *item, int handle );
    static qboolean MenuParse_sliderDef( dnItem *item, int handle );
    static qboolean MenuParse_bindDef( dnItem *item, int handle );
    static qboolean MenuParse_listDef( dnItem *item, int handle );
    /* nested defs: a def keyword inside another item block */
    static qboolean ParseChildDef( dnItem *container, int handle, int defType );
    static qboolean ItemParse_itemDef( dnItem *item, int handle );
    static qboolean ItemParse_windowDef( dnItem *item, int handle );
    static qboolean ItemParse_buttonDef( dnItem *item, int handle );
    static qboolean ItemParse_editDef( dnItem *item, int handle );
    static qboolean ItemParse_choiceDef( dnItem *item, int handle );
    static qboolean ItemParse_sliderDef( dnItem *item, int handle );
    static qboolean ItemParse_bindDef( dnItem *item, int handle );
    static qboolean ItemParse_listDef( dnItem *item, int handle );
    static qboolean MenuParse_onNamedEvent( dnItem *item, int handle );
    static qboolean MenuParse_name( dnItem *item, int handle );
    static qboolean MenuParse_onClose( dnItem *item, int handle );
    static qboolean MenuParse_onESC( dnItem *item, int handle );
    static qboolean MenuParse_onOpen( dnItem *item, int handle );
    static qboolean MenuParse_onROQDone( dnItem *item, int handle );
    static qboolean MenuParse_outlinecolor( dnItem *item, int handle );
    static qboolean MenuParse_outOfBounds( dnItem *item, int handle );
    static qboolean MenuParse_ownerdraw( dnItem *item, int handle );
    static qboolean MenuParse_ownerdrawFlag( dnItem *item, int handle );
    static qboolean MenuParse_popup( dnItem *item, int handle );
    static qboolean MenuParse_rect( dnItem *item, int handle );
    static qboolean MenuParse_slide( dnItem *item, int handle );
    static qboolean MenuParse_soundLoop( dnItem *item, int handle );
    static qboolean MenuParse_style( dnItem *item, int handle );
    static qboolean MenuParse_visible( dnItem *item, int handle );

    static keywordHash_t menuParseKeywords[];
    static keywordHash_t *menuParseKeywordHash[KEYWORDHASH_SIZE];
    static void Menu_SetupKeywordHash( void );
    static qboolean Menu_Parse( int handle, dnMenu *menu );

    /* ---- menu file loading ---- */
    static qboolean Menu_New( int handle );
    static void Menu_Load( const char *menuFile );
};

#endif
