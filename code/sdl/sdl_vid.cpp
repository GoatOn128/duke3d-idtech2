/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sdl_vid.cpp -- client video layer: discovers displays and video modes
// (the Doom 3 BFG model), loads the refresh DLL and tracks the video cvars.
// Window, context and fullscreen all live inside the refresh through SDL
// now, so there is no window procedure here anymore.
#include <assert.h>
#include <float.h>

#include "../client/client.h"
#include "../qcommon/dnjobs.h"
#include "sdl_sys.h"

#include <windows.h>
#include <SDL3/SDL_video.h>

// Structure containing functions exported from refresh DLL
refexport_t re;

// Console variables that we need to access from this module
cvar_t *vid_gamma;
cvar_t *vid_ref; // Name of Refresh DLL loaded
cvar_t *vid_xpos; // X coordinate of window position
cvar_t *vid_ypos; // Y coordinate of window position
cvar_t *vid_fullscreen;
cvar_t *vid_display;        // 0-based index of the display to fullscreen on
cvar_t *vid_displayrefresh; // hz to force in fullscreen; 0 = use the mode list value
cvar_t *vid_customWidth;    // custom width used when gl_mode is -1
cvar_t *vid_customHeight;   // custom height used when gl_mode is -1

// Global variables used internally by this module
viddef_t viddef;          // global video state; used by other modules
HINSTANCE reflib_library; // Handle to refresh DLL
qboolean reflib_active = 0;

/*
==========================================================================

DLL GLUE

==========================================================================
*/

#define MAXPRINTMSG 4096

void VID_Printf(int print_level, char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    if (print_level == PRINT_ALL)
    {
        Com_Printf("%s", msg);
    }
    else if (print_level == PRINT_DEVELOPER)
    {
        Com_DPrintf("%s", msg);
    }
    else if (print_level == PRINT_ALERT)
    {
        MessageBox(0, msg, "PRINT_ALERT", MB_ICONWARNING);
        OutputDebugString(msg);
    }
}

void VID_Error(int err_level, char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    Com_Error(err_level, "%s", msg);
}

//==========================================================================

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f(void)
{
    vid_ref->modified = true;
}

void VID_Front_f(void)
{
    Sys_AppActivate();
}

/*
==========================================================================

DISPLAY AND MODE DISCOVERY

This is the SDL port of the Doom 3 BFG display handling: the client owns
the mode list (R_GetModeListForDisplay there, VID_BuildModeList here) and
the refresh asks for modes through the ref API. Deliberate divergences
from BFG are noted at the spots where they happen.

==========================================================================
*/

// SDL hands the modes back largest-first, so this must be able to hold
// the whole list: a cap that is too small keeps only the largest modes
// and pushes the default window past the screen size
#define VID_MAX_MODES 256
#define VID_MAX_DISPLAYS 16
// one entry now holds every rate the display offers at its size, so this
// must cover the whole rate spread of a TV (60 down to 24)
#define VID_MAX_RATES 16

typedef struct vidmode_s
{
    int width, height;
    int displayHz;
    int rates[VID_MAX_RATES];
    int numRates;
} vidmode_t;

static vidmode_t vid_modes[VID_MAX_MODES];
static int vid_num_modes;

static SDL_DisplayID vid_display_ids[VID_MAX_DISPLAYS];
static char vid_display_names[VID_MAX_DISPLAYS][64];
static int vid_num_displays;
static int vid_desktop_width;    // desktop size of the display the mode
static int vid_desktop_height;   // list was built for, for the fit check
static qboolean vid_windowed_checked;

/*
============
VID_ClampDisplay
============
*/
static int VID_ClampDisplay(void)
{
    int display = (int)vid_display->value;

    if (display < 0)
    {
        display = 0;
    }
    if (vid_num_displays > 0 && display > vid_num_displays - 1)
    {
        display = vid_num_displays - 1;
    }
    return display;
}

