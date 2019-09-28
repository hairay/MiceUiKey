
// MiceUiKeyDlg.h : header file
//

#pragma once

#include "ChartCtrl.h"
#include "ColourPicker.h"
#include "ChartLineSerie.h"

#define MAX_SEN_IO_NUM			200
#define MAX_SEN_IO_TEXT_LEN		40

// CMiceUiKeyDlg dialog
class CMiceUiKeyDlg : public CDialog
{
// Construction
public:
	CMiceUiKeyDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL ReOpenDevice(void);
	void GetSensorIoInfo(void);
	void OpenFileName(char *fileName);
	CChartLineSerie *pLineSeries[MAX_SEN_IO_NUM];
	DWORD	lineCount[MAX_SEN_IO_NUM];
	//CChartLineSerie *pLineSeries2;

	CString m_loop_count;
	UINT    m_points, m_maxXrange;
	CChartCtrl m_ChartCtrl;
	CChartCrossHairCursor *pCursor;
	CChartDragLineCursor* pDragLine;
	double  m_Time1, m_Time2;	
	char m_FileName[2048];

// Dialog Data
	enum { IDD = IDD_MICEUIKEY_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedPlay();		
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOpen();
	afx_msg void OnBnClickedSave();
//	afx_msg BOOL OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnBnClickedSetting();
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnBnClickedGoto();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedSub();
};
