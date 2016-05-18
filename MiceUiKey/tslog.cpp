//*****************************************************************************
//  Copyright (C) 2012 Cambridge Silicon Radio Ltd.
//  $Header: /cvs/MICECore/threads/tsCommon/tslog.c,v 1.2 2015/08/10 07:37:04 AV00287 Exp $
//  $Change: 244438 $ $Date: 2015/08/10 07:37:04 $
//
/// @file
/// Logging utilities
//
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"
#include "qio.h"
#include "bios.h"

#ifndef TS_MAX_LOG_MSG
    #define TS_MAX_LOG_MSG  128
#endif
#ifndef TS_MAX_LOG_LEVEL
    #define TS_MAX_LOG_LEVEL  10
#endif
#ifndef TS_MAX_LOG_CTX
    #define TS_MAX_LOG_CTX  4
#endif

/// Use the global log buffer to keep the stack smaller
/// and suffer occasional munged up log messages if it
/// is shared.
///
#if TS_GLOBAL_LOG_BUFFER
static char     s_emsg0[TS_MAX_LOG_MSG];
static char     s_emsg1[TS_MAX_LOG_MSG];
#endif
static int      s_log_level         = 5;
static int      s_log_console       = 1;

/// Stack of log file settings so even when logging to a file, a new
/// file can be used temporarily without loosing place in the original
/// 
/// This is used, for example, when pipe wants to dump performance
/// statistics to a file and the job is logging to a console log file
/// for automated testing, etc.
///
typedef struct tag_log_file_context
{
    PQIO log_file;
    int  log_bytes;
    int  log_size;
    int  log_no_color;
    int  log_html;
    int  log_errors;
    int  log_severity;
}
LOGCTX, *PLOGCTX;

static LOGCTX s_logctx[TS_MAX_LOG_CTX];

/// top of stack for log contexts
///
static int    s_logtop = 0;

/// context for console log
///
static LOGCTX s_consolectx;

#define TSLOG_FILE  ((s_logtop > 0) ? s_logctx[s_logtop - 1].log_file : NULL)

/// lock to protect log data structures
///
static TsMutex s_logex = NULL;

/// set this non zero to serialize protection to log output
/// (note that this can drastically change thread performance!)
///
static int s_logprotect = 0;

#ifndef OS_Standalone
#define LOG_LOCK()          if (s_logex != NULL) TSmutexWait(s_logex)
#define LOG_UNLOCK()        if (s_logex != NULL) TSmutexSignal(s_logex)
#define LOG_LOCK_LOG()      if (s_logprotect && (s_logex != NULL)) TSmutexWait(s_logex)
#define LOG_UNLOCK_LOG()    if (s_logprotect && (s_logex != NULL)) TSmutexSignal(s_logex)
#else
#define LOG_LOCK()
#define LOG_UNLOCK()
#define LOG_LOCK_LOG()
#define LOG_UNLOCK_LOG()
#endif
//***********************************************************************
int TSlogProtectOn()
{
    LOG_LOCK();
    s_logprotect = 1;
    LOG_UNLOCK();
    return 0;
}

//***********************************************************************
void TSlogProtectOff()
{
    LOG_LOCK();
    s_logprotect = 0;
    LOG_UNLOCK();
}

//***********************************************************************
static const char *ColorForLevel(PLOGCTX pl, int level)
{
    const char *colorstr;

    if (! pl || pl->log_no_color)
    {
        colorstr = NULL;
    }
    else if (pl->log_html)
    {
        if (level <= 0)
        {
            colorstr = "</p><p style=\"font-family:monospace;color:red\">";
        }
        else if (level <= 1)
        {
            colorstr = "</p><p style=\"font-family:monospace;color:yellow\">";
        }
        else if (level <= 2)
        {
            colorstr = "</p><p style=\"font-family:monospace;color:blue\">";
        }
        else
        {
            colorstr = "</p><p style=\"font-family:monospace;color:black\">";
        }
    }
    else
    {
        if (level <= 0)
        {
            colorstr = "\033[31m";
        }
        else if (level <= 1)
        {
            colorstr = "\033[32m";
        }
        else if (level <= 2)
        {
            colorstr = "\033[34m";
        }
        else
        {
            colorstr = "\033[0m";
        }
    }
    return colorstr;
}

