/* Module-private UI declarations. The whole module calls the engine
   import table directly through uii (like the cgame module with cg); there
   is no trap wrapper layer. */
#ifndef DN_UI_LOCAL_H
#define DN_UI_LOCAL_H
#include "ui.h"
#include "ui_shared.h"

extern dn_ui_import_t uii;

/* module-local replacements for the idTech 3 shared helpers the menu
   framework expects but the engine Q2 q_shared lacks */
void    UI_Printf( const char *fmt, ... );
int     UI_PC_ReadToken( int handle, pc_token_t *token );
int     UI_PC_SourceFileAndLine( int handle, char *filename, int *line );
void    Q_strcat( char *dest, int size, const char *src );
char    *Q_strupr( char *s1 );
int     Q_strncmp( const char *s1, const char *s2, int n );
int     Q_isupper( int c );
int     Q_isnumeric( int c );
int     Q_isforfilename( int c );
char    *COM_ParseExt( char **data_p, qboolean allowLineBreak );
void    AnglesToAxis( const vec3_t angles, vec3_t axis[3] );

#endif
