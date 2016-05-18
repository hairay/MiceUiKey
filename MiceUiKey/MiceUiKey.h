
// MiceUiKey.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CMiceUiKeyApp:
// See MiceUiKey.cpp for the implementation of this class
//

class CMiceUiKeyApp : public CWinAppEx
{
public:
    char szFilePathName[2048];
	char szIniFileName[2048];
	char	gName[32];
	char	gName2[32];
	char	gPrintName[128];
	char	gPreFileName[128];
	DWORD	gPreFileNum;
	WORD	gVid, gPid;

public:
	CMiceUiKeyApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMiceUiKeyApp theApp;