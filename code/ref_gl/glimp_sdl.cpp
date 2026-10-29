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
** VID_CreateWindow
*/
static qboolean VID_CreateWindow(int width, int height, qboolean fullscreen)
{
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    cvar_t *vid_xpos, *vid_ypos;

    if (fullscreen)
    {
        // desktop fullscreen; SDL keeps the display resolution and scales
        flags |= SDL_WINDOW_FULLSCREEN;
    }

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
    int width, height;
    const char *win_fs[] = {"W", "FS"};

    ri.Con_Printf(PRINT_ALL, "Initializing OpenGL display\n");

    ri.Con_Printf(PRINT_ALL, "...setting mode %d:", mode);

    if (!ri.Vid_GetModeInfo(&width, &height, mode))
    {
        ri.Con_Printf(PRINT_ALL, " invalid mode\n");
        return rserr_invalid_mode;
    }

    ri.Con_Printf(PRINT_ALL, " %d %d %s\n", width, height, win_fs[fullscreen]);

    // destroy the existing window
    if (window)
    {
        GLimp_Shutdown();
    }

    // do a mode change if needed
    if (fullscreen)
    {
        ri.Con_Printf(PRINT_ALL, "...attempting fullscreen\n");
    }
    else
    {
        ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");
    }

    *pwidth = width;
    *pheight = height;

    gl_state.fullscreen = fullscreen;

    if (!VID_CreateWindow(width, height, fullscreen))
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
