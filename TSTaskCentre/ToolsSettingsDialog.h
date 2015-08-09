#ifndef TSTASKCENTRE_TOOLS_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TOOLS_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"
#include "ListView.h"


namespace TSTaskCentre
{

	class CToolsSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CToolsSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CToolsSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ÉcÅ[Éã"; }
		COLORREF GetTitleColor() const override { return RGB(128,0,192); }
		int GetIcon() const override { return ICON_TOOLS; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;

		void SetItemStatus();
		bool GetToolSettings(CToolsSettings::ToolInfo *pInfo) const;
		CToolsSettings::ToolInfo *GetToolInfo(int Index);
		void InsertToolItem(int Index,CToolsSettings::ToolInfo *pInfo,bool fSelect=false);
		bool UpdateToolItem(int Index,const CToolsSettings::ToolInfo &Info);

		bool m_fStateChanging;
		CListView m_ListView;
	};

}


#endif
