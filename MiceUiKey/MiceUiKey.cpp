
// MiceUiKey.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MiceUiKey.h"
#include "MiceUiKeyDlg.h"
#include "standard.h"
#include "bios.h"
#include "threads.h"
#include "usb_io.h"
#include "file.h"


 class CNewCommandLineInfo : public CCommandLineInfo
{
public:
	BOOL skipUi;
	
	void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);
};

void CNewCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
	if(bFlag) 
	{
		CString sParam(lpszParam);
		if (sParam.Left(1) == "s") {
			skipUi = TRUE;
			return;
		}		
	}

	// Call the base class to ensure proper command line processing
	CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}

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
	wsprintf(szSysFileName, "%s\\SysInfo.ini", szFilePathName);	
}


// The one and only CMiceUiKeyApp object

CMiceUiKeyApp theApp;


// CMiceUiKeyApp initialization
BOOL CMiceUiKeyApp::ReOpenDevice()
{
	BOOL retResult = FALSE;

	if(m_hUSB)
    {
        close_usb(m_hUSB, m_hDbgIn, m_hDbgOut);
		m_hUSB = NULL;
		m_hDbgIn = NULL;
		m_hDbgOut = NULL;
	}
	    	    		
    if(retResult == FALSE)
	{
		char szVidStr[16], szPidStr[16], szTemp[64];
		int retry =1;
		
		GetPrivateProfileString("Basic", "VID", "0638", szTemp, sizeof(szTemp), theApp.szIniFileName);			
		theApp.gVid = (unsigned short)strtol(szTemp, NULL, 16);
			
		GetPrivateProfileString("Basic", "PID", "0", szTemp, sizeof(szTemp), theApp.szIniFileName);
		theApp.gPid = (unsigned short)strtol(szTemp, NULL, 16);


		while(m_hUSB == 0)
		{
			//wsprintf(szTemp, "gVid=%xh gPid=%xh", gVid, gPid);
			//MessageBox(GetFocus(), szTemp, szIniFileName, MB_OK);
			open_usb( &m_hUSB, &m_hDbgIn, &m_hDbgOut ,theApp.gVid , theApp.gPid);
			if(m_hUSB) 
				break;
			wsprintf(szVidStr, "VID%d", retry);
			GetPrivateProfileString("Basic", szVidStr, "0", szTemp, sizeof(szTemp), theApp.szIniFileName);
			
			theApp.gVid = (unsigned short)strtol(szTemp, NULL, 16);
			if(theApp.gVid == 0)
				break;
			wsprintf(szPidStr, "PID%d", retry);
			GetPrivateProfileString("Basic", szPidStr, "0", szTemp, sizeof(szTemp), theApp.szIniFileName);
			theApp.gPid = (unsigned short)strtol(szTemp, NULL, 16);
			retry ++;
		}
		
		if(m_hUSB) 		
		{
			retResult = TRUE;
		}
	}	
	return retResult;
}

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
	LPCTSTR pszParam = _T("Reset");

	CNewCommandLineInfo cmdInfo;

	cmdInfo.skipUi = FALSE;	
	ParseCommandLine(cmdInfo); 	
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
		
	if(cmdInfo.skipUi == FALSE)
	{
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
	}
	else
	{
		m_hUSB = NULL;

		BOOL linkOK = ReOpenDevice();

		if(linkOK)
		{
			BOOL retApi;

			stSystemInfo sysInfo;

			retApi = read_cmd_data(m_hDbgIn,m_hDbgOut,(char *)&sysInfo,sizeof(stSystemInfo), SCSI_RDTC_SYS_INFO, 0,FALSE);	
			if(retApi)
			{				
				char szText[32];
					
				WritePrivateProfileString("basic", "Update", "1", szSysFileName);	
				sprintf(szText, "%d",sysInfo.sysMem);	
				WritePrivateProfileString("basic", "sysMem", szText, szSysFileName);	
				sprintf(szText, "%d",sysInfo.mbVer);	
				WritePrivateProfileString("basic", "mbVer", szText, szSysFileName);	
				sprintf(szText, "%d",sysInfo.pdfSupport);	
				WritePrivateProfileString("basic", "pdfSupport", szText, szSysFileName);	
				WritePrivateProfileString("basic", "sysVersion", (LPCSTR)sysInfo.sysVersion, szSysFileName);	
				WritePrivateProfileString("basic", "venderName", (LPCSTR)sysInfo.venderName, szSysFileName);	
				WritePrivateProfileString("basic", "modelName", (LPCSTR)sysInfo.modelName, szSysFileName);	
				WritePrivateProfileString("basic", "serialNumber", (LPCSTR)sysInfo.serialNumber, szSysFileName);
				WritePrivateProfileString("basic", "loaderVersion", (LPCSTR)sysInfo.loaderVersion, szSysFileName);	
			}
			else
			{
				linkOK = FALSE;
			}
			close_usb(m_hUSB, m_hDbgIn, m_hDbgOut);
		}
		if(linkOK == FALSE)
		{			
			WritePrivateProfileString("basic", "Update", "0", szSysFileName);	
		}
	}
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
