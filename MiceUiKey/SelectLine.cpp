// SelectLine.cpp : implementation file
//

#include "stdafx.h"
#include "MiceUiKey.h"
#include "MiceUiKeyDlg.h"
#include "SelectLine.h"


// SelectLine dialog

IMPLEMENT_DYNAMIC(SelectLine, CDialog)

SelectLine::SelectLine(CWnd* pParent /*=NULL*/)
	: CDialog(SelectLine::IDD, pParent)
{

}

SelectLine::~SelectLine()
{
}

void SelectLine::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SelectLine, CDialog)
	ON_BN_CLICKED(IDOK, &SelectLine::OnBnClickedOk)
END_MESSAGE_MAP()


// SelectLine message handlers
extern char gViewSensor[MAX_SEN_IO_NUM];
extern char gSensorTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN];
extern char gViewIo[MAX_SEN_IO_NUM];
extern char gIoTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN];
extern char gViewTemp[MAX_SEN_IO_NUM];
extern char gTempTextTable[MAX_SEN_IO_NUM][MAX_SEN_IO_TEXT_LEN];

BOOL SelectLine::OnInitDialog()
{
	CDialog::OnInitDialog();
	int i;
	char szShow[MAX_SEN_IO_TEXT_LEN];

	for(i=0; i<MAX_SEN_IO_NUM; i++)
	{
		if(gSensorTextTable[i][0])
		{
			wsprintf(szShow, "%d:%s", i, gSensorTextTable[i]);
			SendDlgItemMessage(IDC_INPUT_LIST, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szShow);
			if(gViewSensor[i])
				SendDlgItemMessage(IDC_INPUT_LIST, LB_SETSEL, (WPARAM)TRUE, i);
			else
				SendDlgItemMessage(IDC_INPUT_LIST, LB_SETSEL, (WPARAM)FALSE, i);
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
			wsprintf(szShow, "%d:%s", i, gIoTextTable[i]);
			SendDlgItemMessage(IDC_OUTPUT_LIST, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szShow);
			if(gViewIo[i])
				SendDlgItemMessage(IDC_OUTPUT_LIST, LB_SETSEL, (WPARAM)TRUE, i);
			else
				SendDlgItemMessage(IDC_OUTPUT_LIST, LB_SETSEL, (WPARAM)FALSE, i);
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
			wsprintf(szShow, "%d:%s", i, gTempTextTable[i]);
			SendDlgItemMessage(IDC_TEMP_LIST, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szShow);
			if(gViewTemp[i])
				SendDlgItemMessage(IDC_TEMP_LIST, LB_SETSEL, (WPARAM)TRUE, i);
			else
				SendDlgItemMessage(IDC_TEMP_LIST, LB_SETSEL, (WPARAM)FALSE, i);
		}
		else
		{
			break;
		}
	}
	return TRUE;
}
void SelectLine::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	int count, i, ret;

	count = SendDlgItemMessage(IDC_INPUT_LIST, LB_GETCOUNT, 0, 0);
	for(i=0; i<count; i++)
	{
		ret = SendDlgItemMessage(IDC_INPUT_LIST, LB_GETSEL, i, 0);
		gViewSensor[i] = (char)ret;
	}

	count = SendDlgItemMessage(IDC_OUTPUT_LIST, LB_GETCOUNT, 0, 0);
	for(i=0; i<count; i++)
	{
		ret = SendDlgItemMessage(IDC_OUTPUT_LIST, LB_GETSEL, i, 0);
		gViewIo[i] = (char)ret;
	}

	count = SendDlgItemMessage(IDC_TEMP_LIST, LB_GETCOUNT, 0, 0);
	for(i=0; i<count; i++)
	{
		ret = SendDlgItemMessage(IDC_TEMP_LIST, LB_GETSEL, i, 0);
		gViewTemp[i] = (char)ret*2;
	}

	OnOK();
}
