/* Duke video settings for the supported OpenGL renderer. */
#include "../client/client.h"
#include "../client/qmenu.h"

extern void M_PopMenu(void);
extern const char *Default_MenuKey(menuframework_s *menu, int key);

static menuframework_s video_menu;
static menulist_s mode, fullscreen, sync;
static menuslider_s screen_size, brightness, texture_quality;
static menuaction_s defaults, apply, cancel;

static const char *resolutions[] = {"320 x 240",  "400 x 300",   "512 x 384",  "640 x 480",
                                    "800 x 600",  "960 x 720",   "1024 x 768", "1152 x 864",
                                    "1280 x 960", "1600 x 1200", NULL};
static const char *yes_no[] = {"NO", "YES", NULL};

static void ResetDefaults(void *unused)
{
    mode.curvalue = 6;
    fullscreen.curvalue = 0;
    screen_size.curvalue = 10;
    brightness.curvalue = 8;
    texture_quality.curvalue = 3;
    sync.curvalue = 0;
}

static void ApplyChanges(void *unused)
{
    qboolean restart =
        Cvar_VariableValue("gl_mode") != mode.curvalue ||
        Cvar_VariableValue("vid_fullscreen") != fullscreen.curvalue ||
        Cvar_VariableValue("gl_picmip") != 3 - texture_quality.curvalue ||
        fabsf(Cvar_VariableValue("vid_gamma") - (1.8f - brightness.curvalue / 10)) > 0.01f;
    Cvar_SetValue("gl_mode", mode.curvalue);
    Cvar_SetValue("vid_fullscreen", fullscreen.curvalue);
    Cvar_SetValue("viewsize", screen_size.curvalue * 10);
    Cvar_SetValue("vid_gamma", 1.8f - brightness.curvalue / 10);
    Cvar_SetValue("gl_picmip", 3 - texture_quality.curvalue);
    Cvar_SetValue("gl_finish", sync.curvalue);
    Cvar_Set("vid_ref", "gl");
    Cvar_Set("gl_driver", "opengl32");
    Cvar_Get("gl_picmip", "0", CVAR_ARCHIVE);
    if (restart)
    {
        Cvar_Get("vid_ref", "gl", CVAR_ARCHIVE)->modified = true;
    }
    Com_Printf("Duke video applied: mode %d fullscreen %d brightness %.1f quality %.0f sync %d\n",
               mode.curvalue, fullscreen.curvalue, 1.8f - brightness.curvalue / 10,
               texture_quality.curvalue, sync.curvalue);
    M_PopMenu();
}

static void CancelChanges(void *unused)
{
    M_PopMenu();
}

static void AddChoice(menulist_s *item, const char *name, const char **choices, int value)
{
    memset(item, 0, sizeof(*item));
    item->generic.type = MTYPE_SPINCONTROL;
    item->generic.name = name;
    item->generic.y = video_menu.nitems * 14;
    item->itemnames = choices;
    item->curvalue = value;
    Menu_AddItem(&video_menu, item);
}

static void AddSlider(menuslider_s *item, const char *name, float minimum, float maximum,
                      float value)
{
    memset(item, 0, sizeof(*item));
    item->generic.type = MTYPE_SLIDER;
    item->generic.name = name;
    item->generic.y = video_menu.nitems * 14;
    item->minvalue = minimum;
    item->maxvalue = maximum;
    item->curvalue = DN_Max(minimum, DN_Min(maximum, value));
    Menu_AddItem(&video_menu, item);
}

static void AddAction(menuaction_s *item, const char *name, void (*callback)(void *))
{
    memset(item, 0, sizeof(*item));
    item->generic.type = MTYPE_ACTION;
    item->generic.name = name;
    item->generic.y = video_menu.nitems * 14 + 6;
    item->generic.callback = callback;
    Menu_AddItem(&video_menu, item);
}

void VID_MenuInit(void)
{
    memset(&video_menu, 0, sizeof(video_menu));
    video_menu.x = viddef.width / 2;
    video_menu.y = viddef.height / 2 - 65;
    AddChoice(&mode, "VIDEO MODE", resolutions, DN_Max(0, DN_Min(9, Cvar_VariableValue("gl_mode"))));
    AddChoice(&fullscreen, "FULLSCREEN", yes_no, !!Cvar_VariableValue("vid_fullscreen"));
    AddSlider(&screen_size, "SCREEN SIZE", 3, 12, Cvar_VariableValue("viewsize") / 10);
    AddSlider(&brightness, "BRIGHTNESS", 5, 13,
              roundf((1.8f - Cvar_VariableValue("vid_gamma")) * 10));
    AddSlider(&texture_quality, "TEXTURE QUALITY", 0, 3, 3 - Cvar_VariableValue("gl_picmip"));
    AddChoice(&sync, "SYNC EVERY FRAME", yes_no, !!Cvar_VariableValue("gl_finish"));
    AddAction(&defaults, "RESET TO DEFAULTS", ResetDefaults);
    AddAction(&apply, "APPLY CHANGES", ApplyChanges);
    AddAction(&cancel, "CANCEL", CancelChanges);
    video_menu.statusbar = "ARROWS TO ADJUST  ENTER OR CLICK TO SELECT  ESC TO CANCEL";
}

void VID_MenuDraw(void)
{
    /* A renderer restart can change the client dimensions while this menu is open. */
    video_menu.x = viddef.width / 2;
    video_menu.y = viddef.height / 2 - 65;
    DN_MenuText(160, 35, "VIDEO SETUP", 1);
    Menu_AdjustCursor(&video_menu, 1);
    Menu_Draw(&video_menu);
}

const char *VID_MenuKey(int key)
{
    return Default_MenuKey(&video_menu, key);
}
