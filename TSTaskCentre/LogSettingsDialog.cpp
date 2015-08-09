#include "stdafx.h"
#include "TSTaskCentre.h"
#include "LogSettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CLogSettingsDialog::CLogSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
	{
	}

	CLogSettingsDialog::~CLogSettingsDialog()
	{
		Destroy();
	}

	bool CLogSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		m_hinst=hinst;

		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_LOG));
	}

	INT_PTR CLogSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

				m_ListView.Attach(GetItemHandle(IDC_SETTINGS_LOG_LIST));
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

				if (m_Core.GetLogger().GetLog(&m_LogList) && !m_LogList.empty()) {
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

				static const LPCTSTR LoggingLevelList[] = {
					TEXT("0: 記録を行わない"),
					TEXT("1: エラーを記録する"),
					TEXT("2: エラーと注意を記録する"),
					TEXT("3: 重要なものを記録する"),
					TEXT("4: 通常の動作を記録する"),
					TEXT("5: 全て記録する"),
				};
				static_assert(_countof(LoggingLevelList)==TSTask::LOG_TRAILER,
							  "ログ記録レベルの数が一致しません。");
				for (size_t i=0;i<_countof(LoggingLevelList);i++) {
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_LOG_LEVEL,CB_ADDSTRING,0,
										 reinterpret_cast<LPARAM>(LoggingLevelList[i]));
				}
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_LOG_LEVEL,CB_SETCURSEL,
									 CGeneralSettings::LoggingTypeToLevel(Settings.General.GetLoggingLevel()),0);
			}
			return TRUE;

		case WM_NOTIFY:
			{
				NMHDR *pnmh=reinterpret_cast<NMHDR*>(lParam);

				switch (pnmh->code) {
				case NM_RCLICK:
					if (pnmh->idFrom==IDC_SETTINGS_LOG_LIST) {
						HMENU hmenu=::CreatePopupMenu();
						::AppendMenu(hmenu,MF_STRING | (m_ListView.GetSelectedItem()>=0?MF_ENABLED:MF_GRAYED),1,TEXT("コピー(&C)"));
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("すべてコピー(&C)"));
						POINT pt;
						::GetCursorPos(&pt);
						int Result=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,nullptr);
						::DestroyMenu(hmenu);
						if (Result==1)
							::SendMessage(hDlg,WM_COMMAND,IDC_SETTINGS_LOG_COPY,0);
						else if (Result==2)
							::SendMessage(hDlg,WM_COMMAND,IDC_SETTINGS_LOG_COPY_ALL,0);
					}
					return TRUE;

				case NM_CUSTOMDRAW:
					if (pnmh->idFrom==IDC_SETTINGS_LOG_LIST) {
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
			case IDC_SETTINGS_LOG_CLEAR:
				m_Core.GetLogger().Clear();
				m_ListView.DeleteAllItems();
				return TRUE;

			case IDC_SETTINGS_LOG_SAVE:
				{
					TSTask::String FilePath,FileName,Directory;
					TCHAR szFileName[MAX_PATH];
					OPENFILENAME ofn;

					TSTask::GetModuleFilePath(nullptr,&FilePath);
					TSTask::PathUtility::Split(FilePath,&Directory,&FileName);
					TSTask::PathUtility::RenameExtension(&FileName,L".log");
					::lstrcpyn(szFileName,FileName.c_str(),_countof(szFileName));
					::ZeroMemory(&ofn,sizeof(ofn));
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
						if (!m_Core.GetLogger().SaveToFile(szFileName))
							::MessageBox(hDlg,L"ファイルを保存できません。",nullptr,MB_OK | MB_ICONEXCLAMATION);
					}
				}
				return TRUE;

			case IDC_SETTINGS_LOG_COPY:
				{
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0 && (size_t)Sel<m_LogList.size()) {
						TSTask::String Text;

						TSTask::CBasicLogger::FormatInfo(m_LogList[Sel],&Text);
						m_Core.CopyTextToClipboard(Text);
					}
				}
				return TRUE;

			case IDC_SETTINGS_LOG_COPY_ALL:
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
			}
			return TRUE;

		case WM_DESTROY:
			m_LogList.clear();
			m_ListView.Detach();
			return TRUE;
		}

		return FALSE;
	}

	bool CLogSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		int Sel=(int)::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_LOG_LEVEL,CB_GETCURSEL,0,0);
		if (Sel>=0)
			Settings.General.SetLoggingLevel(CGeneralSettings::LoggingLevelToType(Sel));

		return true;
	}

}
