/* Loading and engine services for the replaceable front-end UI DLL. */
#include "client.h"
#include "../ui/ui.h"
#include <windows.h>

static HMODULE library;
static dn_ui_export_t *ui;

static void Error(char *format, ...)
{
    char text[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    Com_Error(ERR_DROP, "%s", text);
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
    dn_ui_import_t imports = {DN_UI_API_VERSION, sizeof(imports), Com_Printf, Error};
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
