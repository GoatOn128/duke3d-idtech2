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
// sv_gamethread.h -- game tic thread ported from the Doom 3 BFG Edition
// idGameThread (neo/framework/common_frame.cpp). The BFG Run() also
// generates the draw commands on the game thread; this engine's
// renderer is immediate mode, so only the game tic runs here and the
// main thread waits at the BFG sync point while the tic is in flight.
// License note: derives from the BFG Edition GPL source (GPLv3), ported
// from references/doom3-bfg, which is not compiled.

#ifndef SV_GAMETHREAD_H
#define SV_GAMETHREAD_H

#include <setjmp.h>

#include "../qcommon/dnthreads.h"

class DN_GameThread : public DN_SysThread
{
public:
    // registers the com_smp / com_sleepGame cvars (called from SV_Init)
    void Init();
    // stops the game thread if it is running (called from SV_Shutdown)
    void Shutdown();
    // runs one game tic: on the game thread when com_smp is on, inline
    // on the main thread otherwise, like the BFG game thread Run paths
    void RunGameTic();

protected:
    int Run();

private:
    bool threadStarted;
    bool trapped;
    jmp_buf ticGuard;
};

extern DN_GameThread gameThread;

#endif
