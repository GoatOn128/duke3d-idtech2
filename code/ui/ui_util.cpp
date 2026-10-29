/* Module support helpers for the ported idTech 3 menu framework: console
   print through the engine import table, the q_shared string/classify
   helpers the engine Q2 header lacks, and the tokenizer the framework
   expects. The helpers follow the RTCW q_shared semantics, reimplemented
   here for the module; the .menu source lexer the framework reads its tokens from. */
#include "ui_local.h"

/* prints into a static buffer, then hands the line to the engine */
void UI_Printf( const char *fmt, ... ) {
    va_list argptr;
    static char string[4096];

    va_start( argptr, fmt );
    vsnprintf( string, sizeof( string ), fmt, argptr );
    va_end( argptr );
    string[sizeof( string ) - 1] = '\0';
    uii.Print( "%s", string );
}

/* the engine Q2 q_shared helpers (Com_sprintf, Info_SetValueForKey) call
   Com_Printf; the module has no console of its own, so the lines route to
   the engine through the import table */
void Com_Printf( char *msg, ... ) {
    va_list argptr;
    static char string[4096];

    va_start( argptr, msg );
    vsnprintf( string, sizeof( string ), msg, argptr );
    va_end( argptr );
    string[sizeof( string ) - 1] = '\0';
    uii.Print( "%s", string );
}

/* ---- .menu source lexer ----
   The engine hands the whole file over through uii.ReadFile as a private
   malloc'd copy; the module walks it token by token and frees it when the
   source closes. The token shapes follow the idTech 3 parser the framework
   helpers expect: numbers carry TT_NUMBER with int and float values, and a
   leading minus stays a separate punctuation token so PC_Float_Parse and
   PC_Int_Parse apply the sign themselves. */

#define MAX_MENU_SOURCES 8

typedef struct {
    char *buffer;                       /* whole file text, nul-terminated */
    char *cursor;                       /* next character to read */
    int line;                           /* line the last token started on */
    char filename[MAX_QPATH];
} menuSource_t;

static menuSource_t sources[MAX_MENU_SOURCES];

int UI_PC_LoadSource( const char *file ) {
    int i;
    menuSource_t *src;

    for ( i = 0; i < MAX_MENU_SOURCES; i++ ) {
        if ( !sources[i].buffer ) {
            break;
        }
    }
    if ( i == MAX_MENU_SOURCES ) {
        return 0;
    }

    src = &sources[i];
    src->buffer = ( char * )uii.ReadFile( file, NULL );
    if ( !src->buffer ) {
        return 0;
    }
    src->cursor = src->buffer;
    src->line = 1;
    strncpy( src->filename, file, sizeof( src->filename ) - 1 );
    src->filename[sizeof( src->filename ) - 1] = '\0';
    return i + 1;                       /* handle 0 means failure */
}

int UI_PC_FreeSource( int handle ) {
    menuSource_t *src;

    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        return 0;
    }
    src = &sources[handle - 1];
    if ( !src->buffer ) {
        return 0;
    }
    free( src->buffer );
    src->buffer = NULL;
    src->cursor = NULL;
    src->line = 0;
    src->filename[0] = '\0';
    return 1;
}

int UI_PC_ReadToken( int handle, pc_token_t *token ) {
    menuSource_t *src;
    char *p;
    int line;
    int len = 0;

    memset( token, 0, sizeof( *token ) );
    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        return 0;
    }
    src = &sources[handle - 1];
    if ( !src->buffer ) {
        return 0;
    }

    p = src->cursor;
    line = src->line;

    /* skip whitespace and both comment kinds, counting lines on the way */
    while ( 1 ) {
        if ( !*p ) {
            src->cursor = p;
            src->line = line;
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

    src->line = line;
    src->cursor = p;

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
        src->cursor = p;
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
        src->cursor = p;
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
        src->cursor = p;
        return 1;
    }

    /* name */
    if ( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) || *p == '_' ) {
        while ( ( *p >= 'a' && *p <= 'z' ) || ( *p >= 'A' && *p <= 'Z' ) ||
                ( *p >= '0' && *p <= '9' ) || *p == '_' ) {
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token->string[len++] = *p;
            }
            p++;
        }
        token->string[len] = '\0';
        token->type = TT_NAME;
        src->cursor = p;
        return 1;
    }

    /* anything else is a single punctuation character */
    token->string[len++] = *p++;
    token->string[len] = '\0';
    token->type = TT_PUNCTUATION;
    src->cursor = p;
    return 1;
}

