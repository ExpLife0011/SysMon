// PropRule.cpp : implementation file
//

#include "stdafx.h"
#include "sysmon.h"
#include "PropRule.h"

#include "Selector.h"
#include "Windows.h"
#include "Winsvc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void AutoSizeColumns(CListCtrl *m_List, int col = -1);
extern CString GetLocalMachineName();

/////////////////////////////////////////////////////////////////////////////
// CPropRule property page

IMPLEMENT_DYNCREATE(CPropRule, CPropertyPage)

CPropRule::CPropRule() : CPropertyPage(CPropRule::IDD)
{
	//{{AFX_DATA_INIT(CPropRule)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropRule::~CPropRule()
{
}

void CPropRule::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropRule)
	DDX_Control(pDX, ID_APPLY_NOW, m_applyNow);
	DDX_Control(pDX, IDC_BUTTON_ADDMON, m_addMon);
	DDX_Control(pDX, IDC_BUTTON_DECMON, m_decMon);
	DDX_Control(pDX, IDC_RULELIST, m_ruleList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropRule, CPropertyPage)
	//{{AFX_MSG_MAP(CPropRule)
	ON_BN_CLICKED(IDC_BUTTON_ADDMON, OnButtonAddmon)
	ON_BN_CLICKED(IDC_BUTTON_DECMON, OnButtonDecmon)
	ON_BN_CLICKED(ID_APPLY_NOW, OnApplyNow)
	ON_BN_CLICKED(IDC_CHECK_RESOURCE, OnCheckResource)
	ON_BN_CLICKED(IDC_CHECK_SENDMSG, OnCheckSendmsg)
	ON_NOTIFY(NM_CLICK, IDC_RULELIST, OnClickRulelist)
	ON_NOTIFY(NM_RCLICK, IDC_RULELIST, OnRclickRulelist)
	ON_WM_TIMER()
	ON_EN_CHANGE(IDC_EDIT_PERCENT, OnChangeEditPercent)
	ON_EN_CHANGE(IDC_EDIT_MSGTO, OnChangeEditMsgto)
	ON_BN_CLICKED(IDC_CHECK_STARTMON, OnCheckStartmon)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropRule message handlers

void CPropRule::OnButtonAddmon() 
{
	// TODO: Add your control notification handler code here
	// ֹͣ��ʱ��
	BOOL bPreIsMon = bIsMon;
	if(bIsMon)
	{
		KillTimer(IDT_MONTIMER);
		bIsMon = FALSE;
	}

	// ѡ��Ҫ���ӵķ���
	CSelector select;

	if(!select.Init(&m_ruleList))
		return;

	if(select.DoModal() == IDOK)
	{
		AutoSizeColumns(&m_ruleList);
		m_applyNow.EnableWindow();
	}

	m_ruleList.SetFocus();

	// �ָ���ʱ��
	if(bPreIsMon)
	{
		IDT_MONTIMER = SetTimer(8148, 5000, NULL);
		if(!IDT_MONTIMER)
			AfxMessageBox(_T(
			"������ʱ��ʧ��!��ع����޷�������ת!\n����������������߼����.")); 
	}
}

