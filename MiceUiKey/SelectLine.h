#pragma once


// SelectLine dialog

class SelectLine : public CDialog
{
	DECLARE_DYNAMIC(SelectLine)

public:
	SelectLine(CWnd* pParent = NULL);   // standard constructor
	virtual ~SelectLine();

// Dialog Data
	enum { IDD = IDD_SET_LINE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
