#include "stdafx.h"
#include "TSTaskCentre.h"
#include "SendMessageDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CSendMessageDialog::CSendMessageDialog(CTSTaskManager &Manager)
		: m_Manager(Manager)
		, m_TaskID(TSTask::INVALID_TASK_ID)
	{
	}

	CSendMessageDialog::~CSendMessageDialog()
	{
		Destroy();
	}

	bool CSendMessageDialog::Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID)
	{
		m_TaskID=TaskID;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SEND_MESSAGE))>=0;
	}

	INT_PTR CSendMessageDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			AdjustPos();
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_MESSAGE_CLEAR:
				SetItemText(IDC_MESSAGE_SEND_MESSAGE,TEXT(""));
				SetItemText(IDC_MESSAGE_RECEIVE_MESSAGE,TEXT(""));
				return TRUE;

			case IDC_MESSAGE_SEND:
				{
					TSTask::String Message;

					if (GetItemString(IDC_MESSAGE_SEND_MESSAGE,&Message) && !Message.empty()) {
						TSTask::CMessageTranslator Translator;
						TSTask::CMessage SendMessage,ReceiveMessage;

						if (!Translator.Parse(&SendMessage,Message.c_str())) {
							::MessageBox(hDlg,TEXT("メッセージを変換できません。"),nullptr,
										 MB_OK | MB_ICONEXCLAMATION);
							return TRUE;
						}

						SetItemText(IDC_MESSAGE_RECEIVE_MESSAGE,TEXT(""));

						if (IsItemChecked(IDC_MESSAGE_BROADCAST)) {
							if (!m_Manager.BroadcastMessage(&SendMessage)) {
								::MessageBox(hDlg,TEXT("メッセージの送信エラーが発生しました。"),nullptr,
											 MB_OK | MB_ICONEXCLAMATION);
							}
						} else {
							if (!m_Manager.SendMessage(m_TaskID,&SendMessage,&ReceiveMessage)) {
								::MessageBox(hDlg,TEXT("メッセージの送信エラーが発生しました。"),nullptr,
											 MB_OK | MB_ICONEXCLAMATION);
								return TRUE;
							}

							if (!Translator.Format(&ReceiveMessage,&Message)) {
								::MessageBox(hDlg,TEXT("返信をフォーマットできません。"),nullptr,
											 MB_OK | MB_ICONEXCLAMATION);
								return TRUE;
							}

							SetItemString(IDC_MESSAGE_RECEIVE_MESSAGE,Message);
						}
					}
				}
				return TRUE;

			case IDOK:
			case IDCANCEL:
				End(LOWORD(wParam));
				return TRUE;
			}
			return TRUE;
		}

		return FALSE;
	}

}
