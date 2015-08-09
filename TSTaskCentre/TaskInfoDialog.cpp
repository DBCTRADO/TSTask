#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TaskInfoDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	enum {
		TASK_PROP_TASK_ID,
		TASK_PROP_VERSION,
		TASK_PROP_PROCESS_ID,
		TASK_PROP_ARCHITECTURE,
		TASK_PROP_PATH,
		TASK_PROP_WORKING_SET,
		TASK_PROP_PEAK_WORKING_SET,
		TASK_PROP_PRIVATE_USAGE,
		TASK_PROP_PEAK_PAGE_FILE_USAGE,
		TASK_PROP_PAGE_FAULT,
		TASK_PROP_CREATION_TIME,
		TASK_PROP_USER_TIME,
		TASK_PROP_KERNEL_TIME,
		TASK_PROP_CYCLE_TIME,
		TASK_PROP_IO_READ,
		TASK_PROP_IO_WRITE,
		TASK_PROP_IO_OTHER,
		TASK_PROP_HANDLE_COUNT,
		TASK_PROP_GDI_OBJECTS,
		TASK_PROP_USER_OBJECTS
	};

	static const LPCTSTR TaskPropertyTextList[] = {
		TEXT("タスクID"),
		TEXT("バージョン"),
		TEXT("プロセスID"),
		TEXT("アーキテクチャ"),
		TEXT("パス"),
		TEXT("ワーキングセット"),
		TEXT("最大ワーキングセット"),
		TEXT("ページファイル"),
		TEXT("最大ページファイル"),
		TEXT("ページフォールト"),
		TEXT("開始日時"),
		TEXT("User実行時間"),
		TEXT("Kernel実行時間"),
		TEXT("サイクル時間"),
		TEXT("I/O読み込み"),
		TEXT("I/O書き出し"),
		TEXT("I/Oその他"),
		TEXT("ハンドル数"),
		TEXT("GDIオブジェクト"),
		TEXT("Userオブジェクト"),
	};


	CTaskInfoDialog::CTaskInfoDialog(CTSTaskManager &Manager)
		: m_Manager(Manager)
		, m_TaskID(TSTask::INVALID_TASK_ID)
	{
	}

	CTaskInfoDialog::~CTaskInfoDialog()
	{
		Destroy();
	}

	bool CTaskInfoDialog::Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID)
	{
		m_TaskID=TaskID;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_TASK_INFO))>=0;
	}

	static void FormatTime(const FILETIME &Time,LPWSTR pszText,int MaxLength)
	{
		pszText[0]=L'\0';

		SYSTEMTIME stUTC,stLocal;
		int Length;

		::FileTimeToSystemTime(&Time,&stUTC);
		::SystemTimeToTzSpecificLocalTime(nullptr,&stUTC,&stLocal);
		Length=::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,
							   &stLocal,nullptr,pszText,MaxLength);
		if (Length>0) {
			pszText[Length-1]=L' ';
			Length+=::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,
									&stLocal,nullptr,pszText+Length,MaxLength-Length);
		}
	}

	static void FormatUptime(const ULONGLONG &Time,LPWSTR pszText,int MaxLength)
	{
		const ULONGLONG TimeMs=Time/TSTask::FILETIME_MILLISECOND;

		TSTask::FormatString(pszText,MaxLength,L"%u:%02u:%02u.%03u",
							 (unsigned int)(TimeMs/(60*60*1000)),
							 (unsigned int)(TimeMs/(60*1000)),
							 (unsigned int)(TimeMs/1000%60),
							 (unsigned int)(TimeMs%1000));
	}

	INT_PTR CTaskInfoDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				m_ListView.Attach(GetItemHandle(IDC_TASK_INFO_LIST));
				m_ListView.SetTheme(L"explorer");
				m_ListView.SetExtendedStyle(
					LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

				m_ListView.InsertColumn(0,TEXT("項目"));
				m_ListView.InsertColumn(1,TEXT("値"));

				for (int i=0;i<_countof(TaskPropertyTextList);i++)
					m_ListView.InsertItem(i,TaskPropertyTextList[i],i);

				WCHAR szText[256];

				TSTask::FormatString(szText,_countof(szText),L"%u",UINT(m_TaskID));
				m_ListView.SetItemText(TASK_PROP_TASK_ID,1,szText);

				TSTask::TaskInfo TaskInfo;
				if (m_Manager.GetTaskInfo(m_TaskID,&TaskInfo)) {
					TSTask::FormatString(szText,_countof(szText),L"%d.%d.%d",
										 TSTask::GetVersionMajor(TaskInfo.Version),
										 TSTask::GetVersionMinor(TaskInfo.Version),
										 TSTask::GetVersionRevision(TaskInfo.Version));
					m_ListView.SetItemText(TASK_PROP_VERSION,1,szText);

					TSTask::FormatString(szText,_countof(szText),L"%u (0x%x)",
										 TaskInfo.ProcessID,TaskInfo.ProcessID);
					m_ListView.SetItemText(TASK_PROP_PROCESS_ID,1,szText);
				}

				TSTask::String Path;
				if (m_Manager.GetTaskProcessPath(m_TaskID,&Path))
					m_ListView.SetItemText(TASK_PROP_PATH,1,Path.c_str());

				SetProcessInfo();

				m_ListView.AdjustColumnWidth();

				AddControl(IDC_TASK_INFO_LIST,ALIGN_ALL);
				AddControl(IDC_TASK_INFO_UPDATE,ALIGN_BOTTOM);

				AdjustPos();
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_TASK_INFO_UPDATE:
				SetProcessInfo();
				return TRUE;

			case IDOK:
			case IDCANCEL:
				::EndDialog(hDlg,LOWORD(wParam));
				return TRUE;
			}
			return TRUE;

		case WM_DESTROY:
			m_ListView.Detach();
			return TRUE;
		}

		return FALSE;
	}

	void CTaskInfoDialog::SetProcessInfo()
	{
		CTSTaskManager::TaskProcessInfo ProcessInfo;

		ProcessInfo.Mask=CTSTaskManager::PROCESS_INFO_ALL;

		if (m_Manager.GetTaskProcessInfo(m_TaskID,&ProcessInfo)) {
			WCHAR szText[256];

#ifdef _WIN64
			m_ListView.SetItemText(TASK_PROP_ARCHITECTURE,1,ProcessInfo.fWOW64?L"x86":L"x64");
#else
			SYSTEM_INFO SysInfo;
			::GetNativeSystemInfo(&SysInfo);
			LPCWSTR pszArch;
			switch (SysInfo.wProcessorArchitecture) {
			case PROCESSOR_ARCHITECTURE_INTEL:
				pszArch=L"x86";
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				pszArch=ProcessInfo.fWOW64?L"x86":L"x64";
				break;
			default:
				pszArch=ProcessInfo.fWOW64?L"Unknown (WOW64)":L"Unknown";
				break;
			}
			m_ListView.SetItemText(TASK_PROP_ARCHITECTURE,1,pszArch);
#endif

			TSTask::FormatString(szText,_countof(szText),L"%Iu",ProcessInfo.Memory.WorkingSetSize);
			m_ListView.SetItemText(TASK_PROP_WORKING_SET,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%Iu",ProcessInfo.Memory.PeakWorkingSetSize);
			m_ListView.SetItemText(TASK_PROP_PEAK_WORKING_SET,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%Iu",ProcessInfo.Memory.PrivateUsage);
			m_ListView.SetItemText(TASK_PROP_PRIVATE_USAGE,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%Iu",ProcessInfo.Memory.PeakPageFileUsage);
			m_ListView.SetItemText(TASK_PROP_PEAK_PAGE_FILE_USAGE,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%u",ProcessInfo.Memory.PageFaultCount);
			m_ListView.SetItemText(TASK_PROP_PAGE_FAULT,1,szText);

			if (ProcessInfo.CreationTime.dwLowDateTime!=0
					|| ProcessInfo.CreationTime.dwHighDateTime!=0) {
				FormatTime(ProcessInfo.CreationTime,szText,_countof(szText));
				m_ListView.SetItemText(TASK_PROP_CREATION_TIME,1,szText);
			}

			FormatUptime(ProcessInfo.UserTime,szText,_countof(szText));
			m_ListView.SetItemText(TASK_PROP_USER_TIME,1,szText);

			FormatUptime(ProcessInfo.KernelTime,szText,_countof(szText));
			m_ListView.SetItemText(TASK_PROP_KERNEL_TIME,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%I64u",ProcessInfo.CycleTime);
			m_ListView.SetItemText(TASK_PROP_CYCLE_TIME,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%llu (%llu Bytes)",
								 ProcessInfo.IOCounters.ReadOperationCount,
								 ProcessInfo.IOCounters.ReadTransferCount);
			m_ListView.SetItemText(TASK_PROP_IO_READ,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%llu (%llu Bytes)",
								 ProcessInfo.IOCounters.WriteOperationCount,
								 ProcessInfo.IOCounters.WriteTransferCount);
			m_ListView.SetItemText(TASK_PROP_IO_WRITE,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%llu (%llu Bytes)",
								 ProcessInfo.IOCounters.OtherOperationCount,
								 ProcessInfo.IOCounters.OtherTransferCount);
			m_ListView.SetItemText(TASK_PROP_IO_OTHER,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%u",ProcessInfo.HandleCount);
			m_ListView.SetItemText(TASK_PROP_HANDLE_COUNT,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%u",ProcessInfo.GdiObjects);
			m_ListView.SetItemText(TASK_PROP_GDI_OBJECTS,1,szText);

			TSTask::FormatString(szText,_countof(szText),L"%u",ProcessInfo.UserObjects);
			m_ListView.SetItemText(TASK_PROP_USER_OBJECTS,1,szText);
		}
	}

}
