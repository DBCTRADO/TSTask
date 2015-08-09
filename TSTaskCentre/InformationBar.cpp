#include "stdafx.h"
#include "TSTaskCentre.h"
#include "InformationBar.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{
	enum
	{
		TIMER_ID_END_SINGLE=1
	};


	const LPCTSTR CInformationBar::m_pszWindowClass=APP_NAME_W TEXT("_InformationBar");
	HINSTANCE CInformationBar::m_hinst=nullptr;

	bool CInformationBar::Initialize(HINSTANCE hinst)
	{
		if (m_hinst==nullptr) {
			WNDCLASS wc;

			wc.style=CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc=WndProc;
			wc.cbClsExtra=0;
			wc.cbWndExtra=0;
			wc.hInstance=hinst;
			wc.hIcon=nullptr;
			wc.hCursor=::LoadCursor(nullptr,IDC_ARROW);
			wc.hbrBackground=nullptr;
			wc.lpszMenuName=nullptr;
			wc.lpszClassName=m_pszWindowClass;
			if (::RegisterClass(&wc)==0)
				return false;

			m_hinst=hinst;
		}

		return true;
	}

	bool CInformationBar::GetDefaultTheme(ThemeInfo *pTheme)
	{
		pTheme->BackgroundStyle.Fill.Type=Theme::FILL_GRADIENT;
		pTheme->BackgroundStyle.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
		pTheme->BackgroundStyle.Fill.Gradient.Color1.Set(32,32,32);
		pTheme->BackgroundStyle.Fill.Gradient.Color2.Set(48,48,48);
		pTheme->BackgroundStyle.Border.Type=Theme::BORDER_SOLID;
		pTheme->BackgroundStyle.Border.Color.Set(64,64,64);
		pTheme->ItemStyle.Background.Fill.Type=Theme::FILL_NONE;
		pTheme->ItemStyle.Background.Border.Type=Theme::BORDER_NONE;
		pTheme->ItemStyle.Foreground.Fill.Type=Theme::FILL_SOLID;
		pTheme->ItemStyle.Foreground.Fill.Solid.Color.Set(192,192,192);
		pTheme->ItemStyle.Foreground.Glow.Type=Theme::GLOW_NONE;
		pTheme->InfoStyle.Fill.Type=Theme::FILL_SOLID;
		pTheme->InfoStyle.Fill.Solid.Color.Set(255,255,255);
		pTheme->InfoStyle.Glow.Type=Theme::GLOW_NONE;
		pTheme->ErrorStyle.Fill.Type=Theme::FILL_SOLID;
		pTheme->ErrorStyle.Fill.Solid.Color.Set(255,64,0);
		pTheme->ErrorStyle.Glow.Type=Theme::GLOW_NONE;
		pTheme->WarningStyle.Fill.Type=Theme::FILL_SOLID;
		pTheme->WarningStyle.Fill.Solid.Color.Set(255,192,0);
		pTheme->WarningStyle.Glow.Type=Theme::GLOW_NONE;

		return true;
	}

	CInformationBar::CInformationBar(CTSTaskCentreCore &Core)
		: CLayeredWidget(Core.GetGraphicSystem())
		, m_Core(Core)
		, m_ItemHeight(0)
		, m_fFitItemWidth(true)
		, m_pEventHandler(nullptr)
	{
		GetDefaultTheme(&m_Theme);
		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_STATUS,&m_Font,true);

		m_ItemMargin=GetDefaultItemMargin();
	}

	CInformationBar::~CInformationBar()
	{
		Destroy();
		DeleteAllItems();
	}

	bool CInformationBar::Create(HWND hwndParent,int ID)
	{
		if (m_hwnd!=nullptr)
			return false;

		if (!CreateBasicWindow(hwndParent,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,
							   ID,m_pszWindowClass,nullptr,m_hinst))
			return false;

		return true;
	}

	bool CInformationBar::AddItem(CItem *pItem)
	{
		if (pItem==nullptr)
			return false;

		m_ItemList.push_back(pItem);

		return true;
	}

	CInformationBar::CItem *CInformationBar::GetItem(size_t Index)
	{
		if (Index>=m_ItemList.size())
			return nullptr;

		return m_ItemList[Index];
	}

	const CInformationBar::CItem *CInformationBar::GetItem(size_t Index) const
	{
		if (Index>=m_ItemList.size())
			return nullptr;

		return m_ItemList[Index];
	}

	bool CInformationBar::DeleteItem(size_t Index)
	{
		if (Index>=m_ItemList.size())
			return false;

		auto i=m_ItemList.begin();
		std::advance(i,Index);
		delete *i;
		m_ItemList.erase(i);

		return true;
	}

	void CInformationBar::DeleteAllItems()
	{
		for (auto e:m_ItemList)
			delete e;
		m_ItemList.clear();
	}

	void CInformationBar::UpdateItems(bool fRedraw)
	{
		for (auto e:m_ItemList)
			e->Update();

		if (fRedraw && m_hwnd!=nullptr) {
			Invalidate();
		}
	}

	void CInformationBar::SetSingleText(TextType Type,LPCWSTR pszText,DWORD Time)
	{
		if (TSTask::IsStringEmpty(pszText)) {
			if (m_SingleText.empty())
				return;
			m_SingleText.clear();
		} else {
			m_SingleTextType=Type;
			m_SingleText=pszText;
		}

		if (m_hwnd!=nullptr) {
			Invalidate();
			if (Time>0)
				::SetTimer(m_hwnd,TIMER_ID_END_SINGLE,Time,nullptr);
		}
	}

	bool CInformationBar::SetTheme(const ThemeInfo &Theme)
	{
		m_Theme=Theme;

		if (m_hwnd!=nullptr) {
			AdjustHeight();
			Invalidate();
		}

		return true;
	}

	bool CInformationBar::SetFont(const LOGFONT &Font)
	{
		m_Font=Font;

		if (m_hwnd!=nullptr) {
			CalcItemHeight();
			AdjustHeight();
			Invalidate();
		}

		return true;
	}

	bool CInformationBar::SetItemMargin(const RECT &Margin)
	{
		m_ItemMargin=Margin;
		return true;
	}

	RECT CInformationBar::GetItemMargin() const
	{
		return m_ItemMargin;
	}

	RECT CInformationBar::GetDefaultItemMargin() const
	{
		static const RECT rc={3,2,3,2};
		return rc;
	}

	bool CInformationBar::DrawItemPreview(CItem *pItem,HDC hdc,const RECT &Rect) const
	{
		if (pItem==nullptr || hdc==nullptr)
			return false;

		Graphics::CCanvas *pCanvas=m_GraphicSystem.CreateCanvas(hdc);
		Graphics::CFont *pFont=m_GraphicSystem.CreateFont(m_Font);
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);

		RECT rcItem=Rect;
		StylePainter.DrawBackground(m_Theme.BackgroundStyle,rcItem);
		m_Theme.BackgroundStyle.CalcContentRect(&rcItem);
		StylePainter.DrawBackground(m_Theme.ItemStyle.Background,rcItem);

		CItem::DrawInfo DrawInfo;
		DrawInfo.pSystem=&m_GraphicSystem;
		DrawInfo.pCanvas=pCanvas;
		DrawInfo.pStyle=&m_Theme.ItemStyle;
		DrawInfo.pFont=pFont;
		DrawInfo.pStylePainter=&StylePainter;

		RECT rcText=rcItem;
		rcText.left+=m_ItemMargin.left;
		rcText.right-=m_ItemMargin.right;

		pItem->Draw(DrawInfo,rcText,rcItem);

		delete pFont;
		delete pCanvas;

		return true;
	}

	LRESULT CInformationBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CREATE:
			{
				LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);

				CalcItemHeight();

				RECT rc;
				::SetRectEmpty(&rc);
				rc.bottom=m_ItemHeight;
				m_Theme.BackgroundStyle.CalcBoundingRect(&rc);
				::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
				::SetWindowPos(hwnd,nullptr,0,0,pcs->cx,rc.bottom-rc.top,
							   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
			}
			return 0;

		case WM_TIMER:
			if (wParam==TIMER_ID_END_SINGLE) {
				m_SingleText.clear();
				Invalidate();
				::KillTimer(hwnd,TIMER_ID_END_SINGLE);
			}
			return 0;

		case WM_LBUTTONDOWN:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnInformationBarLButtonDown(this,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_LBUTTONUP:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnInformationBarLButtonUp(this,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONDOWN:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnInformationBarRButtonDown(this,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONUP:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnInformationBarRButtonUp(this,GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;
		}

		return CLayeredWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CInformationBar::Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);
		Graphics::CFont *pFont=m_GraphicSystem.CreateFont(m_Font);

		RECT rcClient,rc;
		GetClientRect(&rcClient);
		StylePainter.DrawBackground(m_Theme.BackgroundStyle,rcClient);
		rc=rcClient;
		m_Theme.BackgroundStyle.CalcContentRect(&rc);

		if (!m_SingleText.empty()) {
			rc.left+=m_ItemMargin.left;
			rc.right-=m_ItemMargin.right;

			HICON hico;
			if (::LoadIconMetric(
					nullptr,
					m_SingleTextType==TEXT_TYPE_ERROR?
						IDI_ERROR:
					m_SingleTextType==TEXT_TYPE_WARNING?
						IDI_WARNING:
						IDI_INFORMATION,
					LIM_SMALL,&hico)==S_OK) {
				Graphics::CImage *pIcon=m_GraphicSystem.CreateImage();
				if (pIcon->CreateFromIcon(hico)) {
					int IconSize=pIcon->GetHeight();
					if (IconSize>rc.bottom-rc.top)
						IconSize=rc.bottom-rc.top;
					pCanvas->DrawImage(rc.left,rc.top+((rc.bottom-rc.top)-IconSize)/2,
									   IconSize,IconSize,
									   pIcon,0,0,pIcon->GetWidth(),pIcon->GetHeight());
					rc.left+=IconSize+IconSize/4;
				}
				delete pIcon;
				::DestroyIcon(hico);
			}

			StylePainter.DrawText(
				m_SingleTextType==TEXT_TYPE_ERROR?m_Theme.ErrorStyle:
				m_SingleTextType==TEXT_TYPE_WARNING?m_Theme.WarningStyle:
				m_Theme.InfoStyle,
				m_SingleText.c_str(),
				rc,rc,pFont,
				Graphics::TEXT_FORMAT_LEFT |
				Graphics::TEXT_FORMAT_VERTICAL_CENTER |
				Graphics::TEXT_FORMAT_NO_WRAP |
				Graphics::TEXT_FORMAT_END_ELLIPSIS);
		} else {
			CItem::DrawInfo DrawInfo;
			DrawInfo.pSystem=&m_GraphicSystem;
			DrawInfo.pCanvas=pCanvas;
			DrawInfo.pStyle=&m_Theme.ItemStyle;
			DrawInfo.pFont=pFont;
			DrawInfo.pStylePainter=&StylePainter;

			RECT rcItem=rc;

			for (auto i=m_ItemList.begin();i!=m_ItemList.end();++i) {
				rcItem.right=rcItem.left+(*i)->GetWidth()+m_ItemMargin.left+m_ItemMargin.right;
				if (m_fFitItemWidth && i+1==m_ItemList.end() && rcItem.right<rc.right)
					rcItem.right=rc.right;
				if (rcItem.left<PaintRect.right && rcItem.right>PaintRect.left) {
					StylePainter.DrawBackground(m_Theme.ItemStyle.Background,rcItem);
					RECT rcText=rcItem;
					rcText.left+=m_ItemMargin.left;
					rcText.right-=m_ItemMargin.right;
					(*i)->Draw(DrawInfo,rcText,rcItem);
				}
				rcItem.left=rcItem.right;
			}
		}

		delete pFont;
	}

	bool CInformationBar::IsTransparentBackground() const
	{
		return m_Theme.BackgroundStyle.IsTransparent();
	}

	void CInformationBar::CalcItemHeight()
	{
		if (m_hwnd==nullptr)
			return;

		HDC hdc=::GetDC(m_hwnd);
		TEXTMETRIC tm;
		Graphics::GetFontMetrics(hdc,m_Font,&tm);
		::ReleaseDC(m_hwnd,hdc);

		m_ItemHeight=tm.tmHeight+m_ItemMargin.top+m_ItemMargin.bottom;
	}

	void CInformationBar::AdjustHeight()
	{
		RECT rcClient,rc;
		GetClientRect(&rcClient);
		rc=rcClient;
		rc.bottom=m_ItemHeight;
		m_Theme.BackgroundStyle.CalcBoundingRect(&rc);
		rc.left=rcClient.left;
		rc.right=rcClient.right;
		::AdjustWindowRectEx(&rc,GetStyle(),FALSE,GetExStyle());
		::SetWindowPos(m_hwnd,nullptr,0,0,rc.right-rc.left,rc.bottom-rc.top,
					   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}


	CInformationBar::CItem::CItem(int ID)
		: m_ID(ID)
		, m_Width(DEFAULT_WIDTH)
		, m_TextAlign(TEXT_ALIGN_LEFT)
	{
	}

	bool CInformationBar::CItem::SetWidth(int Width)
	{
		if (Width<0)
			return false;

		m_Width=Width;

		return true;
	}

	bool CInformationBar::CItem::SetTextAlign(TextAlign Align)
	{
		if (Align<0 || Align>=TEXT_ALIGN_TRAILER)
			return false;

		m_TextAlign=Align;

		return true;
	}

	void CInformationBar::CItem::DrawText(const DrawInfo &Info,LPCTSTR pszText,
										  const RECT &TextRect,const RECT &ClipRect) const
	{
		if (TSTask::IsStringEmpty(pszText))
			return;

		Info.pStylePainter->DrawText(
			Info.pStyle->Foreground,pszText,
			TextRect,ClipRect,
			Info.pFont,
			(m_TextAlign==TEXT_ALIGN_LEFT?Graphics::TEXT_FORMAT_LEFT:
			 m_TextAlign==TEXT_ALIGN_RIGHT?Graphics::TEXT_FORMAT_RIGHT:
			 Graphics::TEXT_FORMAT_HORIZONTAL_CENTER) |
			Graphics::TEXT_FORMAT_VERTICAL_CENTER |
			Graphics::TEXT_FORMAT_NO_WRAP |
			Graphics::TEXT_FORMAT_END_ELLIPSIS);
	}

}