int UI_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
    menuSource_t *src;

    if ( handle < 1 || handle > MAX_MENU_SOURCES ) {
        filename[0] = '\0';
        *line = 0;
        return 0;
    }
    src = &sources[handle - 1];
    if ( !src->buffer ) {
        filename[0] = '\0';
        *line = 0;
        return 0;
    }
    strncpy( filename, src->filename, 127 );
    filename[127] = '\0';
    *line = src->line;
    return 1;
}

/* buffer-size-safe concat, like the idTech 3 Q_strcat */
void Q_strcat( char *dest, int size, const char *src ) {
    int l1 = strlen( dest );
    if ( l1 >= size ) {
        return;
    }
    Q_strncpyz( dest + l1, src, size - l1 );
}

/* size-safe copy, like the idTech 3 Q_strncpyz */
void Q_strncpyz( char *dest, const char *src, int destsize ) {
    if ( !dest || !src || destsize < 1 ) {
        return;
    }
    strncpy( dest, src, (size_t)( destsize - 1 ) );
    dest[destsize - 1] = '\0';
}

/* uppercase in place, like the idTech 3 Q_strupr */
char *Q_strupr( char *s1 ) {
    char *cp;
    for ( cp = s1; *cp; ++cp ) {
        if ( ( *cp >= 'a' ) && ( *cp <= 'z' ) ) {
            *cp += 'A' - 'a';
        }
    }
    return s1;
}

/* length-limited compare, like the idTech 3 Q_strncmp */
int Q_strncmp( const char *s1, const char *s2, int n ) {
    int c1, c2;
    do {
        c1 = *s1++;
        c2 = *s2++;
        if ( c1 != c2 ) {
            return c1 < c2 ? -1 : 1;
        }
    } while ( --n && c1 );
    return 0;
}

int Q_isalpha( int c ) {
    return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}

int Q_isupper( int c ) {
    return ( c >= 'A' && c <= 'Z' );
}

int Q_isnumeric( int c ) {
    return ( c >= '0' && c <= '9' );
}

/* filename-safe characters for the valid-file edit fields */
int Q_isforfilename( int c ) {
    return ( Q_isalpha( c ) || Q_isnumeric( c ) || c == '_' ) &&
           !( c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' ||
              c == '<' || c == '>' || c == '|' || c == ' ' );
}

/* builds the idTech 3 rotation matrix rows from angles; the engine shared
   AngleVectors keeps the convention consistent with the rest of the code */
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
    vec3_t forward, right, up;

    AngleVectors( ( float * )angles, forward, right, up );
    VectorCopy( forward, axis[0] );
    VectorNegate( right, axis[1] );  /* viewaxis[1] is the left vector */
    VectorCopy( up, axis[2] );
}

/* punctuation candidates, longest first so the longest match wins */
static const char *com_punctuations[] = {
    ">>>", "<<=", ">>=", "...",
    "->", "::", "&&", "||", "<=", ">=", "==", "!=", "+=", "-=", "*=", "/=",
    "&=", "|=", "++", "--",
    "(", ")", "{", "}", "[", "]", ",", ";", "<", ">", "+", "-", "*", "/",
    "=", "!", "%", "^", "&", "|", "#", ":", ".", "?", "@", "$", "~",
    NULL
};

/* tokenizer used by the plain-text parse helpers (Float_Parse etc),
   following the idTech 3 COM_ParseExt behavior */
