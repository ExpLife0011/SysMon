// PropProcess.cpp : implementation file
//

#include "stdafx.h"
#include "SysMon.h"
#include "PropProcess.h"

#include "windows.h"
#include "Winbase.h"
#include "winperf.h"   // for Windows NT
#include "tlhelp32.h"  // for Windows 95

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern void AutoSizeColumns(CListCtrl *m_List, int col = -1);

/////////////////////////////////////////////////////////////////////////////
// CPropProcess property page

IMPLEMENT_DYNCREATE(CPropProcess, CPropertyPage)

CPropProcess::CPropProcess() : CPropertyPage(CPropProcess::IDD)
{
	//{{AFX_DATA_INIT(CPropProcess)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPropProcess::~CPropProcess()
{
}

void CPropProcess::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropProcess)
	DDX_Control(pDX, IDC_LIST_PROCESS, m_procList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropProcess, CPropertyPage)
	//{{AFX_MSG_MAP(CPropProcess)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_PROC_TERMINATE, OnProcTerminate)
	ON_COMMAND(IDM_PROC_REFRESH, OnProcRefresh)
	ON_COMMAND(IDM_PROC_SENDMSG, OnProcSendmsg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropProcess message handlers

BOOL CPropProcess::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	ForceKill = TRUE;
	curTaskPos = -1;

	// ��ʼ�� CListCtrl
	m_procList.InsertColumn(0, _T("���"), LVCFMT_LEFT, 70);
	m_procList.InsertColumn(1, _T("��������"), LVCFMT_LEFT, 70);
	m_procList.InsertColumn(2, _T("Process ID"), LVCFMT_LEFT, 70);
	m_procList.InsertColumn(3, _T("Wnd Handle"), LVCFMT_LEFT, 70);
	m_procList.InsertColumn(4, _T("���ڱ���"), LVCFMT_LEFT, 70);

	m_procList.InsertColumn(5, _T("���ȼ�"), LVCFMT_LEFT, 50);
	m_procList.InsertColumn(6, _T("�߳���"), LVCFMT_LEFT, 50);
	m_procList.InsertColumn(7, _T("·��"), LVCFMT_LEFT, 150);

    m_procList.SetExtendedStyle(m_procList.GetExtendedStyle()
        |LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP
		|LVS_EX_ONECLICKACTIVATE);

	m_procList.SetFocus();	// ���Ժ������� FALSE

	RefreshProcList();
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPropProcess::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	// ȡ���б�����
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_PROCESS);
	VERIFY(pList != NULL);

	CRect rect, rectHeader;
	pList->GetWindowRect(&rect);
	CHeaderCtrl* pHeader = pList->GetHeaderCtrl();	
	pHeader->GetWindowRect(&rectHeader);

	rectHeader.SetRect(rect.left, rect.top,
		rect.left + rect.Width(), rect.top + rectHeader.Height() + 2);

	rect.SubtractRect(&rect, &rectHeader);
	if(!rect.PtInRect(point))
		return;
/*
	// ��֤���̲����ĵ���ʽ�˵�
	ScreenToClient(&rect);
	if( point.x == -1 && point.y == -1 )
	{
		CRect rect;
		GetClientRect(&rect);
		point = rect.TopLeft();
		point.Offset( 5, 5 );
		ClientToScreen( &point );
	}
*/
	// ȡ�˵���Դ
	CMenu mnuTop;
	mnuTop.LoadMenu( IDR_MENU_PROCESS );

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

void CPropProcess::OnProcTerminate() 
{
	// TODO: Add your command handler code here
	POSITION pos = m_procList.GetFirstSelectedItemPosition();
    if(pos == NULL)
        AfxMessageBox(_T("����ѡ��Ҫ��ֹ�Ľ���!"));
    else
    {
        while (pos)
        {
            int nItem = m_procList.GetNextSelectedItem(pos);
            CString pid=m_procList.GetItemText(nItem, 2);
			curTaskPos = atoi(m_procList.GetItemText(nItem, 0)) - 1;
            int ret=Terminator(atoi(pid));
            if(ret)
			{
                m_procList.DeleteItem(nItem);
				curTaskPos = -1;
				// OnProcRefresh(); // ���ٶȱȽ����Ļ����ϣ����ܳ�������û����ʱ��ˢ�����б�
			}
        }
    }
}

