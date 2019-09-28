
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
#include "SelectLine.h"

#include "ChartLineSerie.h"
#include "ChartPointsSerie.h"
#include "ChartSurfaceSerie.h"
#include "ChartGrid.h"
#include "ChartMouseListener.h"

#include "ChartBarSerie.h"
#include "ChartLabel.h"

#include "ChartAxisLabel.h"
#include "ChartStandardAxis.h"
#include "ChartDateTimeAxis.h"

#include "ChartCrossHairCursor.h"
#include "ChartDragLineCursor.h"

#include <math.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>

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
#define eMAX_SENSOR_NUM	12
#define eMAX_IO_SN_NUM 28

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


#define TURN_ON_LPH
#define MB716_060

static DWORD gSensorColor[MAX_SEN_IO_NUM];
char gViewSensor[MAX_SEN_IO_NUM];
static BYTE gSensorLineId[MAX_SEN_IO_NUM];
char gSensorTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN] = {
	"REG",
	"EXIT",
    "FrontCover",
    "MainTray_PaperFeed",
    "MainTray_PaperIn",
    "ManualTray_PaperIn",
    "Deskew",
    "FuserOut",
    "BackCover",
    "PaperShift",
    "Duplex",
    "HivChargerError",
};

static DWORD gIoColor[MAX_SEN_IO_NUM];
char gViewIo[MAX_SEN_IO_NUM];
static BYTE gIoLineId[MAX_SEN_IO_NUM];
char gIoTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN] = {
	"LAMP_SW",
	"HV_CHG",
	"HV_DEV",
	"HV_DEV_REV",
	"HV_GRID",
	"HV_TRANS",
	"HV_TRANSREV",
    /* PIOB */
    #ifndef TURN_ON_LPH
    "POLYGON_MOTOR_EN",
    #endif
    "MAIN_MOTOR_DIR",
	"MAIN_MOTOR_EN",
	#ifdef TURN_ON_LPH
	"FW_RESET",
	#endif
	"EXIT_MOTOR_EN",
	"PAPER_SHIFT_MOTOR_MS1",
	"PAPER_SHIFT_MOTOR_MS2",
	/* PIOC */
	"PICKUP_CLUTCH",
	"EXIT_MOTOR_DIR",
	"DESKEW_CLUTCH",
	"SENSOR_POWER_EN",
	"V24_CONTROL",
	"MOTOR_CURRENT_LOW",
	/* PIOD */
	"EXIT_MOTOR_DIR_MB070",
	"PAPER_SHIFT_MOTOR_DIR_MB070",
	"SENSOR_POWER_EN_MB070",
	"V24_EN",
	/* PIOE */
	"MANUAL_PICKUP_CLUTCH",
	"PAPER_SHIFT_MOTOR_EN",
	"PAPER_SHIFT_MOTOR_DIR",
	"FUSER_AC_POWER_SW",
#ifdef MB716_060
	"IR_SENSOR_POWER_EN",
#endif	
};

static DWORD gTempColor[MAX_SEN_IO_NUM];
char gViewTemp[MAX_SEN_IO_NUM];
static BYTE gTempLineId[MAX_SEN_IO_NUM];
char gTempTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN] = {
	"A3_TEMP",
	"A4_TEMP",
	"Env_TEMP"
};

static int sMaxLineId=0, sSelectLine;
static int sSensorMaxCount=eMAX_SENSOR_NUM;
static int sIoMaxCount=eMAX_IO_SN_NUM;
static int sTempMaxCount=0, sSkipTempCount =0;
static char sInitTime;

DWORD GetMsTimeFromStart(DWORD curTime, DWORD startTime)
{	
	DWORD time;

	if(curTime >= startTime)
		time = (curTime - startTime);
	else	
		time = ( 0xFFFFFFFF - startTime + curTime + 1);
	if (time > 0xFFFFFFF)
		time = 0;
	return time;
}

static Sint32 ReadButtonThread(void *parm)
{
	int ret;
	Uint32 data[8];
	Msg eventData; 
	int	retApi;

	while (1)
	{
		ret = GetEvent((LPBYTE)data, sizeof(data));
		if(ret > 0)
		{
			if(ret == 8) //ui key
			{
				eventData.msg1 = 2;
				eventData.msg2 = data[0];
				eventData.msg3 = data[1];
				eventData.msg4.ullval = GetTickCount();
			}
			else if(ret == 12) //printer sensor
			{
				eventData.msg1 = data[0];
				eventData.msg2 = data[1];
				eventData.msg3 = data[2];
				eventData.msg4.ullval = GetTickCount();
			}
			if(ret == 8 || ret == 12)
			{
				retApi = TSmsgSend(sButtonMailBox, &eventData);
				BIOSlog("TSmsgSend ret = %d data=%x %x time=%u\n", retApi, data[0], data[1],eventData.msg4.ullval);
			}
		}
		else
		{
			if(m_hUSB != NULL)
			{
				CMiceUiKeyDlg *dlgPtr = (CMiceUiKeyDlg *)theApp.m_pMainWnd;

				close_usb(m_hUSB, m_hDbgIn, m_hDbgOut);
				m_hUSB = NULL;
				m_hDbgIn = NULL;
				m_hDbgOut = NULL;
				dlgPtr->OnBnClickedStop();
			}			
			BIOSlog("GetEvent ret = %d\n", ret);
			TSsleep(1, 0);	
		}
	}
}

