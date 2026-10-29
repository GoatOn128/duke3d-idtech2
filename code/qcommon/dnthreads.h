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
// dnthreads.h -- thread primitives ported from Doom 3 BFG Edition
// (neo/idlib/Thread.h). The BFG Win32/PS3 system layer is replaced
// with SDL3 here. The parallel job lists in dnjobs.h build on these.

#ifndef DNTHREADS_H
#define DNTHREADS_H

#include <SDL3/SDL_atomic.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_thread.h>

// Waitable signal, like idSysSignal in BFG. A manual reset signal stays
// raised until Clear() is called; an auto reset signal stays raised until
// a Wait() consumes it.
class DN_SysSignal
{
public:
    DN_SysSignal();
    ~DN_SysSignal();

    void Init(bool manualReset);
    void Shutdown();
    void Raise();
    void Clear();
    bool Check();
    // returns false on timeout, timeout -1 waits forever
    bool Wait(int timeout = -1);

private:
    bool manualReset;
    bool raised;
    SDL_Mutex *mutex;
    SDL_Condition *condition;
};

// Atomic integer, like idSysInterlockedInteger in BFG.
class DN_SysInterlockedInteger
{
public:
    DN_SysInterlockedInteger();

    void SetValue(int value);
    int GetValue();
    // returns the new value, not the old one
    int Increment();
    int Decrement();
    int Add(int value);

private:
    SDL_AtomicInt value;
};

// Mutex, like idSysMutex in BFG.
class DN_SysMutex
{
public:
    DN_SysMutex();
    ~DN_SysMutex();

    void Lock();
    void Unlock();

private:
    SDL_Mutex *mutex;
};

// Locks a DN_SysMutex for the lifetime of the scope.
class DN_ScopedCriticalSection
{
public:
    DN_ScopedCriticalSection(DN_SysMutex &mutex) : mutex(mutex) { mutex.Lock(); }
    ~DN_ScopedCriticalSection() { mutex.Unlock(); }

private:
    DN_SysMutex &mutex;
};

// Thread base class, like idSysThread in BFG. Derive and implement Run().
// StartThread() calls Run() once. StartWorkerThread() calls Run() once for
// every SignalWork(); this is the model the job threads use.
class DN_SysThread
{
public:
    DN_SysThread();
    virtual ~DN_SysThread();

    bool StartThread(const char *name);
    bool StartWorkerThread(const char *name);
    void SignalWork();
    // waits for a worker thread to finish its current Run()
    void WaitForThread();
    void StopThread(bool wait = true);
    bool IsTerminating();

protected:
    virtual int Run() = 0; // the return value becomes the thread exit code

private:
    static int SDLCALL ThreadProc(void *self);
    int Handle();

    bool workerThread;
    SDL_AtomicInt terminating;
    DN_SysSignal signalMoreWorkToDo; // auto reset
    DN_SysSignal signalWorkerDone;   // manual reset
    SDL_Thread *thread;
};

#endif
