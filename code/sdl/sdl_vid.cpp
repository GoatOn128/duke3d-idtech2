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
// sdl_vid.cpp -- client video layer: loads the refresh DLL and tracks the
// video cvars. Window, context and fullscreen all live inside the refresh
// through SDL now, so there is no window procedure here anymore.
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
cvar_t *vid_ref;  // Name of Refresh DLL loaded
cvar_t *vid_xpos; // X coordinate of window position
cvar_t *vid_ypos; // Y coordinate of window position
cvar_t *vid_fullscreen;

// Global variables used internally by this module
viddef_t viddef;          // global video state; used by other modules
HINSTANCE reflib_library; // Handle to refresh DLL
qboolean reflib_active = 0;

#define VID_NUM_MODES (sizeof(vid_modes) / sizeof(vid_modes[0]))

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
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int width, height;
    int mode;
} vidmode_t;

vidmode_t vid_modes[] = {{"Mode 0: 320x240", 320, 240, 0},   {"Mode 1: 400x300", 400, 300, 1},
                         {"Mode 2: 512x384", 512, 384, 2},   {"Mode 3: 640x480", 640, 480, 3},
                         {"Mode 4: 800x600", 800, 600, 4},   {"Mode 5: 960x720", 960, 720, 5},
                         {"Mode 6: 1024x768", 1024, 768, 6}, {"Mode 7: 1152x864", 1152, 864, 7},
                         {"Mode 8: 1280x960", 1280, 960, 8}, {"Mode 9: 1600x1200", 1600, 1200, 9}};

qboolean VID_GetModeInfo(int *width, int *height, int mode)
{
    if (mode < 0 || mode >= VID_NUM_MODES)
    {
        return false;
    }

    *width = vid_modes[mode].width;
    *height = vid_modes[mode].height;

    return true;
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
