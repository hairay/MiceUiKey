//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /mfp/MiceUiKey/MiceUiKey/tsmsgq.cpp,v 1.1 2016/05/30 03:48:52 AV00287 Exp $
//  $Change: 245075 $ $Date: 2016/05/30 03:48:52 $
//  
/// @file
/// Mail/Message Queue API functions
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"
#include "bios.h"
// host based and Linux based systems don't have IRQ handlers
// which use message queues but rtos systems do.  Since
// locking a mutex is something an ISR can't do in many RTOS
// systems the msgq just turns interrupts off to protect the
//  message queue from multiple thread access
//
#if defined(HOST_BASED) || defined(Linux)

#define TSMSGQLMTX			1
#define TSMSGQLTYP 			TsMutex
#define TSMSGQLOCK(q)		TSmutexWait(q->lock)
#define TSMSGQUNLOCK(q)		TSmutexSignal(q->lock)

#else

#define TSMSGQLMTX			0
#define TSMSGQLTYP 			IntMask
#define TSMSGQLOCK(q)		q->lock = (IntMask)BIOSintsOff()
#define TSMSGQUNLOCK(q)		BIOSintsRestore((int)q->lock)

#endif

typedef struct tag_mq
{
	
	TSMSGQLTYP	lock;
	TsSemaphore	signal;
	int		msgCount;
	int		msgSize;
	int		msgHead;
	int		msgTail;
	Msg	   *msgPool;
}
MSGQ, *PMSGQ;

static MSGQ		s_msgQueuePool[TS_NUM_QUEUES];
static TsMutex	s_msgLock		 = NULL;

//	TSmutexSignal(pq->lock);

//***********************************************************************
static PMSGQ GetMsgQ(TsMailbox mb)
{
	if ((PMSGQ)mb >= &s_msgQueuePool[0] && (PMSGQ)mb <= &s_msgQueuePool[TS_NUM_QUEUES - 1])
	{
		return (PMSGQ)mb;
	}
	BIOSerror("No such Mailbox %p\n", mb);
	return NULL;
}

//***********************************************************************
TsMailbox TScreateMsgQueue()
{
	PMSGQ   pq;
	int     i;

	if (TS_NUM_QELEMENTS < 1)
	{
		return NULL;
	}
	// allocate a slot in the queue table
	//
	TSmutexWait(s_msgLock);
	for (i = 0, pq = NULL; i < TS_NUM_QUEUES; i++)
	{
		if (s_msgQueuePool[i].signal == NULL)
		{
			pq = &s_msgQueuePool[i];
			#if TSMSGQLMTX
			pq->lock   = TScreateMutex();
			#endif
			pq->signal = TScreateSemaphore(0);
			break;
		}
	}
	TSmutexSignal(s_msgLock);
	if (! pq)
	{
		return NULL;
	}
	#if TSMSGQLMTX
	if (pq->lock == NULL)
	{
		BIOSerror("No lock\n");
		return NULL;
	}
	#endif
	if (pq->signal == NULL)
	{
		BIOSerror("No sem\n");
		return NULL;
	}
	// setup queue
	//
	do 	// try
	{
		pq->msgPool = (Msg*)malloc(sizeof(Msg) * TS_NUM_QELEMENTS);
		if (! pq->msgPool)
		{
			break;
		}
		pq->msgSize  = TS_NUM_QELEMENTS;
		pq->msgHead  = 0;
		pq->msgTail  = 0;
		pq->msgCount = 0;
		return (TsMailbox)pq;
	}
	while (0); // catch
	
	// failed
	#if TSMSGQLMTX
	TSdestroyMutex(pq->lock);
	#endif
	TSdestroySemaphore(pq->signal);
	return NULL;
}

//***********************************************************************
int TSdestroyMsgQueue(TsMailbox msgq)
{
	PMSGQ pq;
	int   rv;
	
	rv = -1;
	TSmutexWait(s_msgLock);
	pq = GetMsgQ(msgq);
	if (pq)
	{
		if (pq->msgPool)
		{
			free(pq->msgPool);
			pq->msgPool = NULL;
		}
		pq->msgCount = 0;
		pq->msgSize  = 0;
		pq->msgHead  = 0;
		pq->msgTail  = 0;
		#if TSMSGQLMTX
		TSdestroyMutex(pq->lock);
		pq->lock   = NULL;
		#endif
		TSdestroySemaphore(pq->signal);
		pq->signal = NULL;
		rv = 0;
	}
	TSmutexSignal(s_msgLock);
	return rv;
}

