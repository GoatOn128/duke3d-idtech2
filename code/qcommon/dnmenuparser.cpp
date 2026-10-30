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


/* The .menu script parser, split out of the UI folder: the keyword
   handlers, hash tables and file loading below come from the RTCW SP
   ui_shared.c parse half, the source lexer follows the idTech 3 token
   shapes the framework helpers expect. The parser only compiles into
   the UI module: it reads the files through the engine import table
   and fills the dnMenu/dnItem objects the framework in
   code/ui/ui_shared.cpp owns. */

#include "dnmenuparser.h"
#include "../ui/ui_local.h"

/* the display context the framework in ui_shared.cpp owns */
extern displayContextDef_t *DC;

/* the menu currently being parsed; nested defs need it to append into the
   owning menu's flat item list */
static dnMenu *s_parseMenu = NULL;

/* ---- .menu source lexer ---- */

dnMenuSource::dnMenuSource( void ) {
    buffer = NULL;
    cursor = NULL;
    line = 0;
    filename[0] = '\0';
}

/* the engine hands the whole file over as one malloc'd block; the
   source owns the copy until Close frees it */
qboolean dnMenuSource::Open( const char *file ) {
    buffer = ( char * )uii.ReadFile( file, NULL );
    if ( !buffer ) {
        return qfalse;
    }
    cursor = buffer;
    line = 1;
    strncpy( filename, file, sizeof( filename ) - 1 );
    filename[sizeof( filename ) - 1] = '\0';
    return qtrue;
}

void dnMenuSource::Close( void ) {
    free( buffer );
    buffer = NULL;
    cursor = NULL;
    line = 0;
    filename[0] = '\0';
}

/* next token, 0 at the end of the file; numbers carry TT_NUMBER with
   int and float values, and a leading minus stays a separate punctuation
   token so PC_Float_Parse and PC_Int_Parse apply the sign themselves */
int dnMenuSource::ReadToken( pc_token_t *token ) {
    char *p;
    int line;
    int len = 0;

    memset( token, 0, sizeof( *token ) );
    if ( !buffer ) {
        return 0;
    }

    p = cursor;
    line = this->line;

    /* skip whitespace and both comment kinds, counting lines on the way */
    while ( 1 ) {
        if ( !*p ) {
            cursor = p;
            this->line = line;
            return 0;
        }
        if ( *p == '\n' ) {
            line++;
            p++;
            continue;
        }
        if ( *( const unsigned char * )p <= ' ' ) {
            p++;
            continue;
        }
        if ( p[0] == '/' && p[1] == '/' ) {
            while ( *p && *p != '\n' ) {
                p++;
            }
            continue;
        }
        if ( p[0] == '/' && p[1] == '*' ) {
            p += 2;
            while ( *p && !( p[0] == '*' && p[1] == '/' ) ) {
                if ( *p == '\n' ) {
                    line++;
                }
                p++;
            }
            if ( *p ) {
                p += 2;
            }
            continue;
        }
        break;
    }

    this->line = line;
    cursor = p;

    /* quoted string: the quotes drop off, an escaped quote stays in */
    if ( *p == '"' ) {
        p++;
        while ( *p && *p != '"' ) {
            if ( len < MAX_TOKENLENGTH - 1 ) {
                if ( p[0] == '\\' && p[1] == '"' ) {
                    p++;
                }
                token->string[len++] = *p;
            }
            p++;
        }
        if ( *p == '"' ) {
            p++;
        }
        token->string[len] = '\0';
        token->type = TT_STRING;
        cursor = p;
        return 1;
    }

    /* single-quoted character literal */
    if ( *p == '\'' ) {
        p++;
        while ( *p && *p != '\'' ) {
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token->string[len++] = *p;
            }
            p++;
        }
        if ( *p == '\'' ) {
            p++;
        }
        token->string[len] = '\0';
        token->type = TT_LITERAL;
        cursor = p;
        return 1;
    }

    /* unsigned number */
    if ( ( *p >= '0' && *p <= '9' ) || ( *p == '.' && p[1] >= '0' && p[1] <= '9' ) ) {
        while ( ( *p >= '0' && *p <= '9' ) || *p == '.' ) {
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token->string[len++] = *p;
            }
            p++;
        }
        token->string[len] = '\0';
        token->type = TT_NUMBER;
        token->intvalue = atoi( token->string );
        token->floatvalue = ( float )atof( token->string );
        cursor = p;
        return 1;
    }

    /* name; the extra characters keep unquoted file paths like
       ui/main.menu in one token instead of splitting on / and . */
    if ( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || *p == '_' ) {
        while ( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) ||
                ( *p >= '0' && *p <= '9' ) || *p == '_' ||
                *p == '/' || *p == '\\' || *p == '.' ) {
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token->string[len++] = *p;
            }
            p++;
        }
        token->string[len] = '\0';
        token->type = TT_NAME;
        cursor = p;
        return 1;
    }

    /* anything else is a single punctuation character */
    token->string[len++] = *p++;
    token->string[len] = '\0';
    token->type = TT_PUNCTUATION;
    cursor = p;
    return 1;
}

qboolean dnMenuSource::FileAndLine( char *out, int *outLine ) const {
    if ( !buffer ) {
        out[0] = '\0';
        *outLine = 0;
        return qfalse;
    }
    strncpy( out, filename, 127 );
    out[127] = '\0';
    *outLine = line;
    return qtrue;
}

/* ---- open sources by handle ---- */

dnMenuSource dnMenuParser::sources[MAX_MENU_SOURCES];

int dnMenuParser::OpenSource( const char *file ) {
    int i;

    for ( i = 0; i < MAX_MENU_SOURCES; i++ ) {
        if ( !sources[i].IsOpen() ) {
            break;
        }
    }
    if ( i == MAX_MENU_SOURCES ) {
        return 0;
    }
    if ( !sources[i].Open( file ) ) {
        return 0;
    }
    return i + 1;                       /* handle 0 means failure */
}

int dnMenuParser::CloseSource( int handle ) {
    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        return 0;
    }
    if ( !sources[handle - 1].IsOpen() ) {
        return 0;
    }
    sources[handle - 1].Close();
    return 1;
}

int dnMenuParser::ReadToken( int handle, pc_token_t *token ) {
    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        memset( token, 0, sizeof( *token ) );
        return 0;
    }
    return sources[handle - 1].ReadToken( token );
}

qboolean dnMenuParser::SourceFileAndLine( int handle, char *out, int *line ) {
    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        out[0] = '\0';
        *line = 0;
        return qfalse;
    }
    return sources[handle - 1].FileAndLine( out, line );
}

/* ---- parse helpers and keyword handlers ---- */

