#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TaskTrayManager.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTaskTrayManager::CTaskTrayManager()
		: m_fShowTrayIcon(false)
		, m_hwnd(nullptr)
		, m_Message(0)
		, m_hIcon(nullptr)
		, m_TaskbarCreatedMessage(0)
	{
	}

	CTaskTrayManager::~CTaskTrayManager()
	{
	}

	bool CTaskTrayManager::Initialize(HWND hwnd,UINT Message,HICON hIcon)
	{
		m_hwnd=hwnd;
		m_Message=Message;
		m_hIcon=hIcon;

		m_TaskbarCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarCreated"));

		auto pChangeWindowMessageFilterEx=
			TSTASK_GET_DLL_FUNC(TEXT("user32.dll"),ChangeWindowMessageFilterEx);
		if (pChangeWindowMessageFilterEx!=nullptr)
			pChangeWindowMessageFilterEx(hwnd,m_TaskbarCreatedMessage,MSGFLT_ALLOW,nullptr);
		else
			::ChangeWindowMessageFilter(m_TaskbarCreatedMessage,MSGFLT_ADD);

		return true;
	}

	void CTaskTrayManager::Finalize()
	{
		if (m_fShowTrayIcon)
			RemoveTrayIcon();

		m_hwnd=nullptr;
		m_Message=0;
		m_hIcon=nullptr;
	}

	bool CTaskTrayManager::AddTrayIcon()
	{
		if (m_hwnd==nullptr)
			return false;

		NOTIFYICONDATA nid;

		nid.cbSize=NOTIFYICONDATA_V2_SIZE;
		nid.hWnd=m_hwnd;
		nid.uID=1;
		nid.uFlags=NIF_MESSAGE | NIF_ICON | NIF_TIP;
		nid.uCallbackMessage=m_Message;
		nid.hIcon=m_hIcon;
		::lstrcpyn(nid.szTip,APP_NAME_W,64);
		if (!::Shell_NotifyIcon(NIM_MODIFY,&nid)
				&& !::Shell_NotifyIcon(NIM_ADD,&nid))
			return false;

		m_fShowTrayIcon=true;

		return true;
	}

	bool CTaskTrayManager::RemoveTrayIcon()
	{
		if (m_fShowTrayIcon) {
			NOTIFYICONDATA nid;

			nid.cbSize=NOTIFYICONDATA_V2_SIZE;
			nid.hWnd=m_hwnd;
			nid.uID=1;
			nid.uFlags=0;
			::Shell_NotifyIcon(NIM_DELETE,&nid);

			m_fShowTrayIcon=false;
		}

		return true;
	}

	bool CTaskTrayManager::HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam)
	{
		if (m_TaskbarCreatedMessage!=0
				&& Message==m_TaskbarCreatedMessage) {
			if (m_fShowTrayIcon)
				AddTrayIcon();

			return true;
		}

		return false;
	}

}
