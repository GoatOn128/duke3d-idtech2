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
// dnjobs.cpp -- parallel job lists ported from Doom 3 BFG Edition
// (neo/idlib/ParallelJobList.cpp, the idParallelJobList_Threads backend).
// Divergences from BFG, kept on purpose:
// - only the threads backend exists (no PS3 SPURS, no HDD/disk jobs)
// - job threads are SDL worker threads woken per submit instead of
//   permanent stall loops, and OS thread priorities are not raised
// - the auto thread count uses SDL_GetNumLogicalCPUCores() instead of
//   querying physical cores minus dedicated cores
// - the sync markers wait on an executed-job counter instead of the
//   BFG doneGuards version machinery; the caller-visible behavior is
//   the same

#include "dnjobs.h"

#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_timer.h>

#include "qcommon.h"

// marker jobs, like the &JOB_* sentinels in the BFG job lists
namespace
{
void JobNoneFunc(void *) {}
void JobSignalFunc(void *) {}
void JobSyncFunc(void *) {}
void JobDoneFunc(void *) {}
}

static void DN_JobsSubmitList(DN_JobList *list);
static void DN_JobsTest_f(void);

#define INITIAL_JOBS 256

// wait until the executed count reaches the given position
static void DN_WaitExecuted(SDL_AtomicInt *count, int target)
{
    while (SDL_GetAtomicInt(count) < target)
    {
        SDL_Delay(0); // like Sys_Yield() in BFG
    }
}

// ----------------------------------------------------------------------
// DN_JobList
// ----------------------------------------------------------------------

DN_JobList::DN_JobList(const char *name, int priority)
{
    this->name = name;
    this->priority = priority;

    jobs = NULL;
    maxJobs = 0;
    numJobs = 0;

    currentJob = 0;
    version = 0;

    SDL_SetAtomicInt(&executedCount, 0);
    SDL_SetAtomicInt(&syncGatePos, -1);
    listDone.Init(true);

    submitted = false;
}

DN_JobList::~DN_JobList()
{
    if (submitted)
    {
        // the caller must Wait() before freeing the list
        Com_Error(ERR_FATAL, "DN_JobList destructor: '%s' still submitted", name);
    }
    listDone.Shutdown();
    free(jobs);
}

void DN_JobList::AddJob(jobRun_t run, void *data)
{
    if (submitted)
    {
        Com_Error(ERR_FATAL, "DN_JobList::AddJob: '%s' is already submitted", name);
    }

    DN_ScopedCriticalSection lock(fetchLock);
    if (numJobs == maxJobs)
    {
        int newMax = (maxJobs == 0) ? INITIAL_JOBS : maxJobs * 2;
        job_t *newJobs = (job_t *)realloc(jobs, newMax * sizeof(job_t));
        if (!newJobs)
        {
            Com_Error(ERR_FATAL, "DN_JobList::AddJob: out of memory for '%s'", name);
        }
        jobs = newJobs;
        maxJobs = newMax;
    }
    jobs[numJobs].function = run;
    jobs[numJobs].data = data;
    numJobs++;
}

void DN_JobList::InsertSyncPoint(jobSyncType_t syncType, DN_SysSignal *signal)
{
    if (syncType == SYNC_SIGNAL)
    {
        AddJob(JobSignalFunc, signal);
    }
    else if (syncType == SYNC_SYNCHRONIZE)
    {
        AddJob(JobSyncFunc, NULL);
    }
    else
    {
        AddJob(JobNoneFunc, NULL);
    }
}

bool DN_JobList::IsSubmitted()
{
    return submitted;
}

bool DN_JobList::FetchNextJob(job_t &job, int &position, int &gate)
{
    DN_ScopedCriticalSection lock(fetchLock);

    if (currentJob >= numJobs)
    {
        return false;
    }
    job = jobs[currentJob];
    position = currentJob;

    // a sync marker gates every job fetched after it
    if (job.function == JobSyncFunc)
    {
        SDL_SetAtomicInt(&syncGatePos, currentJob);
    }
    // the gate that applies to this job is the marker fetched before it
    gate = SDL_GetAtomicInt(&syncGatePos);

    currentJob++;
    return true;
}

void DN_JobList::RunJobs()
{
    job_t job;
    int position, gate;

    while (FetchNextJob(job, position, gate))
    {
        if (job.function == JobSyncFunc)
        {
            // all jobs ahead of the marker must be done before moving on
            DN_WaitExecuted(&executedCount, position);
            SDL_AddAtomicInt(&executedCount, 1);
        }
        else if (job.function == JobSignalFunc)
        {
            DN_WaitExecuted(&executedCount, position);
            DN_SysSignal *signal = (DN_SysSignal *)job.data;
            if (signal)
            {
                signal->Raise();
            }
            SDL_AddAtomicInt(&executedCount, 1);
        }
        else if (job.function == JobDoneFunc)
        {
            DN_WaitExecuted(&executedCount, position);
            submitted = false;
            SDL_AddAtomicInt(&executedCount, 1);
            listDone.Raise();
        }
        else if (job.function == JobNoneFunc)
        {
            SDL_AddAtomicInt(&executedCount, 1);
        }
        else
        {
            // if a sync marker was fetched before this job, wait for it
            while (SDL_GetAtomicInt(&executedCount) <= gate)
            {
                SDL_Delay(0);
            }
            job.function(job.data);
            SDL_AddAtomicInt(&executedCount, 1);
        }
    }
}

