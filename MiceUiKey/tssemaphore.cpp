//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /cvs/MICECore/threads/tsCommon/tssemaphore.c,v 1.2 2015/08/10 07:37:04 AV00287 Exp $
//  $Change: 245416 $ $Date: 2015/08/10 07:37:04 $
//  
/// @file
/// Semaphore API functions
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"
#include "bios.h"

typedef struct tag_mx
{
	int			inuse;		///< mutex is allocated
	void	   *os_sem;		///< underlying os semaphore primitive
#if IN_DEBUG_LOCKS
	const char *lfile;		///< locker, filename
	int			lline;		///< locker, line number
#endif
}
SEMA, *PSEMA;

static SEMA		s_semaphorePool[TS_NUM_SEMS];
static TsMutex	s_semaphoreMutex = NULL;

//***********************************************************************
static PSEMA GetSem(TsSemaphore sem)
{
	if ((PSEMA)sem >= &s_semaphorePool[0] && (PSEMA)sem <= &s_semaphorePool[TS_NUM_SEMS - 1])
	{		
		return (PSEMA)sem;
	}
	BIOSerror("No such semaphore %p\n", sem);
	return NULL;
}

//***********************************************************************
TsSemaphore TScreateSemaphore(int count)
{
	int rv;
	int i;

	rv = -1;
	TSmutexWait(s_semaphoreMutex);
	for (i = 0; i < TS_NUM_SEMS; i++)
	{
		if (s_semaphorePool[i].inuse == 0)
		{
			rv = _ts_semaphoreCreate(&s_semaphorePool[i].os_sem, count);
			if (! rv)
			{
				s_semaphorePool[i].inuse = 1;
			}
			else
			{
				i = TS_NUM_SEMS;
			}
			break;
		}
	}
	TSmutexSignal(s_semaphoreMutex);
	if (i >= TS_NUM_SEMS)
	{
		BIOSerror("Out of semaphores\n");
		return NULL;
	}
	return (TsSemaphore)&s_semaphorePool[i];
}

TsSemaphore TSsemCreateD(int count, const char *file)
{
	return TScreateSemaphore(count);
}

//***********************************************************************
int TSdestroySemaphore(TsSemaphore sem)
{
	PSEMA ps;

	TSmutexWait(s_semaphoreMutex);
	ps = GetSem(sem);
	if (! ps)
	{
		TSmutexSignal(s_semaphoreMutex);
		return -1;
	}
	_ts_semaphoreDestroy(ps->os_sem);
	ps->inuse  = 0;
	ps->os_sem = NULL;
	TSmutexSignal(s_semaphoreMutex);
	return 0;
}

#if IN_DEBUG_LOCKS
	#undef     TSsemWait
#endif

//***********************************************************************
int TSsemWait(TsSemaphore sem)
{
	return TSsemTimeout(sem, -1, 0);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSsemWaitDebug(
                       TsSemaphore sem,
                       const char *file,
                       int         line
                      )
{
	return TSsemTimeoutDebug(sem, -1, 0, file, line);
}
#endif

#if IN_DEBUG_LOCKS
	#undef TSsemTimeout
#endif

//***********************************************************************
int TSsemTimeout(TsSemaphore sem, int secs, int usecs)
{
	PSEMA ps;
	
	ps = GetSem(sem);
	if (! ps)
	{
		return -1;
	}
	while (usecs >= 1000000)
	{
		// some threading implementations (posix) flag usecs >= 1000000 as invalid
		//
		secs++;
		usecs -= 1000000;
	}
	return _ts_semaphoreWait(ps->os_sem, secs, usecs);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSsemTimeoutDebug(
                      TsSemaphore sem,
                      int         secs,
                      int         usecs,
                      const char *file,
                      int         line
                      )
{
	PSEMA os;
	
	ps = GetSem(sem);
	if (! ps)
	{
		return INVALIDSEM;
	}
	ps->lfile = file;
	ps->lline = line;
	return TSsemTimeout(sem, secs, usecs);
}
#endif

#if IN_DEBUG_LOCKS
	#undef TSsemSignal
#endif

//***********************************************************************
int TSsemSignal(TsSemaphore sem)
{
	PSEMA ps;
	
	ps = GetSem(sem);
	if (! ps)
	{
		return -1;
	}
	return _ts_semaphoreSignal(ps->os_sem);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSsemSignalDebug(
                      TsSemaphore sem,
                      const char *file,
                      int         line
                      )
{
	PSEMA ps;
	
	ps = GetSem(sem);
	if (! ps)
	{
		return -1;
	}
	ps->lfile = NULL;
	ps->lline = 0;
	return TSsemSignal(sem);
}
#endif

//***********************************************************************
int TSsemProlog()
{
	int i;

	s_semaphoreMutex = TScreateMutex();
	for (i = 0; i < TS_NUM_SEMS; i++)
	{
		s_semaphorePool[i].inuse  = 0;
		s_semaphorePool[i].os_sem = NULL;
		#if IN_DEBUG_LOCKS
		s_semaphorePool[i].lfile = NULL;
		s_semaphorePool[i].lline = 0;
		#endif
	}
	return 0;
}

//***********************************************************************
void TSsemEpilog()
{
	TSdestroyMutex(s_semaphoreMutex);
	s_semaphoreMutex = NULL;
}

