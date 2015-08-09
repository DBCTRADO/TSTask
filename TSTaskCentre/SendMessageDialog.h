#ifndef TSTASKCENTRE_SEND_MESSAGE_DIALOG_H
#define TSTASKCENTRE_SEND_MESSAGE_DIALOG_H


#include "Dialog.h"
#include "TSTaskManager.h"


namespace TSTaskCentre
{

	class CSendMessageDialog : public CDialog
	{
	public:
		CSendMessageDialog(CTSTaskManager &Manager);
		~CSendMessageDialog();
		bool Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID);

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CTSTaskManager &m_Manager;
		TSTask::TaskID m_TaskID;
	};

}


#endif