/*
============
VID_BuildDisplayList

Enumerates the connected displays once. The vid_display cvar indexes into
this list, mirroring the display number of r_fullscreen in Doom 3 BFG.
============
*/
static void VID_BuildDisplayList(void)
{
    int count = 0;
    SDL_DisplayID *ids = SDL_GetDisplays(&count);
    int i;

    vid_num_displays = 0;

    if (!ids)
    {
        return;
    }

    for (i = 0; i < count && vid_num_displays < VID_MAX_DISPLAYS; i++)
    {
        const char *name = SDL_GetDisplayName(ids[i]);

        vid_display_ids[vid_num_displays] = ids[i];
        if (name && name[0])
        {
            SDL_strlcpy(vid_display_names[vid_num_displays], name,
                        sizeof(vid_display_names[0]));
        }
        else
        {
            Com_sprintf(vid_display_names[vid_num_displays],
                        sizeof(vid_display_names[0]), "Display %i", vid_num_displays);
        }
        vid_num_displays++;
    }

    SDL_free(ids);
}

/*
============
VID_FindResolution

Keep one entry per resolution (BFG AddUnique kept its list short the
same way by only allowing a couple of rates): a list holding the same
size at several refresh rates makes the menu spinner step through
entries that look identical and change nothing while windowed.
============
*/
static qboolean VID_FindResolution(int width, int height, int *index)
{
    int i;

    for (i = 0; i < vid_num_modes; i++)
    {
        if (vid_modes[i].width == width && vid_modes[i].height == height)
        {
            *index = i;
            return true;
        }
    }
    return false;
}

// 0 = rate not discovered (the windowed-only standard sizes carry it),
// and the rate list never stores it
static void VID_AddRate(vidmode_t *mode, int hz)
{
    int i;

    if (hz <= 0)
    {
        return;
    }
    for (i = 0; i < mode->numRates; i++)
    {
        if (mode->rates[i] == hz)
        {
            return;
        }
    }
    if (mode->numRates < VID_MAX_RATES)
    {
        mode->rates[mode->numRates++] = hz;
    }
}

static void VID_AddMode(int width, int height, int hz)
{
    int index;

    // same resolution again: keep the fastest rate on the entry and every
    // rate the display offers at this size so the menu can pick one
    if (VID_FindResolution(width, height, &index))
    {
        if (hz > vid_modes[index].displayHz)
        {
            vid_modes[index].displayHz = hz;
        }
        VID_AddRate(&vid_modes[index], hz);
        return;
    }
    if (vid_num_modes >= VID_MAX_MODES)
    {
        return;
    }
    vid_modes[vid_num_modes].width = width;
    vid_modes[vid_num_modes].height = height;
    vid_modes[vid_num_modes].displayHz = hz;
    vid_modes[vid_num_modes].numRates = 0;
    VID_AddRate(&vid_modes[vid_num_modes], hz);
    vid_num_modes++;
}

/*
============
VID_ModeCompare

Sort like the BFG idSort_VidMode: lowest resolution first, then the
refresh rate.
============
*/
static int VID_ModeCompare(const void *a, const void *b)
{
    const vidmode_t *ma = (const vidmode_t *)a;
    const vidmode_t *mb = (const vidmode_t *)b;

    if (ma->height != mb->height)
    {
        return ma->height < mb->height ? -1 : 1;
    }
    if (ma->width != mb->width)
    {
        return ma->width < mb->width ? -1 : 1;
    }
    if (ma->displayHz != mb->displayHz)
    {
        return ma->displayHz < mb->displayHz ? -1 : 1;
    }
    return 0;
}

