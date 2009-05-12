// PropService.cpp : implementation file
//

#include "stdafx.h"
#include "SysMon.h"
#include "PropService.h"

#include "Windows.h"
#include "Winreg.h"
#include "Winsvc.h"
#include "Winbase.h"
#include "Winerror.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void AutoSizeColumns(CListCtrl *m_List, int col = -1);

/////////////////////////////////////////////////////////////////////////////
// CPropService property page

IMPLEMENT_DYNCREATE(CPropService, CPropertyPage)

CPropService::CPropService() : CPropertyPage(CPropService::IDD)
{
	//{{AFX_DATA_INIT(CPropService)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropService::~CPropService()
{
}

void CPropService::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropService)
	DDX_Control(pDX, IDC_SVRLIST, m_svrList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropService, CPropertyPage)
	//{{AFX_MSG_MAP(CPropService)
	ON_BN_CLICKED(IDC_TYPEDRIVERS, OnTypeDrivers)
	ON_BN_CLICKED(IDC_TYPESERVICES, OnTypeServices)
	ON_BN_CLICKED(IDC_VIEWALL, OnViewAll)
	ON_BN_CLICKED(IDC_VIEWSTOPED, OnViewStoped)
	ON_BN_CLICKED(IDC_VIEWRUNNING, OnViewRunning)
	ON_BN_CLICKED(IDC_CREATESVR, OnCreateSvr)
	ON_BN_CLICKED(IDC_VIEWLOG, OnViewLog)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_SVR_LOAD, OnSvrLoad)
	ON_COMMAND(IDM_SVR_STOP, OnSvrStop)
	ON_COMMAND(IDM_SVR_RESUME, OnSvrResume)
	ON_COMMAND(IDM_SVR_PAUSE, OnSvrPause)
	ON_COMMAND(IDM_SVR_REFRESH, OnSvrRefresh)
	ON_UPDATE_COMMAND_UI(IDM_SVR_LOAD, OnUpdateSvrLoad)
	ON_UPDATE_COMMAND_UI(IDM_SVR_PAUSE, OnUpdateSvrPause)
	ON_UPDATE_COMMAND_UI(IDM_SVR_RESUME, OnUpdateSvrResume)
	ON_UPDATE_COMMAND_UI(IDM_SVR_STOP, OnUpdateSvrStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropService message handlers

BOOL CPropService::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_strMachineName = GetLocalMachineName();

	// ��ʼ�� CListCtrl
	m_svrList.InsertColumn(0, _T("���"), LVCFMT_LEFT, 70);
	m_svrList.InsertColumn(1, _T("��ʾ����"), LVCFMT_LEFT, 70);
	m_svrList.InsertColumn(2, _T("��������"), LVCFMT_LEFT, 70);
	m_svrList.InsertColumn(3, _T("��������"), LVCFMT_LEFT, 70);
	m_svrList.InsertColumn(4, _T("�������"), LVCFMT_LEFT, 70);

	m_svrList.InsertColumn(5, _T("��ǰ״̬"), LVCFMT_LEFT, 50);
	m_svrList.InsertColumn(6, _T("������Ⱥ"), LVCFMT_LEFT, 50);
	m_svrList.InsertColumn(7, _T("�����û�"), LVCFMT_LEFT, 50);
	m_svrList.InsertColumn(8, _T("�����ϵ"), LVCFMT_LEFT, 50);
	m_svrList.InsertColumn(9, _T("�ļ�·��"), LVCFMT_LEFT, 150);
/*// For Future
	m_svrList.InsertColumn(10, _T("����"), LVCFMT_LEFT, 70);
//*/
    m_svrList.SetExtendedStyle(m_svrList.GetExtendedStyle()
        |LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP
		|LVS_EX_ONECLICKACTIVATE);

	m_svrList.SetFocus();	// ���Ժ������� FALSE

	// ˢ���б�
	((CButton*)GetDlgItem(IDC_TYPESERVICES))->SetCheck(1);
	((CButton*)GetDlgItem(IDC_VIEWRUNNING))->SetCheck(1);
	m_dwType = SERVICE_WIN32;
	m_dwStatus = SERVICE_ACTIVE;

	GetSvrList();
	AutoSizeColumns(&m_svrList);

	return FALSE;  // return TRUE  unless you set the focus to a control
}

