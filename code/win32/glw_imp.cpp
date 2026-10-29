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
** GLW_IMP.cpp
**
** Win32 window and context handling for the OpenGL refresh. The context is
** created with the WGL_ARB_create_context extension and must support the
** OpenGL 3.3 core profile; glad loads the rest of the entry points from it.
*/
#include <windows.h>
#include <string.h>
#include "../ref_gl/gl_local.h"
#include "glw_win.h"
#include "winquake.h"
#include "resource.h"

glwstate_t glw_state;

qboolean GLimp_InitGL(void);

BOOL(WINAPI *wglSwapIntervalEXT)(int interval) = NULL;

extern cvar_t *vid_fullscreen;

/*
** VID_CreateWindow
*/
#define WINDOW_CLASS_NAME "Duke Nukem 3D"

qboolean VID_CreateWindow(int width, int height, qboolean fullscreen)
{
    WNDCLASSEX wc = {};
    RECT r;
    cvar_t *vid_xpos, *vid_ypos;
    int stylebits;
    int x, y, w, h;
    int exstyle;

    /* Register the frame class */
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)glw_state.wndproc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = glw_state.hInstance;
    wc.hIcon = (HICON)LoadImage(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
                         GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED);
    wc.hIconSm = (HICON)LoadImage(glw_state.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
                           GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>((void *)COLOR_GRAYTEXT);
    wc.lpszMenuName = 0;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassEx(&wc))
    {
        ri.Sys_Error(ERR_FATAL, "Couldn't register window class");
    }

    if (fullscreen)
    {
        exstyle = WS_EX_TOPMOST;
        stylebits = WS_POPUP | WS_VISIBLE;
    }
    else
    {
        exstyle = 0;
        stylebits = WINDOW_STYLE;
    }

    r.left = 0;
    r.top = 0;
    r.right = width;
    r.bottom = height;

    AdjustWindowRect(&r, stylebits, FALSE);

    w = r.right - r.left;
    h = r.bottom - r.top;

    if (fullscreen)
    {
        x = 0;
        y = 0;
    }
    else
    {
        vid_xpos = ri.Cvar_Get("vid_xpos", "0", 0);
        vid_ypos = ri.Cvar_Get("vid_ypos", "0", 0);
        x = vid_xpos->value;
        y = vid_ypos->value;
    }

    glw_state.hWnd = CreateWindowEx(exstyle, WINDOW_CLASS_NAME, "Duke Nukem 3D", stylebits, x, y, w,
                                    h, NULL, NULL, glw_state.hInstance, NULL);

    if (!glw_state.hWnd)
    {
        ri.Sys_Error(ERR_FATAL, "Couldn't create window");
    }

    ShowWindow(glw_state.hWnd, SW_SHOW);
    UpdateWindow(glw_state.hWnd);

    // init all the gl stuff for the window
    if (!GLimp_InitGL())
    {
        ri.Con_Printf(PRINT_ALL, "VID_CreateWindow() - GLimp_InitGL failed\n");
        return false;
    }

    SetForegroundWindow(glw_state.hWnd);
    SetFocus(glw_state.hWnd);

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
    if (glw_state.hWnd)
    {
        GLimp_Shutdown();
    }

    // do a CDS if needed
    if (fullscreen)
    {
        DEVMODE dm;

        ri.Con_Printf(PRINT_ALL, "...attempting fullscreen\n");

        memset(&dm, 0, sizeof(dm));

        dm.dmSize = sizeof(dm);

        dm.dmPelsWidth = width;
        dm.dmPelsHeight = height;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

        ri.Con_Printf(PRINT_ALL, "...calling CDS: ");
        if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
        {
            *pwidth = width;
            *pheight = height;

            gl_state.fullscreen = true;

            ri.Con_Printf(PRINT_ALL, "ok\n");

            if (!VID_CreateWindow(width, height, true))
            {
                return rserr_invalid_mode;
            }

            return rserr_ok;
        }
        else
        {
            *pwidth = width;
            *pheight = height;

            ri.Con_Printf(PRINT_ALL, "failed\n");

            ri.Con_Printf(PRINT_ALL, "...calling CDS assuming dual monitors:");

            dm.dmPelsWidth = width * 2;
            dm.dmPelsHeight = height;
            dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

            /*
            ** our first CDS failed, so maybe we're running on some weird dual monitor
            ** system
            */
            if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                ri.Con_Printf(PRINT_ALL, " failed\n");

                ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");

                ChangeDisplaySettings(0, 0);

                *pwidth = width;
                *pheight = height;
                gl_state.fullscreen = false;
                if (!VID_CreateWindow(width, height, false))
                {
                    return rserr_invalid_mode;
                }
                return rserr_invalid_fullscreen;
            }
            else
            {
                ri.Con_Printf(PRINT_ALL, " ok\n");
                if (!VID_CreateWindow(width, height, true))
                {
                    return rserr_invalid_mode;
                }

                gl_state.fullscreen = true;
                return rserr_ok;
            }
        }
    }
    else
    {
        ri.Con_Printf(PRINT_ALL, "...setting windowed mode\n");

        ChangeDisplaySettings(0, 0);

        *pwidth = width;
        *pheight = height;
        gl_state.fullscreen = false;
        if (!VID_CreateWindow(width, height, false))
        {
            return rserr_invalid_mode;
        }
    }

    return rserr_ok;
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem: release the current DC and HGLRC, delete the rendering
** context, and destroy the window.
*/
void GLimp_Shutdown(void)
{
    if (glw_state.hGLRC)
    {
        wglMakeCurrent(NULL, NULL);
        if (!wglDeleteContext(glw_state.hGLRC))
        {
            ri.Con_Printf(PRINT_ALL, "ref_gl::R_Shutdown() - wglDeleteContext failed\n");
        }
        glw_state.hGLRC = NULL;
    }
    if (glw_state.hDC)
    {
        if (!ReleaseDC(glw_state.hWnd, glw_state.hDC))
        {
            ri.Con_Printf(PRINT_ALL, "ref_gl::R_Shutdown() - ReleaseDC failed\n");
        }
        glw_state.hDC = NULL;
    }
    if (glw_state.hWnd)
    {
        DestroyWindow(glw_state.hWnd);
        glw_state.hWnd = NULL;
    }

    UnregisterClass(WINDOW_CLASS_NAME, glw_state.hInstance);

    if (gl_state.fullscreen)
    {
        ChangeDisplaySettings(0, 0);
        gl_state.fullscreen = false;
    }
}