//***********************************************************************
int TSmsgSend(
               TsMailbox  msgq,
               Msg       *msg
               )
{
	PMSGQ pq;
	int   rv;
	
	rv = -1;
	pq = GetMsgQ(msgq);
	if (pq)
	{
		Msg *pm;
		
		if (pq->msgCount >= pq->msgSize)
		{
			rv = -2;
		}
		else
		{
			TSMSGQLOCK(pq);
			pm = &pq->msgPool[pq->msgHead++];
			if (pq->msgHead >= pq->msgSize)
			{
				pq->msgHead = 0;
			}
			pm->msg1 = msg->msg1;
			pm->msg2 = msg->msg2;
			pm->msg3 = msg->msg3;
			pm->msg4.ullval = msg->msg4.ullval;
			pq->msgCount++;
			TSMSGQUNLOCK(pq);
			TSsemSignal(pq->signal);
			rv = 0;
		}
	}
	return rv;
}

//***********************************************************************
int TSmsgTimeout(
                   TsMailbox  msgq,
                   Msg       *msg,
                   int        secs,
                   int        usecs
                   )
{					   
	PMSGQ pq;
	int   rv, wv;
	
	rv = -1;
	pq = GetMsgQ(msgq);
	if (! pq)
	{
		return rv;
	}
	rv = 1;
	if (pq->msgCount == 0)
	{
		if (secs == 0 && usecs == 0)
		{
			return 3;
		}
	}
	// wait for message count to grow
	//
	wv = TSsemTimeout(pq->signal, secs, usecs);
	if (wv != 0)
	{
		return wv;
	}
	if (pq->msgCount > 0)
	{
		Msg *pm;
		
		TSMSGQLOCK(pq);
		pq->msgCount--;
		pm = &pq->msgPool[pq->msgTail++];
		if (pq->msgTail >= pq->msgSize)
		{
			pq->msgTail = 0;
		}
		if (msg)
		{
			msg->msg1 = pm->msg1;
			msg->msg2 = pm->msg2;
			msg->msg3 = pm->msg3;
			msg->msg4.ullval = pm->msg4.ullval;
		}
		rv = 0;
		TSMSGQUNLOCK(pq);
	}
	else
	{
		BIOSerror("Got mail sem, but no mail?\n");
		rv = 1;
	}
	return rv;
}

//***********************************************************************
int TSmsgCount(TsMailbox msgq)
{
	PMSGQ pq;
	int   rv;
	
	rv = -1;
	pq = GetMsgQ(msgq);
	if (pq)
	{
		rv = pq->msgCount;
	}
	return rv;	
}

//***********************************************************************
int TSmsgProlog()
{
	int i;

	s_msgLock = TScreateMutex();
	if (s_msgLock == NULL)
	{
		return -1;
	}
	for (i = 0; i < TS_NUM_QUEUES; i++)
	{
		#if TSMSGQLMTX
		s_msgQueuePool[i].lock    	= NULL;
		#endif
		s_msgQueuePool[i].signal  	= NULL;
		s_msgQueuePool[i].msgPool 	= NULL;
		s_msgQueuePool[i].msgCount = 0;
		s_msgQueuePool[i].msgSize  = 0;
		s_msgQueuePool[i].msgHead  = 0;
		s_msgQueuePool[i].msgTail  = 0;
	}
	return 0;
}

//***********************************************************************
void TSmsgEpilog()
{
	int i;
	
	for (i = 0; i < TS_NUM_QUEUES; i++)
	{
		if (s_msgQueuePool[i].signal != NULL)
		{
			TSdestroyMsgQueue((TsMailbox)&s_msgQueuePool[i]);
		}
	}
	if (s_msgLock != NULL)
	{
		TSdestroyMutex(s_msgLock);
		s_msgLock = NULL;
	}
}