void dnMenuParser::PC_SourceWarning( int handle, char *format, ... ) {
    int line;
    char filename[128];
    va_list argptr;
    static char string[4096];

    va_start( argptr, format );
    vsprintf( string, format, argptr );
    va_end( argptr );

    filename[0] = '\0';
    line = 0;
    SourceFileAndLine( handle, filename, &line );

    UI_Printf( S_COLOR_YELLOW "WARNING: %s, line %d: %s\n", filename, line, string );
}

void dnMenuParser::PC_SourceError( int handle, char *format, ... ) {
    int line;
    char filename[128];
    va_list argptr;
    static char string[4096];

    va_start( argptr, format );
    vsprintf( string, format, argptr );
    va_end( argptr );

    filename[0] = '\0';
    line = 0;
    SourceFileAndLine( handle, filename, &line );

    UI_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );
}
qboolean dnMenuParser::PC_Float_Parse( int handle, float *f ) {
    pc_token_t token;
    int negative = qfalse;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( token.string[0] == '-' ) {
        if ( !ReadToken( handle, &token ) ) {
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
qboolean dnMenuParser::PC_Color_Parse( int handle, vec4_t *c ) {
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
qboolean dnMenuParser::PC_Int_Parse( int handle, int *i ) {
    pc_token_t token;
    int negative = qfalse;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( token.string[0] == '-' ) {
        if ( !ReadToken( handle, &token ) ) {
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
qboolean dnMenuParser::PC_Rect_Parse( int handle, rectDef_t *r ) {
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
qboolean dnMenuParser::PC_String_Parse( int handle, const char **out ) {
    pc_token_t token;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }

    *( out ) = String_Alloc( token.string );
    return qtrue;
}
qboolean dnMenuParser::PC_Char_Parse( int handle, char *out ) {
    pc_token_t token;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }

    *( out ) = token.string[0];
    return qtrue;
}
qboolean dnMenuParser::PC_Script_Parse( int handle, const char **out ) {
    char script[1024];
    pc_token_t token;

    memset( script, 0, sizeof( script ) );
    // scripts start with { and have ; separated command lists.. commands are command, arg..
    // basically we want everything between the { } as it will be interpreted at run time

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( Q_strcasecmp( token.string, "{" ) != 0 ) {
        return qfalse;
    }

    while ( 1 ) {
        if ( !ReadToken( handle, &token ) ) {
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
}

int dnMenuParser::KeywordHash_Key( char *keyword ) {
    int hash, i;

    hash = 0;
    for ( i = 0; keyword[i] != '\0'; i++ ) {
        if ( Q_isupper( keyword[i] ) ) {
            hash += ( keyword[i] + ( 'a' - 'A' ) ) * ( 119 + i );
        } else {
            hash += keyword[i] * ( 119 + i );
        }
    }
    hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) ) & ( KEYWORDHASH_SIZE - 1 );
    return hash;
}

void dnMenuParser::KeywordHash_Add( keywordHash_t *table[], keywordHash_t *key ) {
    int hash;

    hash = KeywordHash_Key( key->keyword );
/*
    if (table[hash]) {
        int collision = qtrue;
    }
*/
    key->next = table[hash];
    table[hash] = key;
}

keywordHash_t *dnMenuParser::KeywordHash_Find( keywordHash_t *table[], char *keyword ) {
    keywordHash_t *key;
    int hash;

    hash = KeywordHash_Key( keyword );
    for ( key = table[hash]; key; key = key->next ) {
        if ( !Q_strcasecmp( key->keyword, keyword ) ) {
            return key;
        }
    }
    return NULL;
}

/*
===============
Item Keyword Parse functions
===============
*/

qboolean dnMenuParser::ItemParse_name( dnItem *item, int handle ) {
    if ( !PC_String_Parse( handle, &item->window.name ) ) {
        return qfalse;
    }
    return qtrue;
}

// name <string>
qboolean dnMenuParser::ItemParse_focusSound( dnItem *item, int handle ) {
    const char *temp;
    if ( !PC_String_Parse( handle, &temp ) ) {
        return qfalse;
    }
    item->focusSound = DC->registerSound( temp );
    return qtrue;
}


// text <string>
qboolean dnMenuParser::ItemParse_text( dnItem *item, int handle ) {
    if ( !PC_String_Parse( handle, &item->text ) ) {
        return qfalse;
    }
    return qtrue;
}


// textfile <string>
// read an external textfile into item->text
qboolean dnMenuParser::ItemParse_textfile( dnItem *item, int handle ) {
    const char  *newtext;
    pc_token_t token;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }

    newtext = DC->fileText( token.string );
    item->text = String_Alloc( newtext );

    return qtrue;
}


qboolean dnMenuParser::ItemParse_textsavegame( dnItem *item, int handle ) {

    // this'll get picked up when the savegames are parsed
    item->text = "savegameinfo";
    item->textSavegameInfo = qtrue;
    return qtrue;
}



// group <string>
qboolean dnMenuParser::ItemParse_group( dnItem *item, int handle ) {
    if ( !PC_String_Parse( handle, &item->window.group ) ) {
        return qfalse;
    }
    return qtrue;
}


// asset_model <string>
qboolean dnMenuParser::ItemParse_asset_model( dnItem *item, int handle ) {
    const char *temp;
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( !PC_String_Parse( handle, &temp ) ) {
        return qfalse;
    }
    if ( !( item->asset ) ) {
        item->asset = DC->registerModel( temp );
    }
    return qtrue;
}

// asset_shader <string>
qboolean dnMenuParser::ItemParse_asset_shader( dnItem *item, int handle ) {
    const char *temp;

    if ( !PC_String_Parse( handle, &temp ) ) {
        return qfalse;
    }
    item->asset = DC->registerShaderNoMip( temp );
    return qtrue;
}

// model_origin <number> <number> <number>
qboolean dnMenuParser::ItemParse_model_origin( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( PC_Float_Parse( handle, &modelPtr->origin[0] ) ) {
        if ( PC_Float_Parse( handle, &modelPtr->origin[1] ) ) {
            if ( PC_Float_Parse( handle, &modelPtr->origin[2] ) ) {
                return qtrue;
            }
        }
    }
    return qfalse;
}

// model_fovx <number>
qboolean dnMenuParser::ItemParse_model_fovx( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( !PC_Float_Parse( handle, &modelPtr->fov_x ) ) {
        return qfalse;
    }
    return qtrue;
}

// model_fovy <number>
qboolean dnMenuParser::ItemParse_model_fovy( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( !PC_Float_Parse( handle, &modelPtr->fov_y ) ) {
        return qfalse;
    }
    return qtrue;
}

// model_rotation <integer>
qboolean dnMenuParser::ItemParse_model_rotation( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( !PC_Int_Parse( handle, &modelPtr->rotationSpeed ) ) {
        return qfalse;
    }
    return qtrue;
}

// model_angle <integer>
qboolean dnMenuParser::ItemParse_model_angle( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    if ( !PC_Int_Parse( handle, &modelPtr->angle ) ) {
        return qfalse;
    }
    return qtrue;
}

// model_animplay <int(startframe)> <int(numframes)> <int(loopframes)> <int(fps)>
qboolean dnMenuParser::ItemParse_model_animplay( dnItem *item, int handle ) {
    modelDef_t *modelPtr;
    item->ValidateTypeData();
    modelPtr = (modelDef_t*)item->typeData;

    modelPtr->animated = 1;

    if ( !PC_Int_Parse( handle, &modelPtr->startframe ) ) {
        return qfalse;
    }
    if ( !PC_Int_Parse( handle, &modelPtr->numframes ) ) {
        return qfalse;
    }
    if ( !PC_Int_Parse( handle, &modelPtr->loopframes ) ) {
        return qfalse;
    }
    if ( !PC_Int_Parse( handle, &modelPtr->fps ) ) {
        return qfalse;
    }

    modelPtr->frame     = modelPtr->startframe + 1;
    modelPtr->oldframe  = modelPtr->startframe;
    modelPtr->backlerp  = 0.0f;
    modelPtr->frameTime = DC->realTime;
    return qtrue;
}


// rect <rectangle>
qboolean dnMenuParser::ItemParse_rect( dnItem *item, int handle ) {
    if ( !PC_Rect_Parse( handle, &item->window.rectClient ) ) {
        return qfalse;
    }
    return qtrue;
}

// style <integer>
qboolean dnMenuParser::ItemParse_style( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->window.style ) ) {
        return qfalse;
    }
    return qtrue;
}

// decoration
qboolean dnMenuParser::ItemParse_decoration( dnItem *item, int handle ) {
    item->window.flags |= WINDOW_DECORATION;
    return qtrue;
}

// notselectable
qboolean dnMenuParser::ItemParse_notselectable( dnItem *item, int handle ) {
    listBoxDef_t *listPtr;
    item->ValidateTypeData();
    listPtr = (listBoxDef_t*)item->typeData;
    if ( item->type == ITEM_TYPE_LISTBOX && listPtr ) {
        listPtr->notselectable = qtrue;
    }
    return qtrue;
}

// manually wrapped
qboolean dnMenuParser::ItemParse_wrapped( dnItem *item, int handle ) {
    item->window.flags |= WINDOW_WRAPPED;
    return qtrue;
}

// auto wrapped
qboolean dnMenuParser::ItemParse_autowrapped( dnItem *item, int handle ) {
    item->window.flags |= WINDOW_AUTOWRAPPED;
    return qtrue;
}


// horizontalscroll
qboolean dnMenuParser::ItemParse_horizontalscroll( dnItem *item, int handle ) {
    item->window.flags |= WINDOW_HORIZONTAL;
    return qtrue;
}

// type <integer>
qboolean dnMenuParser::ItemParse_type( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->type ) ) {
        return qfalse;
    }
    item->ValidateTypeData();
    return qtrue;
}

// elementwidth, used for listbox image elements
// uses textalignx for storage
qboolean dnMenuParser::ItemParse_elementwidth( dnItem *item, int handle ) {
    listBoxDef_t *listPtr;

    item->ValidateTypeData();
    listPtr = (listBoxDef_t*)item->typeData;
    if ( !PC_Float_Parse( handle, &listPtr->elementWidth ) ) {
        return qfalse;
    }
    return qtrue;
}

// elementheight, used for listbox image elements
// uses textaligny for storage
qboolean dnMenuParser::ItemParse_elementheight( dnItem *item, int handle ) {
    listBoxDef_t *listPtr;

    item->ValidateTypeData();
    listPtr = (listBoxDef_t*)item->typeData;
    if ( !PC_Float_Parse( handle, &listPtr->elementHeight ) ) {
        return qfalse;
    }
    return qtrue;
}

// feeder <float>
qboolean dnMenuParser::ItemParse_feeder( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->special ) ) {
        return qfalse;
    }
    return qtrue;
}

