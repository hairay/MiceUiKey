#ifndef _OSUTIL_H
#define _OSUTIL_H

typedef union 
{
	unsigned long	dwData;
	unsigned char	byte[4];
} dwDataType;
#endif


#ifndef MAX
	#define MAX(a,b)	((a)>(b)?(a):(b))
#endif

/*SCSI endian transfer*/
#define ScsiSetDouble(var,val)    var[0] = (BYTE)((val) >> 8 ) & 0xff ; \
                                  var[1] = (BYTE)((val)      ) & 0xff
							
#define ScsiSetTriple(var,val)    var[0] = (BYTE)((val) >> 16) & 0xff ; \
                                  var[1] = (BYTE)((val) >> 8 ) & 0xff ; \
                                  var[2] = (BYTE)((val) 	 ) & 0xff 

#define ScsiSetQuad(var,val)      var[0] = (BYTE)((val) >> 24) & 0xff ; \
                                  var[1] = (BYTE)((val) >> 16) & 0xff ; \
                                  var[2] = (BYTE)((val) >> 8 ) & 0xff ; \
                                  var[3] = (BYTE)((val)      ) & 0xff 

#define ScsiGetDouble(var)       ((((DWORD)*(var  )) <<  8) + \
                                   ((DWORD)*(var+1))        )
                                 
#define ScsiGetTriple(var)       ((((DWORD)*(var  )) << 16) + \
                                  (((DWORD)*(var+1)) <<  8) + \
                                  (((DWORD)*(var+2))      ))
						 
                                  
#define ScsiGetQuad(var)         ((((DWORD)*(var  )) << 24) + \
                                  (((DWORD)*(var+1)) << 16) + \
                                  (((DWORD)*(var+2)) <<  8) + \
                                   ((DWORD)*(var+3))       )
//----------------------------------------------------
// Choose your platform here 
//----------------------------------------------------
//#define _WINDOWS
//#define _LS005
//#define __MAC__

//----------------------------------------------------
// Add your Windows system head files here 
//----------------------------------------------------
// Windows Platform
#ifdef _WINDOWS

#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define MIN(a,b)	((a)<(b)?(a):(b))
#endif

//----------------------------------------------------
// Add your MAC system head files here 
//----------------------------------------------------
// MAC Platform
#ifdef __MAC__

#include <Events.h>
#include <Unix.h>
#include <String.h>
#include <stdlib.h>
#include <Math.h>

//----------------------------------------------------
#include "MacInclude.h"

//----------------------------------------------------
//  define constant of Windows on MAC
//----------------------------------------------------
//#define	Sleep(x)	DelayTime(x)			
#define	HWND_BROADCAST		NULL
#define	THREAD_PRIORITY_HIGHEST		0
#define THREAD_PRIORITY_IDLE		4
#define	WAIT_OBJECT_0				0
#define	STILL_ACTIVE				259

//jhn #define	INVALID_HANDLE_VALUE		0

#define		kDebugopCreateNew		1
#define		kDebugopWriteExist		2


//----------------------------------------------------
//	Windows virtual functions on MAC:
//----------------------------------------------------
// Thread
HANDLE WINAPI CreateThread( LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
//jhnBOOL WINAPI TerminateThread(HANDLE hThread,DWORD dwExitCode);
VOID WINAPI ExitThread(DWORD dwExitCode);
DWORD WINAPI ResumeThread(HANDLE hThread);
DWORD WINAPI SuspendThread( HANDLE hThread);
BOOL WINAPI SetThreadPriority(HANDLE hThread,int nPriority);
short WINAPI GetThreadPriority(HANDLE hThread);
BOOL WINAPI GetExitCodeThread(HANDLE hThread,LPDWORD lpExitCode);
LONG WINAPI InterlockedDecrement(LPLONG lpAddend);
LONG WINAPI InterlockedIncrement(LPLONG lpAddend);
BOOL WINAPI CloseHandle(HANDLE hObject);
LONG WINAPI InterlockedExchangeAdd(LPLONG lpAddend, LONG increment);
//jhnLONG WINAPI InterlockedExchange(LPLONG lpAddend, LONG increment);
//jhnDWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
// Mutex
//jhnBOOL WINAPI ReleaseMutex(HANDLE hObject);
//----------------------------------------------------
//	Windows functions on MAC:
//----------------------------------------------------
UINT GetPrivateProfileInt(char* appName, char* keyName, int defaultValue, char* fileName);
BOOL PostMessage(HWND hWnd, OSType Msg, LPWORD wParam, LPWORD lParam);
//DWORD GetTickCount(void);

#ifdef _MacOSX_
VOID	PathNameFromDirID(short vRefNum, LONG dirID, StringPtr fullPathName);
#else
VOID	PathNameFromDirID(short vRefNum, LONG dirID, StringPtr fullPathName);
VOID	PathNameFromWD(LONG vRefNum, StringPtr pathName);
#endif
VOID	pstrcat(StringPtr dst, StringPtr src);
VOID	pstrinsert(StringPtr dst, StringPtr src);
VOID	PToCString(char *dst, StringPtr src);
Ptr		kNewPtr(long bufsize);
void	kDisposePtr(Ptr ptr);
VOID	GetNvRAMFullPathName(BOOL createDesktopFolder, char *inFilename, char *outFullName);
VOID	SaveRawImage( char *fileName, short opFlag, LPBYTE pBuffer, DWORD count );

#endif

//----------------------------------------------------
//	Public functions:
//----------------------------------------------------
void* AVAllocateMemory( DWORD sizes );
void* AVFreeMemory( void *lpBuf );
DWORD MaxAvailMemSize( void );
void DelayTime( DWORD ms );
//WORD ReverseWORD( WORD data );
//DWORD ReverseDWORD( DWORD data );



#define REVERSEWORD(x)         ((( x & 0xFF) << 8) | (x >> 8))
//#define REVERSEDWORD(x)        ( (REVERSEWORD(HIWORD(x)) << 16) | REVERSEWORD(LOWORD(x)) )
#define REVERSEDWORD(x)        ( (REVERSEWORD(LOWORD(x)) << 16) | REVERSEWORD(HIWORD(x)) )




#ifdef _WINDOWS

#define HostToHL16 REVERSEWORD // Big endian
#define HostToLH16(data) (data)
#define HL16ToHost	REVERSEWORD
#define LH16ToHost(data) (data)

#define HostToHL32 REVERSEDWORD 
#define HostToLH32(data) (data) 
#define HL32ToHost	REVERSEDWORD
#define LH32ToHost(data)	(data)

#endif

#ifdef __MAC__
#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD)(a) & 0xff)) | ((WORD)((BYTE)((DWORD)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD)(b) & 0xffff))) << 16))

#ifndef LOWORD
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#endif

#ifndef HIWORD
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#endif
//#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
//#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))

#define HostToLH16 REVERSEWORD
#define HostToHL16(data) (data)
#define LH16ToHost	REVERSEWORD
#define HL16ToHost	HostToHL16

#define HostToLH32 REVERSEDWORD
#define HostToHL32(data) (data)
#define LH32ToHost	REVERSEDWORD
#define HL32ToHost(data) (data)


#endif