static Sint32 SaveButtonThread(void *parm)
{
	Msg eventData; 
	HANDLE hUiFile = INVALID_HANDLE_VALUE;
	HANDLE hMcuFile = INVALID_HANDLE_VALUE;	
	int	retApi;
	CString	t;// = CTime::GetCurrentTime().Format("%Y%m%d-%H%M%S");
	char	*pszDateTime; //=t.GetBuffer(0);
	char    szTemp[MAX_PATH], szUiName[MAX_PATH], szPrinterName[MAX_PATH];
	CMiceUiKeyDlg *dlgPtr = (CMiceUiKeyDlg *)theApp.m_pMainWnd;
	DWORD    startTime, lastTime, uiData, printerData;
	CChartXYSerie* pSeries = NULL;
	double xPos, yPos,yLastPos;
	char    match, lineId, view;
	short  offset, state;
	Uint64	allTime = 0;

	while (1)
	{
		retApi = TSmsgTimeout(sButtonMailBox, &eventData, 10, 0);
		if(retApi == 0)
		{
			match = 0x10;
			
			if(eventData.msg1 & 0x10000000) //printer sensor
			{
				BYTE id = (BYTE)(eventData.msg1 >> 16);
				char *namePtr="unknown1";
				
				printerData ++;
				match = 0;
				lineId = gSensorLineId[id];
				offset = sMaxLineId-lineId-sSkipTempCount-1;
				view = gViewSensor[id];
				pSeries = dlgPtr->pLineSeries[lineId];
				state = (char)(eventData.msg1 >> 8);
				//byte3:printer tag<0x10000000>   byte2: sensor id  byte 1: sensor state
				if(id < sSensorMaxCount)
					namePtr = gSensorTextTable[id];
				sprintf(szTemp, "%40s %6u %8u %12u %12u\r", namePtr, id, (BYTE)(eventData.msg1 >> 8), eventData.msg2, eventData.msg3);
				SaveLineToFile(hMcuFile, szTemp, strlen(szTemp));	
			}
			else if(eventData.msg1 & 0x20000000) //printer io state
			{
				BYTE id = (BYTE)(eventData.msg1 >> 16);
				char *namePtr="unknown2";
				
				printerData ++;
				match = 1;
				lineId = gIoLineId[id];
				offset = sMaxLineId-lineId-sSkipTempCount-1;
				view = gViewIo[id];
				state = (char)(eventData.msg1 >> 8);
				pSeries = dlgPtr->pLineSeries[lineId];
				//byte3:printer io<0x20000000>   byte2: io id  byte 1: io state
				if(id < sIoMaxCount)
					namePtr = gIoTextTable[id];
				sprintf(szTemp, "%40s %6u %8u %12u %12u\r", namePtr, id, (BYTE)(eventData.msg1 >> 8), eventData.msg2, eventData.msg3);
				SaveLineToFile(hMcuFile, szTemp, strlen(szTemp));	
			}
			else if(eventData.msg1 & 0x40000000) //printer temperature
			{
				BYTE id = (BYTE)(eventData.msg1 >> 16);
				char *namePtr="unknown3";
				
				printerData ++;
				match = 2;
				lineId = gTempLineId[id];				
				view = gViewTemp[id];
				offset = state = (short)(eventData.msg1 & 0xFFFF);
				pSeries = dlgPtr->pLineSeries[lineId];
				//byte3:printer io<0x20000000>   byte2: io id  byte 1: io state
				if(id < sTempMaxCount)
					namePtr = gTempTextTable[id];
				sprintf(szTemp, "%40s %6u %8u %12u %12u\r", namePtr, id, (short)(eventData.msg1 & 0xFFFF), eventData.msg2, eventData.msg3);
				SaveLineToFile(hMcuFile, szTemp, strlen(szTemp));	
			}
			else if(eventData.msg1 == 1) //start record key
			{
				uiData = printerData = 0;
				t = CTime::GetCurrentTime().Format("%Y%m%d-%H%M%S");
				pszDateTime = t.GetBuffer(0);
				sprintf(szUiName, "%sUI_%s.txt", theApp.szFilePathName,pszDateTime);
				hUiFile = StartSaveFile(szUiName);
				sprintf(szPrinterName, "%sMCU_%s.txt", theApp.szFilePathName,pszDateTime);
				SetDlgItemText(sDlgWnd, IDC_FILE_NAME, szPrinterName);
				strcpy(dlgPtr->m_FileName, szPrinterName);
				hMcuFile = StartSaveFile(szPrinterName);
				//sprintf(szTemp, "%sMCU_%s.log", theApp.szFilePathName,pszDateTime);				
				//hLogFile = StartSaveFile(szTemp);

				sprintf(szTemp, "1 0 0 %llu\r", eventData.msg4.ullval);
				SaveLineToFile(hUiFile, szTemp, strlen(szTemp));
				sprintf(szTemp, "%40s %6s %8s %12s %12s\r", "Name", "ID", "state","time(us)","time(ms)");
				SaveLineToFile(hMcuFile, szTemp, strlen(szTemp));	
				//sprintf(szTemp, "%40s %6s %8s %12s %20s\r", "Name", "ID", "state","time(us)","time(ms)");
				//SaveLineToFile(hLogFile, szTemp, strlen(szTemp));	
				send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,1, FALSE);  
				allTime = 0;
				sInitTime = 0;
			}
			else if(eventData.msg1 == 0) //stop record key
			{
				sprintf(szTemp, "0 0 0 %llu\r", eventData.msg4.ullval);
				SaveLineToFile(hUiFile, szTemp, strlen(szTemp));	
				
				send_cmd_data(m_hDbgIn,m_hDbgOut,NULL,0,SCSI_SDTC_BUTTON,0, FALSE);  
				if(sInitTime)
				{
					char i;

					dlgPtr->m_ChartCtrl.EnableRefresh(FALSE);	
					for(i=0; i<sMaxLineId; i++)
					{
						//time = GetMsTimeFromStart(lastTime, startTime);
						xPos = (double)allTime/100000;
						if(dlgPtr->lineCount[i] != 0xFFFFFFFF)
						{
							pSeries = dlgPtr->pLineSeries[i];
							yLastPos = dlgPtr->pLineSeries[i]->GetYPointValue(dlgPtr->lineCount[i]);
							pSeries->AddPoint(xPos,yLastPos);
							dlgPtr->lineCount[i] ++;
						}
					}
					dlgPtr->m_ChartCtrl.EnableRefresh(TRUE);
				}
				StopSaveFile(hUiFile);
				StopSaveFile(hMcuFile);
				if(uiData == 0)
					DeleteFile(szUiName);
				if(printerData == 0)
					DeleteFile(szPrinterName);
				
				hUiFile = hMcuFile = INVALID_HANDLE_VALUE;
			}
			else if(hUiFile != INVALID_HANDLE_VALUE && eventData.msg1 == 2)//save key
			{
				uiData ++;
				sprintf(szTemp, "%d %d %d %llu\r", eventData.msg1, eventData.msg2, eventData.msg3,eventData.msg4.ullval);
				SaveLineToFile(hUiFile, szTemp, strlen(szTemp));	
			}	

			if(match <= 2 && hMcuFile != INVALID_HANDLE_VALUE)
			{				
				if(sInitTime == 0)
				{					
					startTime = lastTime = eventData.msg2;
					sInitTime = 1;
				}
				//time = GetMsTimeFromStart(eventData.msg2, startTime);
				allTime += GetMsTimeFromStart(eventData.msg2, lastTime);
				if(view == 1)
				{
					xPos = (double)allTime/100000;
					if(state)
						yPos = (double)offset + 0.9;
					else
						yPos = (double)offset + 0.1;
					dlgPtr->m_ChartCtrl.EnableRefresh(FALSE);
					if(dlgPtr->lineCount[lineId] == 0xFFFFFFFF)
					{						
						pSeries->AddPoint(xPos, yPos);
						dlgPtr->lineCount[lineId] = 0;						
					}
					else
					{						
						yLastPos = dlgPtr->pLineSeries[lineId]->GetYPointValue(dlgPtr->lineCount[lineId]);
						pSeries->AddPoint(xPos,yLastPos);
						pSeries->AddPoint(xPos,yPos);
						dlgPtr->lineCount[lineId] += 2;	
					}		
					dlgPtr->m_ChartCtrl.EnableRefresh(TRUE);
				}
				else if(view == 2)
				{
					xPos = (double)allTime/100000;
					yPos = (double)offset/10.0;
					dlgPtr->m_ChartCtrl.EnableRefresh(FALSE);
					if(dlgPtr->lineCount[lineId] == 0xFFFFFFFF)
					{						
						pSeries->AddPoint(xPos, yPos);
						dlgPtr->lineCount[lineId] = 0;						
					}
					else
					{												
						pSeries->AddPoint(xPos,yPos);
						dlgPtr->lineCount[lineId] ++;	
					}		
					dlgPtr->m_ChartCtrl.EnableRefresh(TRUE);
				}

				BYTE id = (BYTE)(eventData.msg1 >> 16);
				if(match == 0)
					sprintf(szTemp, "%40s %6u %8u %12u %15.4lf\r\n", gSensorTextTable[id], id, (BYTE)(eventData.msg1 >> 8), eventData.msg2, (double)allTime/1000);
				else if(match == 1)
					sprintf(szTemp, "%40s %6u %8u %12u %15.4lf\r\n", gIoTextTable[id], id, (BYTE)(eventData.msg1 >> 8), eventData.msg2, (double)allTime/1000);
				else if(match == 2)
					sprintf(szTemp, "%40s %6u %8u %12u %15.4lf\r\n", gTempTextTable[id], id, (short)(eventData.msg1 & 0xFFFF), eventData.msg2, (double)allTime/1000);
				lastTime = eventData.msg2;
			}
		}
		else
		{
			BIOSlog("TSmsgTimeout ret = %u\n", retApi);
		}
	}
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
			data[tokenNum] = strtoul(token, NULL, 10);//atoi(token);
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
	m_points = 0;
}

