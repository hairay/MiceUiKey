//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /cvs/MICECore/threads/tsCommon/tsmutex.c,v 1.2 2015/08/10 07:37:04 AV00287 Exp $
//  $Change: 245416 $ $Date: 2015/08/10 07:37:04 $
//  
/// @file
/// Mutex API functions
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "bios.h"
#include "threads.h"


typedef struct tag_mx
{
	int			inuse;		///< mutex is allocated
	void	   *os_mutex;	///< underlying os mutex primitive
#if IN_DEBUG_LOCKS
	const char *lfile;		///< locker, filename
	int			lline;		///< locker, line number
#endif
}
MUTEX, *PMUTEX;

static MUTEX	s_mutexPool[TS_NUM_MTXS];
static void    *s_os_mutex = NULL;

//***********************************************************************
static PMUTEX GetMutex(TsMutex mtx)
{
	if ((PMUTEX)mtx >= &s_mutexPool[0] && (PMUTEX)mtx <= &s_mutexPool[TS_NUM_MTXS - 1])
	{
		return (PMUTEX)mtx;
	}
	BIOSerror("No such Mutex %p\n", mtx);
	return NULL;
}

//***********************************************************************
TsMutex TScreateMutex(void)
{
	int i;

	_ts_mutexWait(s_os_mutex, -1, 0);
	for (i = 0; i < TS_NUM_MTXS; i++)
	{
		if (! s_mutexPool[i].inuse)
		{
			s_mutexPool[i].inuse = 1;
			_ts_mutexCreate(&s_mutexPool[i].os_mutex);
			break;
		}
	}
	_ts_mutexSignal(s_os_mutex);
	if (i >= TS_NUM_MTXS)
	{
		return NULL;
	}
	return (TsMutex)&s_mutexPool[i];
}

//***********************************************************************
int TSdestroyMutex(TsMutex mtx)
{
	int i;

	_ts_mutexWait(s_os_mutex, -1 , 0);
	for (i = 0; i < TS_NUM_MTXS; i++)
	{
		if (s_mutexPool[i].inuse && (PMUTEX)mtx == &s_mutexPool[i])
		{
			s_mutexPool[i].inuse = 0;
			_ts_mutexDestroy(s_mutexPool[i].os_mutex);
			s_mutexPool[i].os_mutex = NULL;
			_ts_mutexSignal(s_os_mutex);
			return 0;
		}
	}
	_ts_mutexSignal(s_os_mutex);
	return -1;
}

#if IN_DEBUG_LOCKS
	#undef     TSmutexWait
#endif

//***********************************************************************
int TSmutexWait(TsMutex mtx)
{
	return TSmutexTimeout(mtx, -1, 0);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSmutexWaitDebug(
                       TsMutex     mtx,
                       const char *file,
                       int         line
                      )
{
	return TSmutexTimeoutDebug(mtx, -1, 0, file, line);
}
#endif

#if IN_DEBUG_LOCKS
	#undef TSmutexTimeout
#endif

//***********************************************************************
int TSmutexTimeout(TsMutex mtx, int secs, int usecs)
{
	PMUTEX pmtx;
	
	pmtx = GetMutex(mtx);
	if (! pmtx)
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
	return _ts_mutexWait(pmtx->os_mutex, secs, usecs);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSmutexTimeoutDebug(
                      TsMutex     mtx,
                      int         secs,
                      int         usecs,
                      const char *file,
                      int         line
                      )
{
	PMUTEX pmtx;
	
	pmtx = GetMutex(mtx);
	if (! pmtx)
	{
		return -1;
	}
	pmtx->lfile = file;
	pmtx->lline = line;
	return TSmutexTimeout(mtx, secs, usecs);
}
#endif

#if IN_DEBUG_LOCKS
	#undef TSmutexSignal
#endif

//***********************************************************************
int TSmutexSignal(TsMutex mtx)
{
	PMUTEX pmtx;
	
	pmtx = GetMutex(mtx);
	if (! pmtx)
	{
		return -1;
	}
	return _ts_mutexSignal(pmtx->os_mutex);
}

#if IN_DEBUG_LOCKS
//***********************************************************************
int TSmutexSignalDebug(
                      TsMutex     mtx,
                      const char *file,
                      int         line
                      )
{
	PMUTEX pmtx;
	
	pmtx = GetMutex(mtx);
	if (! pmtx)
	{
		return -1;
	}
	pmtx->lfile = NULL;
	pmtx->lline = 0;
	return TaskMutexSignal(mtx);
}
#endif

//***********************************************************************
int TSmutexProlog()
{
	int i;

	_ts_mutexCreate(&s_os_mutex);
	for (i = 0; i < TS_NUM_MTXS; i++)
	{
		s_mutexPool[i].inuse = 0;
		s_mutexPool[i].os_mutex  = NULL;
		#if IN_DEBUG_LOCKS
		s_mutexPool[i].lfile = NULL;
		s_mutexPool[i].lline = 0;
		#endif
	}
	return 0;
}

//***********************************************************************
void TSmutexEpilog()
{
	_ts_mutexDestroy(s_os_mutex);
	s_os_mutex = NULL;
}