void DN_JobList::Submit()
{
    if (submitted)
    {
        Com_Error(ERR_FATAL, "DN_JobList::Submit: '%s' is already submitted", name);
    }

    listDone.Clear();

    // the done marker runs after every real job and raises listDone
    AddJob(JobDoneFunc, NULL);

    {
        DN_ScopedCriticalSection lock(fetchLock);
        submitted = true;
        currentJob = 0;
        SDL_SetAtomicInt(&executedCount, 0);
        SDL_SetAtomicInt(&syncGatePos, -1);
        version++;
    }

    DN_JobsSubmitList(this);
}

bool DN_JobList::Wait(int timeout)
{
    return listDone.Wait(timeout);
}

bool DN_JobList::TryWait()
{
    return listDone.Check();
}

// reuse: drop the finished jobs so the list can be filled and submitted
// again; safe once Wait() returned, the job threads are done with the list
void DN_JobList::ClearFinishedJobs()
{
    DN_ScopedCriticalSection lock(fetchLock);
    numJobs = 0;
}

// ----------------------------------------------------------------------
// DN_JobThread
// ----------------------------------------------------------------------

// BFG keeps a pending buffer of job lists per thread and picks the
// highest priority entry on every round. Same here, as a plain array
// that drops the oldest entry when full, like the cyclic overwrite.
class DN_JobThread : public DN_SysThread
{
public:
    DN_JobThread(int threadNumber);

    void AddJobList(DN_JobList *list);

private:
    int Run();

    struct entry_t
    {
        DN_JobList *list;
        int version;
    };

    DN_JobList *FetchJobList();

    entry_t entries[MAX_JOBLISTS];
    int numEntries;
    DN_SysMutex lock;
    int threadNumber;
};

DN_JobThread::DN_JobThread(int threadNumber)
{
    this->threadNumber = threadNumber;
    numEntries = 0;
}

void DN_JobThread::AddJobList(DN_JobList *list)
{
    entry_t entry;

    entry.list = list;
    entry.version = list->GetVersion();

    DN_ScopedCriticalSection scope(lock);
    if (numEntries == MAX_JOBLISTS)
    {
        // drop the oldest entry, like the BFG cyclic buffer overwrite
        memmove(entries, entries + 1, (MAX_JOBLISTS - 1) * sizeof(entry_t));
        numEntries--;
    }
    entries[numEntries] = entry;
    numEntries++;
}

DN_JobList *DN_JobThread::FetchJobList()
{
    DN_ScopedCriticalSection scope(lock);

    // drop stale entries (the list was resubmitted and pushed a fresh
    // entry), then pick the highest priority list
    int kept = 0;
    int best = -1;
    for (int i = 0; i < numEntries; i++)
    {
        if (entries[i].version != entries[i].list->GetVersion())
        {
            continue;
        }
        if (best == -1 || entries[i].list->GetPriority() > entries[kept].list->GetPriority())
        {
            best = kept;
        }
        entries[kept] = entries[i];
        kept++;
    }
    numEntries = kept;

    if (best == -1)
    {
        return NULL;
    }
    DN_JobList *list = entries[best].list;
    memmove(entries + best, entries + best + 1, (numEntries - best - 1) * sizeof(entry_t));
    numEntries--;
    return list;
}

int DN_JobThread::Run()
{
    for (;;)
    {
        if (IsTerminating())
        {
            break;
        }
        DN_JobList *list = FetchJobList();
        if (!list)
        {
            break; // out of work: sleep until the next submit
        }
        list->RunJobs();
    }
    return 0;
}

// ----------------------------------------------------------------------
// job manager
// ----------------------------------------------------------------------

static DN_JobThread *jobThreads[MAX_JOB_THREADS];
static int numJobThreads;
static bool jobsInitialized;
static DN_JobList *parallelForList; // utility list, like BFG JOBLIST_UTILITY
static cvar_t *jobs_numThreadsRealtime;
static cvar_t *jobs_numThreadsNonInteractive;

static void DN_JobsSubmitList(DN_JobList *list)
{
    int i;

    if (!jobsInitialized)
    {
        // no threads yet: run the jobs on the submitting thread
        list->RunJobs();
        return;
    }
    for (i = 0; i < numJobThreads; i++)
    {
        jobThreads[i]->AddJobList(list);
    }
    for (i = 0; i < numJobThreads; i++)
    {
        jobThreads[i]->SignalWork();
    }
}

static int DN_JobsThreadCount(void)
{
    int count;

    if (jobs_numThreadsRealtime->value < 0)
    {
        // auto: leave one core for the main thread
        count = SDL_GetNumLogicalCPUCores() - 1;
    }
    else
    {
        count = (int)jobs_numThreadsRealtime->value;
    }
    if (count < 1)
    {
        count = 1;
    }
    if (count > MAX_JOB_THREADS)
    {
        count = MAX_JOB_THREADS;
    }
    return count;
}