void CPropProcess::OnProcRefresh() 
{
	// TODO: Add your command handler code here
    m_procList.DeleteAllItems();
    RefreshProcList();	
}

///////////////////////////////////////////////////////
// Process Access Common Function

// these constants will be used below
#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  "Counters"
#define PROCESS_COUNTER     "process"
#define PROCESSID_COUNTER   "id process"
#define UNKNOWN_TASK        _T("δ֪") //"unknown"

// ȫ�ֺ���
BOOL CALLBACK EnumWindowsProc(HWND hwnd, DWORD lParam)
/*++
����:
    ����ö�ٵ� Callback function

����:
    hwnd -- ���ھ��
    lParam -- ** û��ʹ�� **

����ֵ:
    TRUE -- �����о�
--*/
{
    DWORD             pid = 0;
    DWORD             i;
    CHAR              buf[TITLE_SIZE];
    PPROC_LIST_ENUM   te = (PPROC_LIST_ENUM)lParam;
    PPROC_LIST        tlist = te->tlist;
    DWORD             numTasks = te->numtasks;

    // ȡ�ô��ڵĽ��� ID
    if (!GetWindowThreadProcessId( hwnd, &pid ))
        return TRUE;

    // �������б��в�ѯ��ǰ����
    for (i = 0; i < numTasks; i++)
	{
        if (tlist[i].dwProcessId == pid)
		{
			// ���Ҷ��㴰�ڱ���
			HWND parentHWND = GetParent(hwnd);
 			while (parentHWND != NULL)
			{
				hwnd = parentHWND;
				parentHWND = GetParent(parentHWND);
			}

            tlist[i].hwnd = hwnd;

            // �ҵ�ʱ��ȡ���ڱ���
            if (::GetWindowText(tlist[i].hwnd, buf, sizeof(buf) ))
			{
                //���洰�ڱ���
                strcpy( tlist[i].WindowTitle, buf );
            }
            break;
        }
    }

    // �����о�
    return TRUE;
}

BOOL CPropProcess::EnableDebugPriv95(VOID)
/*++
����:
    �ı�������ȼ�. Win95 ϵ�в��ظı�

����:

����ֵ:
    TRUE -- �ɹ�
    FALSE -- ʧ��

Comments: 
    �ܷ��� TRUE
--*/
{
   return TRUE;
}

