//*****************************************************************************
//  Copyright (C) 2012 Cambridge Silicon Radio Ltd.
//  $Header: //depot/imgeng/sw/inferno_2015.1Candidate/appsrc/bios/biosWindows_NT/biosWindows.c#1 $
//  $Change: 251312 $ $Date: 2015/05/01 $
//  
/// @file
/// BIOS support for Windows builds (host based)
/// 
/// @author Brian Dodge <brian.dodge@csr.com>
//  
//*****************************************************************************
#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "standard.h"
#include "threads.h"
#include <sys/timeb.h>
#include <time.h>

#ifndef _MSC_VER
#include <conio.h>
#endif

//***********************************************************************
void BIOSserialInit(Uint32 baud)
{
}

//***********************************************************************
void BIOSputStr(const char* str)
{
#ifdef _MSC_VER
    OutputDebugString(str);
#else
    fputs(str, stdout);
#endif
}

//***********************************************************************
void BIOSputChar(char c)
{
    if (c == '\n')
    {
        BIOSputChar('\r');
    }
#ifdef _MSC_VER
    {
        char cm[2] = { c, 0 };
        
        OutputDebugString(cm);
    }
#else
    fputc(c, stdout);
#endif
}

//***********************************************************************
int BIOSgetChar()
{
    return EOF;
}


//***********************************************************************
int BIOSgetCPU()
{
    return 0;
}

//***********************************************************************
Uint64 BIOShrtMicroCount()
{
    Uint64 now;
    LARGE_INTEGER lnow;

    QueryPerformanceCounter(&lnow);
    now = lnow.QuadPart;

    return now;
}

//***********************************************************************
Uint32 BIOShrtMilliCount()
{
    return (Uint32)(BIOShrtMicroCount() / 1000);
}

//***********************************************************************
void BIOShrtMicroSleep(Uint32 sleepies)
{
    Sleep((sleepies > 1000) ? sleepies / 1000 : 1);
}

//***********************************************************************
void BIOSinit()
{    
}



//***********************************************************************
void BIOSstartInitProc(Uint32  delay_microseconds)
{
    return;
}

#ifndef BIOS_MAX_LOG_MSG
    #define BIOS_MAX_LOG_MSG    256
#endif
//***********************************************************************
void BIOSlog(const char* format, ...)
{
    va_list arg;
    #if ! BIOS_GLOBAL_LOG_BUFFER
    char emsg0[BIOS_MAX_LOG_MSG];
    char *ptr = emsg0;
    #else
    // use sep. buffers for cpu1/2
    char *ptr = BIOSgetCPU() ? s_emsg1 : s_emsg0;
    #endif

    va_start(arg, format);
#if defined(Solaris) || defined(VxWorks) /* really this means old C */
    vsprintf(ptr, format, arg);
#elif defined(BOOTCODE)
    BIOSvsnprintf(ptr, BIOS_MAX_LOG_MSG, format, arg);
#else
    vsnprintf(ptr, BIOS_MAX_LOG_MSG, format, arg);
#endif
    va_end(arg);
    BIOSputStr(ptr);
}

//***********************************************************************
void BIOSlogError(const char *funcname, const char* format, ...)
{
    va_list arg;
    #if ! BIOS_GLOBAL_LOG_BUFFER
    char emsg0[BIOS_MAX_LOG_MSG];
    char *ptr = emsg0;
    #else
    // use sep. buffers for cpu1/2
    char *ptr = BIOSgetCPU() ? s_emsg1 : s_emsg0;
    #endif

    va_start(arg, format);
#if defined(Solaris) || defined(VxWorks) /* really this means old C */
    vsprintf(ptr, format, arg);
#elif defined(BOOTCODE)
    BIOSvsnprintf(ptr, BIOS_MAX_LOG_MSG, format, arg);
#else
    vsnprintf(ptr, BIOS_MAX_LOG_MSG, format, arg);
#endif
    va_end(arg);
    BIOSputStr("\033[31m");
    if (funcname)
    {
        BIOSputStr(funcname);
    }
    BIOSputStr(" ");
    BIOSputStr(ptr);
    BIOSputStr("\033[0m");
}

//***********************************************************************
void BIOSassertFail(const char* exp, const char* file, int line)
{
    BIOSlog("ASSERT: %s at %s:%d\n", exp, file, line);
    while (1)
    {
        ;
    }
}
