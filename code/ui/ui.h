/* Versioned boundary between the client and the replaceable front-end UI
   module, following the id Tech 3 UI interface (RTCW/ET ui_public.h). */
#ifndef DN_UI_H
#define DN_UI_H
#include "../game/q_shared.h"
#define DN_UI_API_VERSION 1

/* Menus the engine can bring up, like uiMenuCommand_t in the id Tech 3 UI. */
typedef enum
{
    DN_UIMENU_NONE,
    DN_UIMENU_MAIN,
    DN_UIMENU_INGAME
} dn_uimenu_t;

typedef struct
{
    uint32_t version, size;
    void (*Print)(char *format, ...);
    void (*Error)(char *format, ...);
} dn_ui_import_t;

typedef struct
{
    uint32_t version, size;
    int (*Init)(void);
    void (*Shutdown)(void);
    void (*KeyEvent)(int key, int down);
    void (*MouseEvent)(int dx, int dy);
    void (*Refresh)(int realtime);
    int (*IsFullscreen)(void);
    void (*SetActiveMenu)(dn_uimenu_t menu);
    int (*ConsoleCommand)(int realtime);
} dn_ui_export_t;

typedef dn_ui_export_t *(*dn_get_ui_api_t)(const dn_ui_import_t *imports);
#endif
