#ifndef TSTASK_WINDOW_H
#define TSTASK_WINDOW_H


#include "TSTaskAppCore.h"


namespace TSTask
{

	class CTSTaskWindow
	{
	public:
		static bool Initialize(HINSTANCE hinst);

		CTSTaskWindow(CTSTaskAppCore &Core);
		~CTSTaskWindow();
		bool Create();
		void Destroy();
		HWND GetHandle() const { return m_hwnd; }

		void OnRecordingStarted();
		void OnRecordingStopped();

	private:
		static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

		static const LPCWSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		CTSTaskAppCore &m_Core;
		HWND m_hwnd;
	};

}


#endif
