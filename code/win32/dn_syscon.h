/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// Port of the Doom 3 BFG Edition win32 system console interface
// (sys/win32/win_syscon.cpp and its win_local.h declarations) to the
// Quake 2 platform layer. sdl_sys.cpp routes Sys_ConsoleInput and
// Sys_ConsoleOutput through here.

#ifndef DNSYSCON_H
#define DNSYSCON_H

// qboolean and the engine types come from the caller's includes; the
// Q2 common headers are unguarded and must not be pulled in twice

void Sys_CreateConsole(void);
void Sys_DestroyConsole(void);
void Sys_ShowConsole(int visLevel, qboolean quitOnClose);

// true when the system console window exists
qboolean Sys_ConsoleCreated(void);

// one typed line from the system console input line, NULL when none;
// Qcommon_Frame adds the newline when it executes the text
char *Sys_SysconInput(void);

// append text to the system console scroll buffer
void Conbuf_AppendText(const char *text);

// put the error string into the system console error box
void Win_SetErrorText(const char *buf);

#endif
