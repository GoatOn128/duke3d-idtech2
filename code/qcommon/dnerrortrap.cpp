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
// dnerrortrap.cpp -- see dnerrortrap.h

#include "dnerrortrap.h"

#include <string.h>

#define DN_ERRORTRAP_MSGSIZE 4096

static jmp_buf *trapGuard;
static int trapCode;
static char trapMessage[DN_ERRORTRAP_MSGSIZE];

void DN_ErrorTrapInstall(jmp_buf *guard)
{
    trapGuard = guard;
    trapCode = 0;
    trapMessage[0] = 0;
}

void DN_ErrorTrapRemove(void)
{
    trapGuard = NULL;
}

int DN_ErrorTrapActive(void)
{
    return trapGuard != NULL;
}

void DN_ErrorTrapRaise(int code, const char *message)
{
    // the trap is only installed while a guarded tic is in flight
    if (!trapGuard)
    {
        return;
    }
    trapCode = code;
    strncpy(trapMessage, message, sizeof(trapMessage) - 1);
    trapMessage[sizeof(trapMessage) - 1] = 0;
    longjmp(*trapGuard, 1);
}

int DN_ErrorTrapCode(void)
{
    return trapCode;
}

const char *DN_ErrorTrapMessage(void)
{
    return trapMessage;
}
