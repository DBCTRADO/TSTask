#ifndef TSTASKCENTRE_TASK_GENERAL_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TASK_GENERAL_SETTINGS_DIALOG_H


#include "TSTaskSettingsDialog.h"


namespace TSTaskCentre
{

	class CTaskGeneralSettingsDialog : public CTSTaskSettingsDialog::CTaskSettingsPage
	{
	public:
		CTaskGeneralSettingsDialog(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings);
		~CTaskGeneralSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ˆê”Ê"; }
		COLORREF GetTitleColor() const override { return RGB(64,192,0); }
		int GetIcon() const override { return ICON_GENERAL; }

	// CTSTaskSettingsDialog::CTaskSettingsPage
		bool OnOK(TSTask::CTSTaskSettings &Settings) override;
	};

}


#endif
