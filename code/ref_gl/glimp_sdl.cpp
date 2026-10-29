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
/*
** glimp_sdl.cpp
**
** SDL3 window and OpenGL context handling for the OpenGL refresh. The
** refresh library owns the game window; events are pumped by the client
** through the same SDL3 library. SDL creates the OpenGL 3.3 core context
** directly and glad loads the entry points from it.
**
** Display discovery and fullscreen handling follow the Doom 3 BFG win32
** glimp: borderless fullscreen at the desktop resolution, exclusive
** fullscreen through an SDL display mode otherwise, and a refresh rate
** override cvar like r_displayRefresh.
*/
#include <string.h>

#include <SDL3/SDL_video.h>

#include "gl_local.h"

static SDL_Window *window;
static SDL_GLContext context = NULL;

/*
** GLimp_InitGL
*/
static qboolean GLimp_InitGL(void)
{
    if (!SDL_GL_MakeCurrent(window, context))
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }

    if (!gladLoaderLoadGL())
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - gladLoaderLoadGL failed\n");
        return false;
    }

    if (!GLAD_GL_VERSION_3_3)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - OpenGL 3.3 not supported by the driver\n");
        return false;
    }

    return true;
}

/*
** GLimp_ListDisplays_f
**
** Prints every display with its desktop mode and all fullscreen modes SDL
** offers for it. This is the SDL version of DumpAllDisplayDevices from
** the Doom 3 BFG win32 glimp.
*/
void GLimp_ListDisplays_f(void)
{
    SDL_DisplayID *displays;
    int count = 0;
    int i, j;

    displays = SDL_GetDisplays(&count);
    if (!displays)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_ListDisplays_f() - SDL_GetDisplays failed: %s\n",
                      SDL_GetError());
        return;
    }

    ri.Con_Printf(PRINT_ALL, "%i display(s)\n", count);

    for (i = 0; i < count; i++)
    {
        const SDL_DisplayMode *desktop = SDL_GetDesktopDisplayMode(displays[i]);
        const char *name = SDL_GetDisplayName(displays[i]);
        int modeCount = 0;
        SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(displays[i], &modeCount);

        ri.Con_Printf(PRINT_ALL, "display %i: %s\n", i, name ? name : "unknown");
        if (desktop)
        {
            ri.Con_Printf(PRINT_ALL, "  desktop: %ix%i @ %i hz\n", desktop->w, desktop->h,
                          (int)(desktop->refresh_rate + 0.5f));
        }
        if (modes)
        {
            for (j = 0; j < modeCount; j++)
            {
                ri.Con_Printf(PRINT_ALL, "  mode %3i: %ix%i @ %i hz (%i bpp)\n", j, modes[j]->w,
                              modes[j]->h, (int)(modes[j]->refresh_rate + 0.5f),
                              SDL_BITSPERPIXEL(modes[j]->format));
            }
            SDL_free(modes);
        }
    }

    SDL_free(displays);
}

/*
** GLimp_ChooseFullscreenMode
**
** Resolves the fullscreen target for a requested size, like
** GLW_ChangeDislaySettingsIfNeeded in Doom 3 BFG:
**   true and *mode NULL - borderless fullscreen at the desktop resolution
**   true and *mode set   - exclusive fullscreen at that SDL_DisplayMode
**   false                - the display cannot do the requested mode
*/
static qboolean GLimp_ChooseFullscreenMode(int width, int height, int displayHz,
                                            const SDL_DisplayMode **mode)
{
    cvar_t *vid_display = ri.Cvar_Get("vid_display", "0", 0);
    cvar_t *vid_displayrefresh = ri.Cvar_Get("vid_displayrefresh", "0", 0);
    SDL_DisplayID *displays;
    SDL_DisplayID displayID;
    SDL_DisplayMode **modes;
    const SDL_DisplayMode *desktop;
    int count = 0;
    int i;
    int wantHz;

    *mode = NULL;

    displays = SDL_GetDisplays(&count);
    if (!displays || count == 0)
    {
        SDL_free(displays);
        return false;
    }

    i = (int)vid_display->value;
    if (i < 0)
    {
        i = 0;
    }
    if (i >= count)
    {
        i = count - 1;
    }
    displayID = displays[i];
    SDL_free(displays);

    desktop = SDL_GetDesktopDisplayMode(displayID);
    if (!desktop)
    {
        return false;
    }

    // refresh rate override like r_displayRefresh in BFG; 0 means the rate
    // that came with the client mode list entry
    wantHz = (int)(vid_displayrefresh->value > 0 ? vid_displayrefresh->value : displayHz);

    // already at the desktop resolution (and rate): borderless fullscreen,
    // no display mode change needed
    if (desktop->w == width && desktop->h == height &&
        (wantHz == 0 || wantHz == (int)(desktop->refresh_rate + 0.5f)))
    {
        return true;
    }

    modes = SDL_GetFullscreenDisplayModes(displayID, &count);
    if (!modes)
    {
        return false;
    }

    for (i = 0; i < count; i++)
    {
        if (modes[i]->w != width || modes[i]->h != height)
        {
            continue;
        }
        if (wantHz > 0)
        {
            if ((int)(modes[i]->refresh_rate + 0.5f) == wantHz)
            {
                *mode = modes[i];
                break;
            }
        }
        else if (*mode == NULL || modes[i]->refresh_rate > (*mode)->refresh_rate)
        {
            // no rate requested: keep the fastest mode at this size
            *mode = modes[i];
        }
    }

    SDL_free(modes);

    // the mode structs are owned by SDL and stay valid after the array is freed
    return *mode != NULL;
}

