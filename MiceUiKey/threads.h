//*****************************************************************************
// Copyright (C) 2013 Cambridge Silicon Radio Ltd.
// $Header: /cvs/MICECore/Include/threadx/threads.h,v 1.2 2015/08/12 01:09:12 AV00287 Exp $
// $Change: 245218 $ $Date: 2015/08/12 01:09:12 $
//
/// @file
/// Thread and thread primitives Interface
/// Provides a portable interface to thread systems
/// The Inferno threading system provides threads, mutexes, semaphores and mailbox queues
/// and NOTHING else.  The interface is kept as absolutely simple as possible
/// to allow for easier porting on top of any rtos/Linux/etc. system
///
/// @ingroup Framework
/// @defgroup Framework System Framework, Threading
// 
//*****************************************************************************
#ifndef THREADS_H
#define THREADS_H 1

#include "threadport.h"
#include "stdio.h"

//#define BIOSerror(fmt, ...) DbgPrintf(IMPORTANT_MSG_LEVEL, "%s:%04d : "fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#define snprintf(buf, len, fmt, ...) sprintf(buf, fmt, __VA_ARGS__)

#ifndef IN_DEBUG_LOCKS
    /// define as 1 to provide extra mutex/semaphore debugging info
    #define IN_DEBUG_LOCKS 0
#endif
#ifndef IN_DEBUG_THREADS
    /// define as 1 to provide extra thread debugging info
    /// which can be underlying os specific
    ///
    #define IN_DEBUG_THREADS 0
#endif
#ifndef IN_THREAD_TRACE
    /// define as 1 to provide thread execution timing 
    /// which can be underlying os specific
    ///
    #define IN_THREAD_TRACE 0
#endif

/// TS_OS_Ticks_Per_Second comes from threadport.h
///
#define TS_TICK_MS            (1000 / TS_OS_Ticks_Per_Second) 

/// These defines are to to enable code to be compiled for other systems
/// that might not use ptr/handles for primitives to port easier
///
#define INVALIDMTX      NULL
#define INVALIDSEM      NULL
#define INVALIDTHREAD   NULL
#define INVALIDMSGQ     NULL

///  Msg - holds a mail message
///
typedef struct tag_Msg
{
    Uint32  msg1;   ///< First parameter, usually message code
    Uint32  msg2;   ///< Second parameter
    Uint32  msg3;   ///< Third parameter
    union
    {
        Uint64  ullval;     ///< Fourth parameter, can by up to 64 bits
        void   *ptrval;     ///< or, a pointer parameter if needed
    }
    msg4;
}
Msg;

/// Mailbox handle
typedef void *TsMailbox;

/// Thread handle
typedef void *TsThread;

/// Mutex handle
typedef void *TsMutex;

/// Semaphore handle
typedef void *TsSemaphore;

/// Inferno provides 32 priorities, with 31 being the highest
/// and 0 being the lowest.  These are translated to whatever
/// the underlying OS uses in port specific files.  Note that
/// some implementations (like Linux) will not even look at
/// the priority passed in, so don't assume that your priorities
/// will be exactly honored or depend on them.
///
#define TS_LOWEST_PRIORITY     0
#define TS_LOW_PRIORITY        7
#define TS_LEVEL_PRIORITY      15
#define TS_MEDIUM_PRIORITY     15
#define TS_HIGHEST_PRIORITY    31

/// mask for holding interrupt flags
///
typedef void *IntMask;

#ifndef TSintsOff // this can be a macro in tsport.h
//***********************************************************************
/// Turn Threading off (h/w interrupts may or may not be disabled) to
/// avoid the possiblility of a task switch
/// @returns state of task interrupts before turning off to be used to restore
///
IntMask     TSintsOff         (void);

/// Re-enable tasking (and possibly h/w interrupts)
///
void        TSintsRestore     (IntMask mask /**< [in] mask returned from TSintsOff */);
#endif

//***********************************************************************
/// On-time initialization of thread system. 
/// @returns non-0 for errors
///
int         TSprolog            (void);

/// Create a thread of exectution.
/// @returns the id of the created task or INVALIDTHREAD on error
///
TsThread    TScreateThread      (
                                Sint32 (*entry)(void *),///< [in] thread function
                                Sint32      stacksize,  ///< [in] size (bytes) of stack needed
                                Sint32      priority,   ///< [in] thread priority
                                void       *arg,        ///< [in] argument to pass to thread function
                                const char *name        ///< [in] name of thread for debug
                                );
                                
/// @returns the Thread handle for the thread calling the function
/// or NULL if not called in a thread context
///
TsThread    TSgetThread         (void);