BOOL CPropProcess::EnableDebugPrivNT(VOID)
/*++
����:
    �ı�������ȼ��Ա������ֹ���̲���

����:

����ֵ:
    TRUE  -- �ɹ�
    FALSE -- ʧ��
--*/
{
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    // ���Ž��̵Ĵ�ȡ��ʾ
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken))
	{
        AfxMessageBox("OpenProcessToken failed with %d\n", GetLastError());
        return FALSE;
    }

    // ��ȡ SE_DEBUG_NAME ��Ȩ
    if (!LookupPrivilegeValue((LPSTR) NULL,
            SE_DEBUG_NAME,
            &DebugValue))
	{
        AfxMessageBox("LookupPrivilegeValue failed with %d\n", GetLastError());
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
    if (GetLastError() != ERROR_SUCCESS) {
        AfxMessageBox("AdjustTokenPrivileges failed with %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

VOID CPropProcess::GetWindowTitles(PPROC_LIST_ENUM te)
{
    // ö�����еĴ���
    EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM) te );
}

BOOL CPropProcess::MatchPattern(PUCHAR String, PUCHAR Pattern)
{
    UCHAR   c, p, l;

    for (; ;) {
        switch (p = *Pattern++) {
            case 0:                             // end of pattern
                return *String ? FALSE : TRUE;  // if end of string TRUE

            case '*':
                while (*String) {               // match zero or more char
                    if (MatchPattern (String++, Pattern))
                        return TRUE;
                }
                return MatchPattern (String, Pattern);

            case '?':
                if (*String++ == 0)             // match any one char
                    return FALSE;                   // not end of string
                break;

            case '[':
                if ( (c = *String++) == 0)      // match char set
                    return FALSE;                   // syntax

                c = toupper(c);
                l = 0;
                while (p = *Pattern++) {
                    if (p == ']')               // if end of char set, then
                        return FALSE;           // no match found

                    if (p == '-') {             // check a range of chars?
                        p = *Pattern;           // get high limit of range
                        if (p == 0  ||  p == ']')
                            return FALSE;           // syntax

                        if (c >= l  &&  c <= p)
                            break;              // if in range, move on
                    }

                    l = p;
                    if (c == p)                 // if char matches this element
                        break;                  // move on
                }

                while (p  &&  p != ']')         // got a match in char set
                    p = *Pattern++;             // skip to end of set

                break;

            default:
                c = *String++;
                if (toupper(c) != p)            // check for exact char
                    return FALSE;                   // not a match

                break;
        }
    }
}

BOOL CPropProcess::GetTaskList(PPROC_LIST pTaskList, DWORD dwNumTasks)
{
	DWORD dwSysID;
	dwSysID = GetSysID();

	switch (dwSysID)
	{
	case VER_PLATFORM_WIN32_NT:
		return GetTaskListNT(pTaskList, dwNumTasks);

	case VER_PLATFORM_WIN32_WINDOWS:
		return GetTaskList95(pTaskList, dwNumTasks);

	default:
		AfxMessageBox(_T("������ֻ�������� Windows(NT, 95 �����) ����ϵͳ��!"));
		return FALSE;
	}
}

BOOL CPropProcess::EnableDebugPriv(VOID)
{
	DWORD dwSysID;
	dwSysID = GetSysID();

	switch (dwSysID)
	{
	case VER_PLATFORM_WIN32_NT:
		return EnableDebugPrivNT();

	case VER_PLATFORM_WIN32_WINDOWS:
		return EnableDebugPriv95();

	default:
		AfxMessageBox(_T("������ֻ�������� Windows(NT, 95 �����) ����ϵͳ��!"));
		return FALSE;
	}
}

BOOL CPropProcess::Terminator(DWORD pidToKill)
{
	if (pidToKill == 0)
	{
		AfxMessageBox(_T("ȱ�� Process ID !"));
		return FALSE;
	}
    
	// ��ò����������̵�Ȩ��
	EnableDebugPriv();

	ASSERT(curTaskList[curTaskPos].dwProcessId == pidToKill);
	
	return KillProcess(&curTaskList[curTaskPos], ForceKill);

/*// ��ӷ�ֹ��������Ĵ���
	CHAR              pname[MAX_PATH] = "";
    PROC_LIST_ENUM    te;
    DWORD             ThisPid;
    DWORD             i;
    DWORD             numTasks;
    int               rval = 0;
    char              tname[PROCESS_SIZE];
    LPSTR             p;

	// ö�����д��ڲ���ͼ�õõ����ڱ���
    te.tlist = curTaskList;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    ThisPid = GetCurrentProcessId();

    for (i=0; i<numTasks; i++)
	{
        // ��ֹ�û����� KILL.EXE ���丸����
        if (ThisPid == curTaskList[i].dwProcessId) {
            continue;
        }
        if (MatchPattern( (unsigned char *)curTaskList[i].WindowTitle, (unsigned char *)"*KILL*" )) {
            continue;
        }

        tname[0] = 0;
        strcpy( tname, curTaskList[i].ProcessName );
        p = strchr( tname, '.' );
        if (p) {
            p[0] = '\0';
        }
        if (MatchPattern( (unsigned char *)tname, (unsigned char *)pname )) {
            curTaskList[i].flags = TRUE;
        } else if (MatchPattern( (unsigned char *)curTaskList[i].ProcessName, (unsigned char *)pname )) {
            curTaskList[i].flags = TRUE;
        } else if (MatchPattern( (unsigned char *)curTaskList[i].WindowTitle, (unsigned char *)pname )) {
            curTaskList[i].flags = TRUE;
        }
    }

    for (i=0; i<numTasks; i++) {
        if (curTaskList[i].flags) {
            if (KillProcess( &curTaskList[i], ForceKill ))
			{
				CString msg;
				msg.Format("���� #%d [%s] �ѱ���ֹ!", curTaskList[i].dwProcessId, curTaskList[i].ProcessName);
                AfxMessageBox(_T(msg));
            }
			else
			{
				CString msg;
				msg.Format("���� #%d [%s] ������ֹ!", curTaskList[i].dwProcessId, curTaskList[i].ProcessName);
                AfxMessageBox(_T(msg));

                rval = FALSE;
            }
        }
    }

	return TRUE;
//*/
}

BOOL CPropProcess::KillProcess(PPROC_LIST pTask, BOOL fForce)
{
	HANDLE hProcess;

	if (fForce || !pTask->hwnd)
	{
		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pTask->dwProcessId );
	        if (hProcess)
	        {
			hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pTask->dwProcessId );
			if (hProcess == NULL)
			{
				return FALSE;
			}
	
			if (!TerminateProcess( hProcess, 1 )) // ��ֹ���̲������˳��� 1
			{
				CloseHandle( hProcess );
				return FALSE;
			}
	
			CloseHandle( hProcess );
			return TRUE;
		}
	}

	// ���� WM_CLOSE ��Ϣ����������
	::PostMessage( pTask->hwnd, WM_CLOSE, 0, 0 );