CString CPropService::GetStartupString(DWORD dwStartupType)
{
	CString strStartup;

	switch(dwStartupType)
	{
		case SERVICE_BOOT_START:
			strStartup = _T("Boot");
			break;
		case SERVICE_SYSTEM_START:
			strStartup = _T("System");
			break;
		case SERVICE_AUTO_START:
			strStartup = _T("Automatic");
			break;
		case SERVICE_DEMAND_START:
			strStartup = _T("Manual");
			break;
		case SERVICE_DISABLED:
			strStartup = _T("Disabled");
			break;
		default:
			strStartup = _T("Unknown");
			break;
	}

	return strStartup;
}

CString CPropService::GetStatusString(DWORD dwServiceStatus)
{
	CString strStatus;

	switch(dwServiceStatus)
	{
		case SERVICE_STOPPED:
			strStatus = _T("Stopped");
			break;
		case SERVICE_START_PENDING:
			strStatus = _T("Starting");
			break;
		case SERVICE_STOP_PENDING:
			strStatus = _T("Stopping");
			break;
		case SERVICE_RUNNING:
			strStatus = _T("Running");
			break;
		case SERVICE_CONTINUE_PENDING:
			strStatus = _T("Continuing");
			break;
		case SERVICE_PAUSE_PENDING:
			strStatus = _T("Pausing");
			break;
		case SERVICE_PAUSED:
			strStatus = _T("Paused");
			break;
		default:
			strStatus = _T("Unknown");
			break;
	}

	return strStatus;
}