BOOL CPropRule::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	// ��ʼ�� CListCtrl
	m_ruleList.InsertColumn(0, _T("��ʾ����"), LVCFMT_LEFT, 70);
	m_ruleList.InsertColumn(1, _T("��������"), LVCFMT_LEFT, 70);

    m_ruleList.SetExtendedStyle(m_ruleList.GetExtendedStyle()
        |LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP
		|LVS_EX_ONECLICKACTIVATE);

	AutoSizeColumns(&m_ruleList);
	m_ruleList.SetFocus();

	CButton* pButton = (CButton*)GetDlgItem(IDC_RADIO_RESTART);
	VERIFY(pButton != NULL);
	pButton->SetCheck(1);

	bIsMon = FALSE;
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPropRule::RestartSevices()
{
#ifdef _DEBUG
	AfxMessageBox("Do restart services!");
#endif
	// ������״̬
	int number = m_ruleList.GetItemCount();
	if (number < 1)
		return TRUE;

	SC_HANDLE hSCManager = OpenSCManager(GetLocalMachineName(), NULL, GENERIC_READ);
	if(hSCManager == NULL)
	{
		CString errLog;
		errLog.Format(
			"\n\n***********************\n������������ʱ, �򿪹��������ִ���!\n��������:\n#");

		WriteToLog(errLog, GetLastError());

		return FALSE;
	}

	for(int nItem = 0; nItem < number; nItem++)
	{
		BOOL bSuccess = TRUE;
		char pszServiceName[MAX_PATH + 1] = "";
		m_ruleList.GetItemText(nItem, 1, pszServiceName, MAX_PATH);

		SC_HANDLE hService = OpenService(hSCManager, pszServiceName, SERVICE_ALL_ACCESS);
		if(pszServiceName == NULL)
		{
			CString errLog;
			errLog.Format(
				"\n\n***********************\n������������ʱ, �򿪷��������ִ���!\n��������:\n#");

			WriteToLog(errLog, GetLastError());

			continue;
		}

		SERVICE_STATUS ss;
		LONG lRet = QueryServiceStatus(hService, &ss);

		if(lRet == 0)
		{
			CString errLog;
			errLog.Format(
				"\n\n***********************\n������������ʱ, ��ѯ����״̬���ִ���!\n��ǰ������: %s\n��������:\n#",
				pszServiceName);

			WriteToLog(errLog, GetLastError());

			continue;
		}

		switch(ss.dwCurrentState)
		{
			// �������ֹͣ, ��������
			case SERVICE_STOPPED:
			case SERVICE_STOP_PENDING:
				{
					if(StartService(hService, 0, NULL) == 0)
					{
						CString errLog;
						errLog.Format(
							"\n\n***********************\n��������������ʱ, �����������ִ���!\n��ǰ������: %s\n��������:\n#",
							pszServiceName);

						WriteToLog(errLog, GetLastError());
					}

					break;
				}
			// ���������ͣ, �ͻָ�
			case SERVICE_PAUSE_PENDING:
			case SERVICE_PAUSED:
				{
					if(ControlService(hService, SERVICE_CONTROL_CONTINUE, &ss))
					{
						CString errLog;
						errLog.Format(
							"\n\n***********************\n��������������ʱ, �ָ����в������ִ���!\n��ǰ������: %s\n��������:\n#",
							pszServiceName);

						WriteToLog(errLog, GetLastError());
					}
					break;
				}

			//�������������, �ͺ���
			case SERVICE_START_PENDING:
			case SERVICE_RUNNING:
			case SERVICE_CONTINUE_PENDING:
				break;
			default:
				{
					CString errLog;
					errLog.Format(
						"\n\n***********************\n��������������ʱ, �޷�ȷ������״̬!\n��ǰ������: %s\n��������:\n#",
						pszServiceName);

					WriteToLog(errLog, GetLastError());
				}
		}

		CloseServiceHandle(hService);
		hService = NULL;
	}
	CloseServiceHandle(hSCManager);
	hSCManager = NULL;

	return TRUE;
}

void CPropRule::OnButtonDecmon() 
{
	// TODO: Add your control notification handler code here
	// ֹͣ��ʱ��
	BOOL bPreIsMon = bIsMon;
	if(bIsMon)
	{
		KillTimer(IDT_MONTIMER);
		bIsMon = FALSE;
	}

	m_ruleList.SetRedraw(false);
	BOOL bSuccess = TRUE;
	// ÿ��ɾ��������, ���²�׽ѡ�����Ŀ
	while (true)
	{
		POSITION pos = m_ruleList.GetFirstSelectedItemPosition();

		if (pos == NULL)
		{
			m_decMon.EnableWindow(FALSE);
			break;
		}

		int nItem = m_ruleList.GetNextSelectedItem(pos);

		// ɾ���б���Ŀ
		if(!m_ruleList.DeleteItem(nItem))
			bSuccess = FALSE;
	}
	m_ruleList.SetRedraw(true);

	if(!bSuccess)
		AfxMessageBox(_T("������������������!"));
	m_decMon.EnableWindow(FALSE);
	m_ruleList.SetFocus();
	m_applyNow.EnableWindow();

	// �ָ���ʱ��
	if(bPreIsMon)
	{
		IDT_MONTIMER = SetTimer(8148, 5000, NULL);
		if(!IDT_MONTIMER)
			AfxMessageBox(_T(
			"������ʱ��ʧ��!��ع����޷�������ת!\n����������������߼����.")); 
	}
}

void CPropRule::OnApplyNow() 
{
	// TODO: Add your control notification handler code here
	
	m_applyNow.EnableWindow(FALSE);
}

void CPropRule::OnCheckResource() 
{
	// TODO: Add your control notification handler code here
	CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_RESOURCE);
	VERIFY(pButton != NULL);
	int bStatus = pButton->GetCheck();

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_PERCENT);
	VERIFY(pEdit != NULL);
	if(bStatus == 1)
	{
		pEdit->EnableWindow();
		pEdit->SetFocus();
	}
	else if(bStatus == 0)
	{
		pEdit->EnableWindow(FALSE);
		CButton* pButtonFocus = (CButton*)GetDlgItem(ID_APPLY_NOW);
		pButtonFocus->SetFocus();
	}
	else
		AfxMessageBox(_T("ERROR IN GET WINDOW STATUS"));

	m_applyNow.EnableWindow();
}