/*
============
VID_BuildModeList

Builds the mode list for one display. This is the SDL version of
R_GetModeListForDisplay from Doom 3 BFG. Unlike BFG we keep one entry
per resolution with every refresh rate the display offers at that size
(BFG only allowed 60 and 120 hz for its target displays), so the video
menu can set the rate independently of the resolution through
vid_displayrefresh.
============
*/
static void VID_BuildModeList(int display)
{
    const SDL_DisplayMode *desktop;
    SDL_DisplayMode **modes;
    int count = 0;
    int i;

    vid_num_modes = 0;

    if (vid_num_displays == 0)
    {
        return;
    }
    if (display < 0 || display >= vid_num_displays)
    {
        display = 0;
    }

    // remember the desktop size for the windowed fit check
    desktop = SDL_GetDesktopDisplayMode(vid_display_ids[display]);
    if (desktop)
    {
        vid_desktop_width = desktop->w;
        vid_desktop_height = desktop->h;
    }

    modes = SDL_GetFullscreenDisplayModes(vid_display_ids[display], &count);
    if (modes)
    {
        for (i = 0; i < count; i++)
        {
            /* The RGB888 family is what OpenGL wants, but it reports 24
               bits per pixel (SDL_BITSPERPIXEL) stored in 4 bytes, so the
               old 32-bit check rejected every mode a TV offers and left
               only the windowed fallback sizes in the list; ask for the
               4-byte storage instead. Sizes go down to 320x200 because
               the classic low resolutions are real fullscreen modes on
               TV display chains. */
            if (SDL_BYTESPERPIXEL(modes[i]->format) != 4)
            {
                continue;
            }
            if (modes[i]->w < 320 || modes[i]->h < 200)
            {
                continue;
            }
            VID_AddMode(modes[i]->w, modes[i]->h, (int)(modes[i]->refresh_rate + 0.5f));
        }
        SDL_free(modes);
    }

    if (vid_num_modes == 0)
    {
        // never hand out an empty list; fall back to the current desktop mode
        desktop = SDL_GetDesktopDisplayMode(vid_display_ids[display]);
        if (desktop)
        {
            VID_AddMode(desktop->w, desktop->h, (int)(desktop->refresh_rate + 0.5f));
        }
    }

    /* A windowed window can be any size, so also offer the common
       resolutions that fit the desktop: HDTVs often report no fullscreen
       mode under 1080p, which would leave no sensible windowed size in
       the list. The rate stays 0 (undiscovered); exclusive fullscreen
       resolves the size again and falls back to windowed if the display
       cannot do it. */
    if (vid_desktop_width > 0 && vid_desktop_height > 0)
    {
        static const int std_modes[][2] = {
            {640, 480}, {800, 600}, {1024, 768}, {1152, 864},
            {1280, 720}, {1280, 800}, {1280, 960}, {1280, 1024},
            {1366, 768}, {1400, 1050}, {1440, 900}, {1600, 900},
            {1680, 1050}, {1920, 1080}, {2560, 1440}, {3840, 2160}
        };
        for (i = 0; i < (int)(sizeof(std_modes) / sizeof(std_modes[0])); i++)
        {
            if (std_modes[i][0] > vid_desktop_width - 32 ||
                std_modes[i][1] > vid_desktop_height - 48)
            {
                continue;
            }
            VID_AddMode(std_modes[i][0], std_modes[i][1], 0);
        }
    }

    qsort(vid_modes, vid_num_modes, sizeof(vid_modes[0]), VID_ModeCompare);
}

/*
** VID_GetModeInfo
*/
qboolean VID_GetModeInfo(int *width, int *height, int *displayHz, int mode)
{
    if (mode == -1)
    {
        // custom size like r_vidMode -1 with r_customWidth/r_customHeight in BFG
        *width = (int)vid_customWidth->value;
        *height = (int)vid_customHeight->value;
        *displayHz = (int)vid_displayrefresh->value;
        return *width >= 320 && *height >= 240;
    }

    if (vid_num_modes == 0)
    {
        return false;
    }

    // keep archived values usable: negative indices land on the first
    // entry, and an index past the end resets to the default mode like
    // r_vidMode in BFG instead of clamping to the largest entry
    if (mode < 0)
    {
        mode = 0;
    }
    if (mode >= vid_num_modes)
    {
        mode = vid_num_modes > 3 ? 3 : vid_num_modes - 1;
        Cvar_SetValue("gl_mode", mode);
        Com_Printf("gl_mode reset to %i because the mode list changed\n", mode);
    }

    *width = vid_modes[mode].width;
    *height = vid_modes[mode].height;
    *displayHz = vid_modes[mode].displayHz;

    return true;
}

/*
** Accessors for the video menu
*/
int VID_NumModes(void)
{
    return vid_num_modes;
}

void VID_ModeData(int mode, int *width, int *height, int *displayHz)
{
    if (mode < 0 || mode >= vid_num_modes)
    {
        mode = vid_num_modes - 1;
    }
    if (mode < 0)
    {
        *width = 0;
        *height = 0;
        *displayHz = 0;
        return;
    }
    *width = vid_modes[mode].width;
    *height = vid_modes[mode].height;
    *displayHz = vid_modes[mode].displayHz;
}

