//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /cvs/MICECore/threads/tsWindows_NT/tswindows.c,v 1.1 2015/05/04 01:21:51 AV00287 Exp $
//  $Change: 245438 $ $Date: 2015/05/04 01:21:51 $
//  
/// @file
//  Windows NT Thread layer API
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"


#ifdef _UNICODE
	#error "can't turn on _UNICODE"
#endif

int _ts_mutexCreate(void **pos_mtx)
{
    HANDLE hx;
    char mtxname[32];
    static int s_nummtxs = 0;
    
    snprintf(mtxname, 32, "mtx%d", s_nummtxs++);
    hx = CreateMutex(NULL, FALSE, mtxname);
    *pos_mtx = hx;
    return (hx == NULL) ? -1 : 0;
}

int _ts_mutexDestroy(void *os_mtx)
{
    CloseHandle((HANDLE)os_mtx);
    return 0;
}

int _ts_mutexWait(void *os_mtx, int secs, int usecs)
{
    int timeout;
    
    if (secs < 0 || (secs == 0 && usecs < 0))
    {
        timeout = INFINITE;
    }
    else
    {
        timeout = secs * 1000 + ((usecs + 999) / 1000);
    }
    switch (WaitForSingleObject((HANDLE)os_mtx, timeout))
    {
    case WAIT_FAILED:
        return -1;
    case WAIT_TIMEOUT:
        return 1;
    default:
        break;
    }
    return 0;
}

int _ts_mutexSignal(void *os_mtx)
{
    ReleaseMutex((HANDLE)os_mtx);
    return 0;
}

int _ts_semaphoreCreate(void **pos_sem, int count)
{
    HANDLE hs;
    char semname[32];
    static int s_numsems = 0;
    
    snprintf(semname, 32, "sem%d", s_numsems++);
    hs = CreateSemaphore(NULL, count, 0x7FFF, semname);
    *pos_sem = hs;
    return (hs == NULL) ? -1 : 0;
}

int _ts_semaphoreDestroy(void *os_sem)
{
    CloseHandle((HANDLE *)os_sem);
    return 0;
}

int _ts_semaphoreWait(void *os_sem, int secs, int usecs)
{
    int timeout;
    
    if (secs < 0 || (secs == 0 && usecs < 0))
    {
        timeout = INFINITE;
    }
    else
    {
        timeout = secs * 1000 + ((usecs + 999) / 1000);
    }
    switch (WaitForSingleObject((HANDLE)os_sem, timeout))
    {
    case WAIT_FAILED:
        return -1;
    case WAIT_TIMEOUT:
        return 1;
    default:
        break;
    }
    return 0;
}

int _ts_semaphoreSignal(void *os_sem)
{
    ReleaseSemaphore((HANDLE)os_sem, 1, NULL);
    return 0;
}

int _ts_createThread(
        void       **pos_thread,
        Sint32 (*entry)(void *),
        Sint32      stacksize,
        Sint32      priority,
        void       *arg,
        const char *name
)
{
    HANDLE ht;
    unsigned long threadID;
    
    ht = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)entry, arg, 0, &threadID);
    CloseHandle(ht);
    *pos_thread = (void*)threadID;
    return (ht == NULL) ? -1 : 0;
}

int _ts_destroyThread(void *pos_thread)
{
    HANDLE hThread;
    
    hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)pos_thread);
    if (hThread)
    {
        TerminateThread(hThread, -1);
        CloseHandle(hThread);
    }
    return 0;
}

void *_ts_thisThread(void)
{
    return (void *)GetCurrentThreadId();
}

void _ts_sleep(int milliseconds)
{
    Sleep(milliseconds);
}

int _ts_setAffinity(void *pos_thread, int cpu)
{
#if 0 /* need to test this ! */
    HANDLE hThread;
    
    hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, (DWORD)pos_thread);
    if (hThread)
    {
        DWORD_PTR prevmask;
        DWORD newmask;
        
        newmask = 1 << cpu;
        prevmask = SetThreadAffinityMask(hThread, &newmask);
        CloseHandle(hThread);
        
        return (prevmask == 0) ? -1 : 0;
    }
#endif
    return -1;
}


