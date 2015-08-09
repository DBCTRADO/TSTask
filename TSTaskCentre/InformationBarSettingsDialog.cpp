#include "stdafx.h"
#include "TSTaskCentre.h"
#include "InformationBarSettingsDialog.h"
#include "InformationBarItems.h"
#include "MiscDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	static void SetFontInfo(HWND hDlg,int ID,const LOGFONT &Font)
	{
		WCHAR szText[LF_FACESIZE+32];

		HDC hdc=::GetDC(hDlg);
		TSTask::FormatString(szText,_countof(szText),L"%s, %d pt",
							 Font.lfFaceName,Graphics::GetFontPointSize(hdc,Font));
		::ReleaseDC(hDlg,hdc);
		::SetDlgItemText(hDlg,ID,szText);
	}

	static void SetTimeComboBox(HWND hDlg,int ID,unsigned int Min,unsigned int Max,unsigned int Cur)
	{
		int Sel=-1;
		WCHAR szText[16];
		unsigned int Time=Min;
		for (int i=0;Time<=Max;i++,Time++) {
			TSTask::FormatString(szText,_countof(szText),L"%u ïb",Time);
			::SendDlgItemMessage(hDlg,ID,CB_ADDSTRING,0,
								 reinterpret_cast<LPARAM>(szText));
			::SendDlgItemMessage(hDlg,ID,CB_SETITEMDATA,i,Time*1000);
			if (Cur==Time*1000)
				Sel=i;
		}
		if (Sel<0) {
			if (Cur%1000==0)
				TSTask::FormatString(szText,_countof(szText),L"(%u ïb)",Cur/1000);
			else
				TSTask::FormatString(szText,_countof(szText),L"(%u ms)",Cur);
			::SendDlgItemMessage(hDlg,ID,CB_INSERTSTRING,0,
								 reinterpret_cast<LPARAM>(szText));
			::SendDlgItemMessage(hDlg,ID,CB_SETITEMDATA,0,Cur);
			Sel=0;
		}
		::SendDlgItemMessage(hDlg,ID,CB_SETCURSEL,Sel,0);
	}


	CInformationBarSettingsDialog::CInformationBarSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
		, m_InformationBar(Core)
	{
	}

	CInformationBarSettingsDialog::~CInformationBarSettingsDialog()
	{
		Destroy();
	}

	bool CInformationBarSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_INFORMATION_BAR));
	}

	INT_PTR CInformationBarSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

				Settings.MainBoard.GetInformationBarFont(&m_CurrentFont);
				m_InformationBar.SetItemMargin(m_Core.ScaleDPI(m_InformationBar.GetDefaultItemMargin()));
				m_InformationBar.SetFont(m_CurrentFont);
				m_InformationBar.Create(hDlg);

				m_ItemMargin=MapDialogUnitX(2);
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
									 LB_SETITEMHEIGHT,0,m_InformationBar.GetHeight()+m_ItemMargin*2);

				CMainBoardSettings::InformationBarItemList ItemList;
				Settings.MainBoard.GetInformationBarItemList(&ItemList);
				for (size_t i=0;i<ItemList.size();i++) {
					CMainBoardSettings::InformationBarItemInfo *pItemInfo=
						new CMainBoardSettings::InformationBarItemInfo(ItemList[i]);
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
										 LB_ADDSTRING,0,reinterpret_cast<LPARAM>(pItemInfo));

					InformationBarItems::CCustomInformationItem *pItem=
						new InformationBarItems::CCustomInformationItem(0);
					pItem->SetFormat(pItemInfo->Format.c_str());
					m_InformationBar.AddItem(pItem);
				}

				static const LPCTSTR TextAlignList[] = {
					TEXT("ç∂äÒÇπ"),TEXT("âEäÒÇπ"),TEXT("íÜâõ"),
				};
				for (int i=0;i<_countof(TextAlignList);i++) {
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_TEXT_ALIGN,
										 CB_ADDSTRING,0,reinterpret_cast<LPARAM>(TextAlignList[i]));
				}

				::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH_SPIN,
									 UDM_SETRANGE32,0,10000);

				SetItemInfo();

				SetFontInfo(hDlg,IDC_SETTINGS_INFORMATION_BAR_FONT_INFO,m_CurrentFont);

				SetTimeComboBox(hDlg,IDC_SETTINGS_INFORMATION_BAR_UPDATE_INTERVAL,
								1,10,Settings.MainBoard.GetInformationBarUpdateInterval());

				CheckItem(IDC_SETTINGS_INFORMATION_BAR_SHOW_ERROR,
						  Settings.MainBoard.GetInformationBarShowError());
				SetTimeComboBox(hDlg,IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION,
								1,10,Settings.MainBoard.GetInformationBarErrorDuration());
				EnableItems(IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION_LABEL,
							IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION,
							Settings.MainBoard.GetInformationBarShowError());
			}
			return TRUE;

		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

				if (wParam==IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST) {
					switch (pdis->itemAction) {
					case ODA_DRAWENTIRE:
					case ODA_SELECT:
						{
							const bool fSelected=(pdis->itemState & ODS_SELECTED)!=0;

							::FillRect(pdis->hDC,&pdis->rcItem,
								reinterpret_cast<HBRUSH>(
									static_cast<INT_PTR>((fSelected?COLOR_HIGHLIGHT:COLOR_WINDOW)+1)));

							if ((int)pdis->itemID>=0) {
								const CMainBoardSettings::InformationBarItemInfo *pItemInfo=
									reinterpret_cast<const CMainBoardSettings::InformationBarItemInfo*>(pdis->itemData);
								InformationBarItems::CCustomInformationItem *pItem=
									static_cast<InformationBarItems::CCustomInformationItem*>(m_InformationBar.GetItem(pdis->itemID));
								if (pItem==nullptr)
									break;

								pItem->SetTextAlign(CInformationBar::CItem::TextAlign(pItemInfo->TextAlign));

								RECT rcItem=pdis->rcItem;
								::InflateRect(&rcItem,-m_ItemMargin,-m_ItemMargin);
								RECT rcMargin=m_InformationBar.GetItemMargin();
								rcItem.right=rcItem.left+
									(pItemInfo->Width<0?CInformationBar::CItem::DEFAULT_WIDTH:pItemInfo->Width)+
									rcMargin.left+rcMargin.right;

								m_InformationBar.DrawItemPreview(pItem,pdis->hDC,rcItem);
							}
						}
						if ((pdis->itemState & ODS_FOCUS)==0)
							break;

					case ODA_FOCUS:
						if ((pdis->itemState & ODS_NOFOCUSRECT)==0)
							::DrawFocusRect(pdis->hDC,&pdis->rcItem);
						break;
					}
				}
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST:
				if (HIWORD(wParam)==LBN_SELCHANGE) {
					SetItemInfo();
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_NEW_ITEM:
				{
					CMainBoardSettings::InformationBarItemInfo *pItemInfo=
						new CMainBoardSettings::InformationBarItemInfo;

					pItemInfo->TextAlign=0;

					LRESULT Index=::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
													   LB_ADDSTRING,0,reinterpret_cast<LPARAM>(pItemInfo));
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
										 LB_SETCURSEL,Index,0);

					InformationBarItems::CCustomInformationItem *pItem=
						new InformationBarItems::CCustomInformationItem(0);
					m_InformationBar.AddItem(pItem);

					SetItemInfo();
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_DELETE_ITEM:
				{
					LRESULT Sel=::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCURSEL,0,0);

					if (Sel>=0) {
						delete reinterpret_cast<CMainBoardSettings::InformationBarItemInfo*>(
							::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,Sel,0));
						::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_DELETESTRING,Sel,0);
						m_InformationBar.DeleteItem(Sel);
						SetItemInfo();
					}
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_MOVE_UP_ITEM:
			case IDC_SETTINGS_INFORMATION_BAR_MOVE_DOWN_ITEM:
				{
					HWND hwndList=GetItemHandle(IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST);
					bool fUp=LOWORD(wParam)==IDC_SETTINGS_INFORMATION_BAR_MOVE_UP_ITEM;
					LRESULT Sel=::SendMessage(hwndList,LB_GETCURSEL,0,0);
					LRESULT To;

					if (fUp) {
						if (Sel<1)
							return TRUE;
						To=Sel-1;
					} else {
						if (Sel<0 || Sel+1>=::SendMessage(hwndList,LB_GETCOUNT,0,0))
							return TRUE;
						To=Sel+1;
					}

					::SendMessage(hwndList,WM_SETREDRAW,FALSE,0);
					LRESULT ItemData=::SendMessage(hwndList,LB_GETITEMDATA,Sel,0);
					::SendMessage(hwndList,LB_DELETESTRING,Sel,0);
					::SendMessage(hwndList,LB_INSERTSTRING,To,ItemData);
					::SendMessage(hwndList,LB_SETCURSEL,To,0);
					UpdateItemFormat(int(Sel));
					UpdateItemFormat(int(To));
					::SendMessage(hwndList,WM_SETREDRAW,TRUE,0);
					::InvalidateRect(hwndList,nullptr,TRUE);
					SetItemInfo();
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_FORMAT:
				if (HIWORD(wParam)==EN_CHANGE) {
					CMainBoardSettings::InformationBarItemInfo *pItem=GetSelectedItem();

					if (pItem!=nullptr) {
						GetItemString(IDC_SETTINGS_INFORMATION_BAR_FORMAT,&pItem->Format);
						UpdateItemFormat((int)::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCURSEL,0,0));
						UpdateSelectedItem();
					}
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_FORMAT_MENU:
				{
					HMENU hmenu=::CreatePopupMenu();

					InformationBarItems::CCustomInformationItem::ParameterInfo Info;
					for (int i=0;InformationBarItems::CCustomInformationItem::GetParameterInfo(i,&Info);i++) {
						WCHAR szText[128];

						TSTask::FormatString(szText,_countof(szText),L"%s\t%s",
											 Info.pszParameter,Info.pszText);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
					}

					RECT rc;
					::GetWindowRect(GetItemHandle(IDC_SETTINGS_INFORMATION_BAR_FORMAT_MENU),&rc);
					int Result=::TrackPopupMenu(hmenu,TPM_RETURNCMD,rc.left,rc.bottom,0,hDlg,nullptr);
					::DestroyMenu(hmenu);

					if (Result>0
							&& InformationBarItems::CCustomInformationItem::GetParameterInfo(Result-1,&Info)) {
						HWND hwndEdit=GetItemHandle(IDC_SETTINGS_INFORMATION_BAR_FORMAT);
						DWORD Start,End;

						::SetFocus(hwndEdit);
						::SendMessage(hwndEdit,EM_GETSEL,
									  reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
						::SendMessage(hwndEdit,EM_REPLACESEL,
									  TRUE,reinterpret_cast<LPARAM>(Info.pszParameter));
						if (End<Start)
							Start=End;
						::SendMessage(hwndEdit,EM_SETSEL,
									  Start,Start+::lstrlen(Info.pszParameter));
					}
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_TEXT_ALIGN:
				if (HIWORD(wParam)==CBN_SELCHANGE) {
					CMainBoardSettings::InformationBarItemInfo *pItem=GetSelectedItem();

					if (pItem!=nullptr) {
						pItem->TextAlign=
							(int)::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_TEXT_ALIGN,CB_GETCURSEL,0,0);
						UpdateSelectedItem();
					}
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH:
				if (HIWORD(wParam)==EN_CHANGE) {
					CMainBoardSettings::InformationBarItemInfo *pItem=GetSelectedItem();

					if (pItem!=nullptr) {
						pItem->Width=GetItemInt(IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH);
						UpdateSelectedItem();
					}
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_CHOOSE_FONT:
				if (ChooseFontDialog(hDlg,&m_CurrentFont)) {
					SetFontInfo(hDlg,IDC_SETTINGS_INFORMATION_BAR_FONT_INFO,m_CurrentFont);
					m_InformationBar.SetFont(m_CurrentFont);
					::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
										 LB_SETITEMHEIGHT,0,m_InformationBar.GetHeight()+m_ItemMargin*2);
					::InvalidateRect(GetItemHandle(IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST),nullptr,TRUE);
				}
				return TRUE;

			case IDC_SETTINGS_INFORMATION_BAR_SHOW_ERROR:
				EnableItems(IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION_LABEL,
							IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION,
							IsItemChecked(IDC_SETTINGS_INFORMATION_BAR_SHOW_ERROR));
			}
			return TRUE;

		case WM_DESTROY:
			{
				int ItemCount=(int)::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCOUNT,0,0);
				for (int i=0;i<ItemCount;i++) {
					delete reinterpret_cast<CMainBoardSettings::InformationBarItemInfo*>(
						::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,i,0));
				}
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_RESETCONTENT,0,0);

				m_InformationBar.DeleteAllItems();
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CInformationBarSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		Settings.MainBoard.SetInformationBarFont(m_CurrentFont);

		CMainBoardSettings::InformationBarItemList ItemList;
		int ItemCount=(int)::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCOUNT,0,0);
		for (int i=0;i<ItemCount;i++) {
			CMainBoardSettings::InformationBarItemInfo *pItem=
				reinterpret_cast<CMainBoardSettings::InformationBarItemInfo*>(
					::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,i,0));
			ItemList.push_back(*pItem);
		}
		Settings.MainBoard.SetInformationBarItemList(ItemList);

		LRESULT Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_UPDATE_INTERVAL,CB_GETCURSEL,0,0);
		if (Sel>=0) {
			Settings.MainBoard.SetInformationBarUpdateInterval(
				(unsigned int)::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_UPDATE_INTERVAL,
												   CB_GETITEMDATA,Sel,0));
		}

		Settings.MainBoard.SetInformationBarShowError(
			IsItemChecked(IDC_SETTINGS_INFORMATION_BAR_SHOW_ERROR));

		Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION,CB_GETCURSEL,0,0);
		if (Sel>=0) {
			Settings.MainBoard.SetInformationBarErrorDuration(
				(unsigned int)::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ERROR_DURATION,
												   CB_GETITEMDATA,Sel,0));
		}

		return true;
	}

	CMainBoardSettings::InformationBarItemInfo *CInformationBarSettingsDialog::GetSelectedItem()
	{
		LRESULT Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCURSEL,0,0);
		if (Sel<0)
			return nullptr;
		return reinterpret_cast<CMainBoardSettings::InformationBarItemInfo*>(
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,Sel,0));
	}

	void CInformationBarSettingsDialog::UpdateSelectedItem()
	{
		LRESULT Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCURSEL,0,0);
		if (Sel>=0) {
			RECT rc;
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,
								 LB_GETITEMRECT,Sel,reinterpret_cast<LPARAM>(&rc));
			::InvalidateRect(GetItemHandle(IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST),&rc,TRUE);
		}
	}

	void CInformationBarSettingsDialog::SetItemInfo()
	{
		LRESULT Sel=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCURSEL,0,0);
		LRESULT ItemCount=::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETCOUNT,0,0);

		EnableItem(IDC_SETTINGS_INFORMATION_BAR_DELETE_ITEM,Sel>=0);
		EnableItem(IDC_SETTINGS_INFORMATION_BAR_MOVE_UP_ITEM,Sel>0);
		EnableItem(IDC_SETTINGS_INFORMATION_BAR_MOVE_DOWN_ITEM,Sel>=0 && Sel+1<ItemCount);
		EnableItems(IDC_SETTINGS_INFORMATION_BAR_FORMAT_LABEL,
					IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH_SPIN,
					Sel>=0);
		if (Sel>=0) {
			const CMainBoardSettings::InformationBarItemInfo *pItem=
				reinterpret_cast<const CMainBoardSettings::InformationBarItemInfo*>(
					::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,Sel,0));

			SetItemString(IDC_SETTINGS_INFORMATION_BAR_FORMAT,pItem->Format);
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_TEXT_ALIGN,
								 CB_SETCURSEL,pItem->TextAlign,0);
			SetItemInt(IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH,
					   pItem->Width<0?CInformationBar::CItem::DEFAULT_WIDTH:pItem->Width);
		} else {
			SetItemText(IDC_SETTINGS_INFORMATION_BAR_FORMAT,TEXT(""));
			::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_TEXT_ALIGN,
								 CB_SETCURSEL,-1,0);
			SetItemText(IDC_SETTINGS_INFORMATION_BAR_ITEM_WIDTH,TEXT(""));
		}
	}

	void CInformationBarSettingsDialog::UpdateItemFormat(int Item)
	{
		InformationBarItems::CCustomInformationItem *pItem=
			static_cast<InformationBarItems::CCustomInformationItem*>(m_InformationBar.GetItem(Item));
		if (pItem==nullptr)
			return;

		CMainBoardSettings::InformationBarItemInfo *pItemInfo=
			reinterpret_cast<CMainBoardSettings::InformationBarItemInfo*>(
				::SendDlgItemMessage(m_hDlg,IDC_SETTINGS_INFORMATION_BAR_ITEM_LIST,LB_GETITEMDATA,Item,0));

		pItem->SetFormat(pItemInfo->Format.c_str());
	}

}