//***********************************************************************
static void TSlogStringToFileUnlocked(const char *str, int len, int level)
{
    PLOGCTX pl;

    if (s_logtop <= 0)
    {
        return;
    }
    pl = &s_logctx[s_logtop - 1];

    // must be called locked to avoid issues
    //
    if (pl->log_file)
    {
        if (pl->log_size > 0 && (pl->log_bytes + len) > pl->log_size)
        {
            QIOseek(TSLOG_FILE, 0, SEEK_SET);
            pl->log_bytes = 0;
        }
        if (pl->log_html)
        {
            if (! pl->log_no_color)
            {
                if (level != pl->log_severity)
                {
                    const char *colorstr;
                    int len;

                    pl->log_severity = level;
                    
                    colorstr = ColorForLevel(pl, level);
                    if (colorstr)
                    {
                        len = strlen(colorstr);
                        QIOwrite(TSLOG_FILE, (Uint8*)colorstr, len);
                        pl->log_bytes += len;
                    }
                }
            }
            /// @todo - convert string to html so spacing works, etc.
            //str = HTMLforString(htmlbuffer, sizeof(htmlbuffer), str, &len);
        }
        // write actual string contents
        //
        QIOwrite(TSLOG_FILE, (Uint8*)str, len);        
        pl->log_bytes += len;
        
        if (pl->log_html)
        {
            if (strchr(str, '\n'))
            {
                QIOwrite(TSLOG_FILE, (Uint8*)"<br>", 4);
                pl->log_bytes += 4;
            }
        }
    }
}

//***********************************************************************
void TSlogString(const char *str, int len, int level)
{
    if (s_log_console)
    {
        const char *colorstr;

        // colorize the console log for level
        //
        if (! s_consolectx.log_no_color)
        {
            colorstr = ColorForLevel(&s_consolectx, level);
            if (colorstr)
            {
                BIOSputStr(colorstr);
            }
        }
        // the actual log string
        //
        BIOSputStr(str);
        
        // decolor the log
        //
        if (! s_consolectx.log_no_color && ! s_consolectx.log_html)
        {
            colorstr = ColorForLevel(&s_consolectx, TS_MAX_LOG_LEVEL - 1);
            if (colorstr)
            {
                BIOSputStr(colorstr);
            }
        }
    }
    if (s_logtop > 0)
    {
        LOG_LOCK();
        TSlogStringToFileUnlocked(str, len, level);
        LOG_UNLOCK();
    }
}

//***********************************************************************
void TSlogLevel(int level, const char* format, ...)
{
    va_list arg;
    int     tnz;

    #if ! TS_GLOBAL_LOG_BUFFER
    char emsg0[TS_MAX_LOG_MSG];
    char *ptr = emsg0;
    #else
    // use sep. buffers for cpu1/2
    char *ptr = BIOSgetCPU() ? s_emsg1 : s_emsg0;
    #endif

    if (level > s_log_level)
    {
        return;
    }
    LOG_LOCK_LOG();
    va_start(arg, format);
#if defined(Solaris) || defined(VxWorks) /* really this means old C */
    tnz = vsprintf(ptr, format, arg);
#else
    tnz = vsnprintf(ptr, TS_MAX_LOG_MSG, format, arg);
#endif
    va_end(arg);
    TSlogString(ptr, tnz, level);
    LOG_UNLOCK_LOG();
}

//***********************************************************************
void TSlogLevelFunc(int level, const char* func, const char* format, ...)
{
    va_list arg;
    int     fnz;
    int     tnz;

    #if ! TS_GLOBAL_LOG_BUFFER
    char emsg0[TS_MAX_LOG_MSG];
    char *ptr = emsg0;
    #else
    // use sep. buffers for cpu1/2
    char *ptr = BIOSgetCPU() ? s_emsg1 : s_emsg0;
    #endif

    if (level > s_log_level)
    {
        return;
    }
    LOG_LOCK_LOG();
    fnz = snprintf(ptr, TS_MAX_LOG_MSG - 2, "%-20s - ", func);
    va_start(arg, format);
#if defined(Solaris) || defined(VxWorks) // really this means old C 
    tnz = vsprintf(ptr + fnz, format, arg);
#else
    tnz = vsnprintf(ptr + fnz, TS_MAX_LOG_MSG - fnz - 2, format, arg);
#endif
    tnz += fnz;
    va_end(arg);
    TSlogString(ptr, tnz, level);
    LOG_UNLOCK_LOG();
}

