#ifndef TSTASKCENTRE_TASK_STREAMING_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TASK_STREAMING_SETTINGS_DIALOG_H


#include "TSTaskSettingsDialog.h"


namespace TSTaskCentre
{

	class CTaskStreamingSettingsDialog : public CTSTaskSettingsDialog::CTaskSettingsPage
	{
	public:
		CTaskStreamingSettingsDialog(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings);
		~CTaskStreamingSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ストリーミング"; }
		COLORREF GetTitleColor() const override { return RGB(0,192,128); }
		int GetIcon() const override { return ICON_STREAMING; }

	// CTSTaskSettingsDialog::CTaskSettingsPage
		bool OnOK(TSTask::CTSTaskSettings &Settings) override;
	};

}


#endif
