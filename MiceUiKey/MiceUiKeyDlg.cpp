
// MiceUiKeyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MiceUiKey.h"
#include "MiceUiKeyDlg.h"
#include "standard.h"
#include "usb_io.h"
#include "threads.h"
#include "bios.h"
#include "debug.h"
#include "file.h"

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

static HANDLE m_hUSB;
static HANDLE m_hDbgIn;
static HANDLE m_hDbgOut;
static HWND sDlgWnd; 
static TsThread sReadButtonID = INVALIDTHREAD;
static TsThread sSaveButtonID = INVALIDTHREAD;
static TsThread sPlayButtonID = INVALIDTHREAD;
static BOOL gGetDeviceHandle = FALSE;
static TsMailbox sButtonMailBox;
static Uint32 sLoopCount;

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
			eventData.msg1 = 2;
			eventData.msg2 = data[0];
			eventData.msg3 = data[1];
			eventData.msg4.ullval = GetTickCount();
			retApi = TSmsgSend(sButtonMailBox, &eventData);
			BIOSlog("TSmsgSend ret = %d data=%x %x time=%u\n", retApi, data[0], data[1],eventData.msg4.ullval);
		}
		else
		{
			TSsleep(1, 0);	
			BIOSlog("GetEvent ret = %u\n", ret);
		}
	}
}

static Sint32 SaveButtonThread(void *parm)
{
	Msg eventData; 
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int	retApi;
	CString	t;// = CTime::GetCurrentTime().Format("%Y%m%d-%H%M%S");
	char	*pszDateTime; //=t.GetBuffer(0);
	char    szTemp[MAX_PATH];

	while (1)
	{
		retApi = TSmsgTimeout(sButtonMailBox, &eventData, 10, 0);
		if(retApi == 0)
		{
			if(eventData.msg1 == 1) //start record key
			{
				t = CTime::GetCurrentTime().Format("%Y%m%d-%H%M%S");
				pszDateTime = t.GetBuffer(0);
				sprintf(szTemp, "%s%s.txt", theApp.szFilePathName,pszDateTime);
				hFile = StartSaveFile(szTemp);
				sprintf(szTemp, "1 0 0 %lu\r", eventData.msg4.ullval);
				SaveLineToFile(hFile, szTemp, strlen(szTemp));
			}
			else if(eventData.msg1 == 0) //stop record key
			{
				sprintf(szTemp, "0 0 0 %lu\r", eventData.msg4.ullval);
				SaveLineToFile(hFile, szTemp, strlen(szTemp));	
				StopSaveFile(hFile);
			}
			else if(hFile != INVALID_HANDLE_VALUE)//save key
			{
				sprintf(szTemp, "%d %d %d %lu\r", eventData.msg1, eventData.msg2, eventData.msg3,eventData.msg4.ullval);
				SaveLineToFile(hFile, szTemp, strlen(szTemp));	
			}			
		}
		else
		{
			BIOSlog("TSmsgTimeout ret = %u\n", retApi);
		}
	}
}

DWORD GetMsTimeFromStart(DWORD curTime, DWORD startTime)
{	
	if(curTime >= startTime)
		return (curTime - startTime);	
	else	
		return ( 0xFFFFFFFF - startTime + curTime + 1);					
}


