/* Front-end UI module entry. The id Tech 3 menu framework and the Duke
   menus are ported on top of this boundary; the bodies below are the
   placeholders until then. */
#include "ui.h"

static dn_ui_import_t ui;
static dn_ui_export_t exports;

static int Init(void)
{
    ui.Print("Duke UI module loaded\n");
    return 1;
}

static void Shutdown(void)
{
}

static void KeyEvent(int, int)
{
}

static void MouseEvent(int, int)
{
}

static void Refresh(int)
{
}

static int IsFullscreen(void)
{
    return 0;
}

static void SetActiveMenu(dn_uimenu_t)
{
}

static int ConsoleCommand(int)
{
    return 0;
}

extern "C" __declspec(dllexport) dn_ui_export_t *GetUIAPI(const dn_ui_import_t *imports)
{
    if (!imports || imports->version != DN_UI_API_VERSION || imports->size != sizeof(*imports) ||
        !imports->Print || !imports->Error)
    {
        return NULL;
    }
    ui = *imports;
    exports.version = DN_UI_API_VERSION;
    exports.size = sizeof(exports);
    exports.Init = Init;
    exports.Shutdown = Shutdown;
    exports.KeyEvent = KeyEvent;
    exports.MouseEvent = MouseEvent;
    exports.Refresh = Refresh;
    exports.IsFullscreen = IsFullscreen;
    exports.SetActiveMenu = SetActiveMenu;
    exports.ConsoleCommand = ConsoleCommand;
    return &exports;
}
