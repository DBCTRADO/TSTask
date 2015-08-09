#include "stdafx.h"
#include "TSTaskCentre.h"
#include "SettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	enum
	{
		WM_APP_SET_PAGE=WM_APP
	};

	static bool FillGradient(HDC hdc,const RECT &Rect,COLORREF Color1,COLORREF Color2)
	{
		if (hdc==nullptr
				|| Rect.left>=Rect.right || Rect.top>=Rect.bottom)
			return false;

		TRIVERTEX vert[2];
		GRADIENT_RECT rect={0,1};

		vert[0].x=Rect.left;
		vert[0].y=Rect.top;
		vert[0].Red=GetRValue(Color1)<<8;
		vert[0].Green=GetGValue(Color1)<<8;
		vert[0].Blue=GetBValue(Color1)<<8;
		vert[0].Alpha=0x0000;
		vert[1].x=Rect.right;
		vert[1].y=Rect.bottom;
		vert[1].Red=GetRValue(Color2)<<8;
		vert[1].Green=GetGValue(Color2)<<8;
		vert[1].Blue=GetBValue(Color2)<<8;
		vert[1].Alpha=0x0000;
		return ::GdiGradientFill(hdc,vert,2,&rect,1,GRADIENT_FILL_RECT_H)!=FALSE;
	}


	CSettingsDialog::CSettingsDialog(CTSTaskCentreCore &Core)
		: m_Core(Core)
		, m_StartPage(0)
		, m_CurrentPage(-1)
		, m_hinst(nullptr)
		, m_himlIcons(nullptr)
	{
	}

	CSettingsDialog::~CSettingsDialog()
	{
		Destroy();
		DeleteAllPages();
	}

	bool CSettingsDialog::Show(HWND hwndOwner,HINSTANCE hinst)
	{
		m_hinst=hinst;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS))==IDOK;
	}

	bool CSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		m_hinst=hinst;

		if (!CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS)))
			return false;

		m_Core.AddModelessDialog(this);

		return true;
	}

	bool CSettingsDialog::QueryOK()
	{
		for (auto e:m_PageList) {
			if (e->IsCreated()) {
				if (!e->QueryOK()) {
					return false;
				}
			}
		}

		return true;
	}

	bool CSettingsDialog::SetPage(int Page)
	{
		if (Page<0 || (size_t)Page>=m_PageList.size())
			return false;

		if (Page!=m_CurrentPage) {
			CPage *pPage=m_PageList[Page];

			if (!pPage->IsCreated()) {
				HCURSOR hcurOld=::SetCursor(::LoadCursor(nullptr,IDC_WAIT));

				if (!pPage->Create(m_hDlg,m_hinst))
					return false;

				RECT rc;
				::GetWindowRect(::GetDlgItem(m_hDlg,IDC_SETTINGS_PAGE_PLACE),&rc);
				::MapWindowPoints(nullptr,m_hDlg,reinterpret_cast<POINT*>(&rc),2);
				pPage->SetPosition(rc);

				::SetCursor(hcurOld);
			}

			if (m_CurrentPage>=0)
				m_PageList[m_CurrentPage]->SetVisible(false);
			pPage->SetVisible(true);
			m_CurrentPage=Page;

			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_LIST,LB_SETCURSEL,Page,0);
			::InvalidateRect(::GetDlgItem(m_hDlg,IDC_SETTINGS_TITLE),nullptr,TRUE);
		}

		return true;
	}

	void CSettingsDialog::DeleteAllPages()
	{
		for (auto e:m_PageList)
			delete e;
		m_PageList.clear();
	}

	INT_PTR CSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				::SetWindowText(hDlg,GetTitle());

				DeleteAllPages();
				for (int i=0;;i++) {
					CPage *pPage=CreatePage(i);
					if (pPage==nullptr)
						break;

					m_PageList.push_back(pPage);

					::SendDlgItemMessage(hDlg,IDC_SETTINGS_LIST,LB_ADDSTRING,0,i);
				}

				int Page=0;
				if (m_StartPage>=0 && m_StartPage<(int)m_PageList.size())
					Page=m_StartPage;
				else if (m_CurrentPage>=0 && m_CurrentPage<(int)m_PageList.size())
					Page=m_CurrentPage;
				m_CurrentPage=-1;

				if (SetPage(Page)) {
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_LIST,LB_SETCURSEL,Page,0);
				}

				m_himlIcons=::ImageList_LoadImage(m_hinst,MAKEINTRESOURCE(IDB_SETTINGS),16,1,
												  CLR_NONE,IMAGE_BITMAP,LR_CREATEDIBSECTION);

				LRESULT ItemHeight=::SendDlgItemMessage(hDlg,IDC_SETTINGS_LIST,LB_GETITEMHEIGHT,0,0);
				if (ItemHeight<16+8)
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_LIST,LB_SETITEMHEIGHT,0,16+8);

				AdjustPos();
			}
			return TRUE;

		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

				if (wParam==IDC_SETTINGS_LIST) {
					if (pdis->itemData>=m_PageList.size())
						break;

					const CPage *pPage=m_PageList[pdis->itemData];
					const bool fSelected=(pdis->itemState&ODS_SELECTED)!=0;
					COLORREF crText,crOldText;
					int OldBkMode;
					RECT rcBack,rcText;

					rcBack=pdis->rcItem;
					rcBack.left+=4+16+4;
					if (fSelected) {
						RECT rc=pdis->rcItem;
						rc.right=rcBack.left;
						::FillRect(pdis->hDC,&rc,
								   reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
						FillGradient(pdis->hDC,rcBack,
									 RGB(0,0,0),pPage->GetTitleColor());
						crText=RGB(255,255,255);
					} else {
						::FillRect(pdis->hDC,&pdis->rcItem,
								   reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
						crText=::GetSysColor(COLOR_WINDOWTEXT);
					}
					::ImageList_Draw(m_himlIcons,pPage->GetIcon(),pdis->hDC,
									 pdis->rcItem.left+4,
									 rcBack.top+((rcBack.bottom-rcBack.top)-16)/2,
									 ILD_TRANSPARENT);
					rcText=rcBack;
					rcText.left+=4;
					rcText.right-=4;
					crOldText=::SetTextColor(pdis->hDC,crText);
					OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
					::DrawText(pdis->hDC,pPage->GetTitle(),-1,
							   &rcText,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
					::SetTextColor(pdis->hDC,crOldText);
					::SetBkMode(pdis->hDC,OldBkMode);
					if ((pdis->itemState & (ODS_FOCUS | ODS_NOFOCUSRECT))==ODS_FOCUS) {
						::DrawFocusRect(pdis->hDC,&rcBack);
					}
				} else if (wParam==IDC_SETTINGS_TITLE) {
					if (m_CurrentPage<0 || (size_t)m_CurrentPage>=m_PageList.size())
						break;

					CPage *pPage=m_PageList[m_CurrentPage];
					RECT rc;

					FillGradient(pdis->hDC,pdis->rcItem,
								 RGB(0,0,0),pPage->GetTitleColor());
					LOGFONT lf;
					::GetObject(reinterpret_cast<HFONT>(::SendMessage(hDlg,WM_GETFONT,0,0)),sizeof(lf),&lf);
					lf.lfWeight=FW_BOLD;
					HFONT hfont=::CreateFontIndirect(&lf);
					HFONT hfontOld=SelectFont(pdis->hDC,hfont);
					COLORREF crOldText=::SetTextColor(pdis->hDC,RGB(255,255,255));
					int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
					rc.left=pdis->rcItem.left+2;
					rc.top=pdis->rcItem.top;
					rc.right=pdis->rcItem.right-2;
					rc.bottom=pdis->rcItem.bottom;
					::DrawText(pdis->hDC,pPage->GetTitle(),-1,
							   &rc,DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					SelectFont(pdis->hDC,hfontOld);
					::DeleteObject(hfont);
					::SetTextColor(pdis->hDC,crOldText);
					::SetBkMode(pdis->hDC,OldBkMode);
				}
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_SETTINGS_LIST:
				if (HIWORD(wParam)==LBN_SELCHANGE) {
					int Page=(int)::SendDlgItemMessage(hDlg,IDC_SETTINGS_LIST,LB_GETCURSEL,0,0);

					if (Page>=0)
						SetPage(Page);
				}
				return TRUE;

			case IDOK:
				if (QueryOK() && OnOK()) {
					End(IDOK);
				}
				return TRUE;

			case IDCANCEL:
				OnCancel();
				End(IDCANCEL);
				return TRUE;
			}
			return TRUE;

		case WM_APP_SET_PAGE:
			SetPage((int)wParam);
			return TRUE;

		case WM_DESTROY:
			{
				m_StartPage=m_CurrentPage;

				DeleteAllPages();

				if (m_himlIcons!=nullptr) {
					::ImageList_Destroy(m_himlIcons);
					m_himlIcons=nullptr;
				}

				m_Core.RemoveModelessDialog(this);
			}
			return TRUE;
		}

		return FALSE;
	}


	void CSettingsDialog::CPage::OnSettingError(int ID,LPCWSTR pszMessage)
	{
		HWND hwnd=::GetParent(m_hDlg);

		::SendMessage(hwnd,WM_APP_SET_PAGE,m_PageID,0);
		::MessageBox(hwnd,pszMessage,L"Ý’è",MB_OK | MB_ICONINFORMATION);
		if (ID>0)
			::SetFocus(::GetDlgItem(m_hDlg,ID));
	}

}