BOOL CPropService::GetSvrList()
{
	HKEY hRegServicesKey = 0;
	LONG lRet = 0L;

	lRet = RegOpenKey(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services",
		&hRegServicesKey);

	if(lRet == NO_ERROR)
	{
		DWORD dwSubKeys = 0, dwMaxSubKeyNameLen = 0;
		lRet = RegQueryInfoKey(hRegServicesKey, NULL, NULL, NULL, &dwSubKeys, &dwMaxSubKeyNameLen, 
			NULL, NULL, NULL, NULL, NULL, NULL);

		if(lRet)
		{
			WriteToLog(
				"\n****************\n��ѯע�����ʧ��!\nHKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services \n������Ϣ:\n",
				lRet);

			return FALSE;
		}

		SC_HANDLE hSCManager = OpenSCManager(m_strMachineName, NULL, GENERIC_READ);

		if(hSCManager == NULL)
		{
			WriteToLog(
				"\n****************\n�򿪷��������ʧ��!\nHKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services \n������Ϣ:\n",
				GetLastError());

			return FALSE;
		}

		VERIFY(dwMaxSubKeyNameLen <= MAX_PATH + 1);
		m_svrList.SetRedraw(false);
		for (DWORD dwIndex = 0;	dwIndex < dwSubKeys; dwIndex++)
		{
			char pszServiceName[MAX_PATH + 1] = "";
			char pszStartup[128] = "";
			char pszStatus[128] = "";
			DWORD dwServiceNameLen = MAX_PATH + 1;

			//	RegQueryInfoKey
			lRet = RegEnumKey(hRegServicesKey, dwIndex, pszServiceName, dwServiceNameLen);

			if(lRet == ERROR_NO_MORE_ITEMS)
				break;

			if(lRet != NO_ERROR)
			{
				WriteToLog(
					"\n****************\nö��ע�����ʧ��!\nHKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services \nindex = δ֪\n������Ϣ:\n",
					GetLastError());

				continue;
			}

			SC_HANDLE hService = OpenService(hSCManager, pszServiceName, GENERIC_READ);
			if(hService == NULL)
			{
				CString errMsg;
				errMsg.Format("\n\nIndex = %03d\nServiceName = %s\n", dwIndex, pszServiceName);
				errMsg += "****************\n�򿪷������!\n������Ϣ:\n";
				WriteToLog(errMsg, GetLastError());

				continue;
			}

			DWORD cbBytesNeeded;
			LPQUERY_SERVICE_CONFIG pqsc;
			// pass a false size to QueryServiceConfig
			QueryServiceConfig(hService, NULL, 0, &cbBytesNeeded);
			pqsc = (LPQUERY_SERVICE_CONFIG)malloc(cbBytesNeeded);
			if(QueryServiceConfig(hService, pqsc, cbBytesNeeded, &cbBytesNeeded) == 0)
			{
				free(pqsc);
				CloseServiceHandle(hService);
				hService = 0;

				WriteToLog(
					"\n****************\n��ѯ�������ô���!\n������Ϣ:\n",
					GetLastError());

				continue;
			}

			strcpy(pszStartup, GetStartupString(pqsc->dwStartType));

			if(!(pqsc->dwServiceType & m_dwType))
			{
				free(pqsc);
				CloseServiceHandle(hService);
				hService = 0;

				continue;
			}
			
			// ȡ����ǰ״̬
			SERVICE_STATUS ss;
			if(QueryServiceStatus(hService, &ss))
				strcpy(pszStatus, GetStatusString(ss.dwCurrentState));
			else
			{
				strcpy(pszStatus, _T("δ֪"));
				WriteToLog(
					"\n****************\nȡ����ǰ״̬ʧ��!\n������Ϣ:\n",
					GetLastError());
			}

			if(((m_dwStatus == SERVICE_ACTIVE) && (ss.dwCurrentState == SERVICE_STOPPED))	||
			   ((m_dwStatus == SERVICE_INACTIVE) && (ss.dwCurrentState != SERVICE_STOPPED)))
			{
				free(pqsc);
				CloseServiceHandle(hService);
				hService = 0;

				continue;
			}
/*//
			// ȡ������Ϣ
			SERVICE_DESCRIPTION* pSD;
			DWORD dwSDLen;
			// pass it a fail length
			QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION,
				NULL, 0, &dwSDLen);

			if(QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION,
				pSD, dwSDLen, &dwSDLen))
			{
				strSD = pSD->lpDescription;
			}
			else
			{
				WriteToLog(
					"\n****************\nȡ������������!\n������Ϣ:\n",
					GetLastError());
			}
//*/
			InsertInList(pqsc->lpDisplayName, pszServiceName, m_dwType, pszStartup,
				 pszStatus, pqsc->lpLoadOrderGroup, pqsc->lpServiceStartName,
				 pqsc->lpDependencies, pqsc->lpBinaryPathName/*, strSD */);

			free(pqsc);
			CloseServiceHandle(hService);
			hService = 0;
		}
		m_svrList.SetRedraw(true);

		CloseServiceHandle(hSCManager);
	}
	else
	{
		WriteToLog(
			"\n****************\n��ע�����ʧ��!\nHKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services \n������Ϣ:\n",
			lRet);

		return FALSE;
	}
	
	lRet = RegCloseKey(hRegServicesKey);
	if(lRet)
	{
		WriteToLog("\n****************\n�ر�ע�����ʧ��!\nHKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services \n������Ϣ:\n",
			lRet);
	}

	return TRUE;
}

BOOL CPropService::InsertInList(LPCTSTR lpSvrName, LPCTSTR lpDisplayName,
								DWORD dwServiceType, LPCTSTR lpStartType,
								LPCTSTR lpCurrentState, LPTSTR lpLoadOrderGroup,
								LPTSTR lpServiceStartName, LPTSTR lpDependencies,
								LPTSTR lpBinaryPathName/*, CString strDescription */)
{
	int nCount = m_svrList.GetItemCount();
	int iInsPos = m_svrList.InsertItem(nCount, "");
	CString strTemp;

	strTemp.Format("%03d", nCount);
	m_svrList.SetItemText(iInsPos, 0, _T(strTemp));
	m_svrList.SetItemText(iInsPos, 1, _T(lpSvrName));
	m_svrList.SetItemText(iInsPos, 2, _T(lpDisplayName));

	if(dwServiceType == SERVICE_WIN32)
		strTemp = "Service";
	else
		strTemp = "Driver";

	m_svrList.SetItemText(iInsPos, 3, _T(strTemp));
	m_svrList.SetItemText(iInsPos, 4, _T(lpStartType));
	m_svrList.SetItemText(iInsPos, 5, _T(lpCurrentState));
	m_svrList.SetItemText(iInsPos, 6, _T(lpLoadOrderGroup));
	m_svrList.SetItemText(iInsPos, 7, _T(lpServiceStartName));
	m_svrList.SetItemText(iInsPos, 8, _T(lpDependencies));
	m_svrList.SetItemText(iInsPos, 9, _T(lpBinaryPathName));
//	m_svrList.SetItemText(iInsPos, 10, _T(strDescription));

	return TRUE;
}