#define DEFAULT_COLOR_NUM	8
void CMiceUiKeyDlg::GetSensorIoInfo(void)
{
	CChartXYSerie* pSeries = NULL;
	//0x00bbggrr	
	DWORD color[] = { 0x265290, 0x0000FF, 0x0990FE, 0x00D7FF, 0x198319,
		 			  0xF04A4A, 0x821982, 0x808080};
	char szItem[12], szTemp[MAX_SEN_IO_TEXT_LEN];
	int i, len;
	
	m_Time1 = m_Time2 = 0;
	sInitTime = 0;
	SetDlgItemText(IDC_TIME1, "0");
	SetDlgItemText(IDC_TIME2, "0");
	m_ChartCtrl.RemoveAllSeries();
	sMaxLineId = 0;

	for(sSensorMaxCount,i=0; i<MAX_SEN_IO_NUM; i++)
	{
		sprintf(szItem, "%d", i);
		len = GetPrivateProfileString("268435456", szItem, "", szTemp, sizeof(szTemp), theApp.szIniFileName);
		if(len > 1)
		{
			strcpy(gSensorTextTable[i], szTemp);
			//Get gSensorColor
			sprintf(szItem, "Color%d", i);	
			GetPrivateProfileString("268435456", szItem, "1A", szTemp, sizeof(szTemp), theApp.szIniFileName);
			gSensorColor[i] = (DWORD)strtol(szTemp, NULL, 16);

			//Get gViewSensor
			sprintf(szItem, "View%d", i);
			gViewSensor[i] = GetPrivateProfileInt("268435456", szItem, 0, theApp.szIniFileName);			
			if(gViewSensor[i])
			{
				gSensorLineId[i] = sMaxLineId;				
				lineCount[sMaxLineId] =0xFFFFFFFF;
				pLineSeries[sMaxLineId] = m_ChartCtrl.CreateLineSerie(0, 0);						
				pSeries = pLineSeries[sMaxLineId];
				pSeries->SetName(gSensorTextTable[i]);
				pLineSeries[sMaxLineId]->SetWidth(2);
				pLineSeries[sMaxLineId]->SetPenStyle(0);
				if(gSensorColor[i] == 0x1A)
					pSeries->SetColor(color[sMaxLineId%DEFAULT_COLOR_NUM]);
				else
					pSeries->SetColor(gSensorColor[i]);
				sMaxLineId++;
			}
			
			sSensorMaxCount = i+1;
		}
		else
		{
			gSensorTextTable[i][0] = 0;
			break;
		}
	}

	for(sIoMaxCount,i=0; i<MAX_SEN_IO_NUM; i++)
	{
		sprintf(szItem, "%d", i);
		len = GetPrivateProfileString("536870912", szItem, "", szTemp, sizeof(szTemp), theApp.szIniFileName);
		if(len > 1)
		{
			strcpy(gIoTextTable[i], szTemp);
			//Get gIoColor
			sprintf(szItem, "Color%d", i);	
			GetPrivateProfileString("536870912", szItem, "1A", szTemp, sizeof(szTemp), theApp.szIniFileName);
			gIoColor[i] = (DWORD)strtol(szTemp, NULL, 16);

			sprintf(szItem, "View%d", i);
			gViewIo[i] = GetPrivateProfileInt("536870912", szItem, 0, theApp.szIniFileName);			
			if(gViewIo[i])
			{
				gIoLineId[i] = sMaxLineId;				
				lineCount[sMaxLineId] =0xFFFFFFFF;
				pLineSeries[sMaxLineId] = m_ChartCtrl.CreateLineSerie(0, 0);	
				pLineSeries[sMaxLineId]->SetWidth(2);
				pLineSeries[sMaxLineId]->SetPenStyle(0);
				pSeries = pLineSeries[sMaxLineId];
				pSeries->SetName(gIoTextTable[i]);
				if(gIoColor[i] == 0x1A)
					pSeries->SetColor(color[sMaxLineId%DEFAULT_COLOR_NUM]);
				else
					pSeries->SetColor(gIoColor[i]);

				sMaxLineId++;
			}
			
			sIoMaxCount = i+1;
		}
		else
		{
			gIoTextTable[i][0] = 0;
			break;
		}
	}

	for(sTempMaxCount=0,sSkipTempCount=0,i=0; i<MAX_SEN_IO_NUM; i++)
	{
		sprintf(szItem, "%d", i);
		len = GetPrivateProfileString("1073741824", szItem, "", szTemp, sizeof(szTemp), theApp.szIniFileName);
		if(len > 1)
		{
			strcpy(gTempTextTable[i], szTemp);
			//Get gIoColor
			sprintf(szItem, "Color%d", i);	
			GetPrivateProfileString("1073741824", szItem, "1A", szTemp, sizeof(szTemp), theApp.szIniFileName);
			gTempColor[i] = (DWORD)strtol(szTemp, NULL, 16);

			sprintf(szItem, "View%d", i);
			gViewTemp[i] = GetPrivateProfileInt("1073741824", szItem, 0, theApp.szIniFileName);			
			if(gViewTemp[i])
			{
				gViewTemp[i] = 2;
				gTempLineId[i] = sMaxLineId;				
				lineCount[sMaxLineId] =0xFFFFFFFF;
				pLineSeries[sMaxLineId] = m_ChartCtrl.CreateLineSerie(0, 0);	
				pLineSeries[sMaxLineId]->SetWidth(2);
				pLineSeries[sMaxLineId]->SetPenStyle(0);
				pSeries = pLineSeries[sMaxLineId];
				pSeries->SetName(gTempTextTable[i]);
				if(gTempColor[i] == 0x1A)
					pSeries->SetColor(color[sMaxLineId%DEFAULT_COLOR_NUM]);
				else
					pSeries->SetColor(gTempColor[i]);

				sMaxLineId++;
				sSkipTempCount ++;
			}
			
			sTempMaxCount = i+1;
		}
		else
		{
			gTempTextTable[i][0] = 0;
			break;
		}
	}

	for(i=0; i<sMaxLineId-sSkipTempCount; i++)
	{
		TChartString strName;
		char temp[12];

		pSeries = pLineSeries[i];
		strName = pSeries->GetName();
		sprintf(temp, "_%d",sMaxLineId-sSkipTempCount-i-1);
		strName += temp;
		pSeries->SetName(strName);
	}
}

void CMiceUiKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOOP_COUNT, m_loop_count);
	DDX_Text(pDX, IDC_POINT_NUM, m_points);	
	DDX_Control(pDX, IDC_CHARTCTRL, m_ChartCtrl);
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
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_OPEN, &CMiceUiKeyDlg::OnBnClickedOpen)
	ON_BN_CLICKED(IDC_SAVE, &CMiceUiKeyDlg::OnBnClickedSave)
//	ON_WM_MOUSEHWHEEL()
ON_WM_MOUSEWHEEL()
ON_BN_CLICKED(IDC_SETTING, &CMiceUiKeyDlg::OnBnClickedSetting)
ON_WM_MENUSELECT()
ON_BN_CLICKED(IDC_GOTO, &CMiceUiKeyDlg::OnBnClickedGoto)
ON_BN_CLICKED(IDC_ADD, &CMiceUiKeyDlg::OnBnClickedAdd)
ON_BN_CLICKED(IDC_SUB, &CMiceUiKeyDlg::OnBnClickedSub)
END_MESSAGE_MAP()


// CMiceUiKeyDlg message handlers
class CCustomMouseListener : public CChartMouseListener
{
public:
  void OnMouseEventPlotArea(MouseEvent mouseEvent, CPoint point)
  {
    if (mouseEvent == CChartMouseListener::LButtonUp)
    {     
		char temp[512];
		double time;
		CMiceUiKeyDlg *dlgPtr = (CMiceUiKeyDlg *)theApp.m_pMainWnd;

		dlgPtr->m_Time1 = dlgPtr->m_Time2;
		dlgPtr->m_Time2 = dlgPtr->pCursor->XVal * 100;
		sprintf(temp, "%10.4lf",dlgPtr->m_Time1);		
		theApp.m_pMainWnd->SetDlgItemText(IDC_TIME1, temp);
		sprintf(temp, "%10.4lf",dlgPtr->m_Time2);		
		theApp.m_pMainWnd->SetDlgItemText(IDC_TIME2, temp);
		
		time = abs(dlgPtr->m_Time2 - dlgPtr->m_Time1);
		sprintf(temp, "%10.4lf ms = %10.4lf us",time, time*1000);
		theApp.m_pMainWnd->SetDlgItemText(IDC_STATUS, temp);
    }
	else if(mouseEvent == CChartMouseListener::MouseMove)
	{
		char temp[512];		
		CMiceUiKeyDlg *dlgPtr = (CMiceUiKeyDlg *)theApp.m_pMainWnd;
		
		sprintf(temp, "%10.4lf ms %5.1lf *C",dlgPtr->pCursor->XVal*100, dlgPtr->pCursor->YVal * 10);	
		theApp.m_pMainWnd->SetDlgItemText(IDC_FILE_NAME, temp);
	}
  }
};


