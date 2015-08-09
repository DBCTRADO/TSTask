#ifndef TSTASKCENTRE_LOG_DIALOG_H
#define TSTASKCENTRE_LOG_DIALOG_H


#include "Dialog.h"
#include "TSTaskCentreCore.h"
#include "ListView.h"


namespace TSTaskCentre
{

	class CLogDialog : public CResizableDialog
	{
	public:
		CLogDialog(CTSTaskCentreCore &Core);
		~CLogDialog();
		bool Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID);

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		bool SaveLog(LPCTSTR pszFileName) const;

		CTSTaskCentreCore &m_Core;
		HINSTANCE m_hinst;
		TSTask::TaskID m_TaskID;
		TSTask::LogList m_LogList;
		CListView m_ListView;
	};

}


#endif
