/* Versioned boundary between the client and the replaceable front-end UI
   module, following the id Tech 3 UI interface (RTCW/ET ui_public.h). */
#ifndef DN_UI_H
#define DN_UI_H
#include "../game/q_shared.h"
#define DN_UI_API_VERSION 8

/* Command execution timings, mirroring the engine EXEC_* values. */
#define DN_EXEC_NOW 0
#define DN_EXEC_INSERT 1
#define DN_EXEC_APPEND 2

/* Menu sounds: the current menu.cpp select, move and back sounds. */
#define DN_UISOUND_SELECT 0
#define DN_UISOUND_MOVE 1
#define DN_UISOUND_BACK 2

/* Menus the engine can bring up, like uiMenuCommand_t in the id Tech 3 UI. */
typedef enum
{
    DN_UIMENU_NONE,
    DN_UIMENU_MAIN,
    DN_UIMENU_INGAME
} dn_uimenu_t;

/* Connection states, mirroring the client connstate_t without exposing it. */
typedef enum
{
    DN_CONN_DISCONNECTED,
    DN_CONN_CONNECTING,
    DN_CONN_CONNECTED,
    DN_CONN_ACTIVE
} dn_connstate_t;

typedef struct
{
    uint32_t version, size;
    void (*Print)(char *format, ...);
    void (*Error)(char *format, ...);
    int (*Milliseconds)(void);
    void (*Cvar_Register)(const char *name, const char *value, uint32_t flags);
    void (*Cvar_Set)(const char *name, const char *value);
    void (*Cvar_SetValue)(const char *name, float value);
    float (*Cvar_Value)(const char *name);
    void (*Cvar_String)(const char *name, char *buffer, uint32_t size);
    int (*Argc)(void);
    const char *(*Argv)(int index);
    void (*ExecuteText)(int when, const char *text);
    void *(*ReadFile)(const char *path, size_t *length);
    int (*Key_IsDown)(int key);
    void (*Key_SetBinding)(int key, const char *binding);
    void (*Key_GetBindingBuf)(int key, char *buffer, uint32_t size);
    void (*Key_KeynumToStringBuf)(int key, char *buffer, uint32_t size);
    int (*GetOverstrikeMode)(void);
    void (*SetOverstrikeMode)(int mode);
    void (*Key_ClearStates)(void);
    void (*GetVideoMode)(int *width, int *height);
    int (*GetConnState)(void);
    void *(*RegisterPic)(const char *name);
    void (*GetPicSize)(void *pic, int *width, int *height);
    void (*DrawStretchPic)(float x, float y, float w, float h, float s1, float t1, float s2, float t2, void *pic);
    void (*StartLocalSound)(const char *name);
    int (*IsLocalSoundPlaying)(const char *name);
    void (*UpdateScreen)(void);
    void (*UiTile)(float x, float y, int tile, int shade, int palette, int topLeft);
    int (*MenuText)(int x, int y, const char *text, int selected);
    void (*MenuCursor)(int x, int y);
    void (*MenuBackground)(void);
    void (*FadeScreen)(void);
    void (*MenuSound)(int sound);
    int (*SaveSlotInfo)(int slot, char *buffer, uint32_t size);
    int (*CanSave)(void);
    void (*GetLevelName)(char *buffer, uint32_t size);
    int (*ListMaps)(char *buffer, uint32_t size);
    int (*GetVideoModes)(int *widths, int *heights, int maxModes);
    int (*GetRefreshRates)(int mode, int *rates, int maxRates);
    int (*GetDisplays)(char *names, int stride, int maxDisplays);
    void (*PingServers)(void);
    int (*GetServers)(char *names, int stride, int maxServers);
    int (*GetServerAddress)(int index, char *buffer, uint32_t size);
    int (*GetCredits)(char *buffer, uint32_t size);
    int (*MenuTextScaled)(int x, int y, const char *text, int selected, float scale);
    int (*MenuTextWidth)(const char *text);
    /* menu cinematics (client/roq.cpp): the rect is the windowDef rect in
       the 640x480 virtual menu space */
    int (*PlayCinematic)(const char *name, float x, float y, float w, float h);
    void (*StopCinematic)(int handle);
    int (*RunCinematicFrame)(int handle);
    void (*DrawCinematic)(int handle, float x, float y, float w, float h);
} dn_ui_import_t;

typedef struct
{
    uint32_t version, size;
    int (*Init)(void);
    void (*Shutdown)(void);
    void (*KeyEvent)(int key, int down);
    void (*MouseEvent)(int x, int y);
    void (*Refresh)(int realtime);
    int (*IsFullscreen)(void);
    void (*SetActiveMenu)(dn_uimenu_t menu);
    void (*OpenMenu)(const char *name);
    int (*ConsoleCommand)(int realtime);
    void (*DrawLoadScreen)(void);
} dn_ui_export_t;

typedef dn_ui_export_t *(*dn_get_ui_api_t)(const dn_ui_import_t *imports);
#endif
