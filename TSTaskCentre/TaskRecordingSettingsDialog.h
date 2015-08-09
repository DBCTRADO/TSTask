#ifndef TSTASKCENTRE_TASK_RECORDING_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TASK_RECORDING_SETTINGS_DIALOG_H


#include "TSTaskSettingsDialog.h"


namespace TSTaskCentre
{

	class CTaskRecordingSettingsDialog : public CTSTaskSettingsDialog::CTaskSettingsPage
	{
	public:
		CTaskRecordingSettingsDialog(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings);
		~CTaskRecordingSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
	// CDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"˜^‰æ"; }
		COLORREF GetTitleColor() const override { return RGB(192,128,0); }
		int GetIcon() const override { return ICON_RECORDING; }
		bool QueryOK() override;

	// CTSTaskSettingsDialog::CTaskSettingsPage
		bool OnOK(TSTask::CTSTaskSettings &Settings) override;

		bool GetFolderListItem(int Item,TSTask::String *pText);
		bool AddDummyFolderListItem();
		void SetFolderListItemStatus();

		static LRESULT CALLBACK EditSubclassProc(
			HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
			UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

		RECT m_rcEdit;
	};

}


#endif