CCustomMouseListener *m_pMouseListen;

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
	CChartXYSerie* pSeries = NULL;

	m_maxXrange = 60;
	CChartStandardAxis* pBottomAxis = 
		m_ChartCtrl.CreateStandardAxis(CChartCtrl::BottomAxis);
	pBottomAxis->SetMinMax(0, m_maxXrange);
	CChartStandardAxis* pLeftAxis =
		m_ChartCtrl.CreateStandardAxis(CChartCtrl::LeftAxis);
	pLeftAxis->SetMinMax(0, 20);
	//CChartStandardAxis* pTopAxis =
	//	m_ChartCtrl.CreateStandardAxis(CChartCtrl::TopAxis);
	//pTopAxis->SetMinMax(0, 10);
	//CChartStandardAxis* pRightAxis =
	//	m_ChartCtrl.CreateStandardAxis(CChartCtrl::RightAxis);
	//pRightAxis->SetMinMax(0, 10);

	CChartAxis* pAxis = m_ChartCtrl.GetBottomAxis();	
	pAxis->EnableScrollBar(TRUE);
	//pAxis->SetAutoHideScrollBar(FALSE);
	pAxis->GetLabel()->SetText("Time unit: 100 ms");

	m_ChartCtrl.GetLeftAxis()->EnableScrollBar(TRUE);
	//m_ChartCtrl.GetLeftAxis()->SetAutoHideScrollBar(FALSE);
	pCursor = m_ChartCtrl.CreateCrossHairCursor(0,0);
	// Creates a dragline cursor associated with the bottom axis.
	pDragLine =
	  m_ChartCtrl.CreateDragLineCursor(CChartCtrl::BottomAxis);
	// Hides the mouse when it is over the plotting area.
	m_ChartCtrl.ShowMouseCursor(false);
	//m_ChartCtrl.SetBackColor(0x000000);	
	m_ChartCtrl.GetLegend()->SetVisible(true);
	m_pMouseListen = new CCustomMouseListener();
	m_ChartCtrl.RegisterMouseListener(m_pMouseListen);	

	GetSensorIoInfo();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_FileName[0] = 0;
	//ReOpenDevice();
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	//if(m_hUSB == NULL)
	//{
	//	GetDlgItem(IDC_PLAY)->EnableWindow(FALSE);
	//	GetDlgItem(IDC_START)->EnableWindow(FALSE);
	//}
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
	delete m_pMouseListen;
}

void CMiceUiKeyDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here	
	OnCancel();	
}

void CMiceUiKeyDlg::OnBnClickedStart()
{
	// TODO: Add your control notification handler code here	
	Msg eventData={0}; 

	if(m_hUSB == NULL)
		ReOpenDevice();

	if(m_hUSB != NULL)
	{
		sDlgWnd = m_hWnd;
		GetDlgItem(IDC_PLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_OPEN)->EnableWindow(FALSE);
		GetDlgItem(IDC_START)->EnableWindow(FALSE);
		GetDlgItem(IDC_STOP)->EnableWindow(TRUE);
		GetSensorIoInfo();
		eventData.msg1 = 1;
		eventData.msg4.ullval = GetTickCount();	
		//m_ChartCtrl.GetBottomAxis()->SetAutoHideScrollBar(FALSE);
		TSmsgSend(sButtonMailBox, &eventData);
	}
	else
	{
		SetDlgItemText(IDC_STATUS, "Can't link Device");
	}
}

void CMiceUiKeyDlg::OnBnClickedStop()
{
	// TODO: Add your control notification handler code here	
	Msg eventData={0}; 
	
	eventData.msg1 = 0;
	eventData.msg4.ullval = GetTickCount();
		
	TSmsgSend(sButtonMailBox, &eventData);
	GetDlgItem(IDC_PLAY)->EnableWindow(TRUE);
	GetDlgItem(IDC_OPEN)->EnableWindow(TRUE);
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHARTCTRL)->SetFocus();
	//m_ChartCtrl.GetBottomAxis()->SetAutoHideScrollBar(TRUE);
}

void CMiceUiKeyDlg::OnBnClickedPlay()
{
	// TODO: Add your control notification handler code here
	char *pFileName;

	if(m_hUSB == NULL)
		ReOpenDevice();

	if(m_hUSB != NULL)
	{
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
	else
	{
		SetDlgItemText(IDC_STATUS, "Can't link Device");
	}
}


void CMiceUiKeyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnTimer(nIDEvent);
}