void CPropService::OnTypeDrivers()
{
	// TODO: Add your control notification handler code here
	SetType(0, 1);
}

void CPropService::OnTypeServices() 
{
	// TODO: Add your control notification handler code here
	SetType(1, 0);
}

void CPropService::OnViewAll() 
{
	// TODO: Add your control notification handler code here
	SetState(0, 0, 1);
}

void CPropService::OnViewStoped() 
{
	// TODO: Add your control notification handler code here
	SetState(0, 1, 0);
}

void CPropService::OnViewRunning() 
{
	// TODO: Add your control notification handler code here
	SetState(1, 0, 0);
}

void CPropService::SetState(int nActive, int nInactive, int nAll)
{
	((CButton*)GetDlgItem(IDC_VIEWRUNNING))->SetCheck(nActive);	
	((CButton*)GetDlgItem(IDC_VIEWSTOPED))->SetCheck(nInactive);
	((CButton*)GetDlgItem(IDC_VIEWALL))->SetCheck(nAll);

	if(((CButton*)GetDlgItem(IDC_VIEWRUNNING))->GetCheck())
		m_dwStatus = SERVICE_ACTIVE;
	else if(((CButton*)GetDlgItem(IDC_VIEWSTOPED))->GetCheck())
		m_dwStatus = SERVICE_INACTIVE;
	else
		m_dwStatus = SERVICE_ACTIVE | SERVICE_INACTIVE;

	// ˢ���б�
	m_svrList.DeleteAllItems();
	GetSvrList();
	AutoSizeColumns(&m_svrList);
}

void CPropService::SetType(int nServices, int nDrivers)
{
	((CButton*)GetDlgItem(IDC_TYPESERVICES))->SetCheck(nServices);
	((CButton*)GetDlgItem(IDC_TYPEDRIVERS))->SetCheck(nDrivers);

	if(((CButton*)GetDlgItem(IDC_TYPESERVICES))->GetCheck())
		m_dwType = SERVICE_WIN32;
	else
		m_dwType = SERVICE_DRIVER;

	// ˢ���б�
	m_svrList.DeleteAllItems();
	GetSvrList();
	AutoSizeColumns(&m_svrList);
}

BOOL CPropService::WriteToLog(CString strMsg, LONG lErrMsg)
{
	FILE *fLog = 0;

	fLog = fopen("ErrLog.txt", "a+t");
	if(!fLog)
	{
		AfxMessageBox(_T("�޷�������־�ļ�. \n�����޷���ʾ!"),
			MB_OK | MB_ICONEXCLAMATION);
	
		return FALSE;
	}
/*
	if(lErrMsg < 0)
	{
		fprintf(fLog, "%s", _T(strMsg));
		
		return TRUE;
	}
*/
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

	fprintf(fLog, "%s#%s", _T(strMsg), _T(str));
	if(fLog)
		fclose(fLog);

	return TRUE;
}

void CPropService::OnCreateSvr() 
{
	// TODO: Add your control notification handler code here
	AfxMessageBox(_T("Sorry, it's not available for now!"));
}

void CPropService::OnViewLog() 
{
	// TODO: Add your control notification handler code here
	ShellExecute(NULL, "open", "ErrLog.txt", NULL, NULL, SW_SHOWNORMAL);
}