//	::PostMessage( pTask->hwnd, WM_ENDSESSION, 0, 0 );
	return TRUE;
}

DWORD CPropProcess::GetTaskListNT(PPROC_LIST pTask, DWORD dwNumTasks)
{
    DWORD                        rc;
    HKEY                         hKeyNames;
    DWORD                        dwType;
    DWORD                        dwSize;
    LPBYTE                       buf = NULL;
    CHAR                         szSubKey[1024];
    LANGID                       lid;
    LPSTR                        p;
    LPSTR                        p2;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle;
    DWORD                        dwProcessIdCounter;
    CHAR                         szProcessName[MAX_PATH];
    DWORD                        dwLimit = dwNumTasks - 1;



    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    sprintf( szSubKey, "%s\\%03x", REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );
    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) malloc( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) {
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

    p = (char *)buf;
    while (*p) {
        if (p > (char *)buf) {
            for( p2=p-2; isdigit(*p2); p2--) ;
            }
        if (stricmp(p, PROCESS_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; isdigit(*p2); p2--) ;
            strcpy( szSubKey, p2+1 );
        }
        else
        if (stricmp(p, PROCESSID_COUNTER) == 0) {
            //
            // look backwards for the counter number
            //
            for( p2=p-2; isdigit(*p2); p2--) ;
            dwProcessIdTitle = atol( p2+1 );
        }
        //
        // next string
        //
        p += (strlen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );


    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = (unsigned char *)malloc( dwSize );
    if (buf == NULL) {
        goto exit;
    }
    memset( buf, 0, dwSize );


    while (TRUE) {

        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              buf,
                              &dwSize
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize > 0) &&
            (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' &&
            (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F' ) {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) {
            dwSize += EXTEND_SIZE;
            buf = (unsigned char *)realloc( buf, dwSize );
            memset( buf, 0, dwSize );
        }
        else {
            goto exit;
        }
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((DWORD)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //
    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    dwNumTasks = min( dwLimit, (DWORD)pObj->NumInstances );

    pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    for (i=0; i<dwNumTasks; i++) {
        //
        // pointer to the process name
        //
        p = (LPSTR) ((DWORD)pInst + pInst->NameOffset);

        //
        // convert it to ascii
        //
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  (LPCWSTR)p,
                                  -1,
                                  szProcessName,
                                  sizeof(szProcessName),
                                  NULL,
                                  NULL
                                );

        if (!rc) {
            //
	    // if we cant convert the string then use a default value
            //
            strcpy( pTask->ProcessName, UNKNOWN_TASK );
        }

        if (strlen(szProcessName)+4 <= sizeof(pTask->ProcessName)) {
            strcpy( pTask->ProcessName, szProcessName );
            strcat( pTask->ProcessName, ".exe" );
        }

        //
        // get the process id
        //
        pCounter = (PPERF_COUNTER_BLOCK) ((DWORD)pInst + pInst->ByteLength);
        pTask->flags = 0;
        pTask->dwProcessId = *((LPDWORD) ((DWORD)pCounter + dwProcessIdCounter));
        if (pTask->dwProcessId == 0) {
            pTask->dwProcessId = (DWORD)-2;
        }

        //
        // next process
        //
        pTask++;
        pInst = (PPERF_INSTANCE_DEFINITION) ((DWORD)pCounter + pCounter->ByteLength);
    }

exit:
    if (buf) {
        free( buf );
    }

    RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );

    return dwNumTasks;
}

DWORD CPropProcess::GetTaskList95(PPROC_LIST pTaskList, DWORD dwNumTasks)
/*++
����:
    ������ϵͳΪ win95 �����ʱ, ͨ�� API -- Toolhelp32 -- ���õ������б�

����:
    dwNumTasks -- pTaskList �����ܹ�װ�ص���� Task ��

����ֵ:
    ���ر������������ Task ����
--*/
{
	HANDLE         hProcessSnap   = NULL;
	PROCESSENTRY32 procEntry32    = {0};
	DWORD          dwTaskCount    = 0;

	// ��֤������Ҫ�о�һ�� Task
	if (dwNumTasks == 0)
		return 0;

	// ��õ�ǰϵͳ���̿���
	hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == (HANDLE)-1)
		return 0;
	
	// ��ȡ������ÿһ���̵���Ϣ
	dwTaskCount = 0;
	procEntry32.dwSize = sizeof(PROCESSENTRY32);   // ʹ��ǰ��������
	if (::Process32First(hProcessSnap, &procEntry32))
	{
		do
		{
			LPSTR pCurChar; // Ϊ˫�ֽ�ϵͳ׼��

			// �����ļ�����·��
			for (pCurChar = (procEntry32.szExeFile + lstrlen(procEntry32.szExeFile));
				*pCurChar != '\\' && pCurChar != procEntry32.szExeFile; 
				--pCurChar)

			lstrcpy(pTaskList->ProcessName, pCurChar);
			pTaskList->flags = 0;
			pTaskList->dwProcessId = procEntry32.th32ProcessID;

			++dwTaskCount;
			++pTaskList; // ��һ�� Task
		}
		while (dwTaskCount < dwNumTasks && ::Process32Next(hProcessSnap, &procEntry32));
	}
	else
		dwTaskCount = 0;

	// �ͷ� snapshot ����
	CloseHandle (hProcessSnap);

	return dwTaskCount;
}