/*
** GLimp_Init
*/
qboolean GLimp_Init(void *hinstance, void *wndproc)
{
    glw_state.hInstance = (HINSTANCE)hinstance;
    glw_state.wndproc = wndproc;

    return true;
}

/*
** GLimp_InitGL
**
** Core contexts cannot be requested directly, so: create a dummy legacy
** context on the window's pixel format, grab wglCreateContextAttribsARB
** from it, then trade it for a real 3.3 core context and load glad from it.
*/
qboolean GLimp_InitGL(void)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
        1,                             // version number
        PFD_DRAW_TO_WINDOW |           // support window
            PFD_SUPPORT_OPENGL |       // support OpenGL
            PFD_DOUBLEBUFFER,          // double buffered
        PFD_TYPE_RGBA,                 // RGBA type
        24,                            // 24-bit color depth
        0,
        0,
        0,
        0,
        0,
        0, // color bits ignored
        0, // no alpha buffer
        0, // shift bit ignored
        0, // no accumulation buffer
        0,
        0,
        0,
        0,              // accum bits ignored
        32,             // 32-bit z-buffer
        0,              // no stencil buffer
        0,              // no auxiliary buffer
        PFD_MAIN_PLANE, // main layer
        0,              // reserved
        0,
        0,
        0 // layer masks ignored
    };
    int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };
    HGLRC dummy;
    int pixelformat;

    if ((glw_state.hDC = GetDC(glw_state.hWnd)) == NULL)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - GetDC failed\n");
        return false;
    }

    if ((pixelformat = ChoosePixelFormat(glw_state.hDC, &pfd)) == 0)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - ChoosePixelFormat failed\n");
        return false;
    }
    if (SetPixelFormat(glw_state.hDC, pixelformat, &pfd) == FALSE)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - SetPixelFormat failed\n");
        return false;
    }

    // The pixel format can only be set once per window, so the dummy context
    // has to share it.
    if ((dummy = wglCreateContext(glw_state.hDC)) == NULL)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - wglCreateContext failed\n");
        return false;
    }
    if (!wglMakeCurrent(glw_state.hDC, dummy))
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - wglMakeCurrent failed\n");
        wglDeleteContext(dummy);
        return false;
    }

    glw_state.wglCreateContextAttribsARB =
        (wglCreateContextAttribsARB_t)wglGetProcAddress("wglCreateContextAttribsARB");
    if (glw_state.wglCreateContextAttribsARB)
    {
        glw_state.hGLRC = glw_state.wglCreateContextAttribsARB(glw_state.hDC, NULL, attribs);
    }
    wglDeleteContext(dummy);
    wglMakeCurrent(NULL, NULL);

    if (!glw_state.hGLRC)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - couldn't create an OpenGL 3.3 core context\n");
        return false;
    }
    if (!wglMakeCurrent(glw_state.hDC, glw_state.hGLRC))
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - wglMakeCurrent failed\n");
        wglDeleteContext(glw_state.hGLRC);
        glw_state.hGLRC = NULL;
        return false;
    }

    if (!gladLoaderLoadGL())
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - gladLoaderLoadGL failed\n");
        goto fail;
    }
    if (!GLAD_GL_VERSION_3_3)
    {
        ri.Con_Printf(PRINT_ALL, "GLimp_InitGL() - OpenGL 3.3 not supported by the driver\n");
        goto fail;
    }

    wglSwapIntervalEXT = (BOOL(WINAPI *)(int))wglGetProcAddress("wglSwapIntervalEXT");

    ri.Con_Printf(PRINT_ALL, "GL PFD: color(%d-bits) Z(%d-bit)\n", (int)pfd.cColorBits,
                  (int)pfd.cDepthBits);

    return true;

fail:
    if (glw_state.hGLRC)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(glw_state.hGLRC);
        glw_state.hGLRC = NULL;
    }
    if (glw_state.hDC)
    {
        ReleaseDC(glw_state.hWnd, glw_state.hDC);
        glw_state.hDC = NULL;
    }
    return false;
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
    if (stricmp(gl_drawbuffer->string, "GL_BACK") == 0)
    {
        if (!SwapBuffers(glw_state.hDC))
        {
            ri.Sys_Error(ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n");
        }
    }
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate(qboolean active)
{
    if (active)
    {
        SetForegroundWindow(glw_state.hWnd);
        ShowWindow(glw_state.hWnd, SW_RESTORE);
    }
    else
    {
        if (vid_fullscreen->value)
        {
            ShowWindow(glw_state.hWnd, SW_MINIMIZE);
        }
    }
}
