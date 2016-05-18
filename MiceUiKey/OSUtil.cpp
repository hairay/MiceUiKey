#include "stdafx.h"
#include "assert.h"
#include "OSUtil.h"

//**************************************************************
//	Public functions:
//
//	1.HiLoWORD = ToMotoWORD
//	2.LoHiWORD = ToIntelWORD
//	3.HiLoDWORD = ToMotoDWORD
//	4.LoHiDWORD = ToIntelDWORD
//	5.AVAllocateMemory
//	6.AVFreeMemory
//	7.MaxAvailMemSize
//	8.DelayTime
//	9.GetTickCount
// 10.HostToHL16
// 11.HostToLH16
// 12.HostToHL32
// 13.HostToLH32
// 10.HL16ToHost
// 11.LH16ToHost
// 12.HL32ToHost
// 13.LH32ToHost
//**************************************************************

//**************************************************************
//	Windows virtual functions on MAC:
//
//	1.CreateThread
//	2.TerminateThread
//	3.ExitThread
//	4.ResumeThread
//	5.SuspendThread
//	6.SetThreadPriority
//	7.GetThreadPriority
//	8.GetExitCodeThread
//	9.InterlockedDecrement
// 10.InterlockedIncrement
// 11.InterlockedExchangeAdd
// 12.CloseHandle
// 13.WaitForSingleObject
// 14.ReleaseMutex
//**************************************************************
//#pragma optimize( "", on )  
/* 
WORD ReverseWORD( WORD data )			
{
	WORD hiByte, loByte;

	loByte = data & 0xFF;
	hiByte = data & 0xFF00;

	loByte = loByte << 8;
	hiByte = hiByte >> 8;

	return (loByte | hiByte);
}


DWORD ReverseDWORD( DWORD data )
{	
	dwDataType srcLong, dstLong;
	srcLong.dwData = data;
	
	dstLong.byte[0] = srcLong.byte[3];
	dstLong.byte[1] = srcLong.byte[2];
	dstLong.byte[2] = srcLong.byte[1];
	dstLong.byte[3] = srcLong.byte[0];
	
	return	dstLong.dwData;
}
*/
#ifdef _WINDOWS
//--------------------------------------------------------------
//	memory
//--------------------------------------------------------------

void* AVAllocateMemory( DWORD sizes )
{
	HGLOBAL bufHdl;

    
    //if(sizes > 1048576*8)
      //  return NULL;

	bufHdl = GlobalAlloc( GMEM_FIXED, sizes );
	if( bufHdl == NULL )
		return NULL;

	return (void*)bufHdl;
}

void* AVFreeMemory( void *lpBuf )
{
	HGLOBAL bufHdl;

	if( lpBuf )
	{
		bufHdl = GlobalHandle( lpBuf );
		GlobalFree( bufHdl );
	}

	return NULL;
}

DWORD MaxAvailMemSize( void )
{
	MEMORYSTATUS status;
	DWORD size;

	GlobalMemoryStatus( &status );
	size = status.dwTotalPhys;

	return size;
}

//--------------------------------------------------------------
//	time
//--------------------------------------------------------------

void DelayTime( DWORD ms )
{
	DWORD startTick, endTick;

	startTick = endTick = GetTickCount();
	while( endTick - startTick < ms )
	{
		endTick = GetTickCount();
	}
}


#endif

#ifdef __MAC__
// _Mac_
//--------------------------------------------------------------
//	memory
//--------------------------------------------------------------

void* AVAllocateMemory( DWORD sizes )
{
	Ptr bufPtr;

	bufPtr = NewPtr( sizes );
	if( bufPtr == NULL )
		return NULL;

	return (void*)bufPtr;
}

void* AVFreeMemory( void *lpBuf )
{

	if( lpBuf )
	{
		DisposePtr((Ptr)lpBuf);
	}

	return NULL;
}

DWORD MaxAvailMemSize( void )
{
	Size availSize;
	Size growByte;

	availSize = MaxMem(&growByte);
	
	return availSize;
}

//--------------------------------------------------------------
//	time
//--------------------------------------------------------------
void DelayTime( DWORD ms )
{
	DWORD startTick, endTick;

	startTick = endTick = TickCount();
	while( ((endTick - startTick) * 1000 / 60) < ms )
	{
		endTick = TickCount();
	}
}

/*
DWORD GetTickCount(void)
{
	long	targetTick;
	
	targetTick = TickCount();
	targetTick = targetTick * 16.66;
	
	return targetTick;
}
*/
//--------------------------------------------------------------
//	virtual function on MAC
//--------------------------------------------------------------

// Thread
HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,DWORD dwCreationFlags,LPDWORD lpThreadId
    )
{
	return NULL;
}

/* jhn
BOOL WINAPI TerminateThread(HANDLE hThread,DWORD dwExitCode)
{
	return true;
}
*/

VOID WINAPI ExitThread(DWORD dwExitCode)
{
	return;
}

DWORD WINAPI ResumeThread(HANDLE hThread)
{
	return 0;
}