/// Stop and destroy a running thread.
/// @returns non-0 on error (no such thread)
///
int         TSdestroyThread     (TsThread thread /**< thread to destroy */);

/// Set a thread's CPU affinity.  Informs the underlying os that the thread represented
/// by thread should be run on the CPU ordinal cpu.  There is no checking at this level
/// regarding the appropriateness or existence of the cpu specified. 
/// @returns non-0 on error (no such thread, or error from lower layer)
///
int         TSsetThreadAffinity (
                                TsThread    thread,     ///< [in] thread to set affinity for
                                int         cpu         ///< [in] ordinal (from 0) of CPU to set affinity to
                                );

/// Sleep for milliseconds.  Threads may switch, even for 0 waits
/// @returns 0
///
void        TSsleepMilliseconds (int millisecs /**< milli-seconds to wait for */);

/// Sleep for seconds+microseconds.  Tasks may switch, even for 0,0 waits
/// @returns 0
///
void        TSsleep             (
                                int secs,               ///< seconds to wait for
                                int usecs               ///< microseconds to wait for
                                );

/// Mutex (mutual exclusion) functions.  These functions are used to
/// protect shared data from being accessed simultaneously by different
/// threads and to insure critical sections of code are used 
/// sequentially.
///
/// A mutex is held after a wait, and released by signal.  Only one
/// thread can hold the mutex at a time.  The mutex is created in
/// a released state.
///

/// Create a mutex (or non-counting semaphore)
/// @returns NULL on error, or the mutex handle
///
TsMutex     TScreateMutex       (void);

/// Destroy a mutex created with TScreateMutex
/// @returns non-0 on error (no such mutex)
///
int         TSdestroyMutex      (TsMutex mtx /**< [in] mutex to destroy */);

/// Blocking wait to hold a mutex
/// @returns non-0 on error (no such mutex)
///              0 if mutex is held
///
#if IN_DEBUG_LOCKS
int         TSmutexWaitDebug    (
                                TsMutex     mtx,    ///< [in] mutex to wait for
                                const char *file,   ///< [in] file name of caller
                                int         line    ///< [in] line in calling file
                                );
#define     TSmutexWait(mtx)  \
            TSmutexWaitDebug(mtx, __FILE__, __LINE__)
#else
int         TSmutexWait         (TsMutex mtx /**< [in] mutex to wait for */);
#endif

/// Non-blocking wait for a mutex.  Waits for up to secs+usecs to 
/// aquire a mutex.  Use 0,0 for secs,usecs to not wait at all.
/// @returns > 0 if the wait timed-out,
///          < 0 on error (no such mutex)
///            0 if the mutex is aquired
///
#if IN_DEBUG_LOCKS
int         TSmutexTimeoutDebug (
                                TsMutex     mtx,    ///< [in] mutex to try
                                int         secs,   ///< [in] seconds to wait
                                int         usecs,  ///< [in] microseconds to wait
                                const char *file,   ///< [in] file name of caller
                                int         line    ///< [in] line in calling file
                                );
#define     TSmutexTimeout(mtx, s, u)  \
            TSmutexTimeoutDebug(mtx, s, u, __FILE__, __LINE__)
#define     TSmutexTry(mtx)  \
            TSmutexTimeoutDebug(mtx, 0, 0, __FILE__, __LINE__)
#else
int         TSmutexTimeout      (
                                TsMutex     mtx,    ///< [in] mutex to try
                                int         secs,   ///< [in] seconds to wait
                                int         usecs   ///< [in] microseconds to wait
                                );
#define     TSmutexTry(mtx)  \
            TSmutexTimeout      (mtx, 0, 0)
#endif

/// Release a held mutex
/// @returns non-0 on error (no such mutex, mutex not held)
///
#if IN_DEBUG_LOCKS
int         TSmutexSignalDebug(
                                TsMutex     mtx,    ///< [in] mutex to signal
                                const char *file,   ///< [in] file name of caller
                                int         line,   ///< [in] line in calling file
                                );
#define     TSmutexSignal(mtx)  \
            TSmutexSignalDebug(mtx, __FILE__, __LINE__)
#else
int         TSmutexSignal     (TsMutex mtx /**< [in] mutex to signal */);
#endif

/// Semaphore functions.  These functions are used to handle counted
/// resources.
///
/// A Semaphore wait returns and decrements the resource count until the count
/// reaches 0 at which point it blocks until another thread increments the count
/// by calling signal.  For semaphores with an initial count of 1, use the
/// faster and lighter weight Mutex construct instead
///
/// Create a Semaphore
/// @returns NULL on error, or the Semaphore handle
///
TsSemaphore TScreateSemaphore   (int count /**< [in] Initial count */);

