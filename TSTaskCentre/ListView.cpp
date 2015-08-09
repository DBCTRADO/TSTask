#include "stdafx.h"
#include "ListView.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{


CListView::CListView()
	: m_hwnd(nullptr)
{
}


CListView::~CListView()
{
}


bool CListView::Attach(HWND hwnd)
{
	if (hwnd==nullptr)
		return false;

	m_hwnd=hwnd;

	return true;
}


void CListView::Detach()
{
	m_hwnd=nullptr;
}


void CListView::SetExtendedStyle(DWORD Style)
{
	if (m_hwnd!=nullptr)
		ListView_SetExtendedListViewStyle(m_hwnd,Style);
}


bool CListView::InitCheckList()
{
	if (m_hwnd==nullptr)
		return false;

	ListView_SetExtendedListViewStyle(m_hwnd,
		LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

	RECT rc;
	::GetClientRect(m_hwnd,&rc);
	LVCOLUMN lvc;
	lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt=LVCFMT_LEFT;
	lvc.cx=rc.right-::GetSystemMetrics(SM_CXVSCROLL);
	lvc.pszText=TEXT("");
	lvc.iSubItem=0;
	ListView_InsertColumn(m_hwnd,0,&lvc);

	return true;
}


bool CListView::SetTheme(LPCWSTR pszName)
{
	if (m_hwnd==nullptr)
		return false;

	return ::SetWindowTheme(m_hwnd,pszName,nullptr)==S_OK;
}


int CListView::InsertItem(int Index,LPCTSTR pszText,LPARAM Param)
{
	if (m_hwnd==nullptr)
		return -1;

	if (Index<0)
		Index=GetItemCount();

	LVITEM lvi;

	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.pszText=const_cast<LPTSTR>(pszText);
	lvi.lParam=Param;

	return ListView_InsertItem(m_hwnd,&lvi);
}


bool CListView::DeleteItem(int Index)
{
	if (m_hwnd==nullptr)
		return false;
	return ListView_DeleteItem(m_hwnd,Index)!=FALSE;
}


void CListView::DeleteAllItems()
{
	if (m_hwnd!=nullptr)
		ListView_DeleteAllItems(m_hwnd);
}


bool CListView::SetItemText(int Index,LPCTSTR pszText)
{
	return SetItemText(Index,0,pszText);
}


bool CListView::SetItemText(int Index,int SubItem,LPCTSTR pszText)
{
	if (m_hwnd==nullptr)
		return false;

	ListView_SetItemText(m_hwnd,Index,SubItem,const_cast<LPTSTR>(pszText));

	return true;
}


bool CListView::GetItemText(int Index,LPTSTR pszText,int MaxText) const
{
	return GetItemText(Index,0,pszText,MaxText);
}


bool CListView::GetItemText(int Index,int SubItem,LPTSTR pszText,int MaxText) const
{
	if (pszText==nullptr || MaxText<1)
		return false;

	pszText[0]=_T('\0');

	if (m_hwnd==nullptr)
		return false;

	ListView_GetItemText(m_hwnd,Index,SubItem,pszText,MaxText);

	return true;
}


bool CListView::SetItemState(int Index,UINT State,UINT Mask)
{
	if (m_hwnd==nullptr)
		return false;

	ListView_SetItemState(m_hwnd,Index,State,Mask);

	return true;
}


bool CListView::CheckItem(int Index,bool fCheck)
{
	if (m_hwnd==nullptr)
		return false;

	ListView_SetCheckState(m_hwnd,Index,fCheck);

	return true;
}


bool CListView::IsItemChecked(int Index) const
{
	if (m_hwnd==nullptr)
		return false;

	return ListView_GetCheckState(m_hwnd,Index)!=FALSE;
}


bool CListView::SetItemParam(int Index,LPARAM Param)
{
	if (m_hwnd==nullptr)
		return false;

	LVITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.lParam=Param;

	return ListView_SetItem(m_hwnd,&lvi)!=FALSE;
}


LPARAM CListView::GetItemParam(int Index) const
{
	if (m_hwnd==nullptr)
		return 0;

	LVITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;

	if (!ListView_GetItem(m_hwnd,&lvi))
		return 0;

	return lvi.lParam;
}


bool CListView::SetItemImage(int Index,int SubItem,int Image)
{
	if (m_hwnd==nullptr)
		return false;

	LVITEM lvi;

	lvi.mask=LVIF_IMAGE;
	lvi.iItem=Index;
	lvi.iSubItem=SubItem;
	lvi.iImage=Image;

	return ListView_SetItem(m_hwnd,&lvi)!=FALSE;
}


int CListView::GetItemCount() const
{
	if (m_hwnd==nullptr)
		return 0;

	return ListView_GetItemCount(m_hwnd);
}


void CListView::ReserveItemCount(int Count)
{
	if (m_hwnd!=nullptr)
		ListView_SetItemCount(m_hwnd,Count);
}


int CListView::GetSelectedItem() const
{
	if (m_hwnd==nullptr)
		return -1;

	return ListView_GetNextItem(m_hwnd,-1,LVNI_SELECTED);
}


bool CListView::MoveItem(int From,int To)
{
	if (m_hwnd==nullptr)
		return false;

	LVITEM lvi;
	TCHAR szText[1024];
	lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=From;
	lvi.iSubItem=0;
	lvi.stateMask=~0U;
	lvi.pszText=szText;
	lvi.cchTextMax=_countof(szText);
	if (!ListView_GetItem(m_hwnd,&lvi))
		return false;
	BOOL fChecked=ListView_GetCheckState(m_hwnd,From);
	ListView_DeleteItem(m_hwnd,From);
	lvi.iItem=To;
	ListView_InsertItem(m_hwnd,&lvi);
	ListView_SetCheckState(m_hwnd,To,fChecked);

	return true;
}


bool CListView::EnsureItemVisible(int Index,bool fPartialOK)
{
	if (m_hwnd==nullptr)
		return false;

	return ListView_EnsureVisible(m_hwnd,Index,fPartialOK)!=FALSE;
}


int CListView::InsertColumn(int Index,LPCTSTR pszText,int Format)
{
	if (m_hwnd==nullptr)
		return false;

	LVCOLUMN lvc;

	lvc.mask=LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt=Format;
	lvc.pszText=const_cast<LPTSTR>(pszText);
	lvc.iSubItem=Index;

	return ListView_InsertColumn(m_hwnd,Index,&lvc);
}


void CListView::SetColumnWidth(int Column,int Width)
{
	if (m_hwnd==nullptr)
		return;

	ListView_SetColumnWidth(m_hwnd,Column,Width);
}


void CListView::AdjustColumnWidth(bool fUseHeader)
{
	if (m_hwnd==nullptr)
		return;

	const int Count=GetColumnCount();
	for (int i=0;i<Count;i++)
		ListView_SetColumnWidth(m_hwnd,i,fUseHeader?LVSCW_AUTOSIZE_USEHEADER:LVSCW_AUTOSIZE);
}


void CListView::AdjustColumnWidth(int Column,bool fUseHeader)
{
	if (m_hwnd==nullptr)
		return;

	ListView_SetColumnWidth(m_hwnd,Column,fUseHeader?LVSCW_AUTOSIZE_USEHEADER:LVSCW_AUTOSIZE);
}


int CListView::GetColumnCount() const
{
	if (m_hwnd==nullptr)
		return 0;

	return Header_GetItemCount(ListView_GetHeader(m_hwnd));
}


bool CListView::SetImageList(HIMAGELIST himl,int Type)
{
	if (m_hwnd==nullptr)
		return false;

	ListView_SetImageList(m_hwnd,himl,Type);

	return true;
}


}