void CMiceUiKeyDlg::OpenFileName(char *fileName)
{
	// TODO: Add your control notification handler code here
	char *pFileName;
	HANDLE	hFile = INVALID_HANDLE_VALUE; 
	DWORD	dwFileSize, rDWORD, lastTime, startTime;	
	char	*fileData =NULL, szTemp[1024];
	BOOL	retBool, ret = TRUE;	
	char	seps[]   = " \r\n";	
	char    name[MAX_SEN_IO_TEXT_LEN], match, lineId, view;
	int     offset, i, recordState[MAX_SEN_IO_NUM], recordCount;
	char	*token;		
	DWORD	tokenNum = 0, data[8];
	CChartXYSerie* pSeries = NULL;
	double xPos, yPos,yLastPos;
	Uint64	allTime = 0, lastAllTime;

	if(fileName == NULL)
	{
		pFileName = GetFileOpenName("Text File (*.txt)\0*.txt\0");
		if(pFileName == NULL)
			goto RETURN;
	}
	else
	{
		pFileName = fileName;
	}
	hFile = CreateFile(pFileName,           // open MYFILE.TXT 
				GENERIC_READ,              // open for reading 
				FILE_SHARE_READ,           // share for reading 
				NULL,                      // no security 
				OPEN_EXISTING,             // existing file only 
				FILE_ATTRIBUTE_NORMAL,     // normal file 
				NULL);                     // no attr. template 
 	
	if (hFile == INVALID_HANDLE_VALUE)
		goto RETURN;
	
	dwFileSize = GetFileSize (hFile, NULL) ; 	
	fileData = new char[dwFileSize+128];
	fileData[dwFileSize] = 0;
	retBool = ReadFile( hFile, fileData, dwFileSize, &rDWORD, NULL );
	CloseHandle(hFile);
	if(retBool == FALSE || dwFileSize != rDWORD)
	{		
		goto RETURN;
	}
	strcpy(szTemp, pFileName);
	int len = strlen(szTemp);
	szTemp[len-3] = 'c';
	szTemp[len-2] = 's';
	szTemp[len-1] = 'v';
	hFile = CreateFile(szTemp,           
				GENERIC_WRITE,              
				FILE_SHARE_WRITE ,          
				NULL,                      
				CREATE_ALWAYS,             
				FILE_ATTRIBUTE_NORMAL,     // normal file 
				NULL);  

	GetSensorIoInfo();

	for(i=0; i<sMaxLineId; i++)
		recordState[i] = 0xFFFF;
	recordCount = 0;

	sprintf(szTemp, "%s","time(ms)");
	WriteFile(hFile, szTemp, strlen(szTemp), &rDWORD, NULL);
	for(i=0; i<sMaxLineId; i++)
	{
		TChartString strName;
		
		pSeries = pLineSeries[i];
		strName = pSeries->GetName();
		sprintf(szTemp, ",%s",strName.c_str());
		WriteFile(hFile, szTemp, strlen(szTemp), &rDWORD, NULL);		
	}	
	WriteFile(hFile, "\r\n", 2, &rDWORD, NULL);

	if(fileName == NULL)
		strcpy(m_FileName, pFileName);
	token = strtok( fileData, seps );
	while( token != NULL )
	{					
		if(tokenNum == 0)
			strcpy(name, token);
		else
			data[tokenNum-1] = strtoul(token, NULL, 10);//atoi(token);
		tokenNum ++;
		
		if(tokenNum == 5)
		{
			match = 0x10;
			view = 0;
			if(strcmp(gSensorTextTable[data[0]], name) ==0)
			{
				lineId = gSensorLineId[data[0]];
				offset = sMaxLineId-lineId-sSkipTempCount-1;
				view = gViewSensor[data[0]];
				pSeries = pLineSeries[lineId];
				match = 0;
			}
			else if(strcmp(gIoTextTable[data[0]], name) ==0)
			{
				lineId = gIoLineId[data[0]];
				offset = sMaxLineId-lineId-sSkipTempCount-1;
				view = gViewIo[data[0]];
				pSeries = pLineSeries[lineId];
				match = 1;
			}
			else if(strcmp(gTempTextTable[data[0]], name) ==0)
			{
				lineId = gTempLineId[data[0]];
				offset = data[1];
				view = gViewTemp[data[0]];
				pSeries = pLineSeries[lineId];
				match = 2;
			}
			if(match <= 2)
			{				
				if(recordState[lineId] == 0xFFFF && view)
					recordCount ++;

				if(sInitTime == 0)
				{					
					lastTime = startTime = data[2];
					sInitTime = 1;
				}
				//time = GetMsTimeFromStart(data[2], startTime);				
				allTime += GetMsTimeFromStart(data[2], lastTime);
				if(view == 1)
				{
					//xPos = (double)time/100000;
					xPos = (double)allTime/100000;
					if(data[1])
					{
						yPos = (double)offset + 0.9;
						recordState[lineId] = offset*10 + 9;
					}
					else
					{
						yPos = (double)offset + 0.1;
						recordState[lineId] = offset*10 + 1;
					}
					if(lineCount[lineId] == 0xFFFFFFFF)
					{						
						pSeries->AddPoint(xPos, yPos);
						lineCount[lineId] = 0;						
					}
					else
					{						
						yLastPos = pLineSeries[lineId]->GetYPointValue(lineCount[lineId]);
						pSeries->AddPoint(xPos,yLastPos);
						pSeries->AddPoint(xPos,yPos);
						lineCount[lineId] += 2;	
					}									
				}
				else if(view == 2)
				{
					xPos = (double)allTime/100000;					
					yPos = (double)offset/10;
					recordState[lineId] = offset;

					if(lineCount[lineId] == 0xFFFFFFFF)
					{						
						pSeries->AddPoint(xPos, yPos);
						lineCount[lineId] = 0;						
					}
					else
					{												
						pSeries->AddPoint(xPos,yPos);
						lineCount[lineId] ++;	
					}	
				}
				lastTime = data[2];						
			}
			
			if(view && allTime != lastAllTime && recordCount == sMaxLineId)
			{
				sprintf(szTemp, "%-10.4lf", (double)allTime/1000);
				WriteFile(hFile, szTemp, strlen(szTemp), &rDWORD, NULL);	
				for(i=0; i<sMaxLineId; i++)
				{
					sprintf(szTemp, ",%u", recordState[i]);
					WriteFile(hFile, szTemp, strlen(szTemp), &rDWORD, NULL);	
				}
				WriteFile(hFile, "\r\n", 2, &rDWORD, NULL);
			}
			lastAllTime = allTime;
			tokenNum = 0;
		}
		/* Get next token: */
		token = strtok( NULL, seps );
	}
	RETURN:
	
	if(sInitTime)
	{
		char i;
		char temp[128];

		for(i=0; i<sMaxLineId; i++)
		{
			//time = GetMsTimeFromStart(lastTime, startTime);
			//xPos = (double)time/100000;
			xPos = (double)allTime/100000;
			if(lineCount[i] != 0xFFFFFFFF)
			{
				pSeries = pLineSeries[i];
				yLastPos = pLineSeries[i]->GetYPointValue(lineCount[i]);
				pSeries->AddPoint(xPos,yLastPos);
				lineCount[i] ++;
			}
		}		

		sSelectLine = sMaxLineId-1;
		sprintf(temp, "Line%d: %d", 0, lineCount[sSelectLine]/2);
		SetDlgItemText(IDC_MAX_POINTS, temp);	

		sprintf(temp, "%s : end time=%8.2lf ms %8.1lf unit", pFileName, (double)allTime/1000, (double)allTime/100000);
		SetDlgItemText(IDC_FILE_NAME, temp);		
	}

	if(fileData != NULL)
		delete(fileData);
	if(hFile !=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return;
}

void CMiceUiKeyDlg::OnBnClickedOpen()
{
	OpenFileName(NULL);
}

void CMiceUiKeyDlg::OnBnClickedSave()
{
	// TODO: Add your control notification handler code here
	char *pFileName;
	
	pFileName = GetFileSaveName("*.bmp");
	if(pFileName == NULL)
		return;

	m_ChartCtrl.SaveAsImage(pFileName,NULL, 24);
	SetDlgItemText(IDC_FILE_NAME, pFileName);
}

BOOL CMiceUiKeyDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	CChartAxis* pAxis = m_ChartCtrl.GetBottomAxis();	
	//SB_LINERIGHT, SB_LINELEFT, SB_ENDSCROLL

	if(zDelta <0)
		pAxis->m_pScrollBar->OnHScroll(SB_LINERIGHT,0);
	else
		pAxis->m_pScrollBar->OnHScroll(SB_LINELEFT,0);

	pAxis->m_pScrollBar->OnHScroll(SB_ENDSCROLL,0);

	m_ChartCtrl.RefreshCtrl();
	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

void CMiceUiKeyDlg::OnBnClickedSetting()
{
	// TODO: Add your control notification handler code here
	SelectLine dlg;
	int i;
	char szItem[12], szVale[8];

	int nResponse = dlg.DoModal();
	if(nResponse == IDOK)
	{
		for(i=0; i<MAX_SEN_IO_NUM; i++)
		{
			if(gSensorTextTable[i][0])
			{
				sprintf(szItem, "View%d", i);
				sprintf(szVale, "%d", gViewSensor[i]);
				WritePrivateProfileString("268435456", szItem, szVale, theApp.szIniFileName);	
			}
			else
			{
				break;
			}
		}

		for(i=0; i<MAX_SEN_IO_NUM; i++)
		{
			if(gIoTextTable[i][0])
			{
				sprintf(szItem, "View%d", i);
				sprintf(szVale, "%d", gViewIo[i]);
				WritePrivateProfileString("536870912", szItem, szVale, theApp.szIniFileName);	
			}
			else
			{
				break;
			}
		}

		for(i=0; i<MAX_SEN_IO_NUM; i++)
		{
			if(gTempTextTable[i][0])
			{
				sprintf(szItem, "View%d", i);
				sprintf(szVale, "%d", gViewTemp[i]);
				WritePrivateProfileString("1073741824", szItem, szVale, theApp.szIniFileName);	
			}
			else
			{
				break;
			}
		}

		if(m_FileName[0])
			OpenFileName(m_FileName);
	}
}

void CMiceUiKeyDlg::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
	CDialog::OnMenuSelect(nItemID, nFlags, hSysMenu);
	
	// TODO: Add your message handler code here
	if(nItemID >= ID_LINE_0 && nItemID <= ID_LINE39)
	{
		int num = nItemID - ID_LINE_0;
		if(num < sMaxLineId)
		{
			char szTemp[32];
			
			sSelectLine = sMaxLineId-num-1;
			sprintf(szTemp, "Line%d: %d", num, lineCount[sSelectLine]/2);
			//sprintf(szTemp, "%d %x", sSelectLine, nFlags);
			SetDlgItemText(IDC_MAX_POINTS, szTemp);			
		}
	}
}

