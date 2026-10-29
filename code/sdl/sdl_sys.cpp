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
// sdl_sys.cpp -- SDL3 platform layer: entry point, event pump, error
// handling and the dedicated-server console. The game window and GL context
// live in ref_gl (glimp_sdl.cpp); refresh loading and video state live in
// sdl_vid.cpp.

#include "../client/client.h"
#include "sdl_sys.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#include <windows.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_timer.h>

#include "../win32/dn_syscon.h"

int ActiveApp;
qboolean Minimized;

// win_viewlog shows the BFG system console window on non-dedicated runs
static cvar_t *win_viewlog;

// current mouse button state, rebuilt from SDL button events
static int mouse_buttonmask;

unsigned sys_frame_time;

// command line, parsed in WinMain and handed to Qcommon_Init
#define MAX_NUM_ARGVS 128
static int argc;
static char *argv[MAX_NUM_ARGVS];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_Error(char *error, ...)
{
    va_list argptr;
    char text[1024];

    CL_Shutdown();
    Qcommon_Shutdown();

    va_start(argptr, error);
    vsnprintf(text, sizeof(text), error, argptr);
    va_end(argptr);

    if (dedicated && dedicated->value)
    {
        OutputDebugString(text);
        fprintf(stderr, "%s\n", text);
        Win_SetErrorText(text);
        Sys_ShowConsole(1, true);
        Sys_ConsoleOutput(text);
    }
    else
    {
        // the BFG syscon error box, next to the message box
        Win_SetErrorText(text);
        Sys_ShowConsole(1, true);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", text, NULL);
    }

    SDL_Quit();

    exit(1);
}

void Sys_Quit(void)
{
    CL_Shutdown();
    Qcommon_Shutdown();

    SDL_Quit();

    exit(0);
}

//================================================================

/*
================
Sys_Init
================
*/
void Sys_Init(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
    {
        Sys_Error("SDL_Init failed: %s", SDL_GetError());
    }

    // the BFG win32 system console replaces the old Q2 console windows;
    // QHOST (win32/conproc.cpp) stays dormant with it in use
    win_viewlog = Cvar_Get("win_viewlog", "0", 0);
    Sys_CreateConsole();
    if (dedicated->value || win_viewlog->value)
    {
        Sys_ShowConsole(1, false);
    }
}

/*
================
Sys_ConsoleInput

Typed lines come from the BFG system console input line
================
*/
char *Sys_ConsoleInput(void)
{
    if (!Sys_ConsoleCreated())
    {
        return NULL;
    }

    return Sys_SysconInput();
}

/*
================
Sys_ConsoleOutput

Print text to the BFG system console window; standard error keeps
the dedicated server output usable when redirected
================
*/
void Sys_ConsoleOutput(char *string)
{
    if (Sys_ConsoleCreated())
    {
        Conbuf_AppendText(string);
    }

    if (dedicated && dedicated->value)
    {
        fprintf(stderr, "%s", string);
    }
}

