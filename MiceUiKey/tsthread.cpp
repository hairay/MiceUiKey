//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /mfp/MiceUiKey/MiceUiKey/tsthread.cpp,v 1.1 2016/05/30 03:48:52 AV00287 Exp $
//  $Change: 245218 $ $Date: 2016/05/30 03:48:52 $
//  
/// @file
/// Threading API functions
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"
#include "bios.h"

#ifndef IN_IDLE_THREAD
// The idle thread runs when no other thread is ready.  Some underlying os
// types require an idle thread and some don't.  Inferno generally runs
// the UI thread as the idle thread, but different ports require different things.
//
// The idle thread here puts the Arm cpu in a WFI state for power saving
//
#if defined(HOST_BASED) || defined(Linux)

    // non embedded systems don't need idle threads
    // Linux (embedded) has it's own built in WFI typically.
    //
    #define IN_IDLE_THREAD 0
#else
    
    // embedded rtos systems will use our idle thread by default
    //
    #define IN_IDLE_THREAD 0
#endif
#endif

typedef struct tag_tsthread
{
    int         inuse;      ///< is allocated
    void       *os_thread;  ///< ptr to underlying os thread data
    const char *name;       ///< thread name
    #if IN_THREAD_TRACE
    Uint32      tottime;    ///< time spent this thread per period
    #endif
}
TSTHREAD, *PTSTHREAD;

static TSTHREAD s_threadPool[TS_NUM_THREADS];
static TsMutex  s_threadMutex = NULL;

//***********************************************************************
static PTSTHREAD GetThread(TsThread thread)
{
    if ((PTSTHREAD)thread >= &s_threadPool[0] && (PTSTHREAD)thread <= &s_threadPool[TS_NUM_THREADS - 1])
    {       
        return (PTSTHREAD)thread;
    }
    BIOSerror("No such thread %p\n", thread);
    return NULL;
}

//***********************************************************************
void TSsleepMilliseconds(int millisecs)
{
    _ts_sleep(millisecs);
}

//***********************************************************************
void TSsleep(int secs, int usecs)
{
    int millisecs;
    
    if (secs == 0 && usecs < 100)
    {
        BIOShrtMicroSleep(usecs);
    }
    else
    {
       millisecs = (secs * 1000) + (usecs + 999) / 1000;
       _ts_sleep(millisecs);
    }
}

//***********************************************************************
TsThread TScreateThread(
                     Sint32     (*entry)(void *),
                     Sint32      stacksize,
                     Sint32      priority,
                     void       *arg,
                     const char *name
                   )
{
    int i;
    void *osthread;

    TSmutexWait(s_threadMutex);
    for (i = 0; i < TS_NUM_THREADS; i++)
    {
        if (s_threadPool[i].inuse == 0)
        {
            if (! _ts_createThread(&osthread, entry, stacksize, priority, arg, name))
            {
                s_threadPool[i].inuse       = 1;
                s_threadPool[i].os_thread   = osthread;
                s_threadPool[i].name        = name;
                #if IN_THREAD_TRACE
                s_threadPool[i].tottime     = 0;
                #endif
            }
            break;
        }
    }
    TSmutexSignal(s_threadMutex);
    if (i >= TS_NUM_THREADS || s_threadPool[i].inuse == 0)
    {
        return NULL;
    }
    return (TsThread)&s_threadPool[i];
}

//***********************************************************************
TsThread TScreateThreadX(
                     Sint32     (*entry)(void *),
                     Sint32      stacksize,
                     void       *stack,
                     void       *data,
                     Sint32      priority,
                     void       *arg,
                     const char *name
                   )
{
    return TScreateThread(entry, stacksize, priority, arg, name);
}

//***********************************************************************
int TSdestroyThread(TsThread id)
{
    PTSTHREAD pt;

    TSmutexWait(s_threadMutex);
    pt = GetThread(id);
    if (! pt)
    {
        TSmutexSignal(s_threadMutex);
        return -1;
    }
    if (pt->os_thread == _ts_thisThread())
    {
        TSmutexSignal(s_threadMutex);
        BIOSerror("Can't kill thread self\n");
        return -1;
    }
    _ts_destroyThread(pt->os_thread);
    pt->inuse     = 0;
    pt->os_thread = NULL;
    pt->name      = NULL;
    TSmutexSignal(s_threadMutex);
    return 0;
}

