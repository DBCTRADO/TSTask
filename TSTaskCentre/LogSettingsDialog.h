#ifndef TSTASKCENTRE_LOG_SETTINGS_DIALOG_H
#define TSTASKCENTRE_LOG_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"
#include "ListView.h"


namespace TSTaskCentre
{

	class CLogSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CLogSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CLogSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ÉçÉO"; }
		COLORREF GetTitleColor() const override { return RGB(192,0,128); }
		int GetIcon() const override { return ICON_LOG; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;

		HINSTANCE m_hinst;
		TSTask::LogList m_LogList;
		CListView m_ListView;
	};

}


#endif
