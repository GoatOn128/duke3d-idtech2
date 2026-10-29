/* Module support helpers for the ported idTech 3 menu framework: console
   print through the engine import table, the q_shared string/classify
   helpers the engine Q2 header lacks, and the tokenizer the framework
   expects. The helpers follow the RTCW q_shared semantics, reimplemented
   here for the module; the .menu parser itself stays behind stubs because
   the Duke menus are code-defined. */
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

/* .menu tokenizing is not wired up: reading a token always fails so the
   script-driven menu paths simply stay dormant */
int UI_PC_ReadToken( int handle, pc_token_t *token ) {
    ( void )handle;
    memset( token, 0, sizeof( *token ) );
    return 0;
}

int UI_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
    ( void )handle;
    filename[0] = '\0';
    *line = 0;
    return 0;
}

/* buffer-size-safe concat, like the idTech 3 Q_strcat */
void Q_strcat( char *dest, int size, const char *src ) {
    int l1 = strlen( dest );
    if ( l1 >= size ) {
        return;
    }
    Q_strncpyz( dest + l1, src, size - l1 );


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

    AngleVectors( (vec3_t)angles, forward, right, up );
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
