/* Loading, engine services and trap implementations for the replaceable
   front-end UI DLL. The traps map the id Tech 3 UI call set onto the Quake 2
   client; the DN_ traps expose the Duke menu drawing helpers so the module
   can reproduce the current menu look. */
#include "client.h"
#include "../ui/ui.h"
#include "../qcommon/dneditfield.h"
#include <windows.h>

static HMODULE library;
static dn_ui_export_t *ui;

// defined in keys.cpp, not exposed through keys.h
extern qboolean keydown[256];

static void Error(char *format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    Com_Error(ERR_DROP, "%s", text);
}

static int UI_Milliseconds(void)
{
    return Sys_Milliseconds();
}

static void UI_Cvar_Register(const char *name, const char *value, uint32_t flags)
{
    Cvar_Get((char *)name, (char *)value, (int)flags);
}

static void UI_Cvar_Set(const char *name, const char *value)
{
    Cvar_Set((char *)name, (char *)value);
}

static void UI_Cvar_SetValue(const char *name, float value)
{
    Cvar_SetValue((char *)name, value);
}

static float UI_Cvar_Value(const char *name)
{
    return Cvar_VariableValue((char *)name);
}

static void UI_Cvar_String(const char *name, char *buffer, uint32_t size)
{
    if (!buffer || size == 0)
    {
        return;
    }
    strncpy(buffer, Cvar_VariableString((char *)name), size - 1);
    buffer[size - 1] = 0;
}

static int UI_Argc(void)
{
    return Cmd_Argc();
}

static const char *UI_Argv(int index)
{
    return Cmd_Argv(index);
}

static void UI_ExecuteText(int when, const char *text)
{
    Cbuf_ExecuteText(when, (char *)text);
}

// The buffer is a private malloc copy; the module frees it with free().
static void *UI_ReadFile(const char *path, size_t *length)
{
    void *data = NULL;
    int size = FS_LoadFile((char *)path, &data);
    if (size < 0 || !data)
    {
        return NULL;
    }
    char *copy = (char *)malloc((size_t)size + 1);
    if (!copy)
    {
        FS_FreeFile(data);
        return NULL;
    }
    memcpy(copy, data, size);
    copy[size] = 0;
    if (length)
    {
        *length = (size_t)size;
    }
    FS_FreeFile(data);
    return copy;
}

static int UI_Key_IsDown(int key)
{
    return key >= 0 && key < 256 && keydown[key];
}

static void UI_Key_SetBinding(int key, const char *binding)
{
    if (key >= 0 && key < 256)
    {
        Key_SetBinding(key, (char *)binding);
    }
}

static void UI_Key_GetBindingBuf(int key, char *buffer, uint32_t size)
{
    if (!buffer || size == 0)
    {
        return;
    }
    buffer[0] = 0;
    if (key >= 0 && key < 256 && keybindings[key])
    {
        strncpy(buffer, keybindings[key], size - 1);
        buffer[size - 1] = 0;
    }
}

static void UI_Key_KeynumToStringBuf(int key, char *buffer, uint32_t size)
{
    if (!buffer || size == 0)
    {
        return;
    }
    strncpy(buffer, Key_KeynumToString(key), size - 1);
    buffer[size - 1] = 0;
}

static int UI_GetOverstrikeMode(void)
{
    return DN_GetOverstrikeMode();
}

static void UI_SetOverstrikeMode(int mode)
{
    DN_SetOverstrikeMode(mode != 0);
}

static void UI_Key_ClearStates(void)
{
    Key_ClearStates();
}

static void UI_GetVideoMode(int *width, int *height)
{
    if (width)
    {
        *width = viddef.width;
    }
    if (height)
    {
        *height = viddef.height;
    }
}

static int UI_GetConnState(void)
{
    switch (cls.state)
    {
    case ca_connecting:
        return DN_CONN_CONNECTING;
    case ca_connected:
        return DN_CONN_CONNECTED;
    case ca_active:
        return DN_CONN_ACTIVE;
    default:
        return DN_CONN_DISCONNECTED;
    }
}

// Pic handles keep the name because the Q2 2D draw calls are name based.
typedef struct
{
    char name[MAX_QPATH];
} ui_pic_t;

static void *UI_RegisterPic(const char *name)
{
    if (!name || !name[0])
    {
        return NULL;
    }
    ui_pic_t *pic = (ui_pic_t *)malloc(sizeof(ui_pic_t));
    if (!pic)
    {
        return NULL;
    }
    strncpy(pic->name, name, sizeof(pic->name) - 1);
    pic->name[sizeof(pic->name) - 1] = 0;
    re.RegisterPic(pic->name); // cache so the first draw does not hitch
    return pic;
}

static void UI_GetPicSize(void *pic, int *width, int *height)
{
    int w = 0, h = 0;
    if (pic)
    {
        re.DrawGetPicSize(&w, &h, ((ui_pic_t *)pic)->name);
    }
    if (width)
    {
        *width = w;
    }
    if (height)
    {
        *height = h;
    }
}

