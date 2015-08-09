#ifndef TSTASKCENTRE_TASK_INFO_DIALOG_H
#define TSTASKCENTRE_TASK_INFO_DIALOG_H


#include "Dialog.h"
#include "TSTaskManager.h"
#include "ListView.h"


namespace TSTaskCentre
{

	class CTaskInfoDialog : public CResizableDialog
	{
	public:
		CTaskInfoDialog(CTSTaskManager &Manager);
		~CTaskInfoDialog();
		bool Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID);

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void SetProcessInfo();

		CTSTaskManager &m_Manager;
		TSTask::TaskID m_TaskID;
		CListView m_ListView;
	};

}


#endif
