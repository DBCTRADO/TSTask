#include "stdafx.h"
#include "TSTaskCentre.h"
#include "GeneralSettingsDialog.h"
#include "MiscDialog.h"
#include "ThemeSettings.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CGeneralSettingsDialog::CGeneralSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
	{
	}

	CGeneralSettingsDialog::~CGeneralSettingsDialog()
	{
		Destroy();
	}

	bool CGeneralSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_GENERAL));
	}

	INT_PTR CGeneralSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();
				TSTask::String Text;

				CheckItem(IDC_SETTINGS_CONFIRM_CLOSE,Settings.MainBoard.GetConfirmClose());
				CheckItem(IDC_SETTINGS_CONFIRM_CLOSE_TASK_EXISTS,Settings.MainBoard.GetConfirmCloseTaskExists());
				EnableItem(IDC_SETTINGS_CONFIRM_CLOSE_TASK_EXISTS,Settings.MainBoard.GetConfirmClose());
				CheckItem(IDC_SETTINGS_CONFIRM_TASK_EXIT,Settings.MainBoard.GetConfirmTaskExit());
				CheckItem(IDC_SETTINGS_EXIT_WHEN_ALL_TASK_ENDED,Settings.General.GetExitWhenAllTaskEnded());

				CheckItem(IDC_SETTINGS_SHOW_IN_TASKBAR,Settings.MainBoard.GetShowInTaskbar());
				CheckItem(IDC_SETTINGS_SHOW_TRAY_ICON,Settings.MainBoard.GetShowTrayIcon());
				if (Settings.MainBoard.GetShowInTaskbar()) {
					CheckItem(IDC_SETTINGS_MINIMIZE_TO_TRAY,Settings.MainBoard.GetMinimizeToTray());
				} else {
					CheckItem(IDC_SETTINGS_MINIMIZE_TO_TRAY,true);
					EnableItem(IDC_SETTINGS_MINIMIZE_TO_TRAY,false);
				}

				Settings.Task.GetExeFileName(&Text);
				SetItemString(IDC_SETTINGS_TSTASK_FILE_NAME,Text);

				Settings.Task.GetCommandLineOptions(&Text);
				SetItemString(IDC_SETTINGS_TSTASK_OPTIONS,Text);

				::SendDlgItemMessage(hDlg,IDC_SETTINGS_THEME,CB_ADDSTRING,0,
									 reinterpret_cast<LPARAM>(L"デフォルト"));
				int Sel=0;
				CThemeManager &ThemeManager=m_Core.GetThemeManager();
				std::vector<TSTask::String> ThemeFiles;
				if (ThemeManager.GetThemeFileList(&ThemeFiles) && !ThemeFiles.empty()) {
					TSTask::String CurTheme;
					TSTask::String Name;
					Settings.MainBoard.GetThemeFile(&CurTheme);
					for (size_t i=0;i<ThemeFiles.size();i++) {
						Name=ThemeFiles[i];
						TSTask::PathUtility::RemoveExtension(&Name);
						::SendDlgItemMessage(hDlg,IDC_SETTINGS_THEME,CB_ADDSTRING,0,
											 reinterpret_cast<LPARAM>(Name.c_str()));
						if (TSTask::StringUtility::CompareNoCase(ThemeFiles[i],CurTheme)==0)
							Sel=int(i)+1;
					}
				} else {
					EnableItem(IDC_SETTINGS_THEME,false);
				}
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_THEME,CB_SETCURSEL,Sel,0);
				m_fThemeApplied=false;
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_SETTINGS_CONFIRM_CLOSE:
				EnableItem(IDC_SETTINGS_CONFIRM_CLOSE_TASK_EXISTS,
						   IsItemChecked(IDC_SETTINGS_CONFIRM_CLOSE));
				return TRUE;

			case IDC_SETTINGS_SHOW_IN_TASKBAR:
				{
					bool fShow=IsItemChecked(IDC_SETTINGS_SHOW_IN_TASKBAR);

					EnableItem(IDC_SETTINGS_MINIMIZE_TO_TRAY,fShow);
					if (!fShow)
						CheckItem(IDC_SETTINGS_MINIMIZE_TO_TRAY,true);
				}
				return TRUE;

			case IDC_SETTINGS_THEME:
				if (HIWORD(wParam)==CBN_SELCHANGE) {
					TSTask::String Theme;
					GetCurSelTheme(&Theme);
					if (m_Core.GetThemeManager().ApplyTheme(Theme.c_str()))
						m_fThemeApplied=true;
				}
				return TRUE;