static Sint32 PlayButtonThread(void *parm)
{
	int	retApi = 0;
	char *pFileName = (char *)parm;

	HANDLE	hFile = INVALID_HANDLE_VALUE; 
	DWORD	dwFileSize, rDWORD, lastTime, time, loop =0;
	char	*fileArea =NULL;
	char	*fileData =NULL;
	BOOL	retBool, ret = TRUE;	
	char	seps[]   = " \r\n";
	char    temp[128];
	char	*token;		
	DWORD	tokenNum = 0, data[8];

	hFile = CreateFile(pFileName,           // open MYFILE.TXT 
					GENERIC_READ,              // open for reading 
					FILE_SHARE_READ,           // share for reading 
					NULL,                      // no security 
					OPEN_EXISTING,             // existing file only 
					FILE_ATTRIBUTE_NORMAL,     // normal file 
					NULL);                     // no attr. template 
 
	if (hFile == INVALID_HANDLE_VALUE) 
	{	
		goto RETURN;
	}

	dwFileSize = GetFileSize (hFile, NULL) ; 
	fileArea = new char[dwFileSize+128];
	fileData = new char[dwFileSize+128];
	fileArea[dwFileSize] = 0;
	retBool = ReadFile( hFile, fileArea, dwFileSize, &rDWORD, NULL );
	CloseHandle(hFile);
	if(retBool == FALSE || dwFileSize != rDWORD)
	{		
		goto RETURN;
	}

	SetDlgItemText(sDlgWnd, IDC_STATUS, "START");	
	while(sLoopCount)
	{
		loop ++;
		sprintf(temp, "Loop:%u", loop);
		SetDlgItemText(sDlgWnd, IDC_LOOP_TEXT, temp);

		memcpy(fileData, fileArea, (dwFileSize+7)/4*4);
		token = strtok( fileData, seps );
		while( token != NULL )
		{				
			data[tokenNum] = atoi(token);
			tokenNum ++;		
			if(tokenNum == 4)
			{
				if(data[0] == 2 && ret == TRUE) //key data
				{
					time = GetMsTimeFromStart(data[3], lastTime);
					Sleep(time);
					ret = send_cmd_data(m_hDbgIn,m_hDbgOut,(char *)&data[1],8,SCSI_SDTC_BUTTON,2, FALSE);  				
				}
				else if(data[0] == 0 && ret == TRUE)//stop
				{
					time = GetMsTimeFromStart(data[3], lastTime);
					Sleep(time);
				}
				if(ret != TRUE)
				{
					sprintf(temp, "USB I/O FAIL :Loop:%u", loop);
					SetDlgItemText(sDlgWnd, IDC_STATUS, temp);	
					goto RETURN;
				}
				lastTime = data[3];	
				tokenNum = 0;
			}
			/* Get next token: */
			token = strtok( NULL, seps );
		}
		sLoopCount --;
	}	
	SetDlgItemText(sDlgWnd, IDC_STATUS, "END");	

RETURN:	
	if(fileArea != NULL)
		delete(fileArea);
	if(fileData != NULL)
		delete(fileData);
	
	SetDlgItemText(sDlgWnd, IDC_LOOP_TEXT, "Loop Count");
	EnableWindow(GetDlgItem(sDlgWnd, IDC_START), TRUE);
	EnableWindow(GetDlgItem(sDlgWnd, IDC_STOP), TRUE);	
	EnableWindow(GetDlgItem(sDlgWnd, IDC_PLAY), TRUE);	
	return retApi;
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
	, m_loop_count(_T("1"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMiceUiKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOOP_COUNT, m_loop_count);
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
	ON_BN_CLICKED(IDC_PLAY, &CMiceUiKeyDlg::OnBnClickedPlay)	
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
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	if(m_hUSB == NULL)
	{
		GetDlgItem(IDC_PLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_START)->EnableWindow(FALSE);
	}
	sButtonMailBox = TScreateMsgQueue();
	sReadButtonID = TScreateThread(
                            ReadButtonThread,
                            IDLE_STACK_SIZE,
                            IDLE_TASK_PRIORITY,
                            NULL,
                            "readButtonThread"
                            );

	sSaveButtonID = TScreateThread(
                            SaveButtonThread,
                            IDLE_STACK_SIZE,
                            IDLE_TASK_PRIORITY,
                            NULL,
                            "saveButtonThread"
                            );
	

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

	if(sSaveButtonID != INVALIDTHREAD)
		TSdestroyThread(sReadButtonID);

	if(sPlayButtonID != INVALIDTHREAD)
		TSdestroyThread(sPlayButtonID);	
	
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

	Msg eventData={0}; 

	GetDlgItem(IDC_PLAY)->EnableWindow(FALSE);
	GetDlgItem(IDC_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP)->EnableWindow(TRUE);

	eventData.msg1 = 1;
	eventData.msg4.ullval = GetTickCount();	
	ret = send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,1, FALSE);  
	TSmsgSend(sButtonMailBox, &eventData);
}

void CMiceUiKeyDlg::OnBnClickedStop()
{
	// TODO: Add your control notification handler code here
	int ret;
	Msg eventData={0}; 
	
	eventData.msg1 = 0;
	eventData.msg4.ullval = GetTickCount();
	
	ret = send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,0, FALSE);  
	TSmsgSend(sButtonMailBox, &eventData);
	GetDlgItem(IDC_PLAY)->EnableWindow(TRUE);
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
}

void CMiceUiKeyDlg::OnBnClickedPlay()
{
	// TODO: Add your control notification handler code here
	char *pFileName;

	if(sPlayButtonID != INVALIDTHREAD)
		TSdestroyThread(sPlayButtonID);
	UpdateData(TRUE);
	sLoopCount = atoi((LPCTSTR)m_loop_count);
	sDlgWnd = m_hWnd;
	pFileName = GetFileOpenName("*.txt");

	if(pFileName != NULL)
	{
		SetDlgItemText(IDC_LOOP_TEXT, "Start");
		GetDlgItem(IDC_START)->EnableWindow(FALSE);
		GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
		GetDlgItem(IDC_PLAY)->EnableWindow(FALSE);
		sPlayButtonID = TScreateThread(
								PlayButtonThread,
								IDLE_STACK_SIZE,
								IDLE_TASK_PRIORITY,
								(void *)pFileName,
								"playButtonThread"
								);			
	}
}