void CPropRule::OnCheckSendmsg() 
{
	// TODO: Add your control notification handler code here
	CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_SENDMSG);
	VERIFY(pButton != NULL);
	int bStatus = pButton->GetCheck();

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_MSGTO);
	VERIFY(pEdit != NULL);
	if(bStatus == 1)
		pEdit->EnableWindow();
	else if(bStatus == 0)
		pEdit->EnableWindow(FALSE);
	else
		AfxMessageBox(_T("ERROR IN GET WINDOW STATUS"));

	m_applyNow.EnableWindow();
}

void CPropRule::OnClickRulelist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	SetCtrlStatus();
	m_ruleList.SetFocus();

	*pResult = 0;
}

void CPropRule::OnRclickRulelist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	SetCtrlStatus();
	m_ruleList.SetFocus();

	*pResult = 0;
}

BOOL CPropRule::SetCtrlStatus()
{
	POSITION pos = m_ruleList.GetFirstSelectedItemPosition();
	if (pos)
		m_decMon.EnableWindow();
	else
		m_decMon.EnableWindow(FALSE);

	return TRUE;
}

void CPropRule::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if (nIDEvent != IDT_MONTIMER)
	{
		CPropertyPage::OnTimer(nIDEvent);
		return;
	}

	MessageBeep(0xFFFFFFFF);
	if(CheckResource() == FALSE)
	{
#ifdef _DEBUG
		if(!
#endif
			ReBoot()
#ifdef _DEBUG
			)
			AfxMessageBox("ReBoot fail!")
#endif
			;
			
		return;
	}

	if(CheckSvrStatus())
	{
		CPropertyPage::OnTimer(nIDEvent);
		return;
	}

	// Add timer handle
	//AfxMessageBox("Procced!");
	RestartSevices();

	CPropertyPage::OnTimer(nIDEvent);
}

void CPropRule::OnChangeEditPercent() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	m_applyNow.EnableWindow();
}

void CPropRule::OnChangeEditMsgto() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	m_applyNow.EnableWindow();
}

BOOL CPropRule::CheckSvrStatus()
{
	// ������״̬
	int number = m_ruleList.GetItemCount();
	if (number < 1)
		return TRUE;

	for(int nItem = 0; nItem < number; nItem++)
	{
		char pszServiceName[MAX_PATH + 1] = "";
		m_ruleList.GetItemText(nItem, 1, pszServiceName, MAX_PATH);

		SC_HANDLE hSCManager = OpenSCManager(GetLocalMachineName(), NULL, GENERIC_READ);
		SC_HANDLE hService = OpenService(hSCManager, pszServiceName, GENERIC_READ);
		SERVICE_STATUS ss;
		LONG lRet = QueryServiceStatus(hService, &ss);
		if(hService)
		{
			CloseServiceHandle(hService);
			hService = NULL;
		}
		if(hSCManager)
		{
			CloseServiceHandle(hSCManager);
			hSCManager = NULL;
		}

		if(lRet)
		{
			switch(ss.dwCurrentState)
			{
				case SERVICE_STOPPED:
				case SERVICE_STOP_PENDING:
				case SERVICE_PAUSE_PENDING:
				case SERVICE_PAUSED:
					return FALSE;
				case SERVICE_START_PENDING:
				case SERVICE_RUNNING:
				case SERVICE_CONTINUE_PENDING:
					continue;
				default:
					{
						AfxMessageBox(_T("Unknown Service status!"));
					}
			}
		}
		else
		{
			CString errLog;
			errLog.Format(
				"\n\n***********************\n�ڼ�������ӷ���״̬ʱ, ���ִ���!\n��ǰ������: %s\n��������:\n#",
				pszServiceName);

			WriteToLog(errLog, lRet);
		}

	}
	
	return TRUE;
}

BOOL CPropRule::CheckResource()
{
	// ����ڴ���Դ
	CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_RESOURCE);
	VERIFY(pButton != NULL);
	if(pButton->GetCheck() == 0)
		return TRUE;

	// �õ���Դ����
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_PERCENT);
	VERIFY(pEdit != NULL);
	CString limitedPercent;
	pEdit->GetWindowText(limitedPercent);
	if(limitedPercent == "" || limitedPercent == "00" || limitedPercent == "0")
#ifndef _DEBUG
		return TRUE;		
#endif
#ifdef _DEBUG
		limitedPercent = "100";
#endif
	// �õ������ڴ��ʹ����
	LPMEMORYSTATUS pMemStatus = new MEMORYSTATUS;
	pMemStatus->dwLength = sizeof(MEMORYSTATUS);
	::GlobalMemoryStatus(pMemStatus);

	int percent = (pMemStatus->dwAvailVirtual / 1024 ^ 2) * 100 / (pMemStatus->dwTotalVirtual / 1024 ^ 2);
	
	delete pMemStatus;
	pMemStatus = NULL;

	// �ж�
	if(percent < atoi(limitedPercent))
		return FALSE;

	return TRUE;
}