BOOL CPropProcess::RefreshProcList()
{
    DWORD numTasks;
    DWORD i;
    PROC_LIST_ENUM te;

	curTaskPos = -1;

    // ȡ�ò����������̵�Ȩ��
    EnableDebugPriv();

    memset(curTaskList, 0, sizeof(curTaskList));

    // ȡ��ϵͳ��ǰ�Ľ����б�
    numTasks = GetTaskList(curTaskList, MAX_TASKS);

    // ö�����д��ڲ���ͼΪÿ�����̻�ô��ڱ���
    te.tlist = curTaskList;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    // ˢ�½����б�
	m_procList.SetRedraw(false);
    for (i=0; i < numTasks; i++) 
    {
        CString pid,wind,num;
        num.Format("%03d", i + 1);
        pid.Format("%d", curTaskList[i].dwProcessId);
        wind.Format("%d", curTaskList[i].hwnd);
        m_procList.InsertItem(i, num);
        m_procList.SetItemText(i, 1, curTaskList[i].ProcessName);
        m_procList.SetItemText(i, 2, pid);
        m_procList.SetItemText(i, 3, wind);

        if (curTaskList[i].hwnd) 
        {
            m_procList.SetItemText(i, 4, curTaskList[i].WindowTitle);
        } 
    }
	m_procList.SetRedraw(false);

	AutoSizeColumns(&m_procList);
    return TRUE;
}

DWORD CPropProcess::GetSysID()
{
	OSVERSIONINFO sysInfo = {0};
	sysInfo.dwOSVersionInfoSize = sizeof(sysInfo);
	GetVersionEx(&sysInfo);
	return sysInfo.dwPlatformId;
}

void CPropProcess::OnProcSendmsg() 
{
	// TODO: Add your command handler code here
	ForceKill = FALSE;
	OnProcTerminate();
	ForceKill = TRUE;
}