// elementtype, used to specify what type of elements a listbox contains
// uses textstyle for storage
qboolean dnMenuParser::ItemParse_elementtype( dnItem *item, int handle ) {
    listBoxDef_t *listPtr;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }
    listPtr = (listBoxDef_t*)item->typeData;
    if ( !PC_Int_Parse( handle, &listPtr->elementStyle ) ) {
        return qfalse;
    }
    return qtrue;
}

// columns sets a number of columns and an x pos and width per..
qboolean dnMenuParser::ItemParse_columns( dnItem *item, int handle ) {
    int num, i;
    listBoxDef_t *listPtr;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }
    listPtr = (listBoxDef_t*)item->typeData;
    if ( PC_Int_Parse( handle, &num ) ) {
        if ( num > MAX_LB_COLUMNS ) {
            num = MAX_LB_COLUMNS;
        }
        listPtr->numColumns = num;
        for ( i = 0; i < num; i++ ) {
            int pos, width, maxChars;

            if ( PC_Int_Parse( handle, &pos ) && PC_Int_Parse( handle, &width ) && PC_Int_Parse( handle, &maxChars ) ) {
                listPtr->columnInfo[i].pos = pos;
                listPtr->columnInfo[i].width = width;
                listPtr->columnInfo[i].maxChars = maxChars;
            } else {
                return qfalse;
            }
        }
    } else {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_border( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->window.border ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_bordersize( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->window.borderSize ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_visible( dnItem *item, int handle ) {
    int i;

    if ( !PC_Int_Parse( handle, &i ) ) {
        return qfalse;
    }
    if ( i ) {
        item->window.flags |= WINDOW_VISIBLE;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_ownerdraw( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->window.ownerDraw ) ) {
        return qfalse;
    }
    item->type = ITEM_TYPE_OWNERDRAW;
    return qtrue;
}

qboolean dnMenuParser::ItemParse_align( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->alignment ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textalign( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->textalignment ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textalignx( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->textalignx ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textaligny( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->textaligny ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textscale( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->textscale ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textstyle( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->textStyle ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_textfont( dnItem *item, int handle ) {
    if ( !PC_Int_Parse( handle, &item->font ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_backcolor( dnItem *item, int handle ) {
    int i;
    float f;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        item->window.backColor[i]  = f;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_forecolor( dnItem *item, int handle ) {
    int i;
    float f;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        item->window.foreColor[i]  = f;
        item->window.flags |= WINDOW_FORECOLORSET;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_bordercolor( dnItem *item, int handle ) {
    int i;
    float f;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        item->window.borderColor[i]  = f;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_outlinecolor( dnItem *item, int handle ) {
    if ( !PC_Color_Parse( handle, &item->window.outlineColor ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_background( dnItem *item, int handle ) {
    const char *temp;

    if ( !PC_String_Parse( handle, &temp ) ) {
        return qfalse;
    }
    item->window.background = DC->registerShaderNoMip( temp );
    return qtrue;
}

qboolean dnMenuParser::ItemParse_cinematic( dnItem *item, int handle ) {
    if ( !PC_String_Parse( handle, &item->window.cinematicName ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_doubleClick( dnItem *item, int handle ) {
    listBoxDef_t *listPtr;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }

    listPtr = (listBoxDef_t*)item->typeData;

    if ( !PC_Script_Parse( handle, &listPtr->doubleClick ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_onFocus( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->onFocus ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_leaveFocus( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->leaveFocus ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_mouseEnter( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->mouseEnter ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_mouseExit( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->mouseExit ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_mouseEnterText( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->mouseEnterText ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_mouseExitText( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->mouseExitText ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_action( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->action ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_accept( dnItem *item, int handle ) {
    if ( !PC_Script_Parse( handle, &item->onAccept ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_special( dnItem *item, int handle ) {
    if ( !PC_Float_Parse( handle, &item->special ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_cvarTest( dnItem *item, int handle ) {
    if ( !PC_String_Parse( handle, &item->cvarTest ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_cvar( dnItem *item, int handle ) {
    editFieldDef_t *editPtr;

    item->ValidateTypeData();
    if ( !PC_String_Parse( handle, &item->cvar ) ) {
        return qfalse;
    }
    if ( item->typeData ) {
        editPtr = (editFieldDef_t*)item->typeData;
        editPtr->minVal = -1;
        editPtr->maxVal = -1;
        editPtr->defVal = -1;
    }
    return qtrue;
}

qboolean dnMenuParser::ItemParse_maxChars( dnItem *item, int handle ) {
    editFieldDef_t *editPtr;
    int maxChars;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }

    if ( !PC_Int_Parse( handle, &maxChars ) ) {
        return qfalse;
    }
    editPtr = (editFieldDef_t*)item->typeData;
    editPtr->maxChars = maxChars;
    return qtrue;
}

qboolean dnMenuParser::ItemParse_maxPaintChars( dnItem *item, int handle ) {
    editFieldDef_t *editPtr;
    int maxChars;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }

    if ( !PC_Int_Parse( handle, &maxChars ) ) {
        return qfalse;
    }
    editPtr = (editFieldDef_t*)item->typeData;
    editPtr->maxPaintChars = maxChars;
    return qtrue;
}



qboolean dnMenuParser::ItemParse_cvarFloat( dnItem *item, int handle ) {
    editFieldDef_t *editPtr;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }
    editPtr = (editFieldDef_t*)item->typeData;
    if ( PC_String_Parse( handle, &item->cvar ) &&
         PC_Float_Parse( handle, &editPtr->defVal ) &&
         PC_Float_Parse( handle, &editPtr->minVal ) &&
         PC_Float_Parse( handle, &editPtr->maxVal ) ) {
        return qtrue;
    }
    return qfalse;
}

qboolean dnMenuParser::ItemParse_cvarStrList( dnItem *item, int handle ) {
    pc_token_t token;
    multiDef_t *multiPtr;
    int pass;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }
    multiPtr = (multiDef_t*)item->typeData;
    multiPtr->count = 0;
    multiPtr->strDef = qtrue;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( *token.string != '{' ) {
        return qfalse;
    }

    pass = 0;
    while ( 1 ) {
        if ( !ReadToken( handle, &token ) ) {
            PC_SourceError( handle, "end of file inside menu item\n" );
            return qfalse;
        }

        if ( *token.string == '}' ) {
            return qtrue;
        }

        if ( *token.string == ',' || *token.string == ';' ) {
            continue;
        }

        if ( pass == 0 ) {
            multiPtr->cvarList[multiPtr->count] = String_Alloc( token.string );
            pass = 1;
        } else {
            multiPtr->cvarStr[multiPtr->count] = String_Alloc( token.string );
            pass = 0;
            multiPtr->count++;
            if ( multiPtr->count >= MAX_MULTI_CVARS ) {
                return qfalse;
            }
        }

    }

}

qboolean dnMenuParser::ItemParse_cvarFloatList( dnItem *item, int handle ) {
    pc_token_t token;
    multiDef_t *multiPtr;

    item->ValidateTypeData();
    if ( !item->typeData ) {
        return qfalse;
    }
    multiPtr = (multiDef_t*)item->typeData;
    multiPtr->count = 0;
    multiPtr->strDef = qfalse;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( *token.string != '{' ) {
        return qfalse;
    }

    while ( 1 ) {
        if ( !ReadToken( handle, &token ) ) {
            PC_SourceError( handle, "end of file inside menu item\n" );
            return qfalse;
        }

        if ( *token.string == '}' ) {
            return qtrue;
        }

        if ( *token.string == ',' || *token.string == ';' ) {
            continue;
        }

        multiPtr->cvarList[multiPtr->count] = String_Alloc( token.string );
        if ( !PC_Float_Parse( handle, &multiPtr->cvarValue[multiPtr->count] ) ) {
            return qfalse;
        }

        multiPtr->count++;
        if ( multiPtr->count >= MAX_MULTI_CVARS ) {
            return qfalse;
        }

    }

}


qboolean dnMenuParser::ParseColorRange( dnItem *item, int handle, int type ) {
    colorRangeDef_t color;

    if ( item->numColors && type != item->colorRangeType ) {
        PC_SourceError( handle, "both addColorRange and addColorRangeRel - set within same itemdef\n" );
        return qfalse;
    }

    item->colorRangeType = type;

    if ( PC_Float_Parse( handle, &color.low ) &&
         PC_Float_Parse( handle, &color.high ) &&
         PC_Color_Parse( handle, &color.color ) ) {
        if ( item->numColors < MAX_COLOR_RANGES ) {
            memcpy( &item->colorRanges[item->numColors], &color, sizeof( color ) );
            item->numColors++;
        }
        return qtrue;
    }
    return qfalse;
}

qboolean dnMenuParser::ItemParse_addColorRangeRel( dnItem *item, int handle ) {
    return ParseColorRange( item, handle, RANGETYPE_RELATIVE );
}

qboolean dnMenuParser::ItemParse_addColorRange( dnItem *item, int handle ) {
    return ParseColorRange( item, handle, RANGETYPE_ABSOLUTE );
}



qboolean dnMenuParser::ItemParse_ownerdrawFlag( dnItem *item, int handle ) {
    int i;
    if ( !PC_Int_Parse( handle, &i ) ) {
        return qfalse;
    }
    item->window.ownerDrawFlags |= i;
    return qtrue;
}

qboolean dnMenuParser::ItemParse_enableCvar( dnItem *item, int handle ) {
    if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
        item->cvarFlags = CVAR_ENABLE;
        return qtrue;
    }
    return qfalse;
}

qboolean dnMenuParser::ItemParse_disableCvar( dnItem *item, int handle ) {
    if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
        item->cvarFlags = CVAR_DISABLE;
        return qtrue;
    }
    return qfalse;
}

qboolean dnMenuParser::ItemParse_showCvar( dnItem *item, int handle ) {
    if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
        item->cvarFlags = CVAR_SHOW;
        return qtrue;
    }
    return qfalse;
}

qboolean dnMenuParser::ItemParse_hideCvar( dnItem *item, int handle ) {
    if ( PC_Script_Parse( handle, &item->enableCvar ) ) {
        item->cvarFlags = CVAR_HIDE;
        return qtrue;
    }
    return qfalse;
}


keywordHash_t dnMenuParser::itemParseKeywords[] = {
    {"name", &dnMenuParser::ItemParse_name, NULL},
    {"text", &dnMenuParser::ItemParse_text, NULL},
    {"textfile", &dnMenuParser::ItemParse_textfile, NULL},
    {"textsavegame", &dnMenuParser::ItemParse_textsavegame, NULL},
    {"group", &dnMenuParser::ItemParse_group, NULL},
    {"asset_model", &dnMenuParser::ItemParse_asset_model, NULL},
    {"asset_shader", &dnMenuParser::ItemParse_asset_shader, NULL},
    {"model_origin", &dnMenuParser::ItemParse_model_origin, NULL},
    {"model_fovx", &dnMenuParser::ItemParse_model_fovx, NULL},
    {"model_fovy", &dnMenuParser::ItemParse_model_fovy, NULL},
    {"model_rotation", &dnMenuParser::ItemParse_model_rotation, NULL},
    {"model_angle", &dnMenuParser::ItemParse_model_angle, NULL},
    {"model_animplay", &dnMenuParser::ItemParse_model_animplay, NULL},
    {"rect", &dnMenuParser::ItemParse_rect, NULL},
    {"style", &dnMenuParser::ItemParse_style, NULL},
    {"decoration", &dnMenuParser::ItemParse_decoration, NULL},
    {"notselectable", &dnMenuParser::ItemParse_notselectable, NULL},
    {"wrapped", &dnMenuParser::ItemParse_wrapped, NULL},
    {"autowrapped", &dnMenuParser::ItemParse_autowrapped, NULL},
    {"horizontalscroll", &dnMenuParser::ItemParse_horizontalscroll, NULL},
    {"type", &dnMenuParser::ItemParse_type, NULL},
    {"elementwidth", &dnMenuParser::ItemParse_elementwidth, NULL},
    {"elementheight", &dnMenuParser::ItemParse_elementheight, NULL},
    {"feeder", &dnMenuParser::ItemParse_feeder, NULL},
    {"elementtype", &dnMenuParser::ItemParse_elementtype, NULL},
    {"columns", &dnMenuParser::ItemParse_columns, NULL},
    {"border", &dnMenuParser::ItemParse_border, NULL},
    {"bordersize", &dnMenuParser::ItemParse_bordersize, NULL},
    {"visible", &dnMenuParser::ItemParse_visible, NULL},
    {"ownerdraw", &dnMenuParser::ItemParse_ownerdraw, NULL},
    {"align", &dnMenuParser::ItemParse_align, NULL},
    {"textalign", &dnMenuParser::ItemParse_textalign, NULL},
    {"textalignx", &dnMenuParser::ItemParse_textalignx, NULL},
    {"textaligny", &dnMenuParser::ItemParse_textaligny, NULL},
    {"textscale", &dnMenuParser::ItemParse_textscale, NULL},
    {"textstyle", &dnMenuParser::ItemParse_textstyle, NULL},
    {"textfont", &dnMenuParser::ItemParse_textfont, NULL},
    {"backcolor", &dnMenuParser::ItemParse_backcolor, NULL},
    {"forecolor", &dnMenuParser::ItemParse_forecolor, NULL},
    {"bordercolor", &dnMenuParser::ItemParse_bordercolor, NULL},
    {"outlinecolor", &dnMenuParser::ItemParse_outlinecolor, NULL},
    {"background", &dnMenuParser::ItemParse_background, NULL},
    {"onFocus", &dnMenuParser::ItemParse_onFocus, NULL},
    {"leaveFocus", &dnMenuParser::ItemParse_leaveFocus, NULL},
    {"mouseEnter", &dnMenuParser::ItemParse_mouseEnter, NULL},
    {"mouseExit", &dnMenuParser::ItemParse_mouseExit, NULL},
    {"mouseEnterText", &dnMenuParser::ItemParse_mouseEnterText, NULL},
    {"mouseExitText", &dnMenuParser::ItemParse_mouseExitText, NULL},
    {"action", &dnMenuParser::ItemParse_action, NULL},
    {"accept", &dnMenuParser::ItemParse_accept, NULL},
    {"onNamedEvent", &dnMenuParser::ItemParse_onNamedEvent, NULL},
    {"special", &dnMenuParser::ItemParse_special, NULL},
    {"cvar", &dnMenuParser::ItemParse_cvar, NULL},
    {"maxChars", &dnMenuParser::ItemParse_maxChars, NULL},
    {"maxPaintChars", &dnMenuParser::ItemParse_maxPaintChars, NULL},
    {"focusSound", &dnMenuParser::ItemParse_focusSound, NULL},
    {"cvarFloat", &dnMenuParser::ItemParse_cvarFloat, NULL},
    {"cvarStrList", &dnMenuParser::ItemParse_cvarStrList, NULL},
    {"cvarFloatList", &dnMenuParser::ItemParse_cvarFloatList, NULL},
    {"addColorRange", &dnMenuParser::ItemParse_addColorRange, NULL},
    {"addColorRangeRel", &dnMenuParser::ItemParse_addColorRangeRel, NULL},
    {"ownerdrawFlag", &dnMenuParser::ItemParse_ownerdrawFlag, NULL},
    {"enableCvar", &dnMenuParser::ItemParse_enableCvar, NULL},
    {"cvarTest", &dnMenuParser::ItemParse_cvarTest, NULL},
    {"disableCvar", &dnMenuParser::ItemParse_disableCvar, NULL},
    {"showCvar", &dnMenuParser::ItemParse_showCvar, NULL},
    {"hideCvar", &dnMenuParser::ItemParse_hideCvar, NULL},
    {"cinematic", &dnMenuParser::ItemParse_cinematic, NULL},
    {"doubleclick", &dnMenuParser::ItemParse_doubleClick, NULL},
    {"itemDef", &dnMenuParser::ItemParse_itemDef, NULL},
    {"windowDef", &dnMenuParser::ItemParse_windowDef, NULL},
    {"buttonDef", &dnMenuParser::ItemParse_buttonDef, NULL},
    {"editDef", &dnMenuParser::ItemParse_editDef, NULL},
    {"choiceDef", &dnMenuParser::ItemParse_choiceDef, NULL},
    {"sliderDef", &dnMenuParser::ItemParse_sliderDef, NULL},
    {"bindDef", &dnMenuParser::ItemParse_bindDef, NULL},
    {"listDef", &dnMenuParser::ItemParse_listDef, NULL},
    {NULL, NULL, NULL}
};

keywordHash_t *dnMenuParser::itemParseKeywordHash[KEYWORDHASH_SIZE];

void dnMenuParser::Item_SetupKeywordHash( void ) {
    int i;

    memset( itemParseKeywordHash, 0, sizeof( itemParseKeywordHash ) );
    for ( i = 0; itemParseKeywords[i].keyword; i++ ) {
        KeywordHash_Add( itemParseKeywordHash, &itemParseKeywords[i] );
    }
}

qboolean dnMenuParser::Item_Parse( int handle, dnItem *item ) {
    pc_token_t token;
    keywordHash_t *key;


    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( *token.string != '{' ) {
        return qfalse;
    }
    while ( 1 ) {
        if ( !ReadToken( handle, &token ) ) {
            PC_SourceError( handle, "end of file inside menu item\n" );
            return qfalse;
        }

        if ( *token.string == '}' ) {
            return qtrue;
        }

        key = KeywordHash_Find( itemParseKeywordHash, token.string );
        if ( !key ) {
            PC_SourceError( handle, "unknown menu item keyword %s", token.string );
            continue;
        }
        if ( !key->func( item, handle ) ) {
            PC_SourceError( handle, "couldn't parse menu item keyword %s", token.string );
            return qfalse;
        }
    }
}


/*
===============
Menu Keyword Parse functions
===============
*/

qboolean dnMenuParser::MenuParse_font( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_String_Parse( handle, &menu->font ) ) {
        return qfalse;
    }
    if ( !DC->Assets.fontRegistered ) {
        DC->registerFont( menu->font, 48, &DC->Assets.textFont );
        DC->Assets.fontRegistered = qtrue;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_name( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_String_Parse( handle, &menu->window.name ) ) {
        return qfalse;
    }
    if ( Q_strcasecmp( menu->window.name, "main" ) == 0 ) {
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_fullscreen( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Int_Parse( handle, (int*)&menu->fullScreen ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_rect( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Rect_Parse( handle, &menu->window.rect ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_style( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Int_Parse( handle, &menu->window.style ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_visible( dnItem *item, int handle ) {
    int i;
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Int_Parse( handle, &i ) ) {
        return qfalse;
    }
    if ( i ) {
        menu->window.flags |= WINDOW_VISIBLE;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_onOpen( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Script_Parse( handle, &menu->onOpen ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_onClose( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Script_Parse( handle, &menu->onClose ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_onESC( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Script_Parse( handle, &menu->onESC ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_onROQDone( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Script_Parse( handle, &menu->onROQDone ) ) {
        return qfalse;
    }
    return qtrue;
}




qboolean dnMenuParser::MenuParse_border( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Int_Parse( handle, &menu->window.border ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_borderSize( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Float_Parse( handle, &menu->window.borderSize ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_backcolor( dnItem *item, int handle ) {
    int i;
    float f;
    dnMenu *menu = (dnMenu*)item;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        menu->window.backColor[i]  = f;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_forecolor( dnItem *item, int handle ) {
    int i;
    float f;
    dnMenu *menu = (dnMenu*)item;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        menu->window.foreColor[i]  = f;
        menu->window.flags |= WINDOW_FORECOLORSET;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_bordercolor( dnItem *item, int handle ) {
    int i;
    float f;
    dnMenu *menu = (dnMenu*)item;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        menu->window.borderColor[i]  = f;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_focuscolor( dnItem *item, int handle ) {
    int i;
    float f;
    dnMenu *menu = (dnMenu*)item;

    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        menu->focusColor[i]  = f;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_disablecolor( dnItem *item, int handle ) {
    int i;
    float f;
    dnMenu *menu = (dnMenu*)item;
    for ( i = 0; i < 4; i++ ) {
        if ( !PC_Float_Parse( handle, &f ) ) {
            return qfalse;
        }
        menu->disableColor[i]  = f;
    }
    return qtrue;
}


qboolean dnMenuParser::MenuParse_outlinecolor( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    if ( !PC_Color_Parse( handle, &menu->window.outlineColor ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_background( dnItem *item, int handle ) {
    const char *buff;
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_String_Parse( handle, &buff ) ) {
        return qfalse;
    }
    menu->window.background = DC->registerShaderNoMip( buff );
    return qtrue;
}

qboolean dnMenuParser::MenuParse_cinematic( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_String_Parse( handle, &menu->window.cinematicName ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_ownerdrawFlag( dnItem *item, int handle ) {
    int i;
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Int_Parse( handle, &i ) ) {
        return qfalse;
    }
    menu->window.ownerDrawFlags |= i;
    return qtrue;
}

qboolean dnMenuParser::MenuParse_ownerdraw( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Int_Parse( handle, &menu->window.ownerDraw ) ) {
        return qfalse;
    }
    return qtrue;
}


qboolean dnMenuParser::MenuParse_popup( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    menu->window.flags |= WINDOW_POPUP;
    return qtrue;
}


qboolean dnMenuParser::MenuParse_outOfBounds( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    menu->window.flags |= WINDOW_OOB_CLICK;
    return qtrue;
}

qboolean dnMenuParser::MenuParse_soundLoop( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_String_Parse( handle, &menu->soundName ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_fadeClamp( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Float_Parse( handle, &menu->fadeClamp ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_fadeAmount( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Float_Parse( handle, &menu->fadeAmount ) ) {
        return qfalse;
    }
    return qtrue;
}


qboolean dnMenuParser::MenuParse_fadeCycle( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;

    if ( !PC_Int_Parse( handle, &menu->fadeCycle ) ) {
        return qfalse;
    }
    return qtrue;
}


qboolean dnMenuParser::MenuParse_itemDef( dnItem *item, int handle ) {
    dnMenu *menu = (dnMenu*)item;
    int idx = menu->itemCount;
    if ( idx < MAX_MENUITEMS ) {
        menu->items[idx] = UI_ConstructItem( ITEM_TYPE_TEXT );
        if ( menu->items[idx] == NULL ) {
            return qfalse;
        }
        menu->items[idx]->Init();
        /* count the item before parsing so nested defs inside its block
           append after their container */
        menu->itemCount = idx + 1;
        if ( !Item_Parse( handle, menu->items[idx] ) ) {
            return qfalse;
        }
        /* the parsed type picks the subclass, then the control state resets */
        menu->items[idx]->FinalizeType();
        menu->items[idx]->InitControls();
        menu->items[idx]->parent = menu;
    }
    return qtrue;
}

/* the BFG gui def names: the def sets the item type unless the block
   also carries an explicit type keyword, which then wins */
qboolean dnMenuParser::MenuParse_DefItem( dnItem *item, int handle, int defType ) {
    dnMenu *menu = (dnMenu*)item;
    int idx = menu->itemCount;
    if ( idx < MAX_MENUITEMS ) {
        menu->items[idx] = UI_ConstructItem( ITEM_TYPE_TEXT );
        if ( menu->items[idx] == NULL ) {
            return qfalse;
        }
        menu->items[idx]->Init();
        menu->items[idx]->type = defType;
        /* count the item before parsing so nested defs inside its block
           append after their container */
        menu->itemCount = idx + 1;
        if ( !Item_Parse( handle, menu->items[idx] ) ) {
            return qfalse;
        }
        menu->items[idx]->FinalizeType();
        menu->items[idx]->InitControls();
        menu->items[idx]->parent = menu;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_windowDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_TEXT );
}

qboolean dnMenuParser::MenuParse_buttonDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_BUTTON );
}

qboolean dnMenuParser::MenuParse_editDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_EDITFIELD );
}

qboolean dnMenuParser::MenuParse_choiceDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_MULTI );
}

qboolean dnMenuParser::MenuParse_sliderDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_SLIDER );
}

qboolean dnMenuParser::MenuParse_bindDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_BIND );
}

qboolean dnMenuParser::MenuParse_listDef( dnItem *item, int handle ) {
    return MenuParse_DefItem( item, handle, ITEM_TYPE_LISTBOX );
}

/* a def keyword found inside another item block: the new item joins the
   owning menu's flat list, keeps a back link to its container, and its
   rect bakes against the container's origin. The container's rect keyword
   must come before its nested defs for the bake to be correct */
qboolean dnMenuParser::ParseChildDef( dnItem *container, int handle, int defType ) {
    dnMenu *menu = s_parseMenu;
    dnItem *child;
    int idx;
    int j;
    float dx, dy;

    if ( menu == NULL ) {
        return qfalse;
    }
    idx = menu->itemCount;
    if ( idx >= MAX_MENUITEMS ) {
        return qfalse;
    }
    child = UI_ConstructItem( ITEM_TYPE_TEXT );
    if ( child == NULL ) {
        return qfalse;
    }
    menu->items[idx] = child;
    child->Init();
    child->type = defType;
    child->containerItem = container;
    /* count the child before parsing so its own nested defs append after it */
    menu->itemCount = idx + 1;
    if ( !Item_Parse( handle, child ) ) {
        return qfalse;
    }
    child->FinalizeType();
    child->InitControls();
    child->parent = menu;

    /* shift the child and the subtree below it onto the container origin;
       deeper levels already baked against the child's local rect */
    dx = container->window.rect.x;
    dy = container->window.rect.y;
    child->window.rect.x += dx;
    child->window.rect.y += dy;
    for ( j = idx + 1; j < menu->itemCount; j++ ) {
        menu->items[j]->window.rect.x += dx;
        menu->items[j]->window.rect.y += dy;
    }
    return qtrue;
}

/* item-scope def keywords: same names as the menu-scope ones, but they
   attach the new def to the enclosing container instead of the menu */
qboolean dnMenuParser::ItemParse_itemDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_TEXT );
}

qboolean dnMenuParser::ItemParse_windowDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_TEXT );
}

qboolean dnMenuParser::ItemParse_buttonDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_BUTTON );
}

qboolean dnMenuParser::ItemParse_editDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_EDITFIELD );
}

qboolean dnMenuParser::ItemParse_choiceDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_MULTI );
}

qboolean dnMenuParser::ItemParse_sliderDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_SLIDER );
}

qboolean dnMenuParser::ItemParse_bindDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_BIND );
}

qboolean dnMenuParser::ItemParse_listDef( dnItem *item, int handle ) {
    return ParseChildDef( item, handle, ITEM_TYPE_LISTBOX );
}

/* the menu itself can own named event handlers like the BFG top-level
   windowDef */
qboolean dnMenuParser::MenuParse_onNamedEvent( dnItem *item, int handle ) {
    dnMenu *menu = ( dnMenu* )item;
    pc_token_t token;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( menu->namedEventCount >= MAX_NAMED_EVENTS ) {
        return qfalse;
    }
    Q_strncpyz( menu->namedEventNames[menu->namedEventCount], token.string,
                sizeof( menu->namedEventNames[0] ) );
    if ( !PC_Script_Parse( handle, &menu->namedEventScripts[menu->namedEventCount] ) ) {
        return qfalse;
    }
    menu->namedEventCount++;
    return qtrue;
}

/* an item binds a script to a name, run through the namedEvent command */
qboolean dnMenuParser::ItemParse_onNamedEvent( dnItem *item, int handle ) {
    pc_token_t token;

    if ( !ReadToken( handle, &token ) ) {
        return qfalse;
    }
    if ( item->namedEventCount >= MAX_NAMED_EVENTS ) {
        return qfalse;
    }
    Q_strncpyz( item->namedEventNames[item->namedEventCount], token.string,
                sizeof( item->namedEventNames[0] ) );
    if ( !PC_Script_Parse( handle, &item->namedEventScripts[item->namedEventCount] ) ) {
        return qfalse;
    }
    item->namedEventCount++;
    return qtrue;
}

qboolean dnMenuParser::MenuParse_execKey( dnItem *item, int handle ) {
    dnMenu *menu = ( dnMenu* )item;
    char keyname;
    short int keyindex;

    if ( !PC_Char_Parse( handle, &keyname ) ) {
        return qfalse;
    }
    keyindex = ( unsigned char )keyname;

    if ( !PC_Script_Parse( handle, &menu->onKey[keyindex] ) ) {
        return qfalse;
    }
    return qtrue;
}

qboolean dnMenuParser::MenuParse_execKeyInt( dnItem *item, int handle ) {
    dnMenu *menu = ( dnMenu* )item;
    int keyname;

    if ( !PC_Int_Parse( handle, &keyname ) ) {
        return qfalse;
    }

    if ( !PC_Script_Parse( handle, &menu->onKey[keyname] ) ) {
        return qfalse;
    }
    return qtrue;
}

keywordHash_t dnMenuParser::menuParseKeywords[] = {
    {"font", &dnMenuParser::MenuParse_font, NULL},
    {"name", &dnMenuParser::MenuParse_name, NULL},
    {"fullscreen", &dnMenuParser::MenuParse_fullscreen, NULL},
    {"rect", &dnMenuParser::MenuParse_rect, NULL},
    {"style", &dnMenuParser::MenuParse_style, NULL},
    {"visible", &dnMenuParser::MenuParse_visible, NULL},
    {"onOpen", &dnMenuParser::MenuParse_onOpen, NULL},
    {"onClose", &dnMenuParser::MenuParse_onClose, NULL},
    {"onESC", &dnMenuParser::MenuParse_onESC, NULL},
    {"onROQDone", &dnMenuParser::MenuParse_onROQDone, NULL},
    {"border", &dnMenuParser::MenuParse_border, NULL},
    {"borderSize", &dnMenuParser::MenuParse_borderSize, NULL},
    {"backcolor", &dnMenuParser::MenuParse_backcolor, NULL},
    {"forecolor", &dnMenuParser::MenuParse_forecolor, NULL},
    {"bordercolor", &dnMenuParser::MenuParse_bordercolor, NULL},
    {"focuscolor", &dnMenuParser::MenuParse_focuscolor, NULL},
    {"disablecolor", &dnMenuParser::MenuParse_disablecolor, NULL},
    {"outlinecolor", &dnMenuParser::MenuParse_outlinecolor, NULL},
    {"background", &dnMenuParser::MenuParse_background, NULL},
    {"ownerdraw", &dnMenuParser::MenuParse_ownerdraw, NULL},
    {"ownerdrawFlag", &dnMenuParser::MenuParse_ownerdrawFlag, NULL},
    {"outOfBoundsClick", &dnMenuParser::MenuParse_outOfBounds, NULL},
    {"soundLoop", &dnMenuParser::MenuParse_soundLoop, NULL},
    {"itemDef", &dnMenuParser::MenuParse_itemDef, NULL},
    {"windowDef", &dnMenuParser::MenuParse_windowDef, NULL},
    {"buttonDef", &dnMenuParser::MenuParse_buttonDef, NULL},
    {"editDef", &dnMenuParser::MenuParse_editDef, NULL},
    {"choiceDef", &dnMenuParser::MenuParse_choiceDef, NULL},
    {"sliderDef", &dnMenuParser::MenuParse_sliderDef, NULL},
    {"bindDef", &dnMenuParser::MenuParse_bindDef, NULL},
    {"listDef", &dnMenuParser::MenuParse_listDef, NULL},
    {"onNamedEvent", &dnMenuParser::MenuParse_onNamedEvent, NULL},
    {"cinematic", &dnMenuParser::MenuParse_cinematic, NULL},
    {"popup", &dnMenuParser::MenuParse_popup, NULL},
    {"fadeClamp", &dnMenuParser::MenuParse_fadeClamp, NULL},
    {"fadeCycle", &dnMenuParser::MenuParse_fadeCycle, NULL},
    {"fadeAmount", &dnMenuParser::MenuParse_fadeAmount, NULL},
    {"execKey", &dnMenuParser::MenuParse_execKey, NULL},
    {"execKeyInt", &dnMenuParser::MenuParse_execKeyInt, NULL},
    {NULL, NULL, NULL}
};

keywordHash_t *dnMenuParser::menuParseKeywordHash[KEYWORDHASH_SIZE];

void dnMenuParser::Menu_SetupKeywordHash( void ) {
    int i;

    memset( menuParseKeywordHash, 0, sizeof( menuParseKeywordHash ) );
    for ( i = 0; menuParseKeywords[i].keyword; i++ ) {
        KeywordHash_Add( menuParseKeywordHash, &menuParseKeywords[i] );
    }
}

qboolean dnMenuParser::Menu_Parse( int handle, dnMenu *menu ) {
    pc_token_t token;
    keywordHash_t *key;

    s_parseMenu = menu;
    if ( !ReadToken( handle, &token ) ) {
        s_parseMenu = NULL;
        return qfalse;
    }
    if ( *token.string != '{' ) {
        s_parseMenu = NULL;
        return qfalse;
    }

    while ( 1 ) {

        memset( &token, 0, sizeof( pc_token_t ) );
        if ( !ReadToken( handle, &token ) ) {
            PC_SourceError( handle, "end of file inside menu\n" );
            s_parseMenu = NULL;
            return qfalse;
        }

        if ( *token.string == '}' ) {
            s_parseMenu = NULL;
            return qtrue;
        }

        key = KeywordHash_Find( menuParseKeywordHash, token.string );
        if ( !key ) {
            PC_SourceError( handle, "unknown menu keyword %s", token.string );
            continue;
        }
        if ( !key->func( (dnItem*)menu, handle ) ) {
            PC_SourceError( handle, "couldn't parse menu keyword %s", token.string );
            s_parseMenu = NULL;
            return qfalse;
        }
    }
}

qboolean dnMenuParser::Menu_New( int handle ) {
    /* the framework takes the slot and counts it right away; a menu that
       fails to parse stays behind as an invisible empty menu, the load
       stops here anyway */
    dnMenu *menu = dnMenu::Alloc( "" );

    if ( menu == NULL ) {
        return qfalse;
    }
    if ( !Menu_Parse( handle, menu ) ) {
        return qfalse;
    }
    menu->PostParse();
    return qtrue;
}

/*
===============
Menu_Load

loads every menu in a .menu file; each menu is one braced block
===============
*/
void dnMenuParser::Menu_Load( const char *menuFile ) {
    int handle;

    handle = OpenSource( menuFile );
    if ( !handle ) {
        UI_Printf( S_COLOR_YELLOW "menu file not found: %s\n", menuFile );
        return;
    }

    while ( Menu_New( handle ) ) {
    }

    CloseSource( handle );
}

/*
===============
dnMenuParser::LoadMenus

reads the menu list file (load lines naming .menu files) the Duke menus
all live in now, then loads each one
===============
*/
void dnMenuParser::LoadMenus( const char *listFile ) {
    int handle;
    pc_token_t token;

    Item_SetupKeywordHash();
    Menu_SetupKeywordHash();

    handle = OpenSource( listFile );
    if ( !handle ) {
        UI_Printf( S_COLOR_YELLOW "menu list not found: %s\n", listFile );
        return;
    }

    while ( ReadToken( handle, &token ) ) {
        if ( Q_stricmp( token.string, "load" ) == 0 ) {
            if ( ReadToken( handle, &token ) ) {
                Menu_Load( token.string );
            }
        } else {
            PC_SourceWarning( handle, "expected 'load' but found %s\n", token.string );
        }
    }

    CloseSource( handle );
}