BOOL CPropRule::ReBoot()
{
	// �ı�������ȼ��Ա��������ϵͳ����
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    // ���Ž��̵Ĵ�ȡ��ʾ
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken))
	{
		CString errLog;
		errLog.Format(
			"\n\n***********************\n����������ϵͳʱ, ���ִ���!\n��������:\n#");

		WriteToLog(errLog, GetLastError());

        return FALSE;
    }

    // ��ȡ SE_SHUTDOWN_NAME ��Ȩ
    if (!LookupPrivilegeValue((LPSTR) NULL,
            SE_SHUTDOWN_NAME,
            &DebugValue))
	{
		CString errLog;
		errLog.Format(
			"\n\n***********************\n����������ϵͳʱ, ���ִ���!\n��������:\n#");

		WriteToLog(errLog, GetLastError());

        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL);

    // AdjustTokenPrivileges �ķ���ֵ���ܱ�����
    if (GetLastError() != ERROR_SUCCESS)
	{
		CString errLog;
		errLog.Format(
			"\n\n***********************\n����������ϵͳʱ, ���ִ���!\n��������:\n#");

		WriteToLog(errLog, GetLastError());

        return FALSE;
    }

	WriteToLog("\n\n***********************\n������������ϵͳ...");
	// ��������ϵͳ�ɹ����
	return ExitWindowsEx(EWX_REBOOT, (DWORD)5000);
}

void CPropRule::OnCheckStartmon() 
{
	// TODO: Add your control notification handler code here
	CButton* pButton = (CButton*)GetDlgItem(IDC_CHECK_STARTMON);
	VERIFY(pButton != NULL);

	switch(pButton->GetCheck())
	{
	case 0:
		{
			if(IDT_MONTIMER)
				KillTimer(IDT_MONTIMER);
			bIsMon = FALSE;

			return;
		}
	case 1:
		{
			IDT_MONTIMER = SetTimer(8148, 5000, NULL);
			if(!IDT_MONTIMER)
			{
				AfxMessageBox(_T(
				"������ʱ��ʧ��!��ع����޷�������ת!\n����������������߼����.")); 

				return;
			}

			bIsMon = TRUE;
			return;
		}
	default:
		{
			bIsMon = FALSE;
			AfxMessageBox(_T("�������ʧ��!")); 
		}
	}
}

BOOL CPropRule::WriteToLog(CString strMsg, LONG lErrMsg)
{
	FILE *fLog = 0;

	fLog = fopen("MonLog.txt", "a+t");
	if(!fLog)
	{
		AfxMessageBox(_T("�޷�������־�ļ�. \n�����޷���ʾ!"),
			MB_OK | MB_ICONEXCLAMATION);
	
		return FALSE;
	}

	// ȡ��ϵͳ������Ϣ
	static char buffer[256];
	char *end = buffer + 
		FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM, 0, 
		lErrMsg,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),buffer, sizeof(buffer)-1, 0);

	if(end == buffer)
		fprintf(fLog, "%s#%s", _T(strMsg), _T("�޷�ȷ��!"));

	for(end--; end >= buffer && (*end == '\r' || *end == '\n') ;end--) 
		;

    end[1] = 0;

    CString str = (CString)buffer;

	// ȡ��ϵͳʱ��
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
	CString strTime;
	strTime.Format("%d��%d�� %dʱ%d��%d��",
		sysTime.wMonth, sysTime.wDay, sysTime.wHour,
		sysTime.wMinute, sysTime.wSecond);

	fprintf(fLog, "%s#%s\n��ǰʱ��: %s", _T(strMsg), _T(str), _T(strTime));
	if(fLog)
		fclose(fLog);

	return TRUE;
}

BOOL CPropRule::WriteToLog(CString strMsg)
{
	FILE *fLog = 0;

	fLog = fopen("MonLog.txt", "a+t");
	if(!fLog)
	{
		AfxMessageBox(_T("�޷�������־�ļ�. \n�����޷���ʾ!"),
			MB_OK | MB_ICONEXCLAMATION);
	
		return FALSE;
	}

	// ȡ��ϵͳʱ��
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	CString strTime;
	strTime.Format("%d��%d�� %dʱ%d��%d��",
		sysTime.wMonth, sysTime.wDay, sysTime.wHour,
		sysTime.wMinute, sysTime.wSecond);

	fprintf(fLog, "%s\n��ǰʱ��: %s", _T(strMsg), _T(strTime));
	if(fLog)
		fclose(fLog);

	return TRUE;
}