//***********************************************************************
TsThread TSgetThread(void)
{
    int i;
    void *mythread;
    
    mythread = _ts_thisThread();

    for (i = 0; i < TS_NUM_THREADS; i++)
    {
        if (s_threadPool[i].inuse && (s_threadPool[i].os_thread == mythread))
        {
            return (TsThread)&s_threadPool[i];
        }
    }
    return NULL;
}

//***********************************************************************
int TSsetThreadAffinity(TsThread thread, int cpu)
{
    PTSTHREAD pt;

    pt = GetThread(thread);
    if (! pt)
    {
        return -1;
    }
    return _ts_setAffinity(pt->os_thread, cpu);
}

//***********************************************************************
int TSthreadProlog()
{
    int i;

    s_threadMutex = TScreateMutex();
    for (i = 0; i < TS_NUM_THREADS; i++)
    {
        s_threadPool[i].inuse       = 0;
        s_threadPool[i].os_thread   = NULL;
        s_threadPool[i].name        = NULL;
        #if IN_THREAD_TRACE
        s_threadPool[i].tottime     = 0;
        #endif
    }
    return 0;
}

//***********************************************************************
void TSthreadEpilog()
{
    TSdestroyMutex(s_threadMutex);
    s_threadMutex = NULL;
}

#if IN_THREAD_TRACE

// time in seconds between thread time reporting (todo, get from tsport.h?)
//
#ifndef TS_THREAD_REPORT_PERIOD
    #define TS_THREAD_REPORT_PERIOD 2
#endif
//***********************************************************************
void TStraceReport(Uint32 periodus)
{
    static int s_sorted_threads[TS_NUM_THREADS + 2];
    const char *tname;
    Uint32 percpu, cputime;
    int    id;
    int    i, j, n;
    int    t;
    
    for (t = n = 0; t < TS_NUM_THREADS; t++)
    {
        if (s_threadPool[t].os_thread && s_threadPool[t].inuse)
        {
             s_threadPool[t].tottime = _ts_threadTime(s_threadPool[t].os_thread);
             for (i = 0; i < n; i++)
             {
                 if (s_threadPool[t].tottime > s_threadPool[s_sorted_threads[i]].tottime)
                 {
                     for (j = n + 1; j > i; j--)
                     {
                         s_sorted_threads[j] = s_sorted_threads[j - 1];
                     }
                     break;
                 }
             }
             s_sorted_threads[i] = t;
             n++;
        }
    }
    BIOSlog("\e[H"); // cursor home
    for (i = 0; i < n; i++)
    {
        id = s_sorted_threads[i];
        tname = "???";
        if (id >= 0 && id < TS_NUM_THREADS)
        {
            tname = s_threadPool[id].name;
        }
        if (! tname)
        {
            tname = "???";
        }
        cputime = s_threadPool[id].tottime;
        s_threadPool[id].tottime = 0;
        percpu = cputime * 100 / periodus;
        BIOSlog("%10u Task %2d %3d%% (%s)\e[K\n", cputime, id, percpu, tname);
    }
}
#endif

#if IN_IDLE_THREAD

#ifndef IDLE_STACK_SIZE
    #define IDLE_STACK_SIZE TSSMALL_STACK
#endif
#ifndef IDLE_TASK_PRIORITY
    #define IDLE_TASK_PRIORITY TS_LOWEST_PRIORITY
#endif

static TsThread s_idleID = INVALIDTHREAD;

#if defined(CLKGENMISCCTRL) && defined(CLKGENMISCCTRL__LOW_PWR_SEL__MASK)

// For Quatro devices with a clock generator containing dividers,
// to permit instant switching between normal and  a reduced clock speed.
//
#define REDUCE_CLOCK_SUPPORTED   1

// Flag to indicate when a reduced clock is requested.
//
static int s_reduceClockRequested = 0;

//***********************************************************************
int TSreduceClock(int percent)
{
    if (percent <= 0)
    {
        WRREG_UINT32(
                CLKGENMISCCTRL, 
                RDREG_UINT32(CLKGENMISCCTRL)
                    & ~CLKGENMISCCTRL__LOW_PWR_DIV_SEL__MASK
        );
        s_reduceClockRequested = 0;
    }
    else
    {
        WRREG_UINT32(
                    CLKGENMISCCTRL, 
                    RDREG_UINT32(CLKGENMISCCTRL) 
                        | CLKGENMISCCTRL__LOW_PWR_DIV_SEL__MASK
        );
        s_reduceClockRequested = 1;
    }
    return 0;
}

