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
// dnerrortrap.h -- Com_Error trap for the BFG game thread port
// (server/sv_gamethread.cpp). Com_Error longjmps to the main thread's
// abort frame, which cannot unwind across threads, so while a game tic
// is in flight this trap catches any error, stores it, and unwinds to
// the game thread's guard instead. The main thread re-raises the error
// after the tic completes. Only one thread is ever guarded at a time:
// the main thread is blocked waiting for the game thread while the
// trap is installed.

#ifndef DNERRORTRAP_H
#define DNERRORTRAP_H

#include <setjmp.h>

void DN_ErrorTrapInstall(jmp_buf *guard);
void DN_ErrorTrapRemove(void);
int DN_ErrorTrapActive(void);
// stores the error and unwinds to the installed guard; never returns
void DN_ErrorTrapRaise(int code, const char *message);
int DN_ErrorTrapCode(void);
const char *DN_ErrorTrapMessage(void);

#endif
