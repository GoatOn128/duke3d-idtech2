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

#define VID_MAX_MODES 64
#define VID_MAX_DISPLAYS 16

typedef struct vidmode_s
{
    int width, height;
    int displayHz;
} vidmode_t;

static vidmode_t vid_modes[VID_MAX_MODES];
static int vid_num_modes;

static SDL_DisplayID vid_display_ids[VID_MAX_DISPLAYS];
static char vid_display_names[VID_MAX_DISPLAYS][64];
static int vid_num_displays;

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
VID_ModeExists

BFG AddUnique: only keep one entry per width/height/hz triple.
============
*/
static qboolean VID_ModeExists(int width, int height, int hz)
{
    int i;

    for (i = 0; i < vid_num_modes; i++)
    {
        if (vid_modes[i].width == width && vid_modes[i].height == height &&
            vid_modes[i].displayHz == hz)
        {
            return true;
        }
    }
    return false;
}

static void VID_AddMode(int width, int height, int hz)
{
    if (vid_num_modes >= VID_MAX_MODES || VID_ModeExists(width, height, hz))
    {
        return;
    }
    vid_modes[vid_num_modes].width = width;
    vid_modes[vid_num_modes].height = height;
    vid_modes[vid_num_modes].displayHz = hz;
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
R_GetModeListForDisplay from Doom 3 BFG. Unlike BFG we keep every refresh
rate (BFG only allowed 60 and 120 hz for its target displays) and allow
modes down to 480 lines so the classic 640x480 default stays selectable.
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

    modes = SDL_GetFullscreenDisplayModes(vid_display_ids[display], &count);
    if (modes)
    {
        for (i = 0; i < count; i++)
        {
            if (SDL_BITSPERPIXEL(modes[i]->format) != 32)
            {
                continue;
            }
            if (modes[i]->h < 480)
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

    // clamp instead of rejecting so archived gl_mode values stay valid
    if (mode < 0)
    {
        mode = 0;
    }
    if (mode >= vid_num_modes)
    {
        mode = vid_num_modes - 1;
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
