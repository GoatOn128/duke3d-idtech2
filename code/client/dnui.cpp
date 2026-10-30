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

static int UI_IsLocalSoundPlaying(const char *name)
{
    struct sfx_s *sfx = name && name[0] ? S_RegisterSound((char *)name) : NULL;
    return sfx ? S_IsSoundPlaying(sfx) : 0;
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

static int UI_MenuTextScaled(int x, int y, const char *text, int selected, float scale)
{
    return DN_MenuTextScaled(x, y, text, selected, scale);
}

static int UI_MenuTextWidth(const char *text)
{
    return DN_MenuTextWidth(text);
}

static void UI_MenuCursor(int x, int y)
{
    DN_MenuCursor(x, y);
}

static void UI_MenuBackground(void)
{
    DN_MenuBackground();
}

// Dim the rendered frame behind an in-game menu, the old menu.cpp behavior.
static void UI_FadeScreen(void)
{
    re.DrawFadeScreen();
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

// Save slot description: the first 32 bytes of server.ssv hold the text the
// engine save command wrote. Returns 1 when the slot holds a save.
static int UI_SaveSlotInfo(int slot, char *buffer, uint32_t size)
{
    char path[MAX_OSPATH];
    FILE *file;
    char desc[32];
    qboolean valid = false;

    if (!buffer || size == 0)
    {
        return 0;
    }
    buffer[0] = 0;
    if (slot < 0 || slot > 10)
    {
        return 0;
    }
    Com_sprintf(path, sizeof(path), "%s/save/save%d/server.ssv", FS_Gamedir(), slot);
    file = fopen(path, "rb");
    if (file)
    {
        valid = fread(desc, 1, sizeof(desc), file) == sizeof(desc) &&
                memchr(desc, 0, sizeof(desc)) != NULL;
        fclose(file);
    }
    if (valid)
    {
        for (uint32_t i = 0; i < sizeof(desc) && i < size; i++)
        {
            unsigned char c = (unsigned char)desc[i];
            buffer[i] = (c >= 32 && c <= 126) ? (char)c : ' ';
        }
        buffer[size - 1] = 0;
        return 1;
    }
    return 0;
}

static int UI_CanSave(void)
{
    // Same gate as the engine save menu: local server, in game, alive
    return Com_ServerState() && cls.state == ca_active &&
           !Cvar_VariableValue("deathmatch") &&
           cl.frame.playerstate.stats[STAT_HEALTH] > 0;
}

static void UI_GetLevelName(char *buffer, uint32_t size)
{
    if (!buffer || size == 0)
    {
        return;
    }
    buffer[0] = 0;
    if (cls.state == ca_active && cl.configstrings[CS_NAME])
    {
        strncpy(buffer, cl.configstrings[CS_NAME], size - 1);
        buffer[size - 1] = 0;
    }
}

// Map list from maps.lst: the buffer receives short\0long\0 pairs and the
// return value is the map count.
static int UI_ListMaps(char *buffer, uint32_t size)
{
    char *data = NULL, *p;
    int length;
    int count = 0;
    uint32_t used = 0;

    if (buffer && size)
    {
        buffer[0] = 0;
    }
    length = FS_LoadFile("maps.lst", (void **)&data);
    if (length <= 0 || !data)
    {
        return 0;
    }
    p = data;
    while (p - data < length && count < 255)
    {
        char shortname[MAX_TOKEN_CHARS];
        char longname[MAX_TOKEN_CHARS];
        uint32_t slen, llen;

        strncpy(shortname, COM_Parse(&p), sizeof(shortname) - 1);
        shortname[sizeof(shortname) - 1] = 0;
        if (!shortname[0])
        {
            break;
        }
        strncpy(longname, COM_Parse(&p), sizeof(longname) - 1);
        longname[sizeof(longname) - 1] = 0;
        slen = strlen(shortname) + 1;
        llen = strlen(longname) + 1;
        if (buffer && used + slen + llen <= size)
        {
            memcpy(buffer + used, shortname, slen);
            used += slen;
            memcpy(buffer + used, longname, llen);
            used += llen;
            count++;
        }
        else
        {
            break;
        }
    }
    FS_FreeFile(data);
    return count;
}

static int UI_GetVideoModes(int *widths, int *heights, int maxModes)
{
    int count = VID_NumModes();
    int i;

    if (count > maxModes)
    {
        count = maxModes;
    }
    for (i = 0; i < count; i++)
    {
        int w, h, hz;
        VID_ModeData(i, &w, &h, &hz);
        if (widths)
        {
            widths[i] = w;
        }
        if (heights)
        {
            heights[i] = h;
        }
    }
    return count;
}

static int UI_GetRefreshRates(int mode, int *rates, int maxRates)
{
    int count = VID_NumRefreshRates(mode);
    int i;

    if (count > maxRates)
    {
        count = maxRates;
    }
    for (i = 0; i < count; i++)
    {
        if (rates)
        {
            rates[i] = VID_RefreshRate(mode, i);
        }
    }
    return count;
}

static int UI_GetDisplays(char *names, int stride, int maxDisplays)
{
    int count = VID_NumDisplays();
    int i;

    if (count > maxDisplays)
    {
        count = maxDisplays;
    }
    for (i = 0; i < count; i++)
    {
        if (names && stride > 0)
        {
            strncpy(names + (size_t)i * stride, VID_DisplayName(i), stride - 1);
            names[(size_t)i * stride + stride - 1] = 0;
        }
    }
    return count;
}

/* LAN server list collected from the ping replies, replacing the
   M_AddToServerList store in menu.cpp. */
#define UI_MAX_SERVERS 8

static char ui_server_names[UI_MAX_SERVERS][80];
static netadr_t ui_server_adrs[UI_MAX_SERVERS];
static int ui_num_servers;

void UI_AddToServerList(netadr_t adr, char *info)
{
    int i;

    if (ui_num_servers == UI_MAX_SERVERS)
    {
        return;
    }
    while (*info == ' ')
    {
        info++;
    }
    for (i = 0; i < ui_num_servers; i++)
    {
        if (!strcmp(info, ui_server_names[i]))
        {
            return;
        }
    }
    ui_server_adrs[ui_num_servers] = adr;
    strncpy(ui_server_names[ui_num_servers], info, sizeof(ui_server_names[0]) - 1);
    ui_server_names[ui_num_servers][sizeof(ui_server_names[0]) - 1] = 0;
    ui_num_servers++;
}

static void UI_PingServers(void)
{
    ui_num_servers = 0;
    CL_PingServers_f();
}

static int UI_GetServers(char *names, int stride, int maxServers)
{
    int i;
    int count = ui_num_servers;

    if (count > maxServers)
    {
        count = maxServers;
    }
    for (i = 0; i < count; i++)
    {
        if (names && stride > 0)
        {
            strncpy(names + (size_t)i * stride, ui_server_names[i], stride - 1);
            names[(size_t)i * stride + stride - 1] = 0;
        }
    }
    return ui_num_servers;
}

static int UI_GetServerAddress(int index, char *buffer, uint32_t size)
{
    if (!buffer || size == 0)
    {
        return 0;
    }
    buffer[0] = 0;
    if (index < 0 || index >= ui_num_servers)
    {
        return 0;
    }
    strncpy(buffer, NET_AdrToString(ui_server_adrs[index]), size - 1);
    buffer[size - 1] = 0;
    return 1;
}

// Credits text: newline separated lines from the credits file, 0 when absent.
static int UI_GetCredits(char *buffer, uint32_t size)
{
    char *data = NULL;
    int length;
    int count = 0;
    uint32_t used = 0;
    const char *p, *line;

    if (buffer && size)
    {
        buffer[0] = 0;
    }
    length = FS_LoadFile("credits", (void **)&data);
    if (length <= 0 || !data)
    {
        return 0;
    }
    p = data;
    line = p;
    while (p - data < length && count < 255)
    {
        if (*p == '\r' || *p == '\n' || p - data == length - 1)
        {
            size_t llen = p - line;
            if (p - data == length - 1 && *p != '\r' && *p != '\n')
            {
                llen++;
            }
            if (*line == '+' && llen > 0)
            {
                line++;
                llen--;
            }
            if (buffer && used + llen + 1 <= size)
            {
                if (llen > 0)
                {
                    memcpy(buffer + used, line, llen);
                    used += llen;
                }
                buffer[used++] = '\n';
                count++;
            }
            if (*p == '\r' && p - data < length - 1 && p[1] == '\n')
            {
                p++;
            }
            line = p + 1;
        }
        p++;
    }
    if (buffer && used < size)
    {
        buffer[used] = 0;
    }
    FS_FreeFile(data);
    return count;
}

/* ---- menu entry points replacing the old client menu stack ---- */

/* Cooked character flag matching K_CHAR_FLAG in the module keycodes.h:
   the idTech 3 edit fields type through char-flagged printable keys. */
#define UI_CHAR_FLAG 256

void UI_KeyEvent(int key, qboolean down)
{
    if (DN_IntroActive())
    {
        if (down)
        {
            DN_IntroKey(key);
        }
        return;
    }
    if (!ui)
    {
        return;
    }
    if (key >= 32 && key < 127)
    {
        ui->KeyEvent(key | UI_CHAR_FLAG, down != 0);
    }
    else
    {
        ui->KeyEvent(key, down != 0);
    }
}

void UI_MouseMove(int x, int y)
{
    if (ui && cls.key_dest == key_menu)
    {
        ui->MouseEvent(x, y);
    }
}

void UI_Refresh(void)
{
    if (cls.key_dest != key_menu)
    {
        return;
    }
    if (DN_IntroActive())
    {
        DN_IntroDraw();
        return;
    }
    if (!ui)
    {
        return;
    }
    ui->Refresh(Sys_Milliseconds());
    // the module closes its own menus; follow it back out of menu mode
    if (!ui->IsFullscreen())
    {
        cls.key_dest = key_game;
    }
}

void UI_OpenMainMenu(void)
{
    if (ui)
    {
        ui->SetActiveMenu(DN_UIMENU_MAIN);
    }
    cls.key_dest = key_menu;
    // a disconnect stops the level music; bring back the menu theme
    if (cls.state == ca_disconnected)
    {
        S_MusicPlay("grabbag.mp3", true);
    }
}

void UI_ForceMenuOff(void)
{
    if (ui)
    {
        ui->SetActiveMenu(DN_UIMENU_NONE);
    }
    if (DN_IntroActive())
    {
        DN_IntroShutdown();
    }
    cls.key_dest = key_game;
    Key_ClearStates();
    Cvar_Set("paused", "0");
}

/* console commands the engine menu.cpp used to register, mapped onto the
   module menus like uiMenuCommand_t in the id Tech 3 UI */
typedef struct
{
    char *command;
    char *menu;
} ui_menu_command_t;

static const ui_menu_command_t ui_menu_commands[] = {
    {"menu_main", "main"},
    {"menu_game", "game_episodes"},
    {"menu_loadgame", "loadgame"},
    {"menu_savegame", "savegame"},
    {"menu_joinserver", "joinserver"},
    {"menu_addressbook", "addressbook"},
    {"menu_startserver", "startserver"},
    {"menu_dmoptions", "dmoptions"},
    {"menu_playerconfig", "playerconfig"},
    {"menu_downloadoptions", "downloadoptions"},
    {"menu_credits", "credits"},
    {"menu_multiplayer", "multiplayer"},
    {"menu_video", "video"},
    {"menu_options", "options"},
    {"menu_keys", "keys"},
    {"menu_quit", "quit"},
};

static void UI_MenuCommand_f(void)
{
    const char *cmd = Cmd_Argv(0);
    int i;

    for (i = 0; i < (int)(sizeof(ui_menu_commands) / sizeof(ui_menu_commands[0])); i++)
    {
        if (!strcmp(cmd, ui_menu_commands[i].command))
        {
            if (!strcmp(ui_menu_commands[i].menu, "main"))
            {
                UI_OpenMainMenu();
            }
            else
            {
                if (ui && ui->OpenMenu)
                {
                    ui->OpenMenu(ui_menu_commands[i].menu);
                }
                cls.key_dest = key_menu;
            }
            return;
        }
    }
}

static void UI_MenuInfo_f(void)
{
    Com_Printf("Duke menu: input %d module %d\n", cls.key_dest,
               ui && ui->IsFullscreen() ? 1 : 0);
}

static void UI_QuickSave_f(void)
{
    int slot = (int)Cvar_VariableValue("dn_save_slot");

    if (slot < 1 || slot > 10)
    {
        slot = 1;
    }
    if (!Com_ServerState() || cls.state != ca_active || Cvar_VariableValue("deathmatch") ||
        cl.frame.playerstate.stats[STAT_HEALTH] <= 0)
    {
        Com_Printf("Save requires a living player in a local non-deathmatch game.\n");
        return;
    }
    Cbuf_InsertText(va("save save%d \"%s\"\n", slot, cl.configstrings[CS_NAME]));
}

static void UI_QuickLoad_f(void)
{
    int slot = (int)Cvar_VariableValue("dn_save_slot");

    if (slot >= 1 && slot <= 10)
    {
        Cbuf_InsertText(va("load save%d\n", slot));
        return;
    }
    // no remembered slot: open the load menu instead of failing silently
    if (ui && ui->OpenMenu)
    {
        ui->OpenMenu("loadgame");
    }
    cls.key_dest = key_menu;
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
    imports.IsLocalSoundPlaying = UI_IsLocalSoundPlaying;
    imports.UpdateScreen = UI_UpdateScreen;
    imports.UiTile = UI_UiTile;
    imports.MenuText = UI_MenuText;
    imports.MenuTextScaled = UI_MenuTextScaled;
    imports.MenuTextWidth = UI_MenuTextWidth;
    imports.MenuCursor = UI_MenuCursor;
    imports.MenuBackground = UI_MenuBackground;
    imports.FadeScreen = UI_FadeScreen;
    imports.MenuSound = UI_MenuSound;
    imports.SaveSlotInfo = UI_SaveSlotInfo;
    imports.CanSave = UI_CanSave;
    imports.GetLevelName = UI_GetLevelName;
    imports.ListMaps = UI_ListMaps;
    imports.GetVideoModes = UI_GetVideoModes;
    imports.GetRefreshRates = UI_GetRefreshRates;
    imports.GetDisplays = UI_GetDisplays;
    imports.PingServers = UI_PingServers;
    imports.GetServers = UI_GetServers;
    imports.GetServerAddress = UI_GetServerAddress;
    imports.GetCredits = UI_GetCredits;
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
    int i;

    CL_LoadUI();
    // the commands menu.cpp and dnsavemenu.cpp used to register
    Cvar_Get("dn_save_slot", "-1", CVAR_ARCHIVE);
    Cmd_AddCommand("quicksave", UI_QuickSave_f);
    Cmd_AddCommand("quickload", UI_QuickLoad_f);
    Cmd_AddCommand("dn_menuinfo", UI_MenuInfo_f);
    Cmd_AddCommand("dn_intro", DN_IntroStart);
    for (i = 0; i < (int)(sizeof(ui_menu_commands) / sizeof(ui_menu_commands[0])); i++)
    {
        Cmd_AddCommand(ui_menu_commands[i].command, UI_MenuCommand_f);
    }
}