#else

// instant clock switching is not supported.
//
#define REDUCE_CLOCK_SUPPORTED   0

//***********************************************************************
int TSreduceClock(int percent)
{
    return 0;
}

#endif

//***********************************************************************
static Sint32 idleThread(void *parm)
{
    time_t then;

    time(&then);

    while (1)
    {
    #if TS_LOWPOWER_WFI
    
        // When running on Quatro, the Arm CPU is put into a WFI
        // (wait for interrupt) state so it won't clock and use
        // power.  If this thread is running, it means no other 
        // thread is, which means other threads are all waiting
        // for something like I/O completion, etc.  In the worst
        // case the threading timer tick interrupt will wake this
        // which is when a task switch would normally occur anyway
        // so this just voids clocking the Arm until the next tick
        //
        // Disable CPU exception vectoring, let the Quatro wake the ARM.
        //
        ints_mask = BIOSintsOff();

        #if REDUCE_CLOCK_SUPPORTED

        // The low-power clock request bit is set.
        //
        if (s_reduceClockRequested)
        {
            WRREG_UINT32(
                        CLKGENMISCCTRL, 
                            RDREG_UINT32(CLKGENMISCCTRL)
                        |   CLKGENMISCCTRL__LOW_PWR_SEL__MASK
            );
            s_clock_reduced = 1;
        }
        #endif

        //  Halt the CPU and wait for an interrupt ("wfi")
        //
        asm("mcr p15, 0, %0, c7, c0, 4"::"r"(sbz));

        #if REDUCE_CLOCK_SUPPORTED

        // The low-power clock request bit is cleared.
        //
        if (s_clock_reduced)
        {
            WRREG_UINT32(
                        CLKGENMISCCTRL, 
                            RDREG_UINT32(CLKGENMISCCTRL)
                        &   ~CLKGENMISCCTRL__LOW_PWR_SEL__MASK
            );
            s_clock_reduced = 0;
        }
        #endif
        //  Re-enable CPU exception vectoring.
        //
        BIOSintsRestore(ints_mask);

    #else /* not doing WFI for low power */
        
        TSsleep(1, 0);
        
    #endif

    #if IN_THREAD_TRACE
        {
        time_t now;

        // If it's been "top" time since last spill, dump the thread trace
        //
        time(&now);
        if ((now - then) > TS_THREAD_REPORT_PERIOD)
        {
            then = now;
            TStraceReport(TS_THREAD_REPORT_PERIOD * 1000000);
        }
        }
    #endif
    }
    return 0;
}
#else
// no idle thread
//***********************************************************************
int TSreduceClock(int percent)
{
    return 0;
}
#endif

extern int TSmutexProlog(void);
extern int TSsemProlog(void);
extern int TSmsgProlog(void);

//***********************************************************************
int TSprolog()
{
    int rv;
    
    do  // try
    {
        // First setup memory allocator so underlying threads implementations
        // are free to use malloc/free as needed
        //
        #if IN_MEMMGR
        rv = MEMMGRprolog();
        if (rv)
        {
            break;
        }        
        #endif
        rv = TSmutexProlog();
        if (rv)
        {
            break;
        }
        rv = TSsemProlog();
        if (rv)
        {
            break;
        }
        rv = TSmsgProlog();
        if (rv)
        {
            break;
        }
        rv = TSthreadProlog();
        if (rv)
        {
            break;
        }
        // init logging
        //
        TSlogProlog();
        
        //TSlogSetLevel(7, 1, 0);

        // tell mem allocator to use locking protection now before threads start
        //
        #if IN_MEMMGR
        MEMMGRprotectOn();
        #endif
        #if IN_IDLE_THREAD
        // start the "idle" thread which is used for low-power and
        // other misc tasks like thread tracing and timing
        //
        s_idleID = TScreateThread(
                            idleThread,
                            IDLE_STACK_SIZE,
                            IDLE_TASK_PRIORITY,
                            NULL,
                            "IdleThread"
                            );
        if (s_idleID == INVALIDTHREAD)
        {
            BIOSerror("Can't start idle thread\n");
            rv = -2;
            break;
        }
        #endif
    }
    while (0); // catch
    return rv;
}

