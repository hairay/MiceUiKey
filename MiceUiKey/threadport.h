//*****************************************************************************
//  Copyright (C) 2013 Cambridge Silicon Radio Ltd.
//  $Header: /mfp/MiceUiKey/MiceUiKey/threadport.h,v 1.1 2016/05/30 03:48:52 AV00287 Exp $
//  $Change: 245416 $ $Date: 2016/05/30 03:48:52 $
//  
/// @file
/// port specific code for Windows_NT threading API
/// 
//  
//*****************************************************************************
#ifndef TSPORT_H
#define TSPORT_H 1

#define TaskIsrRegister(a,b,c) BIOSintRegister(a,b)

typedef struct _TimeVal {
    Sint32 secs;    /* # of seconds */
    Sint32 usecs;   /* # of microseconds */
} TimeVal;

#define NULLDATA NULL
#define NULLARG  NULL

/* Inferno defaults to a 10mSecond OS tick rate */
#define TS_OS_Ticks_Per_Second         100

/* Define the number of TASKS/THREADS to support. */
#define TS_NUM_THREADS      64

/* Define the number of Mutexes to support. */
#define TS_NUM_MTXS         128

/* Define the number of Semaphores to support. */
#define TS_NUM_SEMS         256

/* Define the number of Queues to support. */
#define TS_NUM_QUEUES       128

/* Define the number of Queue Elements to support. */
#define TS_NUM_QELEMENTS    256

/* Task Stack sizes
 */
#define TSSTACK_MULT 1
 
#define TSHUGE_STACK    (32768 * TSSTACK_MULT)
#define TSLARGE_STACK   (16384 * TSSTACK_MULT)
#define TSNORMAL_STACK  (8192 * TSSTACK_MULT)
#define TSSMALL_STACK   (4096 * TSSTACK_MULT)
 
#endif

