#ifndef TSTASKCENTRE_STATUS_BAR_SETTINGS_DIALOG_H
#define TSTASKCENTRE_STATUS_BAR_SETTINGS_DIALOG_H


#include "TSTaskCentreSettingsDialog.h"


namespace TSTaskCentre
{

	class CStatusBarSettingsDialog : public CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
	{
	public:
		CStatusBarSettingsDialog(CTSTaskCentreCore &Core,int PageID);
		~CStatusBarSettingsDialog();
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
	// CDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CSettingsDialog::CPage
		LPCWSTR GetTitle() const override { return L"ステータスバー"; }
		COLORREF GetTitleColor() const override { return RGB(0,128,192); }
		int GetIcon() const override { return ICON_STATUS_BAR; }

	// CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage
		bool OnOK(CTSTaskCentreSettings &Settings) override;

		static LRESULT CALLBACK ItemListSubclassProc(
			HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,
			UINT_PTR uIdSubclass,DWORD_PTR dwRefData);
		void InitListBox();
		void CalcTextWidth();
		void SetListHExtent();
		void DrawInsertMark(HWND hwndList,int Pos);
		bool GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect);
		bool IsCursorResize(HWND hwndList,int x,int y);

		typedef CMainBoardSettings::StatusItemInfo ItemInfo;

		CTSTaskBar m_TSTaskBar;
		CMainBoardSettings::StatusItemList m_ItemList;
		LOGFONT m_CurrentFont;
		int m_ItemMargin;
		int m_CheckWidth;
		int m_ItemHeight;
		int m_TextWidth;
		int m_DropInsertPos;
		UINT m_DragTimerID;
		bool m_fDragResize;
	};

}


#endif