/// Destroy a Semaphore created with TScreateSemaphore
/// @returns non-0 on error (no such Semaphore)
///
int         TSdestroySemaphore  (TsSemaphore sem /**< [in] Semaphore to destroy */);

/// Blocking wait to hold a Semaphore
/// @returns non-0 on error (no such Semaphore)
///              0 if Sem is held
///
#if IN_DEBUG_LOCKS
int         TSsemWaitDebug      (
                                TsSemaphore sem,    ///< [in] Semaphore to wait for
                                const char *file,   ///< [in] file name of caller
                                int         line    ///< [in] line in calling file
                                );
#define     TSsemWait(mtx)  \
            TSsemWaitDebug(sem, __FILE__, __LINE__)
#else
int         TSsemWait           (TsSemaphore sem /**< [in] Semaphore to wait for */);
#endif

/// Non-blocking wait for a Semaphore.  Waits for up to secs+usecs to 
/// aquire a Semaphore.  Use 0,0 for secs,usecs to not wait at all.
/// @returns > 0 if the wait timed-out,
///          < 0 on error (no such Semaphore)
///            0 if the Sem is aquired
///
#if IN_DEBUG_LOCKS
int         TSsemTimeoutDebug   (
                                TsSemaphore sem,    ///< [in] Semaphore to try
                                int         secs,   ///< [in] seconds to wait
                                int         usecs,  ///< [in] microseconds to wait
                                const char *file,   ///< [in] file name of caller
                                int         line    ///< [in] line in calling file
                                );
#define     TSsemTimeout(sem, s, u)  \
            TSsemTimeoutDebug   (sem, s, u, __FILE__, __LINE__)
#define     TSsemTry(sem)  \
            TSsemTimeoutDebug   (sem, 0, 0, __FILE__, __LINE__)
#else
int         TSsemTimeout        (
                                TsSemaphore sem,    ///< [in] Semaphore to try
                                int         secs,   ///< [in] seconds to wait
                                int         usecs   ///< [in] microseconds to wait
                                );
#define     TSsemTry(sem)  \
            TSsemTimeout        (sem, 0, 0)
#endif

/// Release a held Sem
/// @returns non-0 on error (no such Sem, Sem not held)
///
#if IN_DEBUG_LOCKS
int         TSsemSignalDebug    (
                                TsSemaphore sem,    ///< [in] Semaphore to increment
                                const char *file,   ///< [in] file name of caller
                                int         line,   ///< [in] line in calling file
                                );
#define     TSsemSignal(sem)  \
            TSsemSignalDebug    (sem, __FILE__, __LINE__)
#else
int         TSsemSignal         (TsSemaphore sem /**< [in] Semaphore to increment */);
#endif

/// Mailbox (message queue) functions.  These are used to send
/// messages (commands, information, wakeup events, etc.) from
/// one thread (task) to another.
///
/// Inferno does not (and should not) have an "event" primative
/// and message queues are used to signal events as well as send
/// information messages.  To send an event to multiple recipients
/// a message could be sent to each recipient's message box.
///

/// Create a message queue
/// @returns the queue object handle, or NULL on failure (alloction)
///
TsMailbox   TScreateMsgQueue    (void);

/// Destroy a message queue
/// @returns non-0 on error (no such queue)
///
int         TSdestroyMsgQueue   (TsMailbox msgq /**< [in] queue to destroy */);

/// Send a message to a message queue
/// @returns -1 on error (no such queue)
///          -2 on overflow
///           0 no error
///
int         TSmsgSend           (
                                TsMailbox  msgq,    ///< [in] destination queue
                                Msg       *msg      ///< [in] message to send
                                );
                                
/// Wait for a message to appear in a message queue.  Timeout after
/// waiting secs seconds and usecs microseconds.  If no timeout is
/// specified, the function return immediately
///
/// Use -1 for secs/usecs to wait forever however the concept of forever
/// is not friendly to users who usually have better things to do.
/// If you have a thread that does nothing but respond to mail messages
/// then put a long timeout in, and do nothing in the timeout return case
///
/// @returns < 0 on error (no such queue, queue destroyed, etc.)
///          > 0 on timeout
///            0 there was a message
///
int         TSmsgTimeout        (
                                TsMailbox  msgq,    ///< [in] source message queue
                                Msg       *msg,     ///< [in] received message (on success)
                                int        secs,    ///< [in] seconds to wait
                                int        usecs    ///< [in] microseconds to wait
                                );
                                
