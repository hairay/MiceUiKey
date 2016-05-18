
// MiceUiKeyDlg.h : header file
//

#pragma once


// CMiceUiKeyDlg dialog
class CMiceUiKeyDlg : public CDialog
{
// Construction
public:
	CMiceUiKeyDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL ReOpenDevice();

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
};
