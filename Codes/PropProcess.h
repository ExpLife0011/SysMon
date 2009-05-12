#if !defined(AFX_PROPPROCESS_H__5A1D8958_018A_4750_9144_E9A091678282__INCLUDED_)
#define AFX_PROPPROCESS_H__5A1D8958_018A_4750_9144_E9A091678282__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropProcess.h : header file
//

//////////////////////////////////////////////////////
#define TITLE_SIZE      64
#define PROCESS_SIZE    MAX_PATH
#define MAX_TASKS       256

typedef struct _PROC_LIST {
    DWORD       dwProcessId; //���� ID
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HWND        hwnd; // ���ھ��
    CHAR        ProcessName[PROCESS_SIZE]; //������
	CHAR        ProcessPath[MAX_PATH]; //����·��
    CHAR        WindowTitle[TITLE_SIZE]; //�������
	CHAR        ProcessPriority; //���ȼ�
	int         nThreads; // �߳���
	BOOL        IsSet;
} PROC_LIST, *PPROC_LIST;

typedef struct _PROC_LIST_ENUM {
    PPROC_LIST  tlist;
    DWORD       numtasks;
} PROC_LIST_ENUM, *PPROC_LIST_ENUM;

//
// Function pointer types for accessing platform-specific functions
//
typedef DWORD (*LPGetTaskList)(PPROC_LIST, DWORD);
typedef BOOL  (*LPEnableDebugPriv)(VOID);

/////////////////////////////////////////////////////////////////////////////
// CPropProcess dialog

// �����б�����ҳ����
class CPropProcess : public CPropertyPage
{
	DECLARE_DYNCREATE(CPropProcess)

// Construction
public:
	CPropProcess();
	~CPropProcess();

// Dialog Data
	//{{AFX_DATA(CPropProcess)
	enum { IDD = IDD_PROPPAGE_PROCESS };
	CListCtrl	m_procList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPropProcess)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	DWORD GetSysID(void);

	friend class CSysMonDlg;

protected:
	// Generated message map functions
	//{{AFX_MSG(CPropProcess)
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnProcTerminate();
	afx_msg void OnProcRefresh();
	afx_msg void OnProcSendmsg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

///////////////////////////////////////////////////////
// Process Access Common Function
public:

protected:
	BOOL RefreshProcList();
	PROC_LIST curTaskList[MAX_TASKS];
	int curTaskPos;
	BOOL Terminator(DWORD pidToKill);
	BOOL EnableDebugPriv(VOID);
	BOOL GetTaskList(PPROC_LIST pTaskList, DWORD dwNumTasks);

//
// Function prototypes
//
BOOL KillProcess(PPROC_LIST pTask, BOOL fForce);
VOID GetWindowTitles(PPROC_LIST_ENUM te);
BOOL MatchPattern(PUCHAR String, PUCHAR Pattern);

private:
	BOOL ForceKill;
	DWORD GetTaskList95(PPROC_LIST pTaskList, DWORD dwNumTasks);
	DWORD GetTaskListNT(PPROC_LIST pTask, DWORD dwNumTasks);
	BOOL EnableDebugPrivNT(VOID);
	BOOL EnableDebugPriv95(VOID);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPROCESS_H__5A1D8958_018A_4750_9144_E9A091678282__INCLUDED_)