/// @returns the count of messages in the message queue or < 0 on error (no such queue)
///
int         TSmsgCount          (TsMailbox msgq /**< [in] message queue to poll */);

// Low Power and Debug
//

/// Reduce the CPU clock to reduce power consumption.  Takes a percent of the
/// available range of clocks, with 100% meaning "set lowest clock possible",
/// an 0% meaning "set full speed clock"
/// @returns non-0 on error
///
int         TSreduceClock       (int percent /**< [in] how much to reduce clock */);

// Logging
//
#ifdef _MSC_VER
    // microsoft compiler
#if (_MSC_VER >= 1500) /* 2005 VC introduced variadic macros */
    #define Log(a,...) TSlogLevelFunc(a, __FUNCTION__, __VA_ARGS__)
    #define Error(...) TSlogErrorFunc(__FUNCTION__, __VA_ARGS__)
#else   /* VC++ 6.0 and earlier can't handle it so just use older funcs */
    #define Log   TSlogLevel
    #define Error TSlog
#endif
#else
    // gcc, armcc, etc. can handle variadic macros
    #define Log(a,...) TSlogLevelFunc(a, __FUNCTION__, ## __VA_ARGS__)
    #define Error(...) TSlogErrorFunc(__FUNCTION__, ## __VA_ARGS__)
#endif

#if QIO_H
/// Redirect logging to a log file stream.  Log will also appear on console
/// depending on settings.  See TSlogSetLevel.
/// @returns non-0 on error
///
int         TSlogSetLogFile     (
                                PQIO logfile,       ///< [in] QIO used to write log to
                                Uint32 maxSize,     ///< [in] max bytes to write to log
                                int htmlForm        ///< [in] non-0 means create an HTML format log
                                );
#endif

/// Log a string
///
void        TSlogString         (
                                const char* str,    ///< [in] string to log
                                int len,            ///< [in] length of string in bytes
                                int color           ///< [in] color to show string in, 0-3, implementation dependant
                                );

/// Log debug message withouth including a caller id
///
void        TSlogLevel          (
                                int level,          ///< [in] log level, 0 through 10
                                const char* format, ///< [in] print format
                                ...                 ///< [in] arguments for format
                                );

/// Log a debug message.  Don't call directly, use the Log macro instead
///
void        TSlogLevelFunc      (
                                int level,              ///< [in] log level, 0 through 10
                                const char* funcname,   ///< [in] function name of caller
                                const char* format,     ///< [in] print format
                                ...                     ///< [in] arguments for format
                                );

/// Log an error.  Don't call directly, use the Error macro instead
///
void        TSlogErrorFunc      (
                                const char* funcname,   ///< [in] function name of caller
                                const char* format,     ///< [in] print format
                                ...                     ///< [in] arguments for format
                                );

/// Set the log system settings
///
void        TSlogSetLevel       (
                                int level,          ///< [in] log verbosity level, 0 though 10, 0 is errors only
                                int consoleon,      ///< [in] non-0 enables logging to system console (serial port)
                                int nocolorizing    ///< [in] non-0 turns off color-coding of logs for better inclusion in files
                                );

/// Get the logging system settings level, console, and colorizing
/// @returns non-0 on error
///
int         TSlogGetLevel       (
                                int *level,         ///< [out] current log level
                                int *consoleon,     ///< [out] current console enable setting
                                int *nocolorizing   ///< [out] current colorizing setting
                                );

/// One time init of logging system
/// @returns non-0 on error
///
int         TSlogProlog         (void); // called by TS init, no need to call direcltly

/// Stop log system. Typically never called
///
void        TSlogEpilog         (void);

/// Create and use a mutex to serialize access to logging.  This has SERIOUS system 
/// repercussions.  If log protection is on, you will get all of your log messages
/// cleanly line-by-line but threads will block waiting to log.  If log protection is
/// off, you might miss thousands of lines of log output, but threads will behave more
/// like they would without logging at all. @returns non-0 on error
///
int         TSlogProtectOn      (void);

/// Stop using log protection. See TSlogProtectOn for details
///
void        TSlogProtectOff     (void);

/// Wait for then read a character from the debug console.  This is way to wait
/// for user input without looping on sleep, etc. to avoid effecting system dynamics.
/// In some systems (like non-threaded standalone os) there is no way to sleep, in
/// which case the console is read directly.  Callers in that case should be
/// prepared for instant return of -1 each call.
/// @returns a character from the console, or -1 on timeout, or other error,  In
///
int         TSgetChar           (
                                int secs,       ///< [in] seconds to wait, -1 for wait forever
                                int usecs       ///< [in] micro-seconds to wait, -1 for forever
                                );
                       
