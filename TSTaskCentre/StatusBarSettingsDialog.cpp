#include "stdafx.h"
#include "TSTaskCentre.h"
#include "StatusBarSettingsDialog.h"
#include "MiscDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	static const UINT TIMER_ID_UP	=1;
	static const UINT TIMER_ID_DOWN	=2;


	static void SetFontInfo(HWND hDlg,int ID,const LOGFONT &Font)
	{
		WCHAR szText[LF_FACESIZE+32];

		HDC hdc=::GetDC(hDlg);
		TSTask::FormatString(szText,_countof(szText),L"%s, %d pt",
							 Font.lfFaceName,Graphics::GetFontPointSize(hdc,Font));
		::ReleaseDC(hDlg,hdc);
		::SetDlgItemText(hDlg,ID,szText);
	}


	CStatusBarSettingsDialog::CStatusBarSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
		, m_TSTaskBar(Core.GetGraphicSystem())
	{
	}

	CStatusBarSettingsDialog::~CStatusBarSettingsDialog()
	{
		Destroy();
	}

	bool CStatusBarSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_STATUS_BAR));
	}

	INT_PTR CStatusBarSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

				Settings.MainBoard.GetStatusFont(&m_CurrentFont);

				m_TSTaskBar.SetItemMargin(m_Core.ScaleDPI(m_TSTaskBar.GetDefaultItemMargin()));
				m_TSTaskBar.Create(hDlg);
				m_TSTaskBar.SetFont(m_CurrentFont);

				Settings.MainBoard.GetStatusItemListSortedByOrder(&m_ItemList);
				for (size_t i=0;i<m_ItemList.size();i++) {
					m_TSTaskBar.SetItemWidth(m_ItemList[i].ID,m_ItemList[i].Width);
					m_TSTaskBar.SetItemVisible(m_ItemList[i].ID,m_ItemList[i].fVisible);
				}
				m_DropInsertPos=-1;
				m_DragTimerID=0;
				InitListBox();
				::SetWindowSubclass(GetItemHandle(IDC_SETTINGS_STATUS_BAR_ITEM_LIST),
									ItemListSubclassProc,1,reinterpret_cast<DWORD_PTR>(this));
				m_ItemMargin=MapDialogUnitX(2);
				m_CheckWidth=MapDialogUnitX(8);
				m_ItemHeight=m_TSTaskBar.GetItemHeight()+(1+m_ItemMargin)*2;
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_STATUS_BAR_ITEM_LIST,LB_SETITEMHEIGHT,0,m_ItemHeight);
				CalcTextWidth();
				SetListHExtent();

				CheckItem(IDC_SETTINGS_STATUS_BAR_MULTI_ROW,Settings.MainBoard.GetStatusMultiRow());
				for (int i=1;i<=8;i++) {
					TCHAR szText[16];
					TSTask::FormatString(szText,_countof(szText),L"%d s",i);
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_STATUS_BAR_MAX_ROWS,CB_ADDSTRING,0,
										 reinterpret_cast<LPARAM>(szText));
				}
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_STATUS_BAR_MAX_ROWS,CB_SETCURSEL,
									 Settings.MainBoard.GetStatusMaxRows()-1,0);

				SetFontInfo(hDlg,IDC_SETTINGS_STATUS_BAR_FONT_INFO,m_CurrentFont);
			}
			return TRUE;

		case WM_DRAWITEM:
			{
				const DRAWITEMSTRUCT *pdis=reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);

				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					if ((INT)pdis->itemID>=0) {
						const ItemInfo *pItemInfo=reinterpret_cast<const ItemInfo*>(pdis->itemData);
						int TextColor,BackColor;
						int OldBkMode;
						COLORREF crTextColor,crOldTextColor;
						RECT rc;

						if ((pdis->itemState&ODS_SELECTED)==0) {
							TextColor=COLOR_WINDOWTEXT;
							BackColor=COLOR_WINDOW;
						} else {
							TextColor=COLOR_HIGHLIGHTTEXT;
							BackColor=COLOR_HIGHLIGHT;
						}
						::FillRect(pdis->hDC,&pdis->rcItem,
							reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(BackColor+1)));
						OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
						crTextColor=::GetSysColor(TextColor);
						if (!pItemInfo->fVisible) {
							COLORREF crBackColor=::GetSysColor(BackColor);
							crTextColor=RGB((GetRValue(crTextColor)+GetRValue(crBackColor))/2,
											(GetGValue(crTextColor)+GetGValue(crBackColor))/2,
											(GetBValue(crTextColor)+GetBValue(crBackColor))/2);
						}
						crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);
						rc.left=pdis->rcItem.left+m_ItemMargin;
						rc.top=pdis->rcItem.top+m_ItemMargin;
						rc.right=rc.left+m_CheckWidth;
						rc.bottom=pdis->rcItem.bottom-m_ItemMargin;
						::DrawFrameControl(pdis->hDC,&rc,DFC_BUTTON,
										   DFCS_BUTTONCHECK | (pItemInfo->fVisible?DFCS_CHECKED:0));
						rc.left=pdis->rcItem.left+m_ItemMargin+m_CheckWidth+m_ItemMargin;
						rc.top=pdis->rcItem.top+m_ItemMargin;
						rc.right=rc.left+m_TextWidth;
						rc.bottom=pdis->rcItem.bottom-m_ItemMargin;
						CTSTaskBar::ItemInfo Info;
						CTSTaskBar::GetItemInfo(pItemInfo->ID,&Info);
						::DrawText(pdis->hDC,Info.pszText,-1,&rc,
								   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
						::SetBkMode(pdis->hDC,OldBkMode);
						::SetTextColor(pdis->hDC,crOldTextColor);

						RECT Margin=m_TSTaskBar.GetItemMargin();
						rc.left=rc.right+m_ItemMargin+1;
						rc.right=rc.left+
							(pItemInfo->Width>=0?
								pItemInfo->Width:m_TSTaskBar.GetItemDefaultWidthPixels(pItemInfo->ID))+
							Margin.left+Margin.right;
						rc.top=pdis->rcItem.top+
							((pdis->rcItem.bottom-pdis->rcItem.top)-m_TSTaskBar.GetItemHeight())/2;
						rc.bottom=rc.top+m_TSTaskBar.GetItemHeight();
						m_TSTaskBar.DrawItemPreview(pItemInfo->ID,pdis->hDC,rc);
						::InflateRect(&rc,1,1);
						HBRUSH hbr=::CreateSolidBrush(crTextColor);
						::FrameRect(pdis->hDC,&rc,hbr);
						::DeleteObject(hbr);

						if ((int)pdis->itemID==m_DropInsertPos
								|| (int)pdis->itemID+1==m_DropInsertPos) {
							::PatBlt(pdis->hDC,pdis->rcItem.left,
									 (int)pdis->itemID==m_DropInsertPos?
									 pdis->rcItem.top:pdis->rcItem.bottom-1,
									 pdis->rcItem.right-pdis->rcItem.left,1,DSTINVERT);
						}

						if ((pdis->itemState&ODS_FOCUS)==0)
							break;
					}
				case ODA_FOCUS:
					if ((pdis->itemState&ODS_NOFOCUSRECT)==0)
						::DrawFocusRect(pdis->hDC,&pdis->rcItem);
					break;
				}
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_SETTINGS_STATUS_BAR_DEFAULT:
				{
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_STATUS_BAR_ITEM_LIST,LB_RESETCONTENT,0,0);
					for (size_t i=0;i<m_ItemList.size();i++) {
						m_ItemList[i].Order=(int)i;
						m_ItemList[i].ID=(int)i;
						m_ItemList[i].Width=-1;
						m_ItemList[i].fVisible=true;
					}
					InitListBox();
					SetListHExtent();
				}
				return TRUE;

			case IDC_SETTINGS_STATUS_BAR_CHOOSE_FONT:
				if (ChooseFontDialog(hDlg,&m_CurrentFont)) {
					SetFontInfo(hDlg,IDC_SETTINGS_STATUS_BAR_FONT_INFO,m_CurrentFont);

					m_TSTaskBar.SetFont(m_CurrentFont);
					for (size_t i=0;i<m_ItemList.size();i++) {
						if (m_ItemList[i].Width<0) {
							const int ID=m_ItemList[i].ID;
							m_TSTaskBar.SetItemWidth(ID,m_TSTaskBar.GetItemDefaultWidthPixels(ID));
						}
					}
					m_ItemHeight=m_TSTaskBar.GetItemHeight()+(1+m_ItemMargin)*2;
					HWND hwndList=GetItemHandle(IDC_SETTINGS_STATUS_BAR_ITEM_LIST);
					ListBox_SetItemHeight(hwndList,0,m_ItemHeight);
					SetListHExtent();
					::InvalidateRect(hwndList,nullptr,TRUE);
				}
				return TRUE;
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CStatusBarSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		for (size_t i=0;i<m_ItemList.size();i++) {
			ItemInfo *pItemInfo=reinterpret_cast<ItemInfo*>(
				::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_STATUS_BAR_ITEM_LIST,LB_GETITEMDATA,i,0));

			if (pItemInfo!=nullptr) {
				CMainBoardSettings::StatusItemInfo Item;

				Item.ID=pItemInfo->ID;
				Item.Order=(int)i;
				Item.Width=pItemInfo->Width;
				Item.fVisible=pItemInfo->fVisible;
				Settings.MainBoard.SetStatusItemInfo(Item);
			}
		}

		Settings.MainBoard.SetStatusMultiRow(IsItemChecked(IDC_SETTINGS_STATUS_BAR_MULTI_ROW));

		int Sel=(int)::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_STATUS_BAR_MAX_ROWS,CB_GETCURSEL,0,0);
		if (Sel>=0)
			Settings.MainBoard.SetStatusMaxRows(Sel+1);

		Settings.MainBoard.SetStatusFont(m_CurrentFont);
		m_CurrentFont.lfWeight=FW_BOLD;
		Settings.MainBoard.SetStatusCaptionFont(m_CurrentFont);

		return true;
	}

	void CStatusBarSettingsDialog::InitListBox()
	{
		for (size_t i=0;i<m_ItemList.size();i++) {
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_STATUS_BAR_ITEM_LIST,
								 LB_ADDSTRING,0,reinterpret_cast<LPARAM>(&m_ItemList[i]));
		}
	}

	void CStatusBarSettingsDialog::CalcTextWidth()
	{
		HWND hwndList=GetItemHandle(IDC_SETTINGS_STATUS_BAR_ITEM_LIST);
		HDC hdc=::GetDC(hwndList);
		if (hdc==nullptr)
			return;

		HFONT hfontOld=SelectFont(hdc,GetWindowFont(hwndList));
		int MaxWidth=0;
		for (const auto &e:m_ItemList) {
			CTSTaskBar::ItemInfo Item;

			if (CTSTaskBar::GetItemInfo(e.ID,&Item)) {
				RECT rc={0,0,0,0};
				::DrawText(hdc,Item.pszText,-1,&rc,DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
				if (rc.right>MaxWidth)
					MaxWidth=rc.right;
			}
		}
		SelectFont(hdc,hfontOld);
		::ReleaseDC(hwndList,hdc);
		m_TextWidth=MaxWidth;
	}

	void CStatusBarSettingsDialog::SetListHExtent()
	{
		HWND hwndList=GetItemHandle(IDC_SETTINGS_STATUS_BAR_ITEM_LIST);
		int MaxWidth=0;

		for (const auto &e:m_ItemList) {
			int Width=e.Width>=0?e.Width:m_TSTaskBar.GetItemWidth(e.ID);
			if (Width>MaxWidth)
				MaxWidth=Width;
		}

		RECT Margin=m_TSTaskBar.GetItemMargin();
		MaxWidth+=Margin.left+Margin.right;

		ListBox_SetHorizontalExtent(hwndList,
			m_ItemMargin+m_CheckWidth+m_ItemMargin+m_TextWidth+m_ItemMargin+MaxWidth+2+m_ItemMargin);
	}

	static int ListBox_GetHitItem(HWND hwndList,int x,int y)
	{
		int Index=ListBox_GetTopIndex(hwndList)+y/ListBox_GetItemHeight(hwndList,0);
		if (Index<0 || Index>=ListBox_GetCount(hwndList))
			return -1;
		return Index;
	}

	static void ListBox_MoveItem(HWND hwndList,int From,int To)
	{
		int Top=ListBox_GetTopIndex(hwndList);
		LPARAM lData;

		lData=ListBox_GetItemData(hwndList,From);
		ListBox_DeleteString(hwndList,From);
		ListBox_InsertItemData(hwndList,To,lData);
		ListBox_SetCurSel(hwndList,To);
		ListBox_SetTopIndex(hwndList,Top);
	}

	static void ListBox_EnsureVisible(HWND hwndList,int Index)
	{
		int Top;

		Top=ListBox_GetTopIndex(hwndList);
		if (Index<Top) {
			ListBox_SetTopIndex(hwndList,Index);
		} else if (Index>Top) {
			RECT rc;
			int Rows;

			::GetClientRect(hwndList,&rc);
			Rows=rc.bottom/ListBox_GetItemHeight(hwndList,0);
			if (Rows==0)
				Rows=1;
			if (Index>=Top+Rows)
				ListBox_SetTopIndex(hwndList,Index-Rows+1);
		}
	}

	void CStatusBarSettingsDialog::DrawInsertMark(HWND hwndList,int Pos)
	{
		HDC hdc=GetDC(hwndList);
		RECT rc;

		::GetClientRect(hwndList,&rc);
		rc.top=(Pos-ListBox_GetTopIndex(hwndList))*m_ItemHeight-1;
		::PatBlt(hdc,0,rc.top,rc.right-rc.left,2,DSTINVERT);
		::ReleaseDC(hwndList,hdc);
	}

	bool CStatusBarSettingsDialog::GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect)
	{
		if (Index<0)
			return false;
		ItemInfo *pItemInfo=reinterpret_cast<ItemInfo*>(ListBox_GetItemData(hwndList,Index));
		RECT rc;
		ListBox_GetItemRect(hwndList,Index,&rc);
		::OffsetRect(&rc,-::GetScrollPos(hwndList,SB_HORZ),0);
		RECT Margin=m_TSTaskBar.GetItemMargin();
		rc.left+=m_ItemMargin+m_CheckWidth+m_ItemMargin+m_TextWidth+m_ItemMargin+1;
		rc.right=rc.left+
			(pItemInfo->Width>=0?
				pItemInfo->Width:
				m_TSTaskBar.GetItemDefaultWidthPixels(pItemInfo->ID))+
			Margin.left+Margin.right;
		rc.top+=m_ItemMargin+1;
		rc.bottom-=m_ItemMargin+1;
		*pRect=rc;
		return true;
	}

	bool CStatusBarSettingsDialog::IsCursorResize(HWND hwndList,int x,int y)
	{
		RECT rc;

		if (!GetItemPreviewRect(hwndList,ListBox_GetHitItem(hwndList,x,y),&rc))
			return false;
		int Margin=::GetSystemMetrics(SM_CXSIZEFRAME);
		return x>=rc.right-Margin && x<=rc.right+Margin;
	}

	LRESULT CALLBACK CStatusBarSettingsDialog::ItemListSubclassProc(
		HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
	{
		CStatusBarSettingsDialog *pThis=reinterpret_cast<CStatusBarSettingsDialog*>(dwRefData);

		switch (uMsg) {
		case WM_LBUTTONDOWN:
			{
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				int Sel;

				::SetFocus(hwnd);
				Sel=ListBox_GetHitItem(hwnd,x,y);
				if (Sel>=0) {
					RECT rc;

					ListBox_GetItemRect(hwnd,Sel,&rc);
					OffsetRect(&rc,-GetScrollPos(hwnd,SB_HORZ),0);
					if (x>=rc.left+pThis->m_ItemMargin && x<rc.left+pThis->m_ItemMargin+pThis->m_CheckWidth) {
						ItemInfo *pItemInfo=reinterpret_cast<ItemInfo*>(ListBox_GetItemData(hwnd,Sel));
						RECT rc;

						pItemInfo->fVisible=!pItemInfo->fVisible;
						ListBox_GetItemRect(hwnd,Sel,&rc);
						::InvalidateRect(hwnd,&rc,TRUE);
					} else {
						if (ListBox_GetCurSel(hwnd)!=Sel)
							ListBox_SetCurSel(hwnd,Sel);
						::SetCapture(hwnd);
						pThis->m_fDragResize=pThis->IsCursorResize(hwnd,x,y);
					}
				}
			}
			return 0;

		case WM_LBUTTONUP:
			if (::GetCapture()==hwnd)
				::ReleaseCapture();
			return 0;

		case WM_CAPTURECHANGED:
			if (pThis->m_DragTimerID!=0) {
				::KillTimer(hwnd,pThis->m_DragTimerID);
				pThis->m_DragTimerID=0;
			}
			if (pThis->m_DropInsertPos>=0) {
				int From=ListBox_GetCurSel(hwnd),To;

				pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
				To=pThis->m_DropInsertPos;
				if (To>From)
					To--;
				SetWindowRedraw(hwnd,FALSE);
				ListBox_MoveItem(hwnd,From,To);
				SetWindowRedraw(hwnd,TRUE);
				pThis->m_DropInsertPos=-1;
			}
			return 0;

		case WM_MOUSEMOVE:
			if (::GetCapture()==hwnd) {
				int y=GET_Y_LPARAM(lParam);
				RECT rc;

				::GetClientRect(hwnd,&rc);
				if (pThis->m_fDragResize) {
					int Sel=ListBox_GetCurSel(hwnd);

					if (pThis->GetItemPreviewRect(hwnd,Sel,&rc)) {
						ItemInfo *pItemInfo=reinterpret_cast<ItemInfo*>(ListBox_GetItemData(hwnd,Sel));
						int x=GET_X_LPARAM(lParam);
						RECT Margin=pThis->m_TSTaskBar.GetItemMargin();
						int Width=(x-rc.left)-(Margin.left+Margin.right);
						int MinWidth=pThis->m_TSTaskBar.GetItemMinWidth(pItemInfo->ID);
						if (Width<MinWidth)
							Width=MinWidth;
						if (pItemInfo->Width<0 || pItemInfo->Width!=Width) {
							pItemInfo->Width=Width;
							ListBox_GetItemRect(hwnd,Sel,&rc);
							::InvalidateRect(hwnd,&rc,TRUE);
							pThis->SetListHExtent();
						}
					}
				} else if (y>=0 && y<rc.bottom) {
					int Insert,Count,Sel;

					if (pThis->m_DragTimerID!=0) {
						::KillTimer(hwnd,pThis->m_DragTimerID);
						pThis->m_DragTimerID=0;
					}
					Insert=ListBox_GetTopIndex(hwnd)+
						(y+pThis->m_ItemHeight/2)/pThis->m_ItemHeight;
					Count=ListBox_GetCount(hwnd);
					if (Insert>Count) {
						Insert=Count;
					} else {
						Sel=ListBox_GetCurSel(hwnd);
						if (Insert==Sel || Insert==Sel+1)
							Insert=-1;
					}
					if (pThis->m_DropInsertPos>=0)
						pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
					pThis->m_DropInsertPos=Insert;
					if (pThis->m_DropInsertPos>=0)
						pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
					::SetCursor(::LoadCursor(NULL,IDC_ARROW));
				} else {
					UINT TimerID;

					if (pThis->m_DropInsertPos>=0) {
						pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
						pThis->m_DropInsertPos=-1;
					}
					if (y<0)
						TimerID=TIMER_ID_UP;
					else
						TimerID=TIMER_ID_DOWN;
					if (TimerID!=pThis->m_DragTimerID) {
						if (pThis->m_DragTimerID!=0)
							KillTimer(hwnd,pThis->m_DragTimerID);
						pThis->m_DragTimerID=(UINT)SetTimer(hwnd,TimerID,100,NULL);
					}
					::SetCursor(::LoadCursor(NULL,IDC_NO));
				}
			} else {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

				::SetCursor(::LoadCursor(NULL,pThis->IsCursorResize(hwnd,x,y)?IDC_SIZEWE:IDC_ARROW));
			}
			return 0;

		case WM_RBUTTONDOWN:
			if (::GetCapture()==hwnd) {
				::ReleaseCapture();
				if (pThis->m_DragTimerID!=0) {
					::KillTimer(hwnd,pThis->m_DragTimerID);
					pThis->m_DragTimerID=0;
				}
				if (pThis->m_DropInsertPos>=0) {
					pThis->DrawInsertMark(hwnd,pThis->m_DropInsertPos);
					pThis->m_DropInsertPos=-1;
				}
			}
			return 0;

		case WM_TIMER:
			{
				int Pos;

				Pos=ListBox_GetTopIndex(hwnd);
				if (wParam==TIMER_ID_UP) {
					if (Pos>0)
						Pos--;
				} else
					Pos++;
				ListBox_SetTopIndex(hwnd,Pos);
			}
			return 0;

		case WM_SETCURSOR:
			if (LOWORD(lParam)==HTCLIENT)
				return TRUE;
			break;
		}

		return ::DefSubclassProc(hwnd,uMsg,wParam,lParam);
	}

}