int VID_NumRefreshRates(int mode)
{
    if (mode < 0 || mode >= vid_num_modes)
    {
        return 0;
    }
    return vid_modes[mode].numRates;
}

int VID_RefreshRate(int mode, int index)
{
    if (mode < 0 || mode >= vid_num_modes)
    {
        return 0;
    }
    if (index < 0 || index >= vid_modes[mode].numRates)
    {
        return 0;
    }
    return vid_modes[mode].rates[index];
}

int VID_NumDisplays(void)
{
    return vid_num_displays;
}

const char *VID_DisplayName(int display)
{
    if (display < 0 || display >= vid_num_displays)
    {
        display = 0;
    }
    if (vid_num_displays == 0)
    {
        return "Display 0";
    }
    return vid_display_names[display];
}

/*
** VID_ValidateWindowedMode
**
** A windowed window as big as the desktop does not fit on screen (the
** borders and title bar fall outside it), so when the archived mode is
** that big at startup, go back to the default mode. Runs once per start
** so a deliberate choice made during a session is left alone.
*/
static void VID_ValidateWindowedMode(void)
{
    int width, height, hz;

    if (vid_fullscreen->value)
    {
        return;
    }
    if (vid_desktop_width <= 0 || vid_desktop_height <= 0)
    {
        return;
    }
    if (!VID_GetModeInfo(&width, &height, &hz, (int)Cvar_VariableValue("gl_mode")))
    {
        return;
    }
    if (width > vid_desktop_width - 32 || height > vid_desktop_height - 48)
    {
        Com_Printf("windowed mode %ix%i does not fit the %ix%i desktop, resetting to the default mode\n",
                   width, height, vid_desktop_width, vid_desktop_height);
        Cvar_SetValue("gl_mode", 3);
    }
}

/*
** VID_UpdateWindowPosAndSize
*/
void VID_UpdateWindowPosAndSize(int x, int y)
{
    SDL_Window *window = Sys_GameWindow();

    if (!window)
    {
        return;
    }

    SDL_SetWindowPosition(window, (int)vid_xpos->value, (int)vid_ypos->value);
    SDL_SetWindowSize(window, viddef.width, viddef.height);
}

/*
** VID_NewWindow
*/
void VID_NewWindow(int width, int height)
{
    viddef.width = width;
    viddef.height = height;

    cl.force_refdef = true; // can't use a paused refdef
}

void VID_FreeReflib(void)
{
    if (!FreeLibrary(reflib_library))
    {
        Com_Error(ERR_FATAL, "Reflib FreeLibrary failed");
    }
    memset(&re, 0, sizeof(re));
    reflib_library = NULL;
    reflib_active = false;
}

