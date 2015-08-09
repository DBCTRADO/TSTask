#ifndef TSTASKCENTRE_TASK_TRAY_MANAGER_H
#define TSTASKCENTRE_TASK_TRAY_MANAGER_H


namespace TSTaskCentre
{

	class CTaskTrayManager
	{
	public:
		CTaskTrayManager();
		~CTaskTrayManager();
		bool Initialize(HWND hwnd,UINT Message,HICON hIcon);
		void Finalize();
		bool AddTrayIcon();
		bool RemoveTrayIcon();
		bool HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam);

	private:
		bool m_fShowTrayIcon;
		HWND m_hwnd;
		UINT m_Message;
		HICON m_hIcon;
		UINT m_TaskbarCreatedMessage;
	};

}


#endif
