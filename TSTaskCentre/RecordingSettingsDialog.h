#ifndef TSTASKCENTRE_RECORDING_SETTINGS_DIALOG_H
#define TSTASKCENTRE_RECORDING_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"


namespace TSTaskCentre
{

	class CRecordingSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CRecordingSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CRecordingSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"˜^‰æ"; }
		COLORREF GetTitleColor() const override { return RGB(192,128,0); }
		int GetIcon() const override { return ICON_RECORDING; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;
	};

}


#endif