//***********************************************************************
void TSlogErrorFunc(const char* func, const char* format, ...)
{
    va_list arg;
    int     fnz;
    int     tnz;

    #if ! TS_GLOBAL_LOG_BUFFER
    char emsg0[TS_MAX_LOG_MSG];
    char *ptr = emsg0;
    #else
    // use sep. buffers for cpu1/2
    char *ptr = BIOSgetCPU() ? s_emsg1 : s_emsg0;
    #endif

    LOG_LOCK_LOG();
    fnz = snprintf(ptr, TS_MAX_LOG_MSG - 2, "%-20s ! ", func);
    va_start(arg, format);
#if defined(Solaris) || defined(VxWorks) // really this means old C 
    tnz = vsprintf(ptr + fnz, format, arg);
#else
    tnz = vsnprintf(ptr + fnz, TS_MAX_LOG_MSG - fnz - 2, format, arg);
#endif
    tnz += fnz;
    va_end(arg);
    TSlogString(ptr, tnz, -1);
    LOG_UNLOCK_LOG();
}

//***********************************************************************
void TSlogSetLevel(int level, int consoleon, int nocolorizing)
{
    if (s_logex == NULL)
    {
        BIOSerror("TSlog wasn't prologed\n");
        TSlogProlog();
    }
    LOG_LOCK();
    if (level < 0)
    {
        s_log_level = 0;
    }
    else if (level > TS_MAX_LOG_LEVEL)
    {
        s_log_level = TS_MAX_LOG_LEVEL;
    }
    else
    {
        s_log_level = level;
    }
    s_log_console = consoleon != 0;
    s_consolectx.log_no_color = nocolorizing != 0;
    LOG_UNLOCK();
}

//***********************************************************************
int TSlogGetLevel(int *level, int *consoleon, int *nocolorize)
{
    LOG_LOCK();
    if (level)
    {
        *level = s_log_level;
    }
    if (consoleon)
    {
        *consoleon = s_log_console;
    }
    if (nocolorize)
    {
        *nocolorize = s_consolectx.log_no_color;
    }
    LOG_UNLOCK();
    return s_log_level;
}

//***********************************************************************
int TSlogSetLogFile(PQIO pqio, Uint32 maxsize, int htmlFormat)
{
    static const char s_logprolog[] = "<html><body><p>\n";
    static const char s_logepilog[] = "</p></body></html>\n";

    if (pqio == NULL)
    {
        PQIO oldqio = NULL;
        int  washtml = 0;

        LOG_LOCK();
        
        // popping off a level
        //
        if (s_logtop > 0)
        {
            if (TSLOG_FILE)
            {
                if (s_logctx[s_logtop - 1].log_html)
                {
                    washtml = 1;
                }
            }
            oldqio = TSLOG_FILE;
            s_logctx[s_logtop - 1].log_file = NULL;
            s_logtop--;
        }
        LOG_UNLOCK();
        
        // this is closed outside the lock in case the close call
        // wants to Log()
        //
        if (oldqio)
        {
            if (washtml)
            {
                QIOwrite(oldqio, (Uint8*)s_logepilog, sizeof(s_logepilog));
            }
            QIOclose(oldqio);
        }
    }
    else
    {
        PLOGCTX pl;
    
        LOG_LOCK();
        
        // pushing a level
        //
        if (s_logtop >= TS_MAX_LOG_CTX)
        {
            LOG_UNLOCK();
            Error("Log stack too deep\n");
            return -1;
        }
        s_logtop++;
        pl = &s_logctx[s_logtop - 1];
        pl->log_size   = maxsize;
        pl->log_html   = htmlFormat ? 1 : 0;
        pl->log_bytes  = 0;
        pl->log_errors = 0;
        pl->log_severity = -4;
        pl->log_file   = pqio;
        LOG_UNLOCK();
        if (pqio && htmlFormat)
        {
            pl->log_bytes = QIOwrite(pqio, (Uint8*)s_logprolog, sizeof(s_logprolog));
        }
    }
    return 0;
}