DWORD WINAPI SuspendThread(HANDLE hThread)
{
	return 0;
}

BOOL WINAPI SetThreadPriority(HANDLE hThread,int nPriority)
{
	return true;
}

short WINAPI GetThreadPriority(HANDLE hThread )
{
	return 0;
}

BOOL WINAPI GetExitCodeThread(HANDLE hThread,LPDWORD lpExitCode)
{
	return true;
}

LONG WINAPI InterlockedDecrement(LPLONG lpAddend)
{
	return 0;
}

LONG WINAPI InterlockedIncrement(LPLONG lpAddend)
{
	return 0;
}

BOOL WINAPI CloseHandle(HANDLE hObject)
{
	return true;
}

LONG WINAPI InterlockedExchangeAdd(LPLONG lpAddend, LONG increment)
{
	return 0;
}

/* jhn
LONG WINAPI InterlockedExchange(LPLONG lpAddend, LONG increment)
{
	return 0;
}

// mutex
DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	return WAIT_OBJECT_0;
}

BOOL WINAPI ReleaseMutex(HANDLE hObject)
{
	return true;
}
*/

//--------------------------------------------------------------
//	other function on MAC
//--------------------------------------------------------------

UINT GetPrivateProfileInt(char* appName, char* keyName, int defaultValue, char* fileName)
{
	return defaultValue;
}

#define		kDULBEventID	'dulb'
#define		keyOperand		'OPND'

BOOL PostMessage(HWND hWnd, OSType Msg, LPWORD wParam, LPWORD lParam)
{
	AEAddressDesc		targetAddress;
	AppleEvent			theAppleEvent,reply;
	long 				param = 0;
	OSErr				err;
	
	err = AECreateDesc('sign',&Msg,sizeof(Msg),&targetAddress);
	if (err == noErr)
		err = AECreateAppleEvent(kCoreEventClass,kDULBEventID,&targetAddress,kAutoGenerateReturnID,kAnyTransactionID,&theAppleEvent);
	if (err == noErr)
		err = AEPutParamPtr(&theAppleEvent,keyOperand,typeLongInteger,&param,sizeof(param));
	if (err == noErr)
		err = AESend(&theAppleEvent,&reply,kAENoReply,kAEHighPriority,0,nil,nil);
		
	AEDisposeDesc(&theAppleEvent);
	
	if (err == 0)
		return true;
	else
		return false;

}



// ===========================================================================================
VOID pstrcat(StringPtr dst, StringPtr src)
{
	/* copy string in */
	BlockMove(src + 1, dst + *dst + 1, *src);
//	memcpy(dst + *dst + 1, src + 1, *src);
	/* adjust length byte */
	*dst += *src;
}

/*
 * pstrinsert - insert string 'src' at beginning of string 'dst'
 */
VOID pstrinsert(StringPtr dst, StringPtr src)
{
	/* make room for new string */
	BlockMove(dst + 1, dst + *src + 1, *dst);
	/* copy new string in */
	BlockMove(src + 1, dst + 1, *src);
	/* adjust length byte */
	*dst += *src;
}

/*
 * PToCString - Convert pascal string to C string
 */
VOID PToCString(char *dst, StringPtr src)
{
	BYTE	count=0,i;
	
	count=src[0];
	for(i=0; i<count; i++) {
		dst[i]=src[i+1];
	}
	dst[i]='\0';
}


#ifndef _MacOSX_
VOID PathNameFromDirID(short vRefNum, LONG dirID, StringPtr fullPathName)
{
	DirInfo		block;
	Str255		directoryName;
	OSErr		err;

	fullPathName[0] = '\0';
	fullPathName[1] = '\0';

    block.ioCompletion = 0x00;
    block.ioDrParID = dirID;
    block.ioNamePtr = directoryName;
	do {
		block.ioVRefNum = vRefNum;
		block.ioFDirIndex = -1;
		block.ioDrDirID = block.ioDrParID;
		err = PBGetCatInfo((CInfoPBPtr)&block, FALSE);
		pstrcat(directoryName, (StringPtr)"\p:");
		pstrinsert(fullPathName, directoryName);
	}
	while (block.ioDrDirID != fsRtDirID);
}