static void UI_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2,
                             float t2, void *pic)
{
    // The Q2 2D layer has no sub-rect or color support yet, so whole-pic
    // draws cover every current menu use; s1..t2 arrive when the renderer
    // grows them.
    (void)s1;
    (void)t1;
    (void)s2;
    (void)t2;
    if (!pic)
    {
        return;
    }
    re.DrawStretchPic((int)x, (int)y, (int)(w + 0.5f), (int)(h + 0.5f), ((ui_pic_t *)pic)->name);
}

static void UI_StartLocalSound(const char *name)
{
    if (name && name[0])
    {
        S_StartLocalSound(name);
    }
}

static void UI_UpdateScreen(void)
{
    SCR_UpdateScreen();
}

static void UI_UiTile(float x, float y, int tile, int shade, int palette, int topLeft)
{
    DN_DrawUiTile(x, y, tile, shade, palette, topLeft);
}

static int UI_MenuText(int x, int y, const char *text, int selected)
{
    return DN_MenuText(x, y, text, selected);
}

static void UI_MenuCursor(int x, int y)
{
    DN_MenuCursor(x, y);
}

static void UI_MenuBackground(void)
{
    DN_MenuBackground();
}

static void UI_MenuSound(int sound)
{
    // The three menu sounds from menu.cpp, reachable by the module
    static const char *sounds[] = {"kickhit.wav", "bulithit.wav", "item15.wav"};
    if (sound >= 0 && (size_t)sound < sizeof(sounds) / sizeof(sounds[0]))
    {
        S_StartLocalSound(sounds[sound]);
    }
}

void CL_UnloadUI(void)
{
    if (ui)
    {
        ui->Shutdown();
        ui = NULL;
    }
    if (library)
    {
        FreeLibrary(library);
        library = NULL;
    }
}

dn_ui_export_t *CL_LoadUI(void)
{
    if (ui)
    {
        return ui;
    }
    char name[MAX_OSPATH];
    for (char *path = FS_NextPath(NULL); path; path = FS_NextPath(path))
    {
        Com_sprintf(name, sizeof(name), "%s/uix86.dll", path);
        library = LoadLibraryA(name);
        if (library)
        {
            break;
        }
    }
    if (!library)
    {
        Com_Error(ERR_DROP, "Cannot load basedn/uix86.dll");
    }
    dn_get_ui_api_t get_api = (dn_get_ui_api_t)GetProcAddress(library, "GetUIAPI");
    dn_ui_import_t imports;
    memset(&imports, 0, sizeof(imports));
    imports.version = DN_UI_API_VERSION;
    imports.size = sizeof(imports);
    imports.Print = Com_Printf;
    imports.Error = Error;
    imports.Milliseconds = UI_Milliseconds;
    imports.Cvar_Register = UI_Cvar_Register;
    imports.Cvar_Set = UI_Cvar_Set;
    imports.Cvar_SetValue = UI_Cvar_SetValue;
    imports.Cvar_Value = UI_Cvar_Value;
    imports.Cvar_String = UI_Cvar_String;
    imports.Argc = UI_Argc;
    imports.Argv = UI_Argv;
    imports.ExecuteText = UI_ExecuteText;
    imports.ReadFile = UI_ReadFile;
    imports.Key_IsDown = UI_Key_IsDown;
    imports.Key_SetBinding = UI_Key_SetBinding;
    imports.Key_GetBindingBuf = UI_Key_GetBindingBuf;
    imports.Key_KeynumToStringBuf = UI_Key_KeynumToStringBuf;
    imports.GetOverstrikeMode = UI_GetOverstrikeMode;
    imports.SetOverstrikeMode = UI_SetOverstrikeMode;
    imports.Key_ClearStates = UI_Key_ClearStates;
    imports.GetVideoMode = UI_GetVideoMode;
    imports.GetConnState = UI_GetConnState;
    imports.RegisterPic = UI_RegisterPic;
    imports.GetPicSize = UI_GetPicSize;
    imports.DrawStretchPic = UI_DrawStretchPic;
    imports.StartLocalSound = UI_StartLocalSound;
    imports.UpdateScreen = UI_UpdateScreen;
    imports.UiTile = UI_UiTile;
    imports.MenuText = UI_MenuText;
    imports.MenuCursor = UI_MenuCursor;
    imports.MenuBackground = UI_MenuBackground;
    imports.MenuSound = UI_MenuSound;
    ui = get_api ? get_api(&imports) : NULL;
    if (!ui || ui->version != DN_UI_API_VERSION || ui->size != sizeof(*ui))
    {
        ui = NULL;
        CL_UnloadUI();
        Com_Error(ERR_DROP, "Invalid UI API");
    }
    if (!ui->Init())
    {
        CL_UnloadUI();
        Com_Error(ERR_DROP, "UI module init failed");
    }
    Com_Printf("Duke UI library: %s\n", name);
    return ui;
}

void CL_InitUI(void)
{
    CL_LoadUI();
}
