#ifndef TSTASKCENTRE_CHANNEL_LIST_DIALOG_H
#define TSTASKCENTRE_CHANNEL_LIST_DIALOG_H


#include "Dialog.h"
#include "TSTaskManager.h"
#include "ListView.h"


namespace TSTaskCentre
{

	class CChannelListDialog : public CResizableDialog
	{
	public:
		CChannelListDialog(CTSTaskManager &Manager);
		~CChannelListDialog();
		bool Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID);

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CTSTaskManager &m_TSTaskManager;
		HINSTANCE m_hinst;
		TSTask::TaskID m_TaskID;
		CListView m_ListView;
	};

}


#endif