DWORD CPropService::SvrControl(DWORD dwCtrlCode)
{
	SC_HANDLE       m_scServiceManager;
	BOOL            m_bConnected;
	SC_HANDLE       m_scService;
	BOOL            m_bServiceOpen;
	BOOL            bSuccess = FALSE;
	SERVICE_STATUS	m_ServiceStatus;
	DWORD			m_dwCurState;


	// Open Service Manager
	m_scServiceManager = OpenSCManager((LPCTSTR)m_strMachineName,
		NULL, GENERIC_READ | GENERIC_EXECUTE);
	m_bConnected = (m_scServiceManager != (SC_HANDLE)NULL);
	
	if(!m_bConnected)
	{
		WriteToLog(
			"\n****************\n���ݷ���ʱ, �򿪹�����ʧ��!\n",
			GetLastError());

		return FALSE;
	}

	// Open Service
	m_scService = OpenService(m_scServiceManager,
		(LPCTSTR)m_strCurSvrName, GENERIC_READ | GENERIC_EXECUTE);
	m_bServiceOpen = (m_scService != (SC_HANDLE)NULL);
	
	if(!m_bServiceOpen)
	{
		WriteToLog(
			"\n****************\n���ݷ���ʱ, �򿪷�����ʧ��!\n",
			GetLastError());

		return FALSE;
	}

	// Get service State
	QueryServiceStatus(m_scService, &m_ServiceStatus);
	m_dwCurState = m_ServiceStatus.dwCurrentState;

	// Control Service
	if(m_dwCurState == SERVICE_STOPPED)
	{
		if(dwCtrlCode == SERVICE_CONTROL_PAUSE)
			AfxMessageBox(_T("You cannot pause a stopped service."));
		else if(dwCtrlCode == SERVICE_CONTROL_STOP)
			AfxMessageBox(_T("Service is already stopped."));
		else 
		{
			bSuccess = StartService(m_scService, 0, NULL);
			if(!bSuccess)
			{
				WriteToLog(
					"\n****************\n��������ʱʧ��!\n",
					GetLastError());
			}
		}
	}
	else
	{
		if(m_dwCurState == SERVICE_RUNNING && dwCtrlCode == SERVICE_CONTROL_CONTINUE)
			AfxMessageBox(_T("Service is already started."));
		else
		{
			if(m_dwCurState == SERVICE_PAUSED && dwCtrlCode == SERVICE_CONTROL_PAUSE)
			{	
				bSuccess = ControlService(m_scService, SERVICE_CONTROL_CONTINUE, &m_ServiceStatus);
				if(!bSuccess)
				{
					WriteToLog(
					"\n****************\n��������ʱʧ��!\n",
					GetLastError());
				}
			}
			else
			{
				if(m_dwCurState == SERVICE_STOPPED && dwCtrlCode == SERVICE_CONTROL_STOP)
					AfxMessageBox(_T("Service is already stopped."));
				else
				{
					bSuccess = ControlService(m_scService, dwCtrlCode, &m_ServiceStatus);
					if(!bSuccess)
					{
						WriteToLog(
							"\n****************\n���Ʒ���ʱʧ��!\n",
							GetLastError());
					}
				}
			}
		}
	}		

/*	if(!bSuccess)
	{
		if(m_dwLastError != 0)
			m_strError.Format(_T("%s\r\nSystem Error: %ld (%s)."), 
				m_strCustomError, m_dwLastError, m_strSystemError);
		else
			m_strError.Format(m_strCustomError);
	}
*/
	return bSuccess;
}

void CPropService::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	// ��õ�ǰѡ��
	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;

	// ��֤���̲����ĵ���ʽ�˵�
	if( point.x == -1 && point.y == -1 )
	{
		CRect rect;
		GetClientRect( &rect );
		point = rect.TopLeft();
		point.Offset( 5, 5 );
		ClientToScreen( &point );
	}

	// ȡ�˵���Դ
	CMenu mnuTop;
	mnuTop.LoadMenu( IDR_MENU_SERVICE );

	// ȡ�Ӳ˵�
	CMenu* pPopup = mnuTop.GetSubMenu( 0 );
	ASSERT_VALID( pPopup );

	// ������ MFC �Զ��� UPDATE_COMMAND_UI ���Ƽ��˵�״̬

	// ��ʾ�˵�
	pPopup->TrackPopupMenu(	TPM_LEFTALIGN | TPM_LEFTBUTTON,
							point.x, point.y,
							AfxGetMainWnd(), NULL );

	// ����ʽ�˵�������ᱻ MFC ����Ϣ·�ɻ����Զ�����	
}

