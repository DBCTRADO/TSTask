#include "stdafx.h"
#include "TSTaskCentre.h"
#include "LogDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CLogDialog::CLogDialog(CTSTaskCentreCore &Core)
		: m_Core(Core)
		, m_TaskID(TSTask::INVALID_TASK_ID)
	{
	}

	CLogDialog::~CLogDialog()
	{
		Destroy();
	}

	bool CLogDialog::Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID)
	{
		m_hinst=hinst;
		m_TaskID=TaskID;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_LOG))>=0;
	}

	INT_PTR CLogDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				m_ListView.Attach(GetItemHandle(IDC_LOG_LIST));
				m_ListView.SetTheme(L"explorer");
				m_ListView.SetExtendedStyle(
					LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES | LVS_EX_DOUBLEBUFFER);

				HIMAGELIST himlIcons=
					::ImageList_LoadImage(m_hinst,MAKEINTRESOURCE(IDB_LOG_ICONS),16,1,
										  CLR_NONE,IMAGE_BITMAP,LR_CREATEDIBSECTION);
				m_ListView.SetImageList(himlIcons,LVSIL_SMALL);

				m_ListView.InsertColumn(0,TEXT(""));
				m_ListView.InsertColumn(1,TEXT("日時"));
				m_ListView.InsertColumn(2,TEXT("内容"));

				m_LogList.clear();
				if (m_Core.GetTSTaskManager().GetLog(m_TaskID,&m_LogList) && !m_LogList.empty()) {
					m_ListView.ReserveItemCount((int)m_LogList.size());

					int Index=0;

					for (const auto &e:m_LogList) {
						TCHAR szTime[TSTask::CBasicLogger::MAX_TIME_TEXT];

						TSTask::CBasicLogger::FormatTime(e.Time,szTime,_countof(szTime));

						m_ListView.InsertItem(Index,TEXT(""));
						m_ListView.SetItemText(Index,1,szTime);
						m_ListView.SetItemText(Index,2,e.Text.c_str());
						m_ListView.SetItemImage(Index,2,(int)e.Type);

						Index++;
					}

					m_ListView.EnsureItemVisible((int)m_LogList.size()-1,false);
				}

				m_ListView.SetColumnWidth(0,0);
				for (int i=1;i<=2;i++)
					m_ListView.AdjustColumnWidth(i);

				EnableItem(IDC_LOG_SAVE,!m_LogList.empty());

				AddControl(IDC_LOG_LIST,ALIGN_ALL);
				AddControl(IDC_LOG_SAVE,ALIGN_BOTTOM);

				AdjustPos();
			}
			return TRUE;

		case WM_NOTIFY:
			{
				NMHDR *pnmh=reinterpret_cast<NMHDR*>(lParam);

				switch (pnmh->code) {
				case NM_RCLICK:
					if (pnmh->idFrom==IDC_LOG_LIST) {
						HMENU hmenu=::CreatePopupMenu();
						::AppendMenu(hmenu,MF_STRING | (m_ListView.GetSelectedItem()>=0?MF_ENABLED:MF_GRAYED),1,TEXT("コピー(&C)"));
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("すべてコピー(&C)"));
						POINT pt;
						::GetCursorPos(&pt);
						int Result=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,nullptr);
						::DestroyMenu(hmenu);
						if (Result==1)
							::SendMessage(hDlg,WM_COMMAND,IDC_LOG_COPY,0);
						else if (Result==2)
							::SendMessage(hDlg,WM_COMMAND,IDC_LOG_COPY_ALL,0);
					}
					return TRUE;

				case NM_CUSTOMDRAW:
					if (pnmh->idFrom==IDC_LOG_LIST) {
						LPNMLVCUSTOMDRAW pnmlvcd=reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
						LRESULT Result=CDRF_DODEFAULT;

						switch (pnmlvcd->nmcd.dwDrawStage) {
						case CDDS_PREPAINT:
							Result=CDRF_NOTIFYITEMDRAW;
							break;

						case CDDS_ITEMPREPAINT:
							Result=CDRF_NOTIFYSUBITEMDRAW;
							break;

						case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
							if (pnmlvcd->iSubItem==0)
								Result=CDRF_SKIPDEFAULT;
							break;
						}

						SetMessageResult(Result);
						return TRUE;
					}
					break;
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_LOG_SAVE:
				{
					OPENFILENAME ofn;
					TCHAR szFileName[MAX_PATH];
					TSTask::String Directory;

					::ZeroMemory(&ofn,sizeof(ofn));
					::lstrcpy(szFileName,TEXT("TSTask.log"));
					TSTask::GetModuleDirectory(nullptr,&Directory);
					ofn.lStructSize=sizeof(ofn);
					ofn.hwndOwner=hDlg;
					ofn.lpstrFilter=TEXT("ログファイル(*.log)\0*.log\0全てのファイル(*.*)\0*.*\0");
					ofn.nFilterIndex=1;
					ofn.lpstrFile=szFileName;
					ofn.nMaxFile=_countof(szFileName);
					ofn.lpstrInitialDir=Directory.c_str();
					ofn.Flags=OFN_EXPLORER | OFN_OVERWRITEPROMPT;
					ofn.lpstrDefExt=TEXT("log");
					if (::GetSaveFileName(&ofn)) {
						if (!SaveLog(szFileName)) {
							::MessageBox(hDlg,L"ログの保存ができません。",nullptr,
										 MB_OK | MB_ICONEXCLAMATION);
						}
					}
				}
				return TRUE;

			case IDC_LOG_COPY:
				{
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0 && (size_t)Sel<m_LogList.size()) {
						TSTask::String Text;

						TSTask::CBasicLogger::FormatInfo(m_LogList[Sel],&Text);
						m_Core.CopyTextToClipboard(Text);
					}
				}
				return TRUE;

			case IDC_LOG_COPY_ALL:
				if (!m_LogList.empty()) {
					TSTask::String Text,Info;

					for (const auto &e:m_LogList) {
						TSTask::CBasicLogger::FormatInfo(e,&Info);
						Text+=Info;
						Text+=L"\r\n";
					}

					m_Core.CopyTextToClipboard(Text);
				}
				return TRUE;

			case IDOK:
			case IDCANCEL:
				End(LOWORD(wParam));
				return TRUE;
			}
			return TRUE;

		case WM_DESTROY:
			m_LogList.clear();
			m_ListView.Detach();
			return TRUE;
		}

		return FALSE;
	}

	bool CLogDialog::SaveLog(LPCTSTR pszFileName) const
	{
		if (TSTask::IsStringEmpty(pszFileName))
			return false;

		HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,nullptr,
								  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;

		DWORD Write;

		static const WORD BOM=0xFEFF;
		::WriteFile(hFile,&BOM,sizeof(BOM),&Write,nullptr);

		for (const auto &e:m_LogList) {
			TCHAR szTime[TSTask::CBasicLogger::MAX_TIME_TEXT];

			int Length=TSTask::CBasicLogger::FormatTime(e.Time,szTime,_countof(szTime));
			szTime[Length++]=L' ';

			::WriteFile(hFile,szTime,Length*sizeof(WCHAR),&Write,nullptr);
			::WriteFile(hFile,e.Text.data(),(DWORD)(e.Text.length()*sizeof(WCHAR)),&Write,nullptr);
			::WriteFile(hFile,L"\r\n",2*sizeof(WCHAR),&Write,nullptr);
		}

		::CloseHandle(hFile);

		return true;
	}

}
