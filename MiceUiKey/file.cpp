#include "stdafx.h"
#include "MiceUiKey.h"
#include "file.h"

extern CMiceUiKeyApp theApp;

int CheckIfFileExists (LPCTSTR szFileName)
{
  int      hFile;
  OFSTRUCT ofstruct;

  if (szFileName[0] == '\0')
    return FALSE;

  hFile = OpenFile((LPSTR) szFileName, &ofstruct, OF_EXIST);

  if (hFile > 0)
    return TRUE;
  else
    return FALSE;
}

char* GetFileOpenName(char * szFilter) 
{	
	static char szFilename[1024];	
	OPENFILENAME ofn;
	char *pch;

	if(szFilter)
	{
		for (pch = &szFilter[0]; '\0' != *pch; pch++)
		{
			if ('!' == *pch)
				*pch = '\0';
		}
	}	
	memset (&ofn, 0, sizeof (OPENFILENAME));

	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.hwndOwner = GetFocus();
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 0;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;    
	ofn.lpstrFile = szFilename;
	ofn.nMaxFile = sizeof(szFilename);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	
	ofn.lpstrInitialDir = theApp.szFilePathName;
	
	ofn.lpstrTitle = "Open File";
	ofn.Flags = OFN_FILEMUSTEXIST;
	if (GetOpenFileName (&ofn))
		return szFilename;
	else
		return NULL;
   
}

char* GetFileSaveName(char * szFilter) 
{	
	static char szFilename[1024];	
	OPENFILENAME ofn;	
	char *pch;

	for (pch = &szFilter[0]; '\0' != *pch; pch++)
    {
        if ('!' == *pch)
            *pch = '\0';
    }

	memset (&ofn, 0, sizeof (OPENFILENAME));

	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.hwndOwner = ::GetFocus();
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szFilename;
	ofn.nMaxFile = sizeof(szFilename);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrDefExt = "bmp";	
	ofn.lpstrInitialDir = theApp.szFilePathName;
	
	ofn.lpstrTitle = "Save File";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	if (GetSaveFileName (&ofn))
		return szFilename;
	else
		return NULL;   
}
