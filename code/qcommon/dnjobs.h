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
// dnjobs.h -- parallel job lists ported from Doom 3 BFG Edition
// (neo/idlib/ParallelJobList.h, the idParallelJobList_Threads backend
// merged into one class). Only the threads backend is ported: BFG also
// had a PS3 SPURS backend and an HDD/disk job flag, neither of which
// applies here. Job consumers (multithreaded renderer work, uncapped
// fps interpolation prep) arrive in later phases of the port.

#ifndef DNJOBS_H
#define DNJOBS_H

#include "dnthreads.h"

typedef void (*jobRun_t)(void *data);

enum jobSyncType_t
{
    SYNC_NONE,
    SYNC_SIGNAL,
    SYNC_SYNCHRONIZE
};

// priorities, like the BFG job lists
enum
{
    DN_JOB_PRIORITY_LOW = 0,
    DN_JOB_PRIORITY_NORMAL,
    DN_JOB_PRIORITY_HIGH,
    MAX_JOB_PRIORITIES
};

#define MAX_JOBLISTS 32
#define MAX_JOB_THREADS 32

class DN_JobList
{
public:
    DN_JobList(const char *name, int priority);
    ~DN_JobList();

    void AddJob(jobRun_t run, void *data);
    // SYNC_SIGNAL: the given signal is raised once all earlier jobs ran
    // SYNC_SYNCHRONIZE: no job past this point starts before all earlier jobs ran
    void InsertSyncPoint(jobSyncType_t syncType, DN_SysSignal *signal = NULL);
    void Submit();
    // returns false on timeout, timeout -1 waits forever
    bool Wait(int timeout = -1);
    bool TryWait();
    bool IsSubmitted();

    // reuse: drop the finished jobs so the list can be filled and
    // submitted again; only call after Wait() returned true
    void ClearFinishedJobs();

    const char *GetName() { return name; }
    int GetPriority() { return priority; }
    // bumped by every Submit; used to detect stale job thread entries
    int GetVersion() { return version; }

    // run by the job threads; the lock in FetchNextJob makes it safe for
    // several threads to work the same list at once
    void RunJobs();

private:
    struct job_t
    {
        jobRun_t function;
        void *data;
    };

    bool FetchNextJob(job_t &job, int &position, int &gate);

    const char *name;
    int priority;

    job_t *jobs;
    int maxJobs;
    int numJobs;

    DN_SysMutex fetchLock; // protects the jobs array and the fetch state
    int currentJob;        // index of the next job to hand to a thread
    int version;           // plain int; a stale read only skips one round

    SDL_AtomicInt executedCount; // jobs done, markers included
    SDL_AtomicInt syncGatePos;   // position of the pending sync marker, -1 for none
    DN_SysSignal listDone;       // raised by the done marker
    bool submitted;
};

// global job manager, started from Qcommon_Init
void DN_JobsInit(void);
void DN_JobsShutdown(void);
DN_JobList *DN_JobsAllocJobList(const char *name, int priority);
void DN_JobsFreeJobList(DN_JobList *list);
int DN_JobsNumThreads(void);

// Run one job per index on the job threads, submitted to the utility
// list like the BFG JOBLIST_UTILITY jobs. Returns 0 when the job system
// is not running, in which case the caller should do the work serially.
int DN_JobsParallelFor(int count, void (*run)(int index, void *userdata), void *userdata);

#endif