void CPropService::OnSvrLoad() 
{
	// TODO: Add your command handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	SvrControl(SERVICE_CONTROL_CONTINUE);
}

void CPropService::OnSvrStop() 
{
	// TODO: Add your command handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	SvrControl(SERVICE_CONTROL_STOP);
}

void CPropService::OnSvrResume() 
{
	// TODO: Add your command handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	SvrControl(SERVICE_CONTROL_PAUSE);
}

void CPropService::OnSvrPause() 
{
	// TODO: Add your command handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	SvrControl(SERVICE_CONTROL_PAUSE);
}

void CPropService::OnSvrRefresh() 
{
	// TODO: Add your command handler code here
	m_svrList.DeleteAllItems();
	GetSvrList();
	AutoSizeColumns(&m_svrList);
}

DWORD CPropService::GetSvrStatus(CString strServiceName)
{
	SC_HANDLE hSCManager = OpenSCManager(GetLocalMachineName(), NULL, GENERIC_READ);
	SC_HANDLE hService = OpenService(hSCManager, strServiceName, GENERIC_READ);
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
		return ss.dwCurrentState;

	return NULL;
}

void CPropService::OnUpdateSvrLoad(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	VERIFY(m_strCurSvrName != "" && m_strCurSvrName != NULL);
	switch (GetSvrStatus(m_strCurSvrName))
	{
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
		{
			pCmdUI->Enable(TRUE);
			break;
		}
	case SERVICE_PAUSE_PENDING:
	case SERVICE_PAUSED:
	case SERVICE_START_PENDING:
	case SERVICE_RUNNING:
	case SERVICE_CONTINUE_PENDING:
	default:
		{
			pCmdUI->Enable(FALSE);
			break;
		}
	}
}

void CPropService::OnUpdateSvrPause(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	VERIFY(m_strCurSvrName != "" && m_strCurSvrName != NULL);
	switch (GetSvrStatus(m_strCurSvrName))
	{
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
	case SERVICE_PAUSE_PENDING:
	case SERVICE_PAUSED:
		{
			pCmdUI->Enable(FALSE);
			break;
		}
	case SERVICE_START_PENDING:
	case SERVICE_RUNNING:
	case SERVICE_CONTINUE_PENDING:
	default:
		{
			pCmdUI->Enable(TRUE);
			break;
		}
	}
}

void CPropService::OnUpdateSvrResume(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	VERIFY(m_strCurSvrName != "" && m_strCurSvrName != NULL);
	switch (GetSvrStatus(m_strCurSvrName))
	{
	case SERVICE_PAUSE_PENDING:
	case SERVICE_PAUSED:
		{
			pCmdUI->Enable(TRUE);
			break;
		}
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
	case SERVICE_START_PENDING:
	case SERVICE_RUNNING:
	case SERVICE_CONTINUE_PENDING:
	default:
		{
			pCmdUI->Enable(FALSE);
			break;
		}
	}
}

void CPropService::OnUpdateSvrStop(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
/*	int nCurSel = m_svrList.GetNextItem(-1, LVNI_SELECTED);
	if(nCurSel != -1)
		m_strCurSvrName = m_svrList.GetItemText(nCurSel, 2);
	else
		return;
*/
	switch (GetSvrStatus(m_strCurSvrName))
	{
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
		{
			pCmdUI->Enable(FALSE);
			break;
		}
	case SERVICE_PAUSE_PENDING:
	case SERVICE_PAUSED:
	case SERVICE_START_PENDING:
	case SERVICE_RUNNING:
	case SERVICE_CONTINUE_PENDING:
	default:
		{
			pCmdUI->Enable(TRUE);
			break;
		}
	}
}