void DN_JobsInit(void)
{
    int i;

    if (jobsInitialized)
    {
        return;
    }

    jobs_numThreadsRealtime = Cvar_Get("jobs_numThreadsRealtime", "-1", CVAR_ARCHIVE);
    // kept for BFG parity; nothing submits non-interactive jobs yet
    jobs_numThreadsNonInteractive = Cvar_Get("jobs_numThreadsNonInteractive", "-1", CVAR_ARCHIVE);

    numJobThreads = DN_JobsThreadCount();
    for (i = 0; i < numJobThreads; i++)
    {
        jobThreads[i] = new DN_JobThread(i);
        if (!jobThreads[i]->StartWorkerThread(va("JLProc_%d", i)))
        {
            Com_Error(ERR_FATAL, "DN_JobsInit: could not start job thread %d", i);
        }
    }

    Cmd_AddCommand("jobs_test", DN_JobsTest_f);

    parallelForList = DN_JobsAllocJobList("ParallelFor", DN_JOB_PRIORITY_NORMAL);

    jobsInitialized = true;
    Com_Printf("job system: %d threads\n", numJobThreads);
}

void DN_JobsShutdown(void)
{
    int i;

    if (!jobsInitialized)
    {
        return;
    }
    jobsInitialized = false;

    for (i = 0; i < numJobThreads; i++)
    {
        jobThreads[i]->StopThread(true);
        delete jobThreads[i];
        jobThreads[i] = NULL;
    }
    numJobThreads = 0;

    DN_JobsFreeJobList(parallelForList);
    parallelForList = NULL;
}

DN_JobList *DN_JobsAllocJobList(const char *name, int priority)
{
    return new DN_JobList(name, priority);
}

void DN_JobsFreeJobList(DN_JobList *list)
{
    delete list;
}

int DN_JobsNumThreads(void)
{
    return numJobThreads;
}

// ----------------------------------------------------------------------
// parallel for
// ----------------------------------------------------------------------

typedef struct
{
    int index;
    void (*run)(int index, void *userdata);
    void *userdata;
} parallelForJob_t;

static void DN_ParallelForJob(void *data)
{
    parallelForJob_t *job = (parallelForJob_t *)data;
    job->run(job->index, job->userdata);
}

int DN_JobsParallelFor(int count, void (*run)(int index, void *userdata), void *userdata)
{
    parallelForJob_t *jobs;
    int i;

    if (!jobsInitialized || !parallelForList || count <= 0)
    {
        return 0;
    }

    jobs = (parallelForJob_t *)malloc(count * sizeof(*jobs));
    if (!jobs)
    {
        return 0; // the caller falls back to running the work serially
    }
    for (i = 0; i < count; i++)
    {
        jobs[i].index = i;
        jobs[i].run = run;
        jobs[i].userdata = userdata;
    }

    parallelForList->ClearFinishedJobs();
    for (i = 0; i < count; i++)
    {
        parallelForList->AddJob(DN_ParallelForJob, &jobs[i]);
    }
    parallelForList->Submit();
    parallelForList->Wait();
    free(jobs);
    return 1;
}

// ----------------------------------------------------------------------
// jobs_test
// ----------------------------------------------------------------------

#define JOBS_TEST_JOBS 4096

static SDL_AtomicInt jobsTestCounter;
static DN_SysSignal jobsTestSignal;

static void DN_JobsTestJob(void *)
{
    SDL_AddAtomicInt(&jobsTestCounter, 1);
}

// runs two blocks of parallel jobs with a sync point between them and a
// signal at the end, then prints the result
static void DN_JobsTest_f(void)
{
    DN_JobList *list;
    int i, start, elapsed;

    if (!jobsInitialized)
    {
        Com_Printf("job system not initialized\n");
        return;
    }

    list = DN_JobsAllocJobList("jobs_test", DN_JOB_PRIORITY_NORMAL);

    SDL_SetAtomicInt(&jobsTestCounter, 0);
    jobsTestSignal.Init(false);

    start = Sys_Milliseconds();

    for (i = 0; i < JOBS_TEST_JOBS; i++)
    {
        list->AddJob(DN_JobsTestJob, NULL);
    }
    list->InsertSyncPoint(SYNC_SYNCHRONIZE);
    for (i = 0; i < JOBS_TEST_JOBS; i++)
    {
        list->AddJob(DN_JobsTestJob, NULL);
    }
    list->InsertSyncPoint(SYNC_SIGNAL, &jobsTestSignal);

    list->Submit();
    list->Wait();

    elapsed = Sys_Milliseconds() - start;

    Com_Printf("jobs_test: %d jobs on %d threads in %d ms, counter %d, signal %s\n",
               JOBS_TEST_JOBS * 2, numJobThreads, elapsed,
               SDL_GetAtomicInt(&jobsTestCounter),
               jobsTestSignal.Check() ? "raised" : "missed");

    jobsTestSignal.Shutdown();
    DN_JobsFreeJobList(list);
}
