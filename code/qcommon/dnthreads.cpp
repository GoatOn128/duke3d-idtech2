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
// dnthreads.cpp -- SDL3 implementation of the BFG thread primitives
// (see dnthreads.h). Divergence from BFG: Sys_Yield() is SDL_Delay(0).

#include "dnthreads.h"

#include <SDL3/SDL_timer.h>

// ----------------------------------------------------------------------
// DN_SysSignal
// ----------------------------------------------------------------------

DN_SysSignal::DN_SysSignal()
{
    manualReset = false;
    raised = false;
    mutex = NULL;
    condition = NULL;
}

DN_SysSignal::~DN_SysSignal()
{
    Shutdown();
}

void DN_SysSignal::Init(bool manualReset)
{
    this->manualReset = manualReset;
    raised = false;
    mutex = SDL_CreateMutex();
    condition = SDL_CreateCondition();
}

void DN_SysSignal::Shutdown()
{
    if (condition)
    {
        SDL_DestroyCondition(condition);
        condition = NULL;
    }
    if (mutex)
    {
        SDL_DestroyMutex(mutex);
        mutex = NULL;
    }
}

void DN_SysSignal::Raise()
{
    if (!mutex)
    {
        return;
    }
    SDL_LockMutex(mutex);
    raised = true;
    // several threads can wait on the same signal
    SDL_BroadcastCondition(condition);
    SDL_UnlockMutex(mutex);
}

void DN_SysSignal::Clear()
{
    if (!mutex)
    {
        return;
    }
    SDL_LockMutex(mutex);
    raised = false;
    SDL_UnlockMutex(mutex);
}

bool DN_SysSignal::Check()
{
    bool result;

    if (!mutex)
    {
        return false;
    }
    SDL_LockMutex(mutex);
    result = raised;
    SDL_UnlockMutex(mutex);
    return result;
}

bool DN_SysSignal::Wait(int timeout)
{
    bool result = true;

    if (!mutex)
    {
        return false;
    }
    SDL_LockMutex(mutex);
    while (!raised && result)
    {
        if (timeout < 0)
        {
            SDL_WaitCondition(condition, mutex);
        }
        else
        {
            result = SDL_WaitConditionTimeout(condition, mutex, timeout);
        }
    }
    if (!manualReset)
    {
        raised = false;
    }
    SDL_UnlockMutex(mutex);
    return result;
}

// ----------------------------------------------------------------------
// DN_SysInterlockedInteger
// ----------------------------------------------------------------------

DN_SysInterlockedInteger::DN_SysInterlockedInteger()
{
    SDL_SetAtomicInt(&value, 0);
}

void DN_SysInterlockedInteger::SetValue(int v)
{
    SDL_SetAtomicInt(&value, v);
}

int DN_SysInterlockedInteger::GetValue()
{
    return SDL_GetAtomicInt(&value);
}

int DN_SysInterlockedInteger::Increment()
{
    return SDL_AddAtomicInt(&value, 1) + 1;
}

int DN_SysInterlockedInteger::Decrement()
{
    return SDL_AddAtomicInt(&value, -1) - 1;
}

int DN_SysInterlockedInteger::Add(int v)
{
    return SDL_AddAtomicInt(&value, v) + v;
}

// ----------------------------------------------------------------------
// DN_SysMutex
// ----------------------------------------------------------------------

DN_SysMutex::DN_SysMutex()
{
    mutex = SDL_CreateMutex();
}

DN_SysMutex::~DN_SysMutex()
{
    if (mutex)
    {
        SDL_DestroyMutex(mutex);
        mutex = NULL;
    }
}

void DN_SysMutex::Lock()
{
    if (mutex)
    {
        SDL_LockMutex(mutex);
    }
}

void DN_SysMutex::Unlock()
{
    if (mutex)
    {
        SDL_UnlockMutex(mutex);
    }
}

// ----------------------------------------------------------------------
// DN_SysThread
// ----------------------------------------------------------------------

DN_SysThread::DN_SysThread()
{
    workerThread = false;
    SDL_SetAtomicInt(&terminating, 0);
    thread = NULL;
}

DN_SysThread::~DN_SysThread()
{
    StopThread(true);
}

int SDLCALL DN_SysThread::ThreadProc(void *self)
{
    return ((DN_SysThread *)self)->Handle();
}

int DN_SysThread::Handle()
{
    int result = 0;

    if (workerThread)
    {
        // worker loop from idSysThread: wait for work, run, report done
        while (!IsTerminating())
        {
            signalMoreWorkToDo.Wait();
            if (IsTerminating())
            {
                break;
            }
            result = Run();
            signalWorkerDone.Raise();
        }
    }
    else
    {
        result = Run();
    }
    return result;
}

bool DN_SysThread::StartThread(const char *name)
{
    workerThread = false;
    SDL_SetAtomicInt(&terminating, 0);
    thread = SDL_CreateThread(ThreadProc, name, this);
    return thread != NULL;
}

bool DN_SysThread::StartWorkerThread(const char *name)
{
    workerThread = true;
    SDL_SetAtomicInt(&terminating, 0);
    signalMoreWorkToDo.Init(false);
    signalWorkerDone.Init(true);
    thread = SDL_CreateThread(ThreadProc, name, this);
    return thread != NULL;
}

void DN_SysThread::SignalWork()
{
    signalMoreWorkToDo.Raise();
}

void DN_SysThread::WaitForThread()
{
    if (workerThread)
    {
        signalWorkerDone.Wait();
    }
}

void DN_SysThread::StopThread(bool wait)
{
    if (!thread)
    {
        return;
    }
    SDL_SetAtomicInt(&terminating, 1);
    if (workerThread)
    {
        // wake the worker so it sees the terminate flag
        signalMoreWorkToDo.Raise();
    }
    if (wait)
    {
        SDL_WaitThread(thread, NULL);
    }
    else
    {
        SDL_DetachThread(thread);
    }
    thread = NULL;
}

bool DN_SysThread::IsTerminating()
{
    return SDL_GetAtomicInt(&terminating) != 0;
}