#define THREADSLEEP_MILLISECONDS(msec) TSsleepMilliseconds(msec)
#define THREADSLEEP_SECONDS(sec)       TSsleep(sec, 0)

// ----------------------------------------------------------------------------------------------
// This is the interface to the underlying OS.  By supplying these
// functions, and a TaskInit function, the OS is ported to Inferno
//
// Applications should NOT call these functions directly, they are
// used only by task layer implementations
//
/// Create a mutex in the underlying OS
/// @returns non-0 on error and a pointer to data created
/// for the mutex in the OS.
///
int         _ts_mutexCreate     (void **pos_mtx /**< [out] ptr to is mutex data */);

/// Destroy a mutex in the underlying OS.  Pass the ptr returned from _ts_mutexCreate
/// @returns non-0 on error
///
int         _ts_mutexDestroy    (void *os_mtx   /**< [in] ptr to os mutex data returned in create */);

/// Wait for (hold/lock) a mutex for seconds/microseconds.  Use 0,0 to poll, use -1,0 to wait forever
/// @returns 0 if mutex gotten, 1 if timed-out waiting, < 0 on errors
///
int         _ts_mutexWait       (
                                void *os_mtx,   ///< [in] os mutex data from _ts_mutexCreate
                                int secs,       ///< [in] seconds to wait, -1 for wait forever
                                int usecs       ///< [in] micro-seconds to wait, -1 for forever
                                );
                                
/// Signal (release/unlock) a mutex.
/// @returns non-0 on error
///
int         _ts_mutexSignal     (void *os_mtx   /**< [in] os mutex data from _ts_mutexCreate */);

/// Create a semaphire in the underlying OS
/// @returns non-0 on error and a pointer to data created
/// for the mutex in the OS.
///
int         _ts_semaphoreCreate (
                                void **pos_sem, ///< [out] ptr to os semaphore data
                                int count       ///< [in] initial semaphore count
                                );

/// Destroy a semaphore in the underlying OS.  Pass the ptr returned from _ts_semaphoreCreate
/// @returns non-0 on error
///
int         _ts_semaphoreDestroy(void *os_sem   /**< [in] os data from _ts_semaphoreCreate */);

/// Wait for decrement/lower a semaphore  for seconds/microseconds.  Use 0,0 to poll, use -1,0 to wait forever
/// @returns 0 if semaphore decremented, 1 if timed-out waiting, < 0 on errors
///
int         _ts_semaphoreWait   (
                                void *os_mtx,   ///< [in] os semaphore data from _ts_semaphoreCreate
                                int secs,       ///< [in] seconds to wait, -1 for wait forever
                                int usecs       ///< [in] micro-seconds to wait, -1 for forever
                                );

/// Signal (increment/raise) a semaphore
/// @returns non-0 on error
///
int         _ts_semaphoreSignal (void *os_sem);

/// Create a thread in the underlying OS
/// @returns non-0 on error, and ptr to underlying OS thread data
///
int         _ts_createThread    (
                                void       **pos_thread,///< [out] underlying os thread data
                                Sint32 (*entry)(void *),///< [in] thread function
                                Sint32      stacksize,  ///< [in] size (bytes) of stack
                                Sint32      priority,   ///< [in] thread priority
                                void       *arg,        ///< [in] argument to pass to thread function
                                const char *name        ///< [in] name of thread for debug
                                );

/// Destroy (and stop) a thread
/// @returns non-0 on error
///
int         _ts_destroyThread   (void *pos_thread /**< [in] os thread data returned in _ts_createThread */);       

/// @returns a ptr to the os thread data for the calling thread.  Used for thread identification
///
void       *_ts_thisThread      (void);

/// Set CPU affinity of a thread
/// @returns non-0 on error
///
int         _ts_setAffinity     (
                                void   *pos_thread,     ///< [in] os thread data returned in _ts_createThread
                                int     cpu             ///< [in] cpu to run thread on (0 based ordinal)
                                );

/// Sleep for milliseconds
///
void        _ts_sleep           (int milliseconds /**< [in] ms to sleep */);

/// Return the time, in relative units, the thread has executed since the last call
/// This is used for thread-utilization debug and may or may not be implemented 
/// depending on build flags and underlying OS, so don't count on it normally
///
Uint32      _ts_threadTime      (void *pos_thread /**< [in] ptr to os thread data */);

#endif