#if !defined(Linux) && ! defined(HOST_BASED) && ! defined(OS_Standalone) && ! (BIOS_DO_SERIAL_POLL)

static TsMailbox   s_rxqueue = INVALIDMSGQ;

//***********************************************************************
static void TsOnRxChar(int irq)
{
    Msg msg;
    int port;
    int c;
    
    // got an rx interrupt, add to queue
    // NOTE that this is called in ISR context!
    // 
    if (s_rxqueue != INVALIDMSGQ)
    {
        // read the char from the debug port
        //
        port = BIOSserialGetDebugPort();
        c = BIOSserialRX(port);
        msg.msg1 = irq; // not used for now
        msg.msg2 = c;        
        
        // append it the the rx char queue (ignore overflow)
        //
        TSmsgSend(s_rxqueue, &msg);
    }
}

//***********************************************************************
int TSgetChar(int secs, int usecs)
{
    Msg msg;
    int rc;
    
    rc = TSmsgTimeout(s_rxqueue, &msg, secs, usecs);
    if (rc == 0)
    {
        return msg.msg2;
    }
    // timeout or error
    //
    return -1;
}

#elif defined(Linux)

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

//***********************************************************************
int TSgetChar(int secs, int usecs)
{
    int     sv;
    fd_set  fds;
    struct  timeval timeout;

    FD_ZERO (&fds);
    FD_SET  (fileno(stdin), &fds);

    timeout.tv_sec  = secs;
    timeout.tv_usec = usecs;

    sv = select(fileno(stdin) + 1, &fds, NULL, NULL, &timeout);

    if (sv > 0)
    {
        char rv;
        
        sv = read(fileno(stdin), &rv, 1);
        return (sv == 1) ? (int)(unsigned)rv : -1;
    }
    // timeout or error
    //
    return -1;
}

#else /* OS_Standalone, or non-linux host based */

//***********************************************************************
int TSgetChar(int secs, int usecs)
{
    int rc;
    
    // no OS in use, this function is probably never called
    // but if so, there's no way to wait properly so just return any char
    //
    rc = BIOSgetChar();
    return rc;
}

#endif

//***********************************************************************
int TSlogProlog()
{
    s_logtop = 0;

    s_consolectx.log_file = NULL;
    s_consolectx.log_html = 0;
    s_consolectx.log_size = 0;
    s_consolectx.log_errors = 0;
    s_consolectx.log_no_color = 0;
    s_consolectx.log_severity = -4;
    
#ifndef OS_Standalone
    s_logex = TScreateMutex();
    if (! s_logex)
    {
        BIOSerror("Can't create semaphore\n");
        return -1;
    }    
#endif
    TSlogSetLevel(
                    5,  // initial log level
                    1,  // log to console even if logging to file
#ifdef Windows
                    1   // suppress colorizing when logging into cmd.exe
#else
                    0   // allow colorizing into serial port, linux terminal, etc.
#endif
                );
#if !defined(Linux) && ! defined(HOST_BASED) && ! defined(OS_Standalone) && ! (BIOS_DO_SERIAL_POLL)
    {                
    int rc, port;

    s_rxqueue = TScreateMsgQueue();
    if (s_rxqueue == INVALIDMSGQ)
    {
        BIOSerror("Can't create RX queue\n");
        return -1;
    }
    // install a listener for serial rx interrupts
    // on the debug serial port, so we can wait for them
    //
    port = BIOSserialGetDebugPort();
    rc = BIOSserialRXsetISRcallback(port, TsOnRxChar);
    if (rc)
    {
        BIOSerror("Can't install RX callback\n");
        return rc;
    }
    }
#endif
    return 0;
}

//***********************************************************************
void TSlogEpilog()
{
#ifndef OS_Standalone
    if (s_logex != NULL)
    {
        TSdestroyMutex(s_logex);
        s_logex = NULL;
    }
#endif
}