/*
PathNameFromWD:
Given an HFS working directory, this routine returns the full pathname that
corresponds to it. It does this by calling PBGetWDInfo to get the VRefNum and
DirID of the real directory. It then calls PathNameFromDirID, and returns its
result.

*/
VOID PathNameFromWD(LONG vRefNum, StringPtr pathName)
{
	WDPBRec		myBlock;
	OSErr		err;
	/*
 	PBGetWDInfo has a bug under A/UX 1.1.  If vRefNum is a real
	 vRefNum and not a wdRefNum, then it returns garbage.
 	Since A/UX has only 1 volume (in the Macintosh sense) and
 	only 1 root directory, this can occur only when a file has been
 	selected in the root directory (/).
 	So we look for this and hardcode the DirID and vRefNum.
	*/
	if (vRefNum == -1)
		PathNameFromDirID(-1, fsRtDirID, pathName);
	else {
		myBlock.ioNamePtr = nil;
		myBlock.ioVRefNum = vRefNum;
		myBlock.ioWDIndex = 0;
		myBlock.ioWDProcID = 0;
	/*
	 Change the Working Directory number in vRefnum into a real
	vRefnum and DirID. The real vRefnum is returned in ioVRefnum,
	 and the real DirID is returned in ioWDDirID.
	*/
		err = PBGetWDInfo(&myBlock, FALSE);
		if (err != noErr)	return;
		PathNameFromDirID(myBlock.ioWDVRefNum, myBlock.ioWDDirID, pathName);				
	}
}
#else
VOID PathNameFromDirID(short vRefNum, LONG dirID, StringPtr fullPathName)
{
	CInfoPBRec	block;
	Str255		directoryName;
	OSErr		err;

	fullPathName[0] = '\0';
	fullPathName[1] = '\0';

    //block.ioCompletion = 0x00;
    block.dirInfo.ioDrParID = dirID;
    block.dirInfo.ioNamePtr = directoryName;
	do {
		block.dirInfo.ioVRefNum = vRefNum;
		block.dirInfo.ioFDirIndex = -1;
		block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;
		err = PBGetCatInfoSync(&block);
		pstrcat(directoryName, (StringPtr)"\p:");
		pstrinsert(fullPathName, directoryName);
	}
	while (block.dirInfo.ioDrDirID != fsRtDirID);
}

#endif



//Memory allocation
Ptr kNewPtr(long bufsize)
{
	Handle	  h = nil;
	Ptr 	   ptr = nil;
	OSErr	 theErr = 0;
	
	UInt32 freeMem = TempFreeMem(); //Get temporary memory capacity
	if(freeMem > (5*1024*1024)){//Use temporary memory
		h = TempNewHandle( bufsize, &theErr );
		if(h && (theErr == noErr)){
			HLockHi( h );
			ptr = *h;
			//std::memset(ptr, 0, bufsize);
			memset(ptr, 0, bufsize);
		}
	}
	
	if( h == nil ){//can not use tempporary memory, Use apprication heap memory
		h = NewHandleClear(bufsize);
		if(h){
			HLockHi( h );
			ptr = *h;
		}
	}
	
	return ptr;
}


//Memory release
void kDisposePtr(Ptr ptr)
{
	Handle	  hand;
	if(ptr){
		hand = RecoverHandle(ptr);
		if(hand){
			HUnlock( hand );
			DisposeHandle( hand );
		}
	}
}


#define		kNvRAMFolderName	"\pLouvre"
#define		kMaxPathLength			128


VOID	GetNvRAMFullPathName(BOOL createDesktopFolder, char *inFilename, char *outFullName)
{
	short				theVolRefNum;
	LONG				theDirID;
	LONG				myFolderID;
	//LONG				desFolderID;
	OSErr				myErr = noErr;
	Str255				fullPathFName;
	
	//myErr = FindFolder( kOnSystemDisk, kDesktopFolderType,
	//					kDontCreateFolder, &theVolRefNum, &theDirID );
	myErr = FindFolder( kOnSystemDisk, kPreferencesFolderType,
						kDontCreateFolder, &theVolRefNum, &theDirID );
	PathNameFromDirID( theVolRefNum, theDirID, fullPathFName );

	if( createDesktopFolder == TRUE )	// Create debug folder on desktop
	{
		myErr = DirCreate( theVolRefNum, theDirID, kNvRAMFolderName, &myFolderID );
		if( myErr == 0 || myErr == -48 )
		{
			pstrcat( fullPathFName, kNvRAMFolderName );
			pstrcat( fullPathFName, "\p:" );
			/*myErr = DirCreate( theVolRefNum, myFolderID, kDebugSubFolderName, &desFolderID );
			if( myErr == 0 || myErr == -48 )
			{
				pstrcat( fullPathFName, kDebugSubFolderName );
				pstrcat( fullPathFName, "\p:" );
			}*/
		}
	}
	PToCString( outFullName, fullPathFName );
	strcat( outFullName, inFilename );	

	return;
}


VOID	SaveRawImage( char *fileName, short opFlag, LPBYTE pBuffer, DWORD count )
{
	char				rawFilePath[kMaxPathLength];
	FILE				*fp = NULL;

	GetNvRAMFullPathName( TRUE, fileName, rawFilePath );	// Need create folder
	switch(opFlag)
	{
		case kDebugopCreateNew:
		{
			if( (fp = fopen(rawFilePath, "w")) != NULL )
				fclose(fp);
			break;
		}
		case kDebugopWriteExist:
		{
			if( (fp = fopen(rawFilePath, "ab+")) != NULL )
			{
				fseek( fp, 0L, SEEK_END );			// to end of file.
				fwrite( pBuffer, count, 1, fp );	// Put data
				fflush(fp);							// Clean it
				fclose(fp);
			}
			break;
		}
		default:
		{
			break;
		}
	}
}
#endif