#ifndef TSTASKCENTRE_INFORMATION_BAR_SETTINGS_DIALOG_H
#define TSTASKCENTRE_INFORMATION_BAR_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"
#include "InformationBar.h"


namespace TSTaskCentre
{

	class CInformationBarSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CInformationBarSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CInformationBarSettingsDialog();

	// CDialog
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
	// CDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"èÓïÒÉoÅ["; }
		COLORREF GetTitleColor() const override { return RGB(192,160,0); }
		int GetIcon() const override { return ICON_INFORMATION_BAR; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;

		CMainBoardSettings::InformationBarItemInfo *GetSelectedItem();
		void UpdateSelectedItem();
		void SetItemInfo();
		void UpdateItemFormat(int Item);

		CInformationBar m_InformationBar;
		LOGFONT m_CurrentFont;
		int m_ItemMargin;
	};

}


#endif
