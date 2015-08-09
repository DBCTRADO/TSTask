#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskWindow.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	enum {
		WM_APP_RECORDING_STARTED=WM_APP,
		WM_APP_RECORDING_STOPPED
	};


	const LPCWSTR CTSTaskWindow::m_pszWindowClass=APP_NAME_W L"_Window";
	HINSTANCE CTSTaskWindow::m_hinst=nullptr;

	bool CTSTaskWindow::Initialize(HINSTANCE hinst)
	{
		if (m_hinst==nullptr) {
			WNDCLASS wc;

			wc.style=0;
			wc.lpfnWndProc=WndProc;
			wc.cbClsExtra=0;
			wc.cbWndExtra=0;
			wc.hInstance=hinst;
			wc.hIcon=nullptr;
			wc.hCursor=nullptr;
			wc.hbrBackground=nullptr;
			wc.lpszMenuName=nullptr;
			wc.lpszClassName=m_pszWindowClass;
			if (::RegisterClass(&wc)==0)
				return false;

			m_hinst=hinst;
		}

		return true;
	}

	CTSTaskWindow::CTSTaskWindow(CTSTaskAppCore &Core)
		: m_Core(Core)
		, m_hwnd(nullptr)
	{
	}

	CTSTaskWindow::~CTSTaskWindow()
	{
		Destroy();
	}

	bool CTSTaskWindow::Create()
	{
		if (m_hwnd!=nullptr)
			return false;

		OutLog(LOG_VERBOSE,L"%sを作成します。",m_pszWindowClass);

		return ::CreateWindowEx(0,m_pszWindowClass,nullptr,WS_POPUP,
								0,0,0,0,nullptr,nullptr,m_hinst,this)!=nullptr;
	}

	void CTSTaskWindow::Destroy()
	{
		if (m_hwnd!=nullptr)
			::DestroyWindow(m_hwnd);
	}

	void CTSTaskWindow::OnRecordingStarted()
	{
		if (m_hwnd!=nullptr)
			::PostMessage(m_hwnd,WM_APP_RECORDING_STARTED,0,0);
	}

	void CTSTaskWindow::OnRecordingStopped()
	{
		if (m_hwnd!=nullptr)
			::PostMessage(m_hwnd,WM_APP_RECORDING_STOPPED,0,0);
	}

	LRESULT CALLBACK CTSTaskWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		CTSTaskWindow *pThis;

		if (uMsg==WM_NCCREATE) {
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			pThis=static_cast<CTSTaskWindow*>(pcs->lpCreateParams);

			pThis->m_hwnd=hwnd;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		} else {
			pThis=reinterpret_cast<CTSTaskWindow*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
			if (pThis==nullptr)
				return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

			if (uMsg==WM_NCDESTROY) {
				pThis->OnMessage(hwnd,uMsg,wParam,lParam);
				pThis->m_hwnd=nullptr;
				return 0;
			}
		}

		return pThis->OnMessage(hwnd,uMsg,wParam,lParam);
	}

	LRESULT CTSTaskWindow::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CREATE:
			return 0;

		case WM_APP_RECORDING_STARTED:
			{
				EXECUTION_STATE Flags=ES_CONTINUOUS;
				if (m_Core.GetCurSettings().Recording.GetSystemRequired())
					Flags|=ES_SYSTEM_REQUIRED;
				if (m_Core.GetCurSettings().Recording.GetAwayModeRequired())
					Flags|=ES_AWAYMODE_REQUIRED;

				OutLog(LOG_VERBOSE,L"システム使用ステートを設定します。(0x%x)",UINT(Flags));

				::SetThreadExecutionState(Flags);
			}
			return 0;

		case WM_APP_RECORDING_STOPPED:
			OutLog(LOG_VERBOSE,L"システム使用ステートをクリアします。");

			::SetThreadExecutionState(ES_CONTINUOUS);
			return 0;

		// この辺も何かした方がいいかな?
		/*
		case WM_POWERBROADCAST:
			break;

		case WM_QUERYENDSESSION:
			break;
		*/

		case WM_DESTROY:
			OutLog(LOG_VERBOSE,L"WM_QUITをポストします。");
			::PostQuitMessage(0);
			return 0;
		}

		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	}

}