/*
================
MapKey

Map from SDL scancodes to quake keynums, mirroring the old windows table.
================
*/
static int MapKey(SDL_Scancode scancode)
{
    switch (scancode)
    {
    case SDL_SCANCODE_A: return 'a';
    case SDL_SCANCODE_B: return 'b';
    case SDL_SCANCODE_C: return 'c';
    case SDL_SCANCODE_D: return 'd';
    case SDL_SCANCODE_E: return 'e';
    case SDL_SCANCODE_F: return 'f';
    case SDL_SCANCODE_G: return 'g';
    case SDL_SCANCODE_H: return 'h';
    case SDL_SCANCODE_I: return 'i';
    case SDL_SCANCODE_J: return 'j';
    case SDL_SCANCODE_K: return 'k';
    case SDL_SCANCODE_L: return 'l';
    case SDL_SCANCODE_M: return 'm';
    case SDL_SCANCODE_N: return 'n';
    case SDL_SCANCODE_O: return 'o';
    case SDL_SCANCODE_P: return 'p';
    case SDL_SCANCODE_Q: return 'q';
    case SDL_SCANCODE_R: return 'r';
    case SDL_SCANCODE_S: return 's';
    case SDL_SCANCODE_T: return 't';
    case SDL_SCANCODE_U: return 'u';
    case SDL_SCANCODE_V: return 'v';
    case SDL_SCANCODE_W: return 'w';
    case SDL_SCANCODE_X: return 'x';
    case SDL_SCANCODE_Y: return 'y';
    case SDL_SCANCODE_Z: return 'z';

    case SDL_SCANCODE_1: return '1';
    case SDL_SCANCODE_2: return '2';
    case SDL_SCANCODE_3: return '3';
    case SDL_SCANCODE_4: return '4';
    case SDL_SCANCODE_5: return '5';
    case SDL_SCANCODE_6: return '6';
    case SDL_SCANCODE_7: return '7';
    case SDL_SCANCODE_8: return '8';
    case SDL_SCANCODE_9: return '9';
    case SDL_SCANCODE_0: return '0';

    case SDL_SCANCODE_RETURN: return K_ENTER;
    case SDL_SCANCODE_ESCAPE: return K_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return K_BACKSPACE;
    case SDL_SCANCODE_TAB: return K_TAB;
    case SDL_SCANCODE_SPACE: return ' ';

    case SDL_SCANCODE_MINUS: return '-';
    case SDL_SCANCODE_EQUALS: return '=';
    case SDL_SCANCODE_LEFTBRACKET: return '[';
    case SDL_SCANCODE_RIGHTBRACKET: return ']';
    case SDL_SCANCODE_BACKSLASH: return '\\';
    case SDL_SCANCODE_NONUSBACKSLASH: return '\\';
    case SDL_SCANCODE_SEMICOLON: return ';';
    case SDL_SCANCODE_APOSTROPHE: return '\'';
    case SDL_SCANCODE_GRAVE: return 96; // backtick, opens the console

    case SDL_SCANCODE_COMMA: return ',';
    case SDL_SCANCODE_PERIOD: return '.';
    case SDL_SCANCODE_SLASH: return '/';

    case SDL_SCANCODE_F1: return K_F1;
    case SDL_SCANCODE_F2: return K_F2;
    case SDL_SCANCODE_F3: return K_F3;
    case SDL_SCANCODE_F4: return K_F4;
    case SDL_SCANCODE_F5: return K_F5;
    case SDL_SCANCODE_F6: return K_F6;
    case SDL_SCANCODE_F7: return K_F7;
    case SDL_SCANCODE_F8: return K_F8;
    case SDL_SCANCODE_F9: return K_F9;
    case SDL_SCANCODE_F10: return K_F10;
    case SDL_SCANCODE_F11: return K_F11;
    case SDL_SCANCODE_F12: return K_F12;

    case SDL_SCANCODE_PAUSE: return K_PAUSE;

    case SDL_SCANCODE_INSERT: return K_INS;
    case SDL_SCANCODE_HOME: return K_HOME;
    case SDL_SCANCODE_PAGEUP: return K_PGUP;
    case SDL_SCANCODE_DELETE: return K_DEL;
    case SDL_SCANCODE_END: return K_END;
    case SDL_SCANCODE_PAGEDOWN: return K_PGDN;

    case SDL_SCANCODE_RIGHT: return K_RIGHTARROW;
    case SDL_SCANCODE_LEFT: return K_LEFTARROW;
    case SDL_SCANCODE_DOWN: return K_DOWNARROW;
    case SDL_SCANCODE_UP: return K_UPARROW;

    case SDL_SCANCODE_KP_DIVIDE: return K_KP_SLASH;
    case SDL_SCANCODE_KP_MULTIPLY: return '*';
    case SDL_SCANCODE_KP_MINUS: return K_KP_MINUS;
    case SDL_SCANCODE_KP_PLUS: return K_KP_PLUS;
    case SDL_SCANCODE_KP_ENTER: return K_KP_ENTER;
    case SDL_SCANCODE_KP_1: return K_KP_END;
    case SDL_SCANCODE_KP_2: return K_KP_DOWNARROW;
    case SDL_SCANCODE_KP_3: return K_KP_PGDN;
    case SDL_SCANCODE_KP_4: return K_KP_LEFTARROW;
    case SDL_SCANCODE_KP_5: return K_KP_5;
    case SDL_SCANCODE_KP_6: return K_KP_RIGHTARROW;
    case SDL_SCANCODE_KP_7: return K_KP_HOME;
    case SDL_SCANCODE_KP_8: return K_KP_UPARROW;
    case SDL_SCANCODE_KP_9: return K_KP_PGUP;
    case SDL_SCANCODE_KP_0: return K_KP_INS;
    case SDL_SCANCODE_KP_PERIOD: return K_KP_DEL;

    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL:
        return K_CTRL;

    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT:
        return K_SHIFT;

    case SDL_SCANCODE_LALT:
    case SDL_SCANCODE_RALT:
        return K_ALT;

    default:
        return 0;
    }
}

