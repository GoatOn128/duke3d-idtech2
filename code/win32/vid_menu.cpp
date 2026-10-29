/* Duke video settings for the supported OpenGL renderer. The mode, refresh
rate and display entries come from the lists the client discovers in
sdl_vid.cpp (the Doom 3 BFG model), so what the spinners show is what the
display can actually do. Picking the refresh rate separately from the
resolution is a deliberate deviation from BFG, where the rate is baked
into the mode entry. */
#include "../client/client.h"
#include "../client/qmenu.h"

extern void M_PopMenu(void);
extern const char *Default_MenuKey(menuframework_s *menu, int key);

/* Local caps for the menu copies of the client lists. Must stay in sync
with VID_MAX_MODES in sdl_vid.cpp so the spinner can show every entry. */
#define MENU_MAX_MODES 256
#define MENU_MAX_DISPLAYS 16
#define MENU_MAX_RATES 16

static menuframework_s video_menu;
static menulist_s mode, refresh, fullscreen, sync, display;
static menuslider_s screen_size, brightness, texture_quality;
static menuaction_s defaults, apply, cancel;

static const char *yes_no[] = {"NO", "YES", NULL};

static char mode_names[MENU_MAX_MODES][32];
static const char *mode_choices[MENU_MAX_MODES + 1];
static const char *display_choices[MENU_MAX_DISPLAYS + 1];
// refresh spinner: "auto" plus one entry per rate of the selected mode
static char refresh_names[MENU_MAX_RATES][16];
static const char *refresh_choices[MENU_MAX_RATES + 2];
static int refresh_values[MENU_MAX_RATES + 1];

/*
BuildModeList

One label per resolution of the discovered mode list. The refresh rate
has its own spinner, so the label stays width x height.
*/
static void BuildModeList(void)
{
    int i;
    int width, height, hz;
    int count = VID_NumModes();

    if (count > MENU_MAX_MODES)
    {
        count = MENU_MAX_MODES;
    }
    for (i = 0; i < count; i++)
    {
        VID_ModeData(i, &width, &height, &hz);
        Com_sprintf(mode_names[i], sizeof(mode_names[i]), "%i x %i", width, height);
        mode_choices[i] = mode_names[i];
    }
    mode_choices[count] = NULL;
}

static void BuildDisplayList(void)
{
    int i;
    int count = VID_NumDisplays();

    if (count > MENU_MAX_DISPLAYS)
    {
        count = MENU_MAX_DISPLAYS;
    }
    for (i = 0; i < count; i++)
    {
        display_choices[i] = VID_DisplayName(i);
    }
    display_choices[count] = NULL;
}

/*
BuildRefreshList

Fills the refresh spinner for the resolution the mode spinner sits on:
"auto" (use the fastest rate the entry carries) plus every rate the
display offers at that size. A rate picked on another resolution that
this one cannot do falls back to auto.
*/
static void BuildRefreshList(int modeIndex)
{
    int i;
    int count = VID_NumRefreshRates(modeIndex);

    if (count > MENU_MAX_RATES)
    {
        count = MENU_MAX_RATES;
    }
    refresh_choices[0] = "AUTO";
    refresh_values[0] = 0;
    for (i = 0; i < count; i++)
    {
        refresh_values[i + 1] = VID_RefreshRate(modeIndex, i);
        Com_sprintf(refresh_names[i], sizeof(refresh_names[i]), "%i HZ", refresh_values[i + 1]);
        refresh_choices[i + 1] = refresh_names[i];
    }
    refresh_choices[count + 1] = NULL;

    refresh.curvalue = 0;
    for (i = 1; i <= count; i++)
    {
        if (refresh_values[i] == (int)Cvar_VariableValue("vid_displayrefresh"))
        {
            refresh.curvalue = i;
            break;
        }
    }
}

// the mode spinner moved: rebuild the refresh list for the new resolution
static void ModeChanged(void *self)
{
    (void)self;
    BuildRefreshList(mode.curvalue);
}

static int ClampMode(int value)
{
    int count = VID_NumModes();

    if (count > MENU_MAX_MODES)
    {
        count = MENU_MAX_MODES;
    }
    if (count == 0 || value < 0)
    {
        return 0;
    }
    if (value >= count)
    {
        // mirror the client reset: an off-list gl_mode lands on the
        // default mode, not on the largest entry
        return count > 3 ? 3 : count - 1;
    }
    return value;
}

static int ClampDisplay(int value)
{
    if (VID_NumDisplays() == 0 || value < 0)
    {
        return 0;
    }
    if (value >= VID_NumDisplays())
    {
        return VID_NumDisplays() - 1;
    }
    return value;
}

static void ResetDefaults(void *unused)
{
    mode.curvalue = ClampMode(3);
    BuildRefreshList(mode.curvalue);
    fullscreen.curvalue = 0;
    display.curvalue = ClampDisplay(0);
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
        Cvar_VariableValue("vid_display") != display.curvalue ||
        (fullscreen.curvalue &&
         Cvar_VariableValue("vid_displayrefresh") != refresh_values[refresh.curvalue]) ||
        Cvar_VariableValue("gl_picmip") != 3 - texture_quality.curvalue ||
        fabsf(Cvar_VariableValue("vid_gamma") - (1.8f - brightness.curvalue / 10)) > 0.01f;
    Cvar_SetValue("gl_mode", mode.curvalue);
    Cvar_SetValue("vid_fullscreen", fullscreen.curvalue);
    Cvar_SetValue("vid_display", display.curvalue);
    Cvar_SetValue("vid_displayrefresh", refresh_values[refresh.curvalue]);
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
    Com_Printf("Duke video applied: mode %d refresh %d fullscreen %d display %d brightness %.1f quality %.0f sync %d\n",
               mode.curvalue, refresh_values[refresh.curvalue], fullscreen.curvalue, display.curvalue,
               1.8f - brightness.curvalue / 10, texture_quality.curvalue, sync.curvalue);
    M_PopMenu();
}

static void CancelChanges(void *unused)
{
    M_PopMenu();
}

static void AddChoice(menulist_s *item, const char *name, const char **choices, int value,
                      void (*callback)(void *))
{
    memset(item, 0, sizeof(*item));
    item->generic.type = MTYPE_SPINCONTROL;
    item->generic.name = name;
    item->generic.y = video_menu.nitems * 14;
    item->itemnames = choices;
    item->curvalue = value;
    item->generic.callback = callback;
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
    BuildModeList();
    BuildDisplayList();
    mode.curvalue = ClampMode(Cvar_VariableValue("gl_mode"));
    BuildRefreshList(mode.curvalue);
    AddChoice(&mode, "VIDEO MODE", mode_choices, mode.curvalue, ModeChanged);
    AddChoice(&refresh, "REFRESH RATE", refresh_choices, refresh.curvalue, NULL);
    AddChoice(&fullscreen, "FULLSCREEN", yes_no, !!Cvar_VariableValue("vid_fullscreen"), NULL);
    AddChoice(&display, "DISPLAY", display_choices, ClampDisplay(Cvar_VariableValue("vid_display")), NULL);
    AddSlider(&screen_size, "SCREEN SIZE", 3, 12, Cvar_VariableValue("viewsize") / 10);
    AddSlider(&brightness, "BRIGHTNESS", 5, 13,
              roundf((1.8f - Cvar_VariableValue("vid_gamma")) * 10));
    AddSlider(&texture_quality, "TEXTURE QUALITY", 0, 3, 3 - Cvar_VariableValue("gl_picmip"));
    AddChoice(&sync, "SYNC EVERY FRAME", yes_no, !!Cvar_VariableValue("gl_finish"), NULL);
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
