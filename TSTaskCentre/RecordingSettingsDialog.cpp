#include "stdafx.h"
#include "TSTaskCentre.h"
#include "RecordingSettingsDialog.h"
#include "MiscDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CRecordingSettingsDialog::CRecordingSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
	{
	}

	CRecordingSettingsDialog::~CRecordingSettingsDialog()
	{
		Destroy();
	}

	bool CRecordingSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_RECORDING));
	}

	INT_PTR CRecordingSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

				CheckItem(IDC_SETTINGS_RECORDING_CONFIRM_CHANNEL_CHANGE,
						  Settings.Recording.GetConfirmChannelChange());
				CheckItem(IDC_SETTINGS_RECORDING_CONFIRM_EXIT,
						  Settings.Recording.GetConfirmExit());
				CheckItem(IDC_SETTINGS_RECORDING_CONFIRM_STOP,
						  Settings.Recording.GetConfirmStop());
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CRecordingSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		Settings.Recording.SetConfirmChannelChange(
			IsItemChecked(IDC_SETTINGS_RECORDING_CONFIRM_CHANNEL_CHANGE));
		Settings.Recording.SetConfirmExit(
			IsItemChecked(IDC_SETTINGS_RECORDING_CONFIRM_EXIT));
		Settings.Recording.SetConfirmStop(
			IsItemChecked(IDC_SETTINGS_RECORDING_CONFIRM_STOP));

		return true;
	}

}