/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents(void)
{
    SDL_Event event;
    MSG msg;

    // dispatch the BFG system console window messages; its close and
    // quit paths post WM_QUIT, which asks the engine to quit cleanly
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            Cbuf_AddText("quit\n");
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            Cbuf_AddText("quit\n");
            break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            qboolean down = (event.type == SDL_EVENT_KEY_DOWN);
            int key;

            if (event.key.repeat)
            {
                break;
            }

            // alt-enter toggles fullscreen, like the old window proc
            if (down && (event.key.mod & SDL_KMOD_ALT) && event.key.scancode == SDL_SCANCODE_RETURN)
            {
                if (vid_fullscreen)
                {
                    Cvar_SetValue("vid_fullscreen", !vid_fullscreen->value);
                }
                break;
            }

            key = MapKey(event.key.scancode);
            if (key)
            {
                Key_Event(key, down, Sys_Milliseconds());
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
            // absolute coordinates drive the menu cursor, relative deltas the view
            M_MouseMove((int)event.motion.x, (int)event.motion.y);
            mx_accum += (int)event.motion.xrel;
            my_accum += (int)event.motion.yrel;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            int mask = 0;

            // SDL buttons are 1=left, 2=middle, 3=right; the engine wants left, right, middle
            if (event.button.button == 1)
            {
                mask = 1;
            }
            else if (event.button.button == 3)
            {
                mask = 2;
            }
            else if (event.button.button == 2)
            {
                mask = 4;
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                mouse_buttonmask &= ~mask;
            }
            else
            {
                mouse_buttonmask |= mask;
            }

            IN_MouseEvent(mouse_buttonmask);
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
            if (event.wheel.y > 0)
            {
                Key_Event(K_MWHEELUP, true, Sys_Milliseconds());
                Key_Event(K_MWHEELUP, false, Sys_Milliseconds());
            }
            else if (event.wheel.y < 0)
            {
                Key_Event(K_MWHEELDOWN, true, Sys_Milliseconds());
                Key_Event(K_MWHEELDOWN, false, Sys_Milliseconds());
            }
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            AppActivate(true, Minimized);
            if (reflib_active)
            {
                re.AppActivate(true);
            }
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            AppActivate(false, false);
            if (reflib_active)
            {
                re.AppActivate(false);
            }
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            Minimized = true;
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            Minimized = false;
            break;

        case SDL_EVENT_WINDOW_MOVED:
            // track the windowed position like the old WM_MOVE handler
            if (vid_fullscreen && !vid_fullscreen->value)
            {
                Cvar_SetValue("vid_xpos", event.window.data1);
                Cvar_SetValue("vid_ypos", event.window.data2);
                vid_xpos->modified = false;
                vid_ypos->modified = false;
            }
            break;

        default:
            break;
        }
    }

    // grab frame time
    sys_frame_time = (unsigned)Sys_Milliseconds();
}

