#include "stdafx.h"
#include "TSTaskCentre.h"
#include "StatusBar.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CStatusBar::CItem::CItem(int ID,int DefaultWidth,int MinWidth)
		: m_pStatusBar(nullptr)
		, m_ID(ID)
		, m_DefaultWidth(DefaultWidth)
		, m_MinWidth(MinWidth)
		, m_fVisible(true)
		, m_fBreak(false)
	{
		m_Width=GetDefaultWidthPixels();
	}

	int CStatusBar::CItem::GetIndex() const
	{
		if (m_pStatusBar!=nullptr)
			return m_pStatusBar->IDToIndex(m_ID);
		return -1;
	}

	bool CStatusBar::CItem::GetRect(RECT *pRect) const
	{
		if (m_pStatusBar==nullptr)
			return false;
		return m_pStatusBar->GetItemRectByID(m_ID,pRect);
	}

	bool CStatusBar::CItem::GetClientRect(RECT *pRect) const
	{
		if (m_pStatusBar==nullptr)
			return false;
		return m_pStatusBar->GetItemClientRectByID(m_ID,pRect);
	}

	int CStatusBar::CItem::GetDefaultWidthPixels() const
	{
		if (m_DefaultWidth<0) {
			if (m_pStatusBar!=nullptr)
				return ::MulDiv(m_DefaultWidth,m_pStatusBar->GetFontHeight(),EM_UNIT);
			return ::MulDiv(m_DefaultWidth,12,EM_UNIT);
		}
		return m_DefaultWidth;
	}

	bool CStatusBar::CItem::SetWidth(int Width)
	{
		if (Width<0)
			m_Width=GetDefaultWidthPixels();
		else
			m_Width=max(Width,m_MinWidth);
		return true;
	}

	void CStatusBar::CItem::SetVisible(bool fVisible)
	{
		if (m_fVisible!=fVisible) {
			m_fVisible=fVisible;
			OnVisibleChange(fVisible && m_pStatusBar!=nullptr && m_pStatusBar->GetVisible());
		}
	}

	bool CStatusBar::CItem::Update() const
	{
		if (m_pStatusBar==nullptr)
			return false;
		return m_pStatusBar->UpdateItem(m_ID);
	}

	void CStatusBar::CItem::DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const
	{
		return Draw(Info,TextRect,ClipRect);
	}

	bool CStatusBar::CItem::GetMenuPos(POINT *pPos,UINT *pFlags)
	{
		if (m_pStatusBar==nullptr)
			return false;

		RECT rc;
		POINT pt;

		if (!GetRect(&rc))
			return false;
		if (pFlags!=nullptr)
			*pFlags=0;
		pt.x=rc.left;
		pt.y=rc.bottom;
		::ClientToScreen(m_pStatusBar->GetHandle(),&pt);
		HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONULL);
		if (hMonitor!=nullptr) {
			MONITORINFO mi;

			mi.cbSize=sizeof(mi);
			if (::GetMonitorInfo(hMonitor,&mi)) {
				if (pt.y>=mi.rcMonitor.bottom-32) {
					pt.x=rc.left;
					pt.y=rc.top;
					::ClientToScreen(m_pStatusBar->GetHandle(),&pt);
					if (pFlags!=nullptr)
						*pFlags=TPM_BOTTOMALIGN;
				}
			}
		}
		*pPos=pt;
		return true;
	}

	void CStatusBar::CItem::DrawText(const DrawInfo &Info,LPCTSTR pszText,
									 const RECT &TextRect,const RECT &ClipRect) const
	{
		if (TSTask::IsStringEmpty(pszText))
			return;

		Info.pStylePainter->DrawText(Info.pStyle->Foreground,pszText,
									 TextRect,ClipRect,
									 Info.pFont,
									 Graphics::TEXT_FORMAT_LEFT |
									 Graphics::TEXT_FORMAT_VERTICAL_CENTER |
									 Graphics::TEXT_FORMAT_NO_WRAP |
									 Graphics::TEXT_FORMAT_END_ELLIPSIS);
	}


	CStatusBar::CEventHandler::CEventHandler()
		: m_pStatusBar(nullptr)
	{
	}

	CStatusBar::CEventHandler::~CEventHandler()
	{
		if (m_pStatusBar!=nullptr)
			m_pStatusBar->SetEventHandler(nullptr);
	}


	const LPCTSTR CStatusBar::m_pszWindowClass=APP_NAME_W TEXT("_StatusBar");
	HINSTANCE CStatusBar::m_hinst=nullptr;

	bool CStatusBar::Initialize(HINSTANCE hinst)
	{
		if (m_hinst==nullptr) {
			WNDCLASS wc;

			wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			wc.lpfnWndProc=WndProc;
			wc.cbClsExtra=0;
			wc.cbWndExtra=0;
			wc.hInstance=hinst;
			wc.hIcon=nullptr;
			wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
			wc.hbrBackground=nullptr;
			wc.lpszMenuName=nullptr;
			wc.lpszClassName=m_pszWindowClass;
			if (::RegisterClass(&wc)==0)
				return false;

			m_hinst=hinst;
		}

		return true;
	}

	bool CStatusBar::GetDefaultTheme(ThemeInfo *pTheme)
	{
		if (pTheme==nullptr)
			return false;

		pTheme->BackgroundStyle.Fill.Type=Theme::FILL_NONE;
		pTheme->BackgroundStyle.Border.Type=Theme::BORDER_RAISED;
		pTheme->BackgroundStyle.Border.Color.Set(16,16,16);
		pTheme->RowBackgroundStyle.Fill.Type=Theme::FILL_GRADIENT;
		pTheme->RowBackgroundStyle.Fill.Gradient.Direction=Graphics::DIRECTION_VERTICAL;
		pTheme->RowBackgroundStyle.Fill.Gradient.Color1.Set(48,48,48);
		pTheme->RowBackgroundStyle.Fill.Gradient.Color2.Set(16,16,16);
		pTheme->RowBackgroundStyle.Border.Type=Theme::BORDER_NONE;
		pTheme->TopRowBackgroundStyle=pTheme->RowBackgroundStyle;
		pTheme->MiddleRowBackgroundStyle=pTheme->RowBackgroundStyle;
		pTheme->BottomRowBackgroundStyle=pTheme->RowBackgroundStyle;
		pTheme->ItemStyle.Background.Fill.Type=Theme::FILL_NONE;
		pTheme->ItemStyle.Background.Border.Type=Theme::BORDER_NONE;
		pTheme->ItemStyle.Foreground.Fill.Type=Theme::FILL_SOLID;
		pTheme->ItemStyle.Foreground.Fill.Solid.Color.Set(179,209,255);
		pTheme->HighlightItemStyle.Background.Fill.Type=Theme::FILL_GRADIENT;
		pTheme->HighlightItemStyle.Background.Fill.Gradient.Direction=Graphics::DIRECTION_VERTICAL;
		pTheme->HighlightItemStyle.Background.Fill.Gradient.Color1.Set(204,224,255);
		pTheme->HighlightItemStyle.Background.Fill.Gradient.Color2.Set(153,193,255);
		pTheme->HighlightItemStyle.Background.Border.Type=Theme::BORDER_SOLID;
		pTheme->HighlightItemStyle.Background.Border.Color.Set(102,163,255);
		pTheme->HighlightItemStyle.Foreground.Fill.Type=Theme::FILL_SOLID;
		pTheme->HighlightItemStyle.Foreground.Fill.Solid.Color.Set(0,0,0);
		pTheme->HighlightItemStyle.Foreground.Glow.Type=Theme::GLOW_FADEOUT;
		pTheme->HighlightItemStyle.Foreground.Glow.Color.Set(102,163,255,128);
		pTheme->HighlightItemStyle.Foreground.Glow.Radius=3;

		return true;
	}

	CStatusBar::CStatusBar(Graphics::CSystem &GraphicSystem)
		: CLayeredWidget(GraphicSystem)
		, m_FontHeight(0)
		, m_ItemHeight(0)
		, m_fMultiRow(false)
		, m_MaxRows(2)
		, m_Rows(1)
		, m_fSingleMode(false)
		, m_HotItem(-1)
		, m_fTrackMouseEvent(false)
		, m_fOnButtonDown(false)
		, m_pEventHandler(nullptr)
		, m_fAdjustSize(true)
	{
		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_STATUS,&m_Font,true);

		m_ItemMargin=GetDefaultItemMargin();

		GetDefaultTheme(&m_Theme);
	}

	CStatusBar::~CStatusBar()
	{
		Destroy();

		if (m_pEventHandler!=nullptr)
			m_pEventHandler->m_pStatusBar=nullptr;

		std::for_each(m_ItemList.begin(),m_ItemList.end(),
					  [](CItem *pItem) { delete pItem; });
		m_ItemList.clear();
	}

	bool CStatusBar::Create(HWND hwndParent,int ID)
	{
		if (m_hwnd!=nullptr)
			return false;

		if (!CreateBasicWindow(hwndParent,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,
							   ID,m_pszWindowClass,nullptr,m_hinst))
			return false;

		return true;
	}

	const CStatusBar::CItem *CStatusBar::GetItemByIndex(int Index) const
	{
		if (Index<0 || (size_t)Index>=m_ItemList.size())
			return nullptr;
		return m_ItemList[Index];
	}

	CStatusBar::CItem *CStatusBar::GetItemByIndex(int Index)
	{
		if (Index<0 || (size_t)Index>=m_ItemList.size())
			return nullptr;
		return m_ItemList[Index];
	}

	const CStatusBar::CItem *CStatusBar::GetItemByID(int ID) const
	{
		int Index=IDToIndex(ID);

		if (Index<0)
			return nullptr;
		return m_ItemList[Index];
	}

	CStatusBar::CItem *CStatusBar::GetItemByID(int ID)
	{
		int Index=IDToIndex(ID);

		if (Index<0)
			return nullptr;
		return m_ItemList[Index];
	}

	bool CStatusBar::AddItem(CItem *pItem)
	{
		if (pItem==nullptr)
			return false;

		m_ItemList.push_back(pItem);
		pItem->m_pStatusBar=this;

		return true;
	}

	int CStatusBar::IDToIndex(int ID) const
	{
		for (size_t i=0;i<m_ItemList.size();i++) {
			if (m_ItemList[i]->GetID()==ID)
				return (int)i;
		}
		return -1;
	}

	int CStatusBar::IndexToID(int Index) const
	{
		if (Index<0 || (size_t)Index>=m_ItemList.size())
			return -1;
		return m_ItemList[Index]->GetID();
	}

	bool CStatusBar::UpdateItem(int ID)
	{
		if (m_hwnd!=nullptr) {
			RECT rc;

			if (!GetItemRectByID(ID,&rc))
				return false;
			if (rc.right>rc.left)
				Invalidate(rc);
		}
		return true;
	}

	bool CStatusBar::GetItemRectByID(int ID,RECT *pRect) const
	{
		int Index;

		Index=IDToIndex(ID);
		if (Index<0)
			return false;
		return GetItemRectByIndex(Index,pRect);
	}

	bool CStatusBar::GetItemRectByIndex(int Index,RECT *pRect) const
	{
		if (Index<0 || (size_t)Index>=m_ItemList.size())
			return false;

		RECT rc;
		GetClientRect(&rc);
		m_Theme.BackgroundStyle.CalcContentRect(&rc);
		if (m_fMultiRow)
			rc.bottom=rc.top+m_ItemHeight;
		const int Margin=m_ItemMargin.left+m_ItemMargin.right;
		int Left=rc.left;
		const CItem *pItem;
		for (int i=0;i<Index;i++) {
			pItem=m_ItemList[i];
			if (m_fMultiRow && pItem->m_fBreak) {
				rc.left=Left;
				rc.top=rc.bottom;
				rc.bottom+=m_ItemHeight;
			} else if (pItem->GetVisible()) {
				rc.left+=pItem->GetWidth()+Margin;
			}
		}
		rc.right=rc.left;
		pItem=m_ItemList[Index];
		if (pItem->GetVisible())
			rc.right+=pItem->GetWidth()+Margin;
		*pRect=rc;

		return true;
	}

	bool CStatusBar::GetItemClientRectByID(int ID,RECT *pRect) const
	{
		RECT rc;

		if (!GetItemRectByID(ID,&rc))
			return false;
		if (rc.left<rc.right) {
			rc.left+=m_ItemMargin.left;
			rc.top+=m_ItemMargin.top;
			rc.right-=m_ItemMargin.right;
			rc.bottom-=m_ItemMargin.bottom;
		}
		*pRect=rc;
		return true;
	}

	int CStatusBar::GetItemHeight() const
	{
		RECT rc;

		if (m_fMultiRow)
			return m_ItemHeight;
		GetClientRect(&rc);
		m_Theme.BackgroundStyle.CalcContentRect(&rc);
		return rc.bottom-rc.top;
	}

	bool CStatusBar::SetItemMargin(const RECT &Margin)
	{
		m_ItemMargin=Margin;
		return true;
	}

	RECT CStatusBar::GetItemMargin() const
	{
		return m_ItemMargin;
	}

	RECT CStatusBar::GetDefaultItemMargin() const
	{
		static const RECT rc={3,3,3,3};
		return rc;
	}

	int CStatusBar::GetIntegralWidth() const
	{
		int Width;

		Width=0;
		for (size_t i=0;i<m_ItemList.size();i++) {
			if (m_ItemList[i]->GetVisible())
				Width+=m_ItemList[i]->GetWidth()+(m_ItemMargin.left+m_ItemMargin.right);
		}
		RECT rc={0,0,Width,0};
		m_Theme.BackgroundStyle.CalcContentRect(&rc);
		return rc.right-rc.left;
	}

	void CStatusBar::SetSingleText(LPCTSTR pszText)
	{
		if (pszText!=nullptr) {
			m_SingleText=pszText;
			m_fSingleMode=true;
			SetHotItem(-1);
		} else {
			if (!m_fSingleMode)
				return;
			m_SingleText.clear();
			m_fSingleMode=false;
		}
		if (m_hwnd!=nullptr)
			Redraw(nullptr,RDW_INVALIDATE | RDW_UPDATENOW);
	}

	bool CStatusBar::SetTheme(const ThemeInfo &Theme)
	{
		m_Theme=Theme;
		if (m_hwnd!=nullptr) {
			AdjustSize();
			Invalidate();
		}
		return true;
	}

	bool CStatusBar::GetTheme(ThemeInfo *pTheme) const
	{
		if (pTheme==nullptr)
			return false;
		*pTheme=m_Theme;
		return true;
	}

	bool CStatusBar::SetFont(const LOGFONT &Font)
	{
		m_Font=Font;
		if (m_hwnd!=nullptr) {
			CalcItemHeight();
			AdjustSize();
			Invalidate();
		}
		return true;
	}

	bool CStatusBar::GetFont(LOGFONT *pFont) const
	{
		*pFont=m_Font;
		return true;
	}

	int CStatusBar::GetCurItem() const
	{
		if (m_HotItem<0)
			return -1;
		return m_ItemList[m_HotItem]->GetID();
	}

	bool CStatusBar::SetMultiRow(bool fMultiRow)
	{
		if (m_fMultiRow!=fMultiRow) {
			m_fMultiRow=fMultiRow;
			if (m_hwnd!=nullptr)
				AdjustSize();
		}
		return true;
	}

	bool CStatusBar::SetMaxRows(int MaxRows)
	{
		if (MaxRows<1)
			return false;
		if (m_MaxRows!=MaxRows) {
			m_MaxRows=MaxRows;
			if (m_hwnd!=nullptr && m_fMultiRow && m_Rows>MaxRows)
				AdjustSize();
		}
		return true;
	}

	int CStatusBar::CalcHeight(int Width) const
	{
		int Rows=1;

		if (m_fMultiRow) {
			RECT rc={0,0,Width,0};
			m_Theme.BackgroundStyle.CalcContentRect(&rc);
			int RowWidth=0;
			for (size_t i=0;i<m_ItemList.size();i++) {
				const CItem *pItem=m_ItemList[i];

				if (pItem->GetVisible()) {
					const int ItemWidth=pItem->GetWidth()+(m_ItemMargin.left+m_ItemMargin.right);

					if (RowWidth==0) {
						RowWidth=ItemWidth;
					} else {
						if (RowWidth+ItemWidth>rc.right-rc.left && Rows<m_MaxRows) {
							Rows++;
							RowWidth=ItemWidth;
						} else {
							RowWidth+=ItemWidth;
						}
					}
				}
			}
		}

		RECT rcBorder={0,0,0,m_ItemHeight*Rows};
		m_Theme.BackgroundStyle.CalcBoundingRect(&rcBorder);
		return rcBorder.bottom-rcBorder.top;
	}

	bool CStatusBar::SetEventHandler(CEventHandler *pEventHandler)
	{
		if (m_pEventHandler!=nullptr)
			m_pEventHandler->m_pStatusBar=nullptr;
		if (pEventHandler!=nullptr)
			pEventHandler->m_pStatusBar=this;
		m_pEventHandler=pEventHandler;
		return true;
	}

	bool CStatusBar::SetItemOrder(const int *pOrderList)
	{
		std::vector<CItem*> NewList;
		size_t i,j;

		for (i=0;i<m_ItemList.size();i++) {
			CItem *pItem=GetItemByID(pOrderList[i]);

			if (pItem==nullptr)
				return false;
			NewList.push_back(pItem);
			for (j=0;j<i;j++) {
				if (NewList[i]==NewList[j])
					return false;
			}
		}
		m_ItemList=NewList;
		if (m_hwnd!=nullptr && !m_fSingleMode) {
			if (m_fMultiRow)
				AdjustSize();
			else
				Invalidate();
		}
		return true;
	}

	bool CStatusBar::DrawItemPreview(const CItem *pItem,HDC hdc,const RECT &Rect,
									 bool fHighlight,const LOGFONT *pFont) const
	{
		if (pItem==nullptr || hdc==nullptr)
			return false;

		Graphics::CCanvas *pCanvas=m_GraphicSystem.CreateCanvas(hdc);
		Graphics::CFont *pCFont=m_GraphicSystem.CreateFont(pFont!=nullptr?*pFont:m_Font);
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);

		CItem::DrawInfo DrawInfo;
		DrawInfo.pSystem=&m_GraphicSystem;
		DrawInfo.pCanvas=pCanvas;
		DrawInfo.pStyle=fHighlight?&m_Theme.HighlightItemStyle:&m_Theme.ItemStyle;
		DrawInfo.pFont=pCFont;
		DrawInfo.pStylePainter=&StylePainter;

		if (DrawInfo.pStyle->Background.IsTransparent()) {
			if (m_Theme.RowBackgroundStyle.IsTransparent())
				StylePainter.DrawBackground(m_Theme.BackgroundStyle,Rect);
			StylePainter.DrawBackground(m_Theme.RowBackgroundStyle,Rect);
		}

		RECT rc=Rect;
		StylePainter.DrawBackground(DrawInfo.pStyle->Background,rc);
		rc.left+=m_ItemMargin.left;
		rc.right-=m_ItemMargin.right;
		pItem->DrawPreview(DrawInfo,rc,Rect);

		delete pCFont;
		delete pCanvas;

		return true;
	}

	void CStatusBar::EnableSizeAdjustment(bool fEnable)
	{
		if (m_fAdjustSize!=fEnable) {
			m_fAdjustSize=fEnable;
			if (fEnable && m_hwnd!=nullptr)
				AdjustSize();
		}
	}

	LRESULT CStatusBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CREATE:
			{
				LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
				RECT rc;

				CalcItemHeight();

				::SetRectEmpty(&rc);
				rc.bottom=m_ItemHeight;
				m_Theme.BackgroundStyle.CalcBoundingRect(&rc);
				::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
				::SetWindowPos(hwnd,nullptr,0,0,pcs->cx,rc.bottom-rc.top,
							   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

				m_HotItem=-1;
				m_fTrackMouseEvent=false;
			}
			return 0;

		case WM_SIZE:
			if (m_fMultiRow)
				AdjustSize();
			return 0;

		case WM_MOUSEMOVE:
			{
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				if (::GetCapture()==hwnd) {
					GetItemRectByIndex(m_HotItem,&rc);
					x-=rc.left;
					m_ItemList[m_HotItem]->OnMouseMove(x,y);
				} else {
					if (m_fSingleMode)
						break;

					POINT pt;
					pt.x=x;
					pt.y=y;
					int i;
					for (i=0;i<(int)m_ItemList.size();i++) {
						if (!m_ItemList[i]->GetVisible())
							continue;
						GetItemRectByIndex(i,&rc);
						if (::PtInRect(&rc,pt))
							break;
					}
					if (i==(int)m_ItemList.size())
						i=-1;
					if (i!=m_HotItem)
						SetHotItem(i);
					// WM_MOUSELEAVE ‚ª‘—‚ç‚ê‚È‚­‚Ä‚à–³Œø‚É‚³‚ê‚éŽ–‚ª‚ ‚é‚æ‚¤‚¾
					/*if (!m_fTrackMouseEvent)*/ {
						TRACKMOUSEEVENT tme;

						tme.cbSize=sizeof(TRACKMOUSEEVENT);
						tme.dwFlags=TME_LEAVE;
						tme.hwndTrack=hwnd;
						if (TrackMouseEvent(&tme))
							m_fTrackMouseEvent=true;
					}
				}
			}
			return 0;

		case WM_MOUSELEAVE:
			m_fTrackMouseEvent=false;
			if (!m_fOnButtonDown) {
				if (m_HotItem>=0)
					SetHotItem(-1);
				if (m_pEventHandler)
					m_pEventHandler->OnMouseLeave();
			}
			return 0;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			if (m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				GetItemRectByIndex(m_HotItem,&rc);
				x-=rc.left;
				m_fOnButtonDown=true;
				switch (uMsg) {
				case WM_LBUTTONDOWN:
					m_ItemList[m_HotItem]->OnLButtonDown(x,y);
					break;
				case WM_RBUTTONDOWN:
					m_ItemList[m_HotItem]->OnRButtonDown(x,y);
					break;
				case WM_LBUTTONDBLCLK:
					m_ItemList[m_HotItem]->OnLButtonDoubleClick(x,y);
					break;
				}
				m_fOnButtonDown=false;
				if (!m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::ScreenToClient(hwnd,&pt);
					::GetClientRect(hwnd,&rc);
					if (::PtInRect(&rc,pt)) {
						::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(pt.x,pt.y));
					} else {
						SetHotItem(-1);
						if (m_pEventHandler)
							m_pEventHandler->OnMouseLeave();
					}
				}
			}
			return 0;

		case WM_LBUTTONUP:
			if (::GetCapture()==hwnd) {
				::ReleaseCapture();
			}
			return 0;

		case WM_MOUSEHOVER:
			if (m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				GetItemRectByIndex(m_HotItem,&rc);
				x-=rc.left;
				if (m_ItemList[m_HotItem]->OnMouseHover(x,y)) {
					TRACKMOUSEEVENT tme;
					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_HOVER;
					tme.hwndTrack=hwnd;
					tme.dwHoverTime=HOVER_DEFAULT;
					::TrackMouseEvent(&tme);
				}
			}
			return 0;

		case WM_SETCURSOR:
			if (LOWORD(lParam)==HTCLIENT) {
				if (m_HotItem>=0) {
					::SetCursor(::LoadCursor(nullptr,IDC_HAND));
					return TRUE;
				}
			}
			break;

		case WM_NOTIFY:
			if (m_HotItem>=0)
				return m_ItemList[m_HotItem]->OnNotifyMessage(reinterpret_cast<LPNMHDR>(lParam));
			break;
		}

		return CLayeredWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CStatusBar::CalcItemHeight()
	{
		HDC hdc=::GetDC(m_hwnd);
		TEXTMETRIC tm;
		Graphics::GetFontMetrics(hdc,m_Font,&tm);
		::ReleaseDC(m_hwnd,hdc);

		m_FontHeight=tm.tmHeight-tm.tmInternalLeading;
		m_ItemHeight=m_FontHeight+m_ItemMargin.top+m_ItemMargin.bottom;
	}

	void CStatusBar::SetHotItem(int Item)
	{
		if (Item<0 || (size_t)Item>=m_ItemList.size())
			Item=-1;
		if (m_HotItem!=Item) {
			int OldHotItem=m_HotItem;

			m_HotItem=Item;
			if (OldHotItem>=0) {
				m_ItemList[OldHotItem]->OnFocus(false);
				UpdateItem(IndexToID(OldHotItem));
			}
			if (m_HotItem>=0) {
				m_ItemList[m_HotItem]->OnFocus(true);
				UpdateItem(IndexToID(m_HotItem));
			}

			TRACKMOUSEEVENT tme;
			tme.cbSize=sizeof(TRACKMOUSEEVENT);
			tme.dwFlags=TME_HOVER;
			if (m_HotItem<0)
				tme.dwFlags|=TME_CANCEL;
			tme.hwndTrack=m_hwnd;
			tme.dwHoverTime=HOVER_DEFAULT;
			::TrackMouseEvent(&tme);
		}
	}

	void CStatusBar::Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);
		Graphics::CFont *pFont=m_GraphicSystem.CreateFont(m_Font);

		RECT rcClient,rc;

		GetClientRect(&rcClient);
		StylePainter.DrawBackground(m_Theme.BackgroundStyle,rcClient);
		rc=rcClient;
		m_Theme.BackgroundStyle.CalcContentRect(&rc);
		const int ItemHeight=m_fMultiRow?m_ItemHeight:rc.bottom-rc.top;

		if (m_fMultiRow)
			rc.bottom=rc.top+ItemHeight;

		RECT rcRow=rc;
		for (int i=0;i<m_Rows;i++) {
			const Theme::BackgroundStyle &Style=
				m_Rows==1?m_Theme.RowBackgroundStyle:
				i==0?m_Theme.TopRowBackgroundStyle:
				i+1==m_Rows?m_Theme.BottomRowBackgroundStyle:
				m_Theme.MiddleRowBackgroundStyle;

			StylePainter.DrawBackground(Style,rcRow);
			rcRow.top=rcRow.bottom;
			rcRow.bottom+=ItemHeight;
		}

		if (m_fSingleMode) {
			RECT rcText=rc;
			rcText.left+=m_ItemMargin.left;
			rcText.right-=m_ItemMargin.right;
			StylePainter.DrawText(m_Theme.ItemStyle.Foreground,m_SingleText.c_str(),
								  rcText,rc,pFont);
		} else {
			const int Left=rc.left;
			int Row=0;

			CItem::DrawInfo DrawInfo;
			DrawInfo.pSystem=&m_GraphicSystem;
			DrawInfo.pCanvas=pCanvas;
			DrawInfo.pFont=pFont;
			DrawInfo.pStylePainter=&StylePainter;

			rc.right=Left;

			for (size_t i=0;i<m_ItemList.size();i++) {
				const CItem *pItem=m_ItemList[i];

				if (pItem->GetVisible()) {
					rc.left=rc.right;
					rc.right=rc.left+pItem->GetWidth()+m_ItemMargin.left+m_ItemMargin.right;
					if (rc.right>PaintRect.left && rc.left<PaintRect.right) {
						const bool fHighlight=i==m_HotItem;
						const Theme::ItemStyle &Style=
							fHighlight?m_Theme.HighlightItemStyle:m_Theme.ItemStyle;
						RECT rcDraw=rc;

						StylePainter.DrawBackground(Style.Background,rcDraw);
						rcDraw.left+=m_ItemMargin.left;
						rcDraw.right-=m_ItemMargin.right;
						DrawInfo.pStyle=&Style;
						pItem->Draw(DrawInfo,rcDraw,rc);
					}
				}

				if (m_fMultiRow && pItem->m_fBreak) {
					rc.right=Left;
					rc.top=rc.bottom;
					rc.bottom+=ItemHeight;
					Row++;
				}
			}
		}

		delete pFont;
	}

	bool CStatusBar::IsTransparentBackground() const
	{
		return m_Theme.BackgroundStyle.IsTransparent();
	}

	void CStatusBar::AdjustSize()
	{
		if (!m_fAdjustSize)
			return;

		int OldRows=m_Rows;
		RECT rcWindow,rc;

		CalcRows();
		GetPosition(&rcWindow);
		::SetRectEmpty(&rc);
		rc.bottom=m_ItemHeight*m_Rows;
		m_Theme.BackgroundStyle.CalcBoundingRect(&rc);
		CalcPositionFromClientRect(&rc);
		int Height=rc.bottom-rc.top;
		if (Height!=rcWindow.bottom-rcWindow.top) {
			::SetWindowPos(m_hwnd,nullptr,0,0,rcWindow.right-rcWindow.left,Height,
						   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
			Invalidate();
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnHeightChanged(Height);
		} else if (m_Rows!=OldRows) {
			Invalidate();
		}
	}

	void CStatusBar::CalcRows()
	{
		if (m_fMultiRow) {
			RECT rc;
			int Rows,RowWidth;

			GetClientRect(&rc);
			m_Theme.BackgroundStyle.CalcContentRect(&rc);
			Rows=1;
			RowWidth=0;
			for (size_t i=0;i<m_ItemList.size();i++) {
				CItem *pItem=m_ItemList[i];

				pItem->m_fBreak=false;
				if (pItem->GetVisible()) {
					const int ItemWidth=pItem->GetWidth()+m_ItemMargin.left+m_ItemMargin.right;

					if (RowWidth==0) {
						RowWidth=ItemWidth;
					} else {
						if (RowWidth+ItemWidth>rc.right-rc.left && Rows<m_MaxRows) {
							m_ItemList[i-1]->m_fBreak=true;
							Rows++;
							RowWidth=ItemWidth;
						} else {
							RowWidth+=ItemWidth;
						}
					}
				}
			}
			m_Rows=Rows;
		} else {
			for (size_t i=0;i<m_ItemList.size();i++)
				m_ItemList[i]->m_fBreak=false;
			m_Rows=1;
		}
	}

}
