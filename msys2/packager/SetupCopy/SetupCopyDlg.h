
// SetupCopyDlg.h : header file
//

#pragma once


// CSetupCopyDlg dialog
class CSetupCopyDlg : public CDialog
{
// Construction
public:
	CSetupCopyDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETUPCOPY_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	CString source_directory1;	// This folder holds source_directory2
	CString source_directory2;	// This folder contains our actual files
	CString path_to_original_executable; // Path to cobc.exe when we launched

	void CheckEnvironment(const CString &destination);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	};
