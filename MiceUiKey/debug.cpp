#include "stdafx.h"
#include "debug.h"

HANDLE StartSaveFile(char *szFileName)
{
	HANDLE  hFile;

	hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if( INVALID_HANDLE_VALUE != hFile )
    {
        
	    SetFilePointer (hFile, NULL, NULL, FILE_END);
	}
	return hFile;
}

void StopSaveFile(HANDLE hFile)
{
	if( INVALID_HANDLE_VALUE != hFile )
	{
		CloseHandle(hFile);
	}
}

BOOL SaveLineToFile(HANDLE hFile,  char *pData, int len)
{		
	BOOL	retApi = FALSE;	
	DWORD   rDWORD;
	int count;    
    char newLine[] = "\r\n";    
		
    if( INVALID_HANDLE_VALUE != hFile )
    {
        
	    retApi = SetFilePointer (hFile, NULL, NULL, FILE_END);
       		
		if(pData[len-1] == '\r' || pData[len-1] == '\n')
		{
			count = 1;
			if(pData[len-2] == '\r' || pData[len-2] == '\n')
				count ++;

			retApi = WriteFile(hFile, pData, len - count, &rDWORD, NULL);
			retApi = WriteFile(hFile, newLine, 2, &rDWORD, NULL);
		}
		else
		{
			retApi = WriteFile(hFile, pData, len, &rDWORD, NULL);
		}                    		    
    }	
		
	return TRUE;
}

