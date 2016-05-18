
// MiceUiKey.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MiceUiKey.h"
#include "MiceUiKeyDlg.h"
#include "standard.h"
#include "bios.h"
#include "threads.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMiceUiKeyApp

BEGIN_MESSAGE_MAP(CMiceUiKeyApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMiceUiKeyApp construction

CMiceUiKeyApp::CMiceUiKeyApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	GetModuleFileName(NULL, szFilePathName, sizeof(szFilePathName));
	int i;
	
    for(i=strlen(szFilePathName)-4; i>0; i--)
        if(szFilePathName[i] == '\\')
            break;
	szFilePathName[i+1]=0;
	wsprintf(szIniFileName, "%s\\am3xtest.ini", szFilePathName);
}


// The one and only CMiceUiKeyApp object

CMiceUiKeyApp theApp;


// CMiceUiKeyApp initialization

BOOL CMiceUiKeyApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	char szTemp[128];	
	char *ptr;

	TSprolog();
		
	GetPrivateProfileString("Basic", "VID", "0638", szTemp, sizeof(szTemp), szIniFileName);
	gVid = (Uint16)strtol(szTemp, &ptr, 16);
	GetPrivateProfileString("Basic", "PID", "0", szTemp, sizeof(szTemp), szIniFileName);
	gPid = (Uint16)strtol(szTemp, &ptr, 16);
	GetPrivateProfileString("Basic", "Model", "SCAN", gName, sizeof(gName), szIniFileName);
	GetPrivateProfileString("Basic", "Model2", "SCAN", gName2, sizeof(gName2), szIniFileName);
	GetPrivateProfileString("Basic", "Printer", "Panther", gPrintName, sizeof(gPrintName), szIniFileName);

	GetPrivateProfileString("OutFileName", "PreName", "", gPreFileName, sizeof(gPreFileName), szIniFileName);
	gPreFileNum = GetPrivateProfileInt("OutFileName", "PreNum", 1,szIniFileName);

	CMiceUiKeyDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
