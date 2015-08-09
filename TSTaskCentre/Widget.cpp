#include "stdafx.h"
#include "TSTaskCentre.h"
#include "Widget.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CWidget::CWidget()
		: m_hwnd(nullptr)
	{
		m_WindowPosition.Left=0;
		m_WindowPosition.Top=0;
		m_WindowPosition.Width=0;
		m_WindowPosition.Height=0;
		m_WindowPosition.fMaximized=false;
	}

	CWidget::~CWidget()
	{
		Destroy();
	}

	void CWidget::Destroy()
	{
		if (m_hwnd!=nullptr) {
			::DestroyWindow(m_hwnd);
			m_hwnd=nullptr;
		}
	}

	bool CWidget::SetPosition(const Position &Pos)
	{
		if (Pos.Width<0 || Pos.Height<0)
			return false;

		if (m_hwnd!=nullptr) {
			::MoveWindow(m_hwnd,Pos.Left,Pos.Top,Pos.Width,Pos.Height,TRUE);
		} else {
			m_WindowPosition=Pos;
		}
		return true;
	}

	bool CWidget::SetPosition(int Left,int Top,int Width,int Height)
	{
		Position Pos={Left,Top,Width,Height,false};
		return SetPosition(Pos);
	}

	bool CWidget::SetPosition(const RECT &Position)
	{
		return SetPosition(Position.left,Position.top,
						   Position.right-Position.left,
						   Position.bottom-Position.top);
	}

	void CWidget::GetPosition(Position *pPosition) const
	{
		if (m_hwnd!=nullptr) {
			RECT rc;

			if ((GetWindowStyle(m_hwnd)&WS_CHILD)!=0) {
				::GetWindowRect(m_hwnd,&rc);
				::MapWindowPoints(nullptr,::GetParent(m_hwnd),reinterpret_cast<POINT*>(&rc),2);
				pPosition->Left=rc.left;
				pPosition->Top=rc.top;
				pPosition->Width=rc.right-rc.left;
				pPosition->Height=rc.bottom-rc.top;
				pPosition->fMaximized=false;
			} else {
				WINDOWPLACEMENT wp;

				wp.length=sizeof(WINDOWPLACEMENT);
				::GetWindowPlacement(m_hwnd,&wp);
				if (wp.showCmd==SW_SHOWNORMAL) {
					::GetWindowRect(m_hwnd,&rc);
					pPosition->fMaximized=false;
				} else {
					if ((GetWindowExStyle(m_hwnd)&WS_EX_TOOLWINDOW)==0) {
						HMONITOR hMonitor=::MonitorFromWindow(m_hwnd,MONITOR_DEFAULTTONEAREST);
						MONITORINFO mi;

						mi.cbSize=sizeof(MONITORINFO);
						::GetMonitorInfo(hMonitor,&mi);
						::OffsetRect(&wp.rcNormalPosition,
									 mi.rcWork.left-mi.rcMonitor.left,
									 mi.rcWork.top-mi.rcMonitor.top);
					}
					rc=wp.rcNormalPosition;
					pPosition->fMaximized=wp.showCmd==SW_SHOWMAXIMIZED;
				}
				pPosition->Left=rc.left;
				pPosition->Top=rc.top;
				pPosition->Width=rc.right-rc.left;
				pPosition->Height=rc.bottom-rc.top;
			}
		} else {
			*pPosition=m_WindowPosition;
		}
	}

	void CWidget::GetPosition(RECT *pPosition) const
	{
		Position Pos;

		GetPosition(&Pos);
		::SetRect(pPosition,Pos.Left,Pos.Top,Pos.Left+Pos.Width,Pos.Top+Pos.Height);
	}

	int CWidget::GetWidth() const
	{
		Position Pos;

		GetPosition(&Pos);
		return Pos.Width;
	}

	int CWidget::GetHeight() const
	{
		Position Pos;

		GetPosition(&Pos);
		return Pos.Height;
	}

	// 最小化されているウィンドウ(またはその子ウィンドウ)で呼んじゃダ・メ
	bool CWidget::GetScreenPosition(RECT *pPosition) const
	{
		if (m_hwnd==nullptr) {
			GetPosition(pPosition);
			return true;
		}
		return ::GetWindowRect(m_hwnd,pPosition)!=FALSE;
	}

	void CWidget::SetVisible(bool fVisible)
	{
		if (m_hwnd!=nullptr)
			::ShowWindow(m_hwnd,fVisible?SW_SHOW:SW_HIDE);
	}

	bool CWidget::GetVisible() const
	{
		return m_hwnd!=nullptr && ::IsWindowVisible(m_hwnd);
	}

	bool CWidget::SetMaximize(bool fMaximize)
	{
		if (m_hwnd!=nullptr) {
			::ShowWindow(m_hwnd,fMaximize?SW_MAXIMIZE:SW_RESTORE);
		} else {
			m_WindowPosition.fMaximized=fMaximize;
		}
		return true;
	}

	bool CWidget::GetMaximize() const
	{
		if (m_hwnd!=nullptr)
			return ::IsZoomed(m_hwnd)!=FALSE;
		return m_WindowPosition.fMaximized;
	}

	bool CWidget::Invalidate(bool fErase) const
	{
		return m_hwnd!=nullptr && ::InvalidateRect(m_hwnd,nullptr,fErase);
	}

	bool CWidget::Invalidate(const RECT &Rect,bool fErase) const
	{
		return m_hwnd!=nullptr && ::InvalidateRect(m_hwnd,&Rect,fErase);
	}

	bool CWidget::Update() const
	{
		return m_hwnd!=nullptr && ::UpdateWindow(m_hwnd);
	}

	bool CWidget::Redraw(const RECT *pRect,UINT Flags) const
	{
		return m_hwnd!=nullptr && ::RedrawWindow(m_hwnd,pRect,nullptr,Flags);
	}

	bool CWidget::GetClientRect(RECT *pRect) const
	{
		return m_hwnd!=nullptr && ::GetClientRect(m_hwnd,pRect);
	}

	bool CWidget::GetClientSize(SIZE *pSize) const
	{
		RECT rc;

		if (m_hwnd==nullptr || !::GetClientRect(m_hwnd,&rc))
			return false;
		pSize->cx=rc.right;
		pSize->cy=rc.bottom;
		return true;
	}

	bool CWidget::CalcPositionFromClientRect(RECT *pRect) const
	{
		if (m_hwnd==nullptr)
			return false;
		return ::AdjustWindowRectEx(pRect,GetStyle(),FALSE,GetExStyle())!=FALSE;
	}

	bool CWidget::SetParent(HWND hwnd)
	{
		return m_hwnd!=nullptr && ::SetParent(m_hwnd,hwnd);
	}

	bool CWidget::SetParent(CWidget *pWidget)
	{
		return m_hwnd!=nullptr && ::SetParent(m_hwnd,pWidget->m_hwnd);
	}

	HWND CWidget::GetParent() const
	{
		if (m_hwnd==nullptr)
			return nullptr;
		return ::GetParent(m_hwnd);
	}

	DWORD CWidget::GetStyle() const
	{
		if (m_hwnd==nullptr)
			return 0;
		return ::GetWindowLong(m_hwnd,GWL_STYLE);
	}

	bool CWidget::SetStyle(DWORD Style,bool fFrameChange)
	{
		if (m_hwnd==nullptr)
			return false;
		::SetWindowLong(m_hwnd,GWL_STYLE,Style);
		if (fFrameChange)
			::SetWindowPos(m_hwnd,nullptr,0,0,0,0,
				SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		return true;
	}

	DWORD CWidget::GetExStyle() const
	{
		if (m_hwnd==nullptr)
			return 0;
		return ::GetWindowLong(m_hwnd,GWL_EXSTYLE);
	}

	bool CWidget::SetExStyle(DWORD ExStyle,bool fFrameChange)
	{
		if (m_hwnd==nullptr)
			return false;
		::SetWindowLong(m_hwnd,GWL_EXSTYLE,ExStyle);
		if (fFrameChange)
			::SetWindowPos(m_hwnd,nullptr,0,0,0,0,
				SWP_FRAMECHANGED | SWP_DRAWFRAME | SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		return true;
	}

	LRESULT CWidget::SendMessage(UINT Msg,WPARAM wParam,LPARAM lParam)
	{
		if (m_hwnd==nullptr)
			return 0;
		return ::SendMessage(m_hwnd,Msg,wParam,lParam);
	}

	bool CWidget::PostMessage(UINT Msg,WPARAM wParam,LPARAM lParam)
	{
		if (m_hwnd==nullptr)
			return false;
		return ::PostMessage(m_hwnd,Msg,wParam,lParam)!=FALSE;
	}

	bool CWidget::SendSizeMessage()
	{
		if (m_hwnd==nullptr)
			return false;

		RECT rc;
		if (!::GetClientRect(m_hwnd,&rc))
			return false;
		::SendMessage(m_hwnd,WM_SIZE,0,MAKELONG(rc.right,rc.bottom));
		return true;
	}

	bool CWidget::MoveToMonitorInside()
	{
		RECT rc;
		HMONITOR hMonitor;
		MONITORINFO mi;

		GetPosition(&rc);
		hMonitor=::MonitorFromRect(&rc,MONITOR_DEFAULTTONEAREST);
		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		if (rc.left>=mi.rcMonitor.right || rc.top>=mi.rcMonitor.bottom
				|| rc.right<=mi.rcMonitor.left || rc.bottom<=mi.rcMonitor.top) {
			int XOffset=0,YOffset=0;

			if (rc.left>=mi.rcMonitor.right)
				XOffset=mi.rcMonitor.right-rc.right;
			else if (rc.right<=mi.rcMonitor.left)
				XOffset=mi.rcMonitor.left-rc.left;
			if (rc.top>=mi.rcMonitor.bottom)
				YOffset=mi.rcMonitor.bottom-rc.bottom;
			else if (rc.bottom<=mi.rcMonitor.top)
				YOffset=mi.rcMonitor.top-rc.top;
			::OffsetRect(&rc,XOffset,YOffset);
			SetPosition(rc);
			return true;
		}
		return false;
	}


	CCustomWidget::CCustomWidget()
	{
	}

	CCustomWidget::~CCustomWidget()
	{
		Destroy();
	}

	LRESULT CALLBACK CCustomWidget::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		CCustomWidget *pThis;

		if (uMsg==WM_NCCREATE) {
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			pThis=static_cast<CCustomWidget*>(pcs->lpCreateParams);

			pThis->m_hwnd=hwnd;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		} else {
			pThis=static_cast<CCustomWidget*>(GetThisFromHandle(hwnd));
			if (pThis==nullptr)
				return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
			if (uMsg==WM_NCDESTROY) {
				pThis->OnMessage(hwnd,uMsg,wParam,lParam);
				pThis->GetPosition(&pThis->m_WindowPosition);
				::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(nullptr));
				pThis->m_hwnd=nullptr;
				return 0;
			}
		}
		return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
	}

	CCustomWidget *CCustomWidget::GetThisFromHandle(HWND hwnd)
	{
		return reinterpret_cast<CCustomWidget*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
	}

	LRESULT CCustomWidget::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

	bool CCustomWidget::CreateBasicWindow(HWND hwndParent,DWORD Style,DWORD ExStyle,INT_PTR ID,
										  LPCTSTR pszClassName,LPCTSTR pszText,HINSTANCE hinst)
	{
		if (m_hwnd!=nullptr)
			return false;
		m_hwnd=::CreateWindowEx(ExStyle,pszClassName,pszText,Style,
			m_WindowPosition.Left,m_WindowPosition.Top,
			m_WindowPosition.Width,m_WindowPosition.Height,
			hwndParent,reinterpret_cast<HMENU>(ID),hinst,this);
		return m_hwnd!=nullptr;
	}

}