void CMiceUiKeyDlg::OnBnClickedGoto()
{
	// TODO: Add your control notification handler code here
	char szTemp[32];
	int thumbPos;

	UpdateData(true);
	CChartAxis* pAxis = m_ChartCtrl.GetBottomAxis();

	
	if(m_points > (lineCount[sSelectLine]/2))
	{
		m_points = lineCount[sSelectLine]/2;
		sprintf(szTemp, "%d", m_points);
		SetDlgItemText(IDC_POINT_NUM, szTemp);	
	}

	int num = m_points * 2;
	double xPos;
	xPos = pLineSeries[sSelectLine]->GetXPointValue(num);
	thumbPos = (int)(xPos/(m_maxXrange/10));
	pAxis->m_pScrollBar->OnHScroll(SB_THUMBPOSITION, thumbPos);
	m_ChartCtrl.RefreshCtrl();
}

void CMiceUiKeyDlg::OnBnClickedAdd()
{
	// TODO: Add your control notification handler code here
	CChartAxis* pAxis = m_ChartCtrl.GetBottomAxis();
	if(m_maxXrange >= 24)
		m_maxXrange /= 2;

	pAxis->SetMinMax(0, m_maxXrange);
}

void CMiceUiKeyDlg::OnBnClickedSub()
{
	// TODO: Add your control notification handler code here
	CChartAxis* pAxis = m_ChartCtrl.GetBottomAxis();
	if(m_maxXrange < 60000)
		m_maxXrange *= 2;
	
	pAxis->SetMinMax(0, m_maxXrange);	
}
