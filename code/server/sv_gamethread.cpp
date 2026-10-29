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
// sv_gamethread.cpp -- see sv_gamethread.h. Ported from the Doom 3 BFG
// Edition idGameThread: the BFG RunGameAndDraw became RunGameTic, the
// BFG game->RunFrame loop became the single ge->RunFrame() tic (this
// engine feeds usercmds during packet reads, not through a usercmd
// manager), and the latched gameReturn_t is unused because SV_Frame
// consumes the tic results directly after the wait.
// License note: derives from the BFG Edition GPL source (GPLv3), ported
// from references/doom3-bfg, which is not compiled.

#include "server.h"
#include "sv_gamethread.h"

#include <SDL3/SDL_timer.h>

#include "../qcommon/dnerrortrap.h"

static cvar_t *com_smp;
static cvar_t *com_sleepGame;

DN_GameThread gameThread;

DN_GameThread::DN_GameThread()
{
    threadStarted = false;
    trapped = false;
}

void DN_GameThread::Init()
{
    // BFG com_smp: run the game code on its own thread; com_sleepGame
    // adds an artificial sleep to the tic for frame pacing tests
    com_smp = Cvar_Get("com_smp", "1", 0);
    com_sleepGame = Cvar_Get("com_sleepGame", "0", 0);
}

void DN_GameThread::Shutdown()
{
    if (threadStarted)
    {
        StopThread(true);
        threadStarted = false;
    }
}

int DN_GameThread::Run()
{
    int i;

    // debugging tool to test frame pacing, like the BFG com_sleepGame
    if (com_sleepGame->value)
    {
        SDL_Delay((int)com_sleepGame->value);
    }

    // Com_Error cannot unwind across threads: while the trap is
    // installed, a game error unwinds to the guard below, and the main
    // thread re-raises it after the tic
    DN_ErrorTrapInstall(&ticGuard);
    if (setjmp(ticGuard) == 0)
    {
        // the game tic: everything the game module does this frame
        ge->RunFrame();

        // push the changed Duke world states to the collision model
        for (i = 1; i < ge->num_edicts; i++)
        {
            edict_t *e = EDICT_NUM(i);
            if (e->inuse && (e->s.renderfx & RF_DUKE_WORLD))
            {
                CM_DukeApplyWorldState(&e->s, false);
            }
        }
    }
    else
    {
        trapped = true;
    }
    DN_ErrorTrapRemove();
    return 0;
}

void DN_GameThread::RunGameTic()
{
    if (com_smp->value)
    {
        // like the BFG RunGameAndDraw: the thread starts once and every
        // tic is one SignalWork / WaitForThread cycle
        if (!threadStarted)
        {
            if (!StartWorkerThread("GameThread"))
            {
                Com_Error(ERR_FATAL, "game thread: could not start the game thread");
            }
            threadStarted = true;
        }
        SignalWork();
        WaitForThread();
    }
    else
    {
        // like the BFG non-smp path: run the tic in the main thread so
        // profiling catches everything
        Run();
    }

    // re-raise a game error trapped during the tic, now on the main
    // thread where the normal drop handling runs
    if (trapped)
    {
        trapped = false;
        Com_Error(DN_ErrorTrapCode(), "%s", DN_ErrorTrapMessage());
    }
}
