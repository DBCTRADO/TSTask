#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TaskStreamingSettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTaskStreamingSettingsDialog::CTaskStreamingSettingsDialog(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings)
		: CTaskSettingsPage(Core,PageID,Settings)
	{
	}

	CTaskStreamingSettingsDialog::~CTaskStreamingSettingsDialog()
	{
		Destroy();
	}

	bool CTaskStreamingSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_TASK_SETTINGS_STREAMING));
	}

	INT_PTR CTaskStreamingSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				TSTask::String Text;

				::CheckRadioButton(hDlg,
								   IDC_TASK_SETTINGS_STREAMING_PROTOCOL_UDP,
								   IDC_TASK_SETTINGS_STREAMING_PROTOCOL_TCP,
								   IDC_TASK_SETTINGS_STREAMING_PROTOCOL_UDP+(int)m_Settings.Streaming.GetProtocol());
				m_Settings.Streaming.GetAddress(&Text);
				SetItemText(IDC_TASK_SETTINGS_STREAMING_ADDRESS,Text.empty()?L"127.0.0.1":Text.c_str());
				SetItemUInt(IDC_TASK_SETTINGS_STREAMING_PORT,m_Settings.Streaming.GetPort());
				CheckItem(IDC_TASK_SETTINGS_STREAMING_FIND_UNUSED_PORT,
						  m_Settings.Streaming.GetFindUnusedPort());

				CheckItem(IDC_TASK_SETTINGS_STREAMING_CURRENT_SERVICE,
						  m_Settings.Streaming.GetServiceSelectType()==TSTask::SERVICE_SELECT_CURRENT);
				CheckItem(IDC_TASK_SETTINGS_STREAMING_SEND_CAPTION,
						  (m_Settings.Streaming.GetStreams()&TSTask::STREAM_CAPTION)!=0);
				CheckItem(IDC_TASK_SETTINGS_STREAMING_SEND_DATA_CARROUSEL,
						  (m_Settings.Streaming.GetStreams()&TSTask::STREAM_DATA_CARROUSEL)!=0);
				CheckItem(IDC_TASK_SETTINGS_STREAMING_1SEG_ONLY,
						  (m_Settings.Streaming.GetStreams()&TSTask::STREAM_1SEG)!=0);

				SetItemUInt(IDC_TASK_SETTINGS_STREAMING_SEND_SIZE,m_Settings.Streaming.GetSendSize());
				SetItemUInt(IDC_TASK_SETTINGS_STREAMING_SEND_WAIT,m_Settings.Streaming.GetSendWait());
				CheckItem(IDC_TASK_SETTINGS_STREAMING_ADJUST_SEND_WAIT,m_Settings.Streaming.GetAdjustSendWait());
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CTaskStreamingSettingsDialog::OnOK(TSTask::CTSTaskSettings &Settings)
	{
		TSTask::String Text;

		Settings.Streaming.SetProtocol(
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_PROTOCOL_UDP)?
				TSTask::PROTOCOL_UDP:TSTask::PROTOCOL_TCP);
		if (GetItemString(IDC_TASK_SETTINGS_STREAMING_ADDRESS,&Text))
			Settings.Streaming.SetAddress(Text.c_str());
		Settings.Streaming.SetPort((WORD)GetItemUInt(IDC_TASK_SETTINGS_STREAMING_PORT));
		Settings.Streaming.SetFindUnusedPort(
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_FIND_UNUSED_PORT));

		Settings.Streaming.SetServiceSelectType(
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_CURRENT_SERVICE)?
				TSTask::SERVICE_SELECT_CURRENT:TSTask::SERVICE_SELECT_ALL);
		Settings.Streaming.SetStreamFlag(TSTask::STREAM_CAPTION,
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_SEND_CAPTION));
		Settings.Streaming.SetStreamFlag(TSTask::STREAM_DATA_CARROUSEL,
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_SEND_DATA_CARROUSEL));
		Settings.Streaming.SetStreamFlag(TSTask::STREAM_1SEG,
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_1SEG_ONLY));

		Settings.Streaming.SetSendSize(
			GetItemUInt(IDC_TASK_SETTINGS_STREAMING_SEND_SIZE));
		Settings.Streaming.SetSendWait(
			GetItemUInt(IDC_TASK_SETTINGS_STREAMING_SEND_WAIT));
		Settings.Streaming.SetAdjustSendWait(
			IsItemChecked(IDC_TASK_SETTINGS_STREAMING_ADJUST_SEND_WAIT));

		return true;
	}

}
