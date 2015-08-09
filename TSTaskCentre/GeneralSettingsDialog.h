#ifndef TSTASKCENTRE_GENERAL_SETTINGS_DIALOG_H
#define TSTASKCENTRE_GENERAL_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"


namespace TSTaskCentre
{

	class CGeneralSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CGeneralSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CGeneralSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ˆê”Ê"; }
		COLORREF GetTitleColor() const override { return RGB(64,192,0); }
		int GetIcon() const override { return ICON_GENERAL; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;
		void OnCancel(const CTSTaskCentreSettings &Settings) override;

		void GetCurSelTheme(TSTask::String *pTheme) const;

		bool m_fThemeApplied;
	};

}


#endif