/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData(void)
{
    char *data = NULL;
    char *cliptext;

    cliptext = SDL_GetClipboardText();
    if (cliptext && cliptext[0])
    {
        data = (char *)(malloc(strlen(cliptext) + 1));
        strcpy(data, cliptext);
    }
    if (cliptext)
    {
        SDL_free(cliptext);
    }
    return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate(void)
{
    SDL_Window *window = Sys_GameWindow();

    if (window)
    {
        SDL_RestoreWindow(window);
        SDL_RaiseWindow(window);
    }
}

SDL_Window *Sys_GameWindow(void)
{
    int count = 0;
    SDL_Window **windows;
    SDL_Window *window = NULL;

    // the engine only ever creates one window (ref_gl)
    windows = SDL_GetWindows(&count);
    if (windows)
    {
        if (count > 0)
        {
            window = windows[0];
        }
        SDL_free(windows);
    }
    return window;
}

/*
============
AppActivate

Called from the focus events; mirrors the old WM_ACTIVATE handler.
============
*/
void AppActivate(qboolean active, qboolean minimized)
{
    Minimized = minimized;

    Key_ClearStates();

    // we don't want to act like we're active if we're minimized
    ActiveApp = (active && !Minimized);

    // minimize/restore mouse-capture and audio on demand
    if (!ActiveApp)
    {
        IN_Activate(false);
        S_MusicActivate(false);
        S_Activate(false);
    }
    else
    {
        IN_Activate(true);
        S_MusicActivate(true);
        S_Activate(true);
    }
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame(void)
{
    if (!FreeLibrary(game_library))
    {
        Com_Error(ERR_FATAL, "FreeLibrary failed for game library");
    }
    game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI(void *parms)
{
    void *(*GetGameAPI)(void *);
    char name[MAX_OSPATH];
    char *path;
    const char *gamename = "dngamex86.dll";

    if (game_library)
    {
        Com_Error(ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");
    }

    /* Use the active game search path so a stale root/development DLL cannot
       override basedn's CMake output. Mods retain their normal precedence. */
    for (path = FS_NextPath(NULL); path; path = FS_NextPath(path))
    {
        Com_sprintf(name, sizeof(name), "%s/%s", path, gamename);
        game_library = LoadLibrary(name);
        if (game_library)
        {
            Com_Printf("Duke game library: %s\n", name);
            break;
        }
    }
    if (!game_library)
    {
        return NULL;
    }

    GetGameAPI = reinterpret_cast<decltype(GetGameAPI)>((void *)GetProcAddress(game_library, "GetGameAPI"));
    if (!GetGameAPI)
    {
        Sys_UnloadGame();
        return NULL;
    }

    return GetGameAPI(parms);
}

//=======================================================================

/*
==================
ParseCommandLine

==================
*/
static void ParseCommandLine(LPSTR lpCmdLine)
{
    argc = 1;
    argv[0] = "exe";

    while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
    {
        while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
        {
            lpCmdLine++;
        }

        if (*lpCmdLine)
        {
            argv[argc] = lpCmdLine;
            argc++;

            while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
            {
                lpCmdLine++;
            }

            if (*lpCmdLine)
            {
                *lpCmdLine = 0;
                lpCmdLine++;
            }
        }
    }
}

/*
==================
WinMain

==================
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int time, oldtime, newtime;
    const char *base;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
    {
        return 0;
    }

    (void)hInstance;
    (void)nCmdShow;

    /* keep loading relative to the executable, like the old WinMain */
    base = SDL_GetBasePath();
    if (base)
    {
        SetCurrentDirectoryA(base);
        SDL_free((void *)base);
    }

    ParseCommandLine(lpCmdLine);

    Qcommon_Init(argc, argv);
    oldtime = Sys_Milliseconds();

    /* main loop */
    while (1)
    {
        // if minimized or dedicated, don't spin the CPU
        if (Minimized || (dedicated && dedicated->value))
        {
            SDL_Delay(1);
        }

        Sys_SendKeyEvents();

        do
        {
            newtime = Sys_Milliseconds();
            time = newtime - oldtime;
        } while (time < 1);

        Qcommon_Frame(time);
        oldtime = newtime;
    }

    // never gets here
    return 0;
}