/*
** VID_CreateWindow
*/
static qboolean VID_CreateWindow(int width, int height, qboolean fullscreen,
                                 const SDL_DisplayMode *fsmode)
{
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    cvar_t *vid_xpos, *vid_ypos;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    window = SDL_CreateWindow("Duke Nukem 3D", width, height, flags);
    if (!window)
    {
        ri.Sys_Error(ERR_FATAL, "Couldn't create window: %s", SDL_GetError());
        return false;
    }

    if (!fullscreen)
    {
        vid_xpos = ri.Cvar_Get("vid_xpos", "0", 0);
        vid_ypos = ri.Cvar_Get("vid_ypos", "0", 0);
        SDL_SetWindowPosition(window, (int)vid_xpos->value, (int)vid_ypos->value);
    }

    context = SDL_GL_CreateContext(window);
    if (!context)
    {
        ri.Con_Printf(PRINT_ALL, "VID_CreateWindow() - couldn't create an OpenGL 3.3 core context: %s\n",
                      SDL_GetError());
        return false;
    }

    // init all the gl stuff for the window
    if (!GLimp_InitGL())
    {
        return false;
    }

    if (fullscreen)
    {
        // NULL mode = borderless at the desktop resolution, otherwise the
        // exclusive mode picked by GLimp_ChooseFullscreenMode
        if (!SDL_SetWindowFullscreenMode(window, fsmode))
        {
            ri.Con_Printf(PRINT_ALL, "VID_CreateWindow() - SDL_SetWindowFullscreenMode failed: %s\n",
                          SDL_GetError());
            return false;
        }
        if (!SDL_SetWindowFullscreen(window, true))
        {
            ri.Con_Printf(PRINT_ALL, "VID_CreateWindow() - SDL_SetWindowFullscreen failed: %s\n",
                          SDL_GetError());
            return false;
        }
    }

    SDL_GL_SetSwapInterval((int)gl_swapinterval->value);

    // let the sound and input subsystems know about the new window
    ri.Vid_NewWindow(width, height);

    return true;
}

/*
** GLimp_SetMode
*/
rserr_t GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen)
{
    int width, height, displayHz = 0;
    const char *win_fs[] = {"W", "FS"};
    const SDL_DisplayMode *fsmode = NULL;

    ri.Con_Printf(PRINT_ALL, "Initializing OpenGL display\n");

    ri.Con_Printf(PRINT_ALL, "...setting mode %d:", mode);

    if (!ri.Vid_GetModeInfo(&width, &height, &displayHz, mode))
    {
        ri.Con_Printf(PRINT_ALL, " invalid mode\n");
        return rserr_invalid_mode;
    }

    ri.Con_Printf(PRINT_ALL, " %d %d %s\n", width, height, win_fs[fullscreen]);

    // resolve the fullscreen target first, like the BFG display change
    // check, so a failed mode never destroys the running window
    if (fullscreen)
    {
        if (!GLimp_ChooseFullscreenMode(width, height, displayHz, &fsmode))
        {
            ri.Con_Printf(PRINT_ALL, "...fullscreen mode %ix%i @ %i hz is unavailable\n",
                          width, height, displayHz);
            return rserr_invalid_fullscreen;
        }
        ri.Con_Printf(PRINT_ALL, "...attempting fullscreen\n");
    }
    else
    {
        ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");
    }

    // destroy the existing window
    if (window)
    {
        GLimp_Shutdown();
    }

    *pwidth = width;
    *pheight = height;

    gl_state.fullscreen = fullscreen;

    if (!VID_CreateWindow(width, height, fullscreen, fsmode))
    {
        return rserr_invalid_mode;
    }

    return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem: destroy the rendering context and the window.
*/
void GLimp_Shutdown(void)
{
    if (context)
    {
        SDL_GL_DestroyContext(context);
        context = NULL;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    gl_state.fullscreen = false;
}

/*
** GLimp_Init
*/
qboolean GLimp_Init(void *hinstance, void *hWnd)
{
    // SDL is initialized once by the client; the parameters only exist so
    // the refresh API keeps its shape.
    (void)hinstance;
    (void)hWnd;

    // print the display inventory like the BFG win32 glimp does at startup
    GLimp_ListDisplays_f();

    return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame(void)
{
    glDrawBuffer(GL_BACK);
}

/*
** GLimp_EndFrame
*/
void GLimp_EndFrame(void)
{
    if (Q_stricmp(gl_drawbuffer->string, "GL_BACK") == 0)
    {
        if (!SDL_GL_SwapWindow(window))
        {
            ri.Sys_Error(ERR_FATAL, "GLimp_EndFrame() - SDL_GL_SwapWindow() failed: %s\n",
                         SDL_GetError());
        }
    }
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate(qboolean active)
{
    // focus handling is driven by the client's SDL event pump; the context
    // stays current on its own window
    (void)active;
}