/*
==============
VID_LoadRefresh
==============
*/
qboolean VID_LoadRefresh(char *name)
{
    refimport_t ri;
    GetRefAPI_t GetRefAPI;

    if (reflib_active)
    {
        re.Shutdown();
        VID_FreeReflib();
    }

    Com_Printf("------- Loading %s -------\n", name);

    if ((reflib_library = LoadLibrary(name)) == 0)
    {
        Com_Printf("LoadLibrary(\"%s\") failed\n", name);

        return false;
    }

    ri.Cmd_AddCommand = Cmd_AddCommand;
    ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
    ri.Cmd_Argc = Cmd_Argc;
    ri.Cmd_Argv = Cmd_Argv;
    ri.Cmd_ExecuteText = Cbuf_ExecuteText;
    ri.Con_Printf = VID_Printf;
    ri.Sys_Error = VID_Error;
    ri.FS_LoadFile = FS_LoadFile;
    ri.FS_FreeFile = FS_FreeFile;
    ri.FS_Gamedir = FS_Gamedir;
    ri.Cvar_Get = Cvar_Get;
    ri.Cvar_Set = Cvar_Set;
    ri.Cvar_SetValue = Cvar_SetValue;
    ri.Vid_GetModeInfo = VID_GetModeInfo;
    ri.Vid_MenuInit = VID_MenuInit;
    ri.Vid_NewWindow = VID_NewWindow;
    ri.DukeWeaponView = CL_CGameWeaponView;
    ri.ParallelFor = DN_JobsParallelFor;

    if ((GetRefAPI = (GetRefAPI_t)(void *)GetProcAddress(reflib_library, "GetRefAPI")) == 0)
    {
        Com_Error(ERR_FATAL, "GetProcAddress failed on %s", name);
    }

    re = GetRefAPI(ri);

    if (re.api_version != API_VERSION)
    {
        VID_FreeReflib();
        Com_Error(ERR_FATAL, "%s has incompatible api_version", name);
    }

    // the refresh creates and owns the window through SDL now
    if (re.Init(NULL, NULL) == -1)
    {
        re.Shutdown();
        VID_FreeReflib();
        return false;
    }

    Com_Printf("------------------------------------\n");
    reflib_active = true;

    //======
    // PGM
    vidref_val = VIDREF_OTHER;
    if (vid_ref)
    {
        if (!strcmp(vid_ref->string, "gl"))
        {
            vidref_val = VIDREF_GL;
        }
        else if (!strcmp(vid_ref->string, "soft"))
        {
            vidref_val = VIDREF_SOFT;
        }
    }
    // PGM
    //======

    return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges(void)
{
    char name[100];

    if (vid_ref->modified)
    {
        cl.force_refdef = true; // can't use a paused refdef
        S_StopAllSounds();
    }
    while (vid_ref->modified)
    {
        /*
        ** refresh has changed
        */
        vid_ref->modified = false;
        vid_fullscreen->modified = true;
        cl.refresh_prepped = false;
        cls.disable_screen = true;

        // a display switch changes the available mode list, so rebuild it
        // before the refresh asks for modes again
        if (vid_display->modified)
        {
            VID_BuildModeList(VID_ClampDisplay());
        }

        // once per start: make sure the archived windowed mode still fits
        // on screen now that the lists are known
        if (!vid_windowed_checked)
        {
            vid_windowed_checked = true;
            VID_ValidateWindowedMode();
        }

        Com_sprintf(name, sizeof(name), "ref_%s.dll", vid_ref->string);
        if (!VID_LoadRefresh(name))
        {
            // ref_gl is the only refresh DLL in this tree, so there is nothing to
            // fall back to; the console above shows why the load failed.
            Com_Error(ERR_FATAL, "Couldn't load %s", name);
        }
        cls.disable_screen = false;
    }

    /*
    ** update our window position
    */
    if (vid_xpos->modified || vid_ypos->modified)
    {
        if (!vid_fullscreen->value)
        {
            VID_UpdateWindowPosAndSize(vid_xpos->value, vid_ypos->value);
        }

        vid_xpos->modified = false;
        vid_ypos->modified = false;
    }
}

/*
============
VID_Init
============
*/
void VID_Init(void)
{
    /* Create the video variables so we know how to start the graphics drivers */
    vid_ref = Cvar_Get("vid_ref", "gl", CVAR_ARCHIVE);
    vid_xpos = Cvar_Get("vid_xpos", "3", CVAR_ARCHIVE);
    vid_ypos = Cvar_Get("vid_ypos", "22", CVAR_ARCHIVE);
    vid_fullscreen = Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
    vid_gamma = Cvar_Get("vid_gamma", "1", CVAR_ARCHIVE);
    vid_display = Cvar_Get("vid_display", "0", CVAR_ARCHIVE);
    vid_displayrefresh = Cvar_Get("vid_displayrefresh", "0", CVAR_ARCHIVE);
    vid_customWidth = Cvar_Get("vid_customWidth", "640", CVAR_ARCHIVE);
    vid_customHeight = Cvar_Get("vid_customHeight", "480", CVAR_ARCHIVE);

    /* Discover the connected displays and their video modes */
    VID_BuildDisplayList();
    VID_BuildModeList(VID_ClampDisplay());

    /* Add some console commands that we want to handle */
    Cmd_AddCommand("vid_restart", VID_Restart_f);
    Cmd_AddCommand("vid_front", VID_Front_f);

    /* Start the graphics mode and load refresh DLL */
    VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown(void)
{
    if (reflib_active)
    {
        re.Shutdown();
        VID_FreeReflib();
    }
}
