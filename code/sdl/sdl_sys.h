#ifndef SDL_SYS_H
#define SDL_SYS_H

#include "../qcommon/qcommon.h"

typedef struct SDL_Window SDL_Window;

// set by the focus handling in sdl_sys.cpp
extern int ActiveApp;
extern qboolean Minimized;

// defined in sdl_vid.cpp
extern qboolean reflib_active;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_xpos;
extern cvar_t *vid_ypos;

// relative mouse deltas, accumulated by the event pump in sdl_sys.cpp
extern int mx_accum, my_accum;

void Sys_SendKeyEvents(void);
void Sys_AppActivate(void);
SDL_Window *Sys_GameWindow(void);
void AppActivate(qboolean active, qboolean minimized);
void IN_MouseEvent(int mstate);
void M_MouseMove(int x, int y);

#endif
