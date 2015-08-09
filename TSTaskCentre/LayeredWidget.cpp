#include "stdafx.h"
#include "TSTaskCentre.h"
#include "LayeredWidget.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CLayeredWidget::CLayeredWidget(Graphics::CSystem &GraphicSystem)
		: m_GraphicSystem(GraphicSystem)
		, m_pOffscreen(nullptr)
	{
	}

	CLayeredWidget::~CLayeredWidget()
	{
		Destroy();
		DeleteOffscreen();
	}

	LRESULT CLayeredWidget::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_PAINT:
			{
				::PAINTSTRUCT ps;

				::BeginPaint(hwnd,&ps);
				Draw(ps.hdc,ps.rcPaint);
				::EndPaint(hwnd,&ps);
			}
			return 0;

		case WM_APP_DRAW:
			{
				const DrawInfo *pInfo=reinterpret_cast<const DrawInfo*>(lParam);

				if (DrawOffscreen(pInfo->PaintRect,pInfo->fDrawParent)) {
					pInfo->pCanvas->DrawImage(
						pInfo->DstPos.x,pInfo->DstPos.y,
						pInfo->PaintRect.right-pInfo->PaintRect.left,
						pInfo->PaintRect.bottom-pInfo->PaintRect.top,
						m_pOffscreen->GetImage(),
						pInfo->PaintRect.left,pInfo->PaintRect.top,
						pInfo->PaintRect.right-pInfo->PaintRect.left,
						pInfo->PaintRect.bottom-pInfo->PaintRect.top);
					return TRUE;
				}
			}
			return FALSE;

		case WM_APP_DELETE_OFFSCREEN:
			DeleteOffscreen();
			return TRUE;

		case WM_DESTROY:
			DeleteOffscreen();
			return 0;
		}

		return CCustomWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CLayeredWidget::Draw(HDC hdc,const RECT &PaintRect)
	{
		Graphics::CCanvas *pCanvas=m_GraphicSystem.CreateCanvas(hdc);

		RECT rcClient;
		GetClientRect(&rcClient);

		if (m_GraphicSystem.CreateOffscreen(&m_pOffscreen,rcClient.right,rcClient.bottom)) {
			Graphics::CCanvas *pOffscreenCanvas=m_pOffscreen->GetCanvas();

			pOffscreenCanvas->SetClip(PaintRect);
			if (IsTransparentBackground()) {
				pOffscreenCanvas->Clear();
				DrawParent(pOffscreenCanvas,PaintRect);
			}
			Draw(pOffscreenCanvas,PaintRect);
			pOffscreenCanvas->ResetClip();
			m_pOffscreen->Transfer(pCanvas,PaintRect);
		} else {
			pCanvas->SetClip(PaintRect);
			if (IsTransparentBackground())
				pCanvas->Clear();
			Draw(pCanvas,PaintRect);
			pCanvas->ResetClip();
		}

		delete pCanvas;
	}

	bool CLayeredWidget::DrawOffscreen(const RECT &PaintRect,bool fDrawParent)
	{
		RECT rcClient;
		GetClientRect(&rcClient);

		if (!m_GraphicSystem.CreateOffscreen(&m_pOffscreen,rcClient.right,rcClient.bottom))
			return false;

		Graphics::CCanvas *pCanvas=m_pOffscreen->GetCanvas();
		pCanvas->SetClip(PaintRect);

		if (IsTransparentBackground()) {
			pCanvas->Clear();
			if (fDrawParent)
				DrawParent(pCanvas,PaintRect);
		}

		Draw(pCanvas,PaintRect);

		pCanvas->ResetClip();

		return true;
	}

	bool CLayeredWidget::DrawParent(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		HWND hwndParent=::GetAncestor(m_hwnd,GA_PARENT);
		if (hwndParent==nullptr)
			return false;

		RECT rcParent,rcPaint,rc;
		::GetClientRect(hwndParent,&rcParent);
		rcPaint=PaintRect;
		::MapWindowPoints(m_hwnd,hwndParent,reinterpret_cast<LPPOINT>(&rcPaint),2);
		if (!::IntersectRect(&rc,&rcParent,&rcPaint))
			return false;

		DrawInfo Info;
		Info.pCanvas=pCanvas;
		Info.PaintRect=rc;
		Info.DstPos.x=rc.left;
		Info.DstPos.y=rc.top;
		::MapWindowPoints(hwndParent,m_hwnd,&Info.DstPos,1);
		Info.fDrawParent=true;

		return ::SendMessage(hwndParent,WM_APP_DRAW,0,reinterpret_cast<LPARAM>(&Info))!=0;
	}

	void CLayeredWidget::DeleteOffscreen()
	{
		TSTask::SafeDelete(m_pOffscreen);
	}

	void CLayeredWidget::DeleteChildrenOffscreen()
	{
		if (m_hwnd!=nullptr)
			::EnumChildWindows(m_hwnd,EnumChildProc,0);
	}

	BOOL CALLBACK CLayeredWidget::EnumChildProc(HWND hwnd,LPARAM lParam)
	{
		::SendMessage(hwnd,WM_APP_DELETE_OFFSCREEN,0,0);
		return TRUE;
	}

}