#ifdef _DEBUG
			case IDC_SETTINGS_SAVE_DEFAULT_THEME:
				{
					TSTask::String FileName;

					m_Core.GetThemeManager().GetThemesDirectory(&FileName);
					::CreateDirectory(FileName.c_str(),nullptr);
					TSTask::PathUtility::Append(&FileName,L"Default");
					FileName+=CThemeManager::GetThemeExtension();

					CThemeSettings Theme;
					Theme.SaveToFile(FileName.c_str());
				}
				return TRUE;
#endif
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CGeneralSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		TSTask::String Text;

		Settings.MainBoard.SetConfirmClose(IsItemChecked(IDC_SETTINGS_CONFIRM_CLOSE));
		Settings.MainBoard.SetConfirmCloseTaskExists(IsItemChecked(IDC_SETTINGS_CONFIRM_CLOSE_TASK_EXISTS));
		Settings.MainBoard.SetConfirmTaskExit(IsItemChecked(IDC_SETTINGS_CONFIRM_TASK_EXIT));
		Settings.General.SetExitWhenAllTaskEnded(IsItemChecked(IDC_SETTINGS_EXIT_WHEN_ALL_TASK_ENDED));

		Settings.MainBoard.SetShowInTaskbar(IsItemChecked(IDC_SETTINGS_SHOW_IN_TASKBAR));
		Settings.MainBoard.SetShowTrayIcon(IsItemChecked(IDC_SETTINGS_SHOW_TRAY_ICON));
		Settings.MainBoard.SetMinimizeToTray(
			!Settings.MainBoard.GetShowInTaskbar()
				|| IsItemChecked(IDC_SETTINGS_MINIMIZE_TO_TRAY));

		if (GetItemString(IDC_SETTINGS_TSTASK_FILE_NAME,&Text))
			Settings.Task.SetExeFileName(Text);

		if (GetItemString(IDC_SETTINGS_TSTASK_OPTIONS,&Text))
			Settings.Task.SetCommandLineOptions(Text);

		GetCurSelTheme(&Text);
		TSTask::String CurTheme;
		Settings.MainBoard.GetThemeFile(&CurTheme);
		if (TSTask::StringUtility::CompareNoCase(Text,CurTheme)!=0) {
			Settings.MainBoard.SetThemeFile(Text);
			m_Core.GetThemeManager().ApplyTheme(Text.c_str());
		} else if (m_fThemeApplied) {
			m_Core.GetThemeManager().ApplyTheme(Text.c_str());
		}

		return true;
	}

	void CGeneralSettingsDialog::OnCancel(const CTSTaskCentreSettings &Settings)
	{
		if (m_fThemeApplied) {
			TSTask::String CurTheme;
			Settings.MainBoard.GetThemeFile(&CurTheme);
			m_Core.GetThemeManager().ApplyTheme(CurTheme.c_str());
		}
	}

	void CGeneralSettingsDialog::GetCurSelTheme(TSTask::String *pTheme) const
	{
		LRESULT Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_THEME,CB_GETCURSEL,0,0);
		if (Sel>0) {
			WCHAR szFile[MAX_PATH];
			::ZeroMemory(szFile,sizeof(szFile));
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_THEME,CB_GETLBTEXT,Sel,
								 reinterpret_cast<LPARAM>(szFile));
			pTheme->assign(szFile);
			pTheme->append(CThemeManager::GetThemeExtension());
		} else {
			pTheme->clear();
		}
	}

}