char *COM_ParseExt( char **data_p, qboolean allowLineBreak ) {
    static char token[MAX_TOKENLENGTH];
    char *data;
    const char **punct;
    int len = 0;
    int c;

    token[0] = '\0';
    data = *data_p;
    if ( !data ) {
        *data_p = NULL;
        return token;
    }

skip:
    // skip whitespace and track line breaks when they are not allowed
    while ( ( c = *(const unsigned char *)data ) <= ' ' ) {
        if ( c == 0 ) {
            *data_p = NULL;
            return token;
        }
        if ( c == '\n' && !allowLineBreak ) {
            *data_p = data;
            return token;
        }
        data++;
    }

    // skip single-line comments
    if ( c == '/' && data[1] == '/' ) {
        while ( *data && *data != '\n' ) {
            data++;
        }
        goto skip;
    }

    // skip block comments
    if ( c == '/' && data[1] == '*' ) {
        data += 2;
        while ( *data && !( *data == '*' && data[1] == '/' ) ) {
            data++;
        }
        if ( *data ) {
            data += 2;
        }
        goto skip;
    }

    // double-quoted string with escaped quote support
    if ( c == '"' ) {
        data++;
        while ( 1 ) {
            c = *data;
            if ( c == '"' ) {
                data++;
                break;
            }
            if ( c == 0 ) {
                break;
            }
            if ( c == '\\' && data[1] == '"' ) {
                data++;
                c = '"';
            }
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token[len] = c;
                len++;
            }
            data++;
        }
        token[len] = '\0';
        *data_p = data;
        return token;
    }

    // single-quoted character literal
    if ( c == '\'' ) {
        data++;
        while ( 1 ) {
            c = *data;
            if ( c == '\'' ) {
                data++;
                break;
            }
            if ( c == 0 ) {
                break;
            }
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token[len] = c;
                len++;
            }
            data++;
        }
        token[len] = '\0';
        *data_p = data;
        return token;
    }

    // number, optionally signed, with a decimal point
    if ( ( c >= '0' && c <= '9' ) ||
         ( ( c == '-' || c == '+' ) && ( ( data[1] >= '0' && data[1] <= '9' ) || data[1] == '.' ) ) ||
         ( c == '.' && ( data[1] >= '0' && data[1] <= '9' ) ) ) {
        if ( c == '-' || c == '+' ) {
            token[len] = c;
            len++;
            data++;
        }
        while ( 1 ) {
            c = *data;
            if ( ( c < '0' || c > '9' ) && c != '.' ) {
                break;
            }
            if ( len < MAX_TOKENLENGTH - 1 ) {
                token[len] = c;
                len++;
            }
            data++;
        }
        token[len] = '\0';
        *data_p = data;
        return token;
    }

    // name: letters, digits and underscore after a leading letter or underscore
    if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' ) {
        while ( 1 ) {
            c = *data;
            if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                 ( c >= '0' && c <= '9' ) || c == '_' ) {
                if ( len < MAX_TOKENLENGTH - 1 ) {
                    token[len] = c;
                    len++;
                }
                data++;
            } else {
                break;
            }
        }
        token[len] = '\0';
        *data_p = data;
        return token;
    }

    // otherwise return the longest matching punctuation
    for ( punct = com_punctuations; *punct; ++punct ) {
        const char *p = *punct;
        int i;
        for ( i = 0; p[i]; i++ ) {
            if ( data[i] != p[i] ) {
                break;
            }
        }
        if ( !p[i] ) {
            for ( i = 0; p[i]; i++ ) {
                if ( len < MAX_TOKENLENGTH - 1 ) {
                    token[len] = p[i];
                    len++;
                }
            }
            data += i;
            token[len] = '\0';
            *data_p = data;
            return token;
        }
    }

    // unknown character: consume it so the parser cannot loop forever
    token[len] = c;
    len++;
    data++;
    token[len] = '\0';
    *data_p = data;
    return token;
}
