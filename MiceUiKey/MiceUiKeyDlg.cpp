
// MiceUiKeyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MiceUiKey.h"
#include "MiceUiKeyDlg.h"
#include "standard.h"
#include "usb_io.h"
#include "threads.h"
#include "bios.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CMiceUiKeyApp theApp;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CMiceUiKeyDlg dialog
#define IDLE_TASK_PRIORITY TS_LOWEST_PRIORITY
#define IDLE_STACK_SIZE TSSMALL_STACK

HANDLE m_hUSB;
HANDLE m_hDbgIn;
HANDLE m_hDbgOut;
static TsThread sReadButtonID = INVALIDTHREAD;
BOOL gGetDeviceHandle = FALSE;
static TsMailbox sButtonMailBox;

static Sint32 ReadButtonThread(void *parm)
{
	BOOL ret;
	Uint32 data[8];
	Msg eventData; 
	int	retApi;

	while (1)
	{
		ret = GetEvent((LPBYTE)data, sizeof(data));
		if(ret == TRUE)
		{
			eventData.msg1 = data[0];
			eventData.msg2 = data[1];
			retApi = TSmsgSend(sButtonMailBox, &eventData);
			BIOSlog("TSmsgSend ret = %d data=%x %x\n", retApi, data[0], data[1]);
		}
		else
		{
			TSsleep(1, 0);	
			BIOSlog("GetEvent ret = %u\n", ret);
		}
	}
}

BOOL CMiceUiKeyDlg::ReOpenDevice()
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
	if(retResult == TRUE)
		gGetDeviceHandle = TRUE;
	else
		gGetDeviceHandle = FALSE;

	return retResult;
}

CMiceUiKeyDlg::CMiceUiKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMiceUiKeyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMiceUiKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMiceUiKeyDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDCANCEL, &CMiceUiKeyDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_START, &CMiceUiKeyDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CMiceUiKeyDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CMiceUiKeyDlg message handlers

BOOL CMiceUiKeyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	ReOpenDevice();
	sReadButtonID = TScreateThread(
                            ReadButtonThread,
                            IDLE_STACK_SIZE,
                            IDLE_TASK_PRIORITY,
                            NULL,
                            "readButtonThread"
                            );

	sButtonMailBox = TScreateMsgQueue();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMiceUiKeyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMiceUiKeyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMiceUiKeyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMiceUiKeyDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: Add your message handler code here
	if (sReadButtonID != INVALIDTHREAD)
		TSdestroyThread(sReadButtonID);

	if(sButtonMailBox != NULL)
		TSdestroyMsgQueue(sButtonMailBox);

	if(m_hUSB)
	{
        close_usb(m_hUSB, m_hDbgIn, m_hDbgOut);
		m_hUSB = NULL;
		m_hDbgIn = NULL;
		m_hDbgOut = NULL;
	}		
}

void CMiceUiKeyDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here	
	OnCancel();	
}

void CMiceUiKeyDlg::OnBnClickedStart()
{
	// TODO: Add your control notification handler code here
	int ret;
	Uint32 data[2];

	data[0] = 2;
	data[1] = 0;
	//ret = send_cmd_data(m_hDbgIn,m_hDbgOut,(char *)data,sizeof(data),SCSI_SDTC_BUTTON,2, FALSE);  
	ret = send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,1, FALSE);  
}

void CMiceUiKeyDlg::OnBnClickedStop()
{
	// TODO: Add your control notification handler code here
	int ret;
	Uint32 data[2];

	data[0] = 3;
	data[1] = 0;
	//ret = send_cmd_data(m_hDbgIn,m_hDbgOut,(char *)data,sizeof(data),SCSI_SDTC_BUTTON,2, FALSE);  
	ret = send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,0, FALSE);  
}
