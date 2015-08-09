#include "stdafx.h"
#include <dwmapi.h>
#include "TSTaskCentre.h"
#include "MainBoard.h"
#include "InformationBarItems.h"
#include "TSTaskSettingsDialog.h"
#include "LogDialog.h"
#include "ChannelListDialog.h"
#include "TaskInfoDialog.h"
#include "SendMessageDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"

#pragma comment(lib, "dwmapi.lib")


namespace TSTaskCentre
{

	enum
	{
		WM_APP_CREATE_TS_TASK_BAR=WM_APP,
		WM_APP_REMOVE_TS_TASK_BAR,
		WM_APP_BONDRIVER_LOADED,
		WM_APP_BONDRIVER_UNLOADED,
		WM_APP_TUNER_OPENED,
		WM_APP_TUNER_CLOSED,
		WM_APP_CHANNEL_CHANGED,
		WM_APP_SERVICE_CHANGED,
		WM_APP_RECORDING_STARTED,
		WM_APP_RECORDING_STOPPED,
		WM_APP_RECORDING_FILE_CHANGED,
		WM_APP_STREAMING_STARTED,
		WM_APP_STREAMING_STOPPED,
		WM_APP_EVENT_CHANGED,
		WM_APP_ERROR_LOG,
		WM_APP_TASK_TRAY
	};

	enum
	{
		TIMER_ID_UPDATE_STATUS=1,
		TIMER_ID_UPDATE_INFORMATION_BAR
	};

	const LPCTSTR CMainBoard::m_pszWindowClass=APP_NAME_W TEXT("_MainBoard");
	HINSTANCE CMainBoard::m_hinst=nullptr;

	bool CMainBoard::Initialize(HINSTANCE hinst)
	{
		if (m_hinst==nullptr) {
			WNDCLASS wc;

			wc.style=CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc=WndProc;
			wc.cbClsExtra=0;
			wc.cbWndExtra=0;
			wc.hInstance=hinst;
			wc.hIcon=::LoadIcon(hinst,MAKEINTRESOURCE(IDI_MAIN));
			wc.hCursor=::LoadCursor(nullptr,IDC_ARROW);
			wc.hbrBackground=::CreateSolidBrush(0xFF000000);
			wc.lpszMenuName=nullptr;
			wc.lpszClassName=m_pszWindowClass;
			if (::RegisterClass(&wc)==0)
				return false;

			m_hinst=hinst;
		}

		if (!CTSTaskBarBase::Initialize(hinst))
			return false;

		if (!CInformationBar::Initialize(hinst))
			return false;

		return true;
	}

	bool CMainBoard::GetDefaultTheme(ThemeInfo *pTheme)
	{
		if (pTheme==nullptr)
			return false;

		pTheme->Background.Fill.Type=Theme::FILL_SOLID;
		pTheme->Background.Fill.Solid.Color.Set(0,0,0);
		pTheme->Background.Border.Type=Theme::BORDER_NONE;

		CTSTaskBarBase::GetDefaultTheme(&pTheme->TSTaskBarBase);

		pTheme->Caption.Active.Background.Fill.Type=Theme::FILL_GRADIENT;
		pTheme->Caption.Active.Background.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
		pTheme->Caption.Active.Background.Fill.Gradient.Color1.Set(164,194,240);
		pTheme->Caption.Active.Background.Fill.Gradient.Color2.Set(195,213,240);
		pTheme->Caption.Active.Background.Border.Type=Theme::BORDER_SOLID;
		pTheme->Caption.Active.Background.Border.Color.Set(150,186,240);
		pTheme->Caption.Inactive.Background.Fill.Type=Theme::FILL_GRADIENT;
		pTheme->Caption.Inactive.Background.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
		pTheme->Caption.Inactive.Background.Fill.Gradient.Color1.Set(160,160,160);
		pTheme->Caption.Inactive.Background.Fill.Gradient.Color2.Set(184,184,184);
		pTheme->Caption.Inactive.Background.Border.Type=Theme::BORDER_SOLID;
		pTheme->Caption.Inactive.Background.Border.Color.Set(152,152,152);

		CTSTaskBar::GetDefaultTheme(&pTheme->TSTaskBar);

		static const struct {
			COLORREF Color1;
			COLORREF Color2;
		} TvRockCaptionColors[_countof(pTheme->TvRockCaptionStyles)] = {
			{RGB(120,138,192),	RGB( 94,108,150)},
			{RGB(120,192,192),	RGB( 94,150,150)},
			{RGB(120,192,138),	RGB( 94,150,108)},
			{RGB(156,192,120),	RGB(122,150, 94)},
			{RGB(192,174,120),	RGB(150,116, 94)},
			{RGB(192,120,120),	RGB(150, 94, 94)},
			{RGB(192,120,174),	RGB(150, 94,116)},
			{RGB(156,120,192),	RGB(122, 94,150)},
		};
		for (int i=0;i<_countof(pTheme->TvRockCaptionStyles);i++) {
			Theme::ItemStyle &Style=pTheme->TvRockCaptionStyles[i];
			Style.Background.Fill.Type=Theme::FILL_GRADIENT;
			Style.Background.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
			Style.Background.Fill.Gradient.Color1.Set(TvRockCaptionColors[i].Color1);
			Style.Background.Fill.Gradient.Color2.Set(TvRockCaptionColors[i].Color2);
			Style.Background.Border.Type=Theme::BORDER_RAISED;
			Style.Background.Border.Color=
				Graphics::MixColor(Style.Background.Fill.Gradient.Color1,
								   Style.Background.Fill.Gradient.Color2);
			Style.Foreground=pTheme->TSTaskBar.CaptionStyle.Foreground;
		}

		CInformationBar::GetDefaultTheme(&pTheme->InformationBar);

		return true;
	}

	CMainBoard::CMainBoard(CTSTaskCentreCore &Core,
						   CSettingsHandler *pSettingsHandler)
		: CLayeredWidget(Core.GetGraphicSystem())
		, m_Core(Core)
		, m_TSTaskBarBase(Core)
		, m_InformationBar(Core)
		, m_fShowInformationBar(true)
		, m_fAdjustSize(true)
		, m_fAdjustSizeOnRestore(false)

		, m_fShowTrayIcon(false)
		, m_fMinimizeToTray(false)
		, m_fInTray(false)
		, m_fShowInTaskbar(true)
		, m_fAlwaysOnTop(false)
		, m_fTranslucent(false)
		, m_fFixedHeight(true)

		, m_fEndAllTaskByUser(false)

		, m_SettingsDialog(Core,pSettingsHandler)
		, m_AboutDialog(Core)
	{
		m_TSTaskBarBase.SetEventHandler(this);

		GetDefaultTheme(&m_Theme);
	}

	CMainBoard::~CMainBoard()
	{
		Destroy();
	}

	bool CMainBoard::Create(HWND hwndParent,int ID)
	{
		if (m_hwnd!=nullptr)
			return false;

		DWORD Style=(WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX) | WS_CLIPCHILDREN;
		DWORD ExStyle=0;//WS_EX_COMPOSITED;
		if (m_fAlwaysOnTop)
			ExStyle|=WS_EX_TOPMOST;
		if (m_fTranslucent)
			ExStyle|=WS_EX_LAYERED;
		if (!m_fShowInTaskbar)
			ExStyle|=WS_EX_TOOLWINDOW;

		if (!CreateBasicWindow(hwndParent,Style,ExStyle,
							   0,m_pszWindowClass,APP_NAME_W,m_hinst))
			return false;

		// 後でスタイルを変えないとシステムメニューが出ない
		SetStyle(GetStyle() & ~WS_CAPTION);

		::SetWindowPos(m_hwnd,nullptr,0,0,m_WindowPosition.Width,m_WindowPosition.Height,
					   SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

		return true;
	}

	bool CMainBoard::Show(int CmdShow)
	{
		if (m_hwnd==nullptr)
			return false;

		MoveToMonitorInside();

		return ::ShowWindow(m_hwnd,CmdShow)!=FALSE;
	}

	bool CMainBoard::CreateTSTaskBar(TSTask::TaskID ID)
	{
		TSTask::String Caption;
		TSTask::TaskUtility::TaskIDToString(ID,&Caption);
		int DeviceID=-1;
		m_Core.GetTSTaskManager().GetTvRockInfo(ID,&DeviceID);
		if (DeviceID>=0) {
			WCHAR szDID[4];
			TSTask::FormatString(szDID,_countof(szDID),L":%c",L'A'+DeviceID);
			Caption+=szDID;
		}

		CTSTaskBar::ThemeInfo Theme=m_Theme.TSTaskBar;
		if (DeviceID>=0)
			Theme.CaptionStyle=m_Theme.TvRockCaptionStyles[DeviceID%_countof(m_Theme.TvRockCaptionStyles)];

		if (!m_TSTaskBarBase.CreateTSTaskBar(ID,Caption.c_str(),Theme))
			return false;

		if (m_fAdjustSize)
			AdjustSize();

		Redraw(nullptr,RDW_UPDATENOW | RDW_ALLCHILDREN);

		UpdateTSTaskBar(ID);

		m_fEndAllTaskByUser=false;

		return true;
	}

	bool CMainBoard::RemoveTSTaskBar(TSTask::TaskID ID)
	{
		if (!m_TSTaskBarBase.RemoveTSTaskBar(ID))
			return false;

		if (m_fAdjustSize && m_TSTaskBarBase.GetTSTaskBarCount()>0)
			AdjustSize();

		if (!m_fEndAllTaskByUser
				&& m_Core.GetSettings().General.GetExitWhenAllTaskEnded()
				&& m_TSTaskBarBase.GetTSTaskBarCount()==0) {
			TSTask::OutLog(TSTask::LOG_INFO,L"全てのタスクが終了したので" APP_NAME_W L"を終了します。");
			PostMessage(WM_COMMAND,CM_EXIT,0);
		}

		return true;
	}

	bool CMainBoard::SendCreateTSTaskBar(TSTask::TaskID ID)
	{
		if (m_hwnd==nullptr)
			return false;

		DWORD_PTR Result=0;
		if (::SendMessageTimeout(m_hwnd,WM_APP_CREATE_TS_TASK_BAR,ID,0,
								 SMTO_NORMAL,10000,&Result)==0)
			return false;

		return Result!=0;
	}

	bool CMainBoard::SendRemoveTSTaskBar(TSTask::TaskID ID)
	{
		if (m_hwnd==nullptr)
			return false;

		DWORD_PTR Result=0;
		if (::SendMessageTimeout(m_hwnd,WM_APP_REMOVE_TS_TASK_BAR,ID,0,
								 SMTO_NORMAL,10000,&Result)==0)
			return false;

		return Result!=0;
	}

	bool CMainBoard::AdjustSize()
	{
		if (m_hwnd==nullptr)
			return false;

		if (::IsZoomed(m_hwnd) || ::IsIconic(m_hwnd)) {
			m_fAdjustSizeOnRestore=true;
			return true;
		}

		if (m_TSTaskBarBase.GetTSTaskBarCount()==0) {
			SendSizeMessage();
			return true;
		}

		RECT rc={0,0,0,0},rcCurPos,rcNewPos;
		rc.bottom=m_TSTaskBarBase.CalcHeight();
		if (m_fShowInformationBar)
			rc.bottom+=m_InformationBar.GetHeight();
		CalcPositionFromClientRect(&rc);
		GetPosition(&rcCurPos);
		rcNewPos=rcCurPos;
		rcNewPos.bottom=rcCurPos.top+(rc.bottom-rc.top);

		MONITORINFO mi;
		mi.cbSize=sizeof(mi);
		if (::GetMonitorInfo(::MonitorFromRect(&rcNewPos,MONITOR_DEFAULTTONEAREST),&mi)) {
			if (rcNewPos.bottom>mi.rcWork.bottom && rcNewPos.bottom>rcCurPos.bottom) {
				::OffsetRect(&rcNewPos,0,mi.rcWork.bottom-rcNewPos.bottom);
			} else if (rcCurPos.bottom>=mi.rcWork.bottom-16) {
				rcNewPos.bottom=rcCurPos.bottom;
				rcNewPos.top=rcCurPos.bottom-(rc.bottom-rc.top);
			}
		}

		SetPosition(rcNewPos);

		return true;
	}

	bool CMainBoard::SetTheme(const ThemeInfo &Theme)
	{
		m_Theme=Theme;

		m_GraphicSystem.ClearImagePool();

		m_TSTaskBarBase.SetTheme(Theme.TSTaskBarBase);

		TSTask::TaskID ID;
		for (int i=0;(ID=m_TSTaskBarBase.GetTaskID(i))!=TSTask::INVALID_TASK_ID;i++) {
			CTSTaskBar::ThemeInfo Theme=m_Theme.TSTaskBar;
			int DeviceID=-1;
			if (m_Core.GetTSTaskManager().GetTvRockInfo(ID,&DeviceID) && DeviceID>=0)
				Theme.CaptionStyle=m_Theme.TvRockCaptionStyles[DeviceID%_countof(m_Theme.TvRockCaptionStyles)];
			m_TSTaskBarBase.SetTSTaskBarTheme(ID,Theme);
		}

		m_InformationBar.SetTheme(Theme.InformationBar);

		AdjustSize();
		UpdateTransparent();

		return true;
	}

	bool CMainBoard::SetShowTrayIcon(bool fShow)
	{
		if (m_fShowTrayIcon!=fShow) {
			m_fShowTrayIcon=fShow;

			if (m_hwnd!=nullptr) {
				if (fShow)
					m_TaskTrayManager.AddTrayIcon();
				else if (!m_fMinimizeToTray || !::IsIconic(m_hwnd))
					m_TaskTrayManager.RemoveTrayIcon();
			}
		}

		return true;
	}

	bool CMainBoard::SetMinimizeToTray(bool fMinimizeToTray)
	{
		if (m_fMinimizeToTray!=fMinimizeToTray) {
			m_fMinimizeToTray=fMinimizeToTray;

			if (!m_fShowTrayIcon && m_hwnd!=nullptr && ::IsIconic(m_hwnd)) {
				if (fMinimizeToTray)
					m_TaskTrayManager.AddTrayIcon();
				else
					m_TaskTrayManager.RemoveTrayIcon();
			}
		}

		return true;
	}

	bool CMainBoard::SetShowInTaskbar(bool fShow)
	{
		if (m_fShowInTaskbar!=fShow) {
			m_fShowInTaskbar=fShow;

			if (m_hwnd!=nullptr
					&& (!m_fMinimizeToTray || !::IsIconic(m_hwnd))) {
				SetVisible(false);
				SetExStyle(GetExStyle() ^ WS_EX_TOOLWINDOW);
				SetVisible(true);
			}
		}

		return true;
	}

	bool CMainBoard::SetAlwaysOnTop(bool fAlwaysOnTop)
	{
		if (m_fAlwaysOnTop!=fAlwaysOnTop) {
			m_fAlwaysOnTop=fAlwaysOnTop;

			if (m_hwnd!=nullptr) {
				::SetWindowPos(m_hwnd,fAlwaysOnTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,
							   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			}
		}

		return true;
	}

	bool CMainBoard::SetTranslucent(bool fTranslucent)
	{
		if (m_fTranslucent!=fTranslucent) {
			m_fTranslucent=fTranslucent;

			if (m_hwnd!=nullptr) {
				const DWORD ExStyle=GetExStyle();

				if (((ExStyle & WS_EX_LAYERED)!=0)!=fTranslucent)
					SetExStyle(ExStyle ^ WS_EX_LAYERED);
				if (fTranslucent) {
					::SetLayeredWindowAttributes(m_hwnd,0,
						m_Core.GetSettings().MainBoard.GetTranslucentOpacity8(),LWA_ALPHA);
				}
			}
		}

		return true;
	}

	bool CMainBoard::ShowInformationBar(bool fShow)
	{
		if (m_fShowInformationBar!=fShow) {
			m_fShowInformationBar=fShow;

			if (m_hwnd!=nullptr) {
				if (fShow) {
					m_InformationBar.UpdateItems(false);
				} else {
					::KillTimer(m_hwnd,TIMER_ID_UPDATE_INFORMATION_BAR);
					m_InformationBar.SetVisible(false);
				}
				AdjustSize();
				if (fShow) {
					m_InformationBar.UpdateItems(false);
					m_InformationBar.SetVisible(true);
					::SetTimer(m_hwnd,TIMER_ID_UPDATE_INFORMATION_BAR,
							   m_Core.GetSettings().MainBoard.GetInformationBarUpdateInterval(),nullptr);
				}
			}
		}

		return true;
	}

	bool CMainBoard::RestoreSettings(const CMainBoardSettings &Settings)
	{
		RECT rc;
		if (Settings.GetPosition(&rc))
			SetPosition(rc);

		ChangeSettings(Settings);

		return true;
	}

	bool CMainBoard::ChangeSettings(const CMainBoardSettings &Settings)
	{
		SetShowTrayIcon(Settings.GetShowTrayIcon());
		SetMinimizeToTray(Settings.GetMinimizeToTray());
		SetShowInTaskbar(Settings.GetShowInTaskbar());
		SetAlwaysOnTop(Settings.GetAlwaysOnTop());
		SetTranslucent(Settings.GetTranslucent());
		ShowInformationBar(Settings.GetShowInformationBar());

		const int TSTaskBarCount=m_TSTaskBarBase.GetTSTaskBarCount();
		int OldTSTaskBarHeight;
		if (TSTaskBarCount>0)
			OldTSTaskBarHeight=m_TSTaskBarBase.CalcHeight();

		if (TSTaskBarCount>0) {
			CMainBoardSettings::StatusItemList ItemList;
			Settings.GetStatusItemListSortedByOrder(&ItemList);
			std::vector<int> OrderList;
			OrderList.resize(ItemList.size());
			for (size_t i=0;i<ItemList.size();i++)
				OrderList[i]=ItemList[i].ID;

			for (int i=0;i<TSTaskBarCount;i++) {
				CTSTaskBar *pBar=m_TSTaskBarBase.GetTSTaskBar(m_TSTaskBarBase.GetTaskID(i));
				if (pBar!=nullptr) {
					for (size_t j=0;j<ItemList.size();j++) {
						pBar->SetItemWidth(ItemList[j].ID,ItemList[j].Width);
						pBar->SetItemVisible(ItemList[j].ID,ItemList[j].fVisible);
					}
					pBar->SetItemOrder(&OrderList[0]);

					if (pBar->GetMaxRows()>1)
						pBar->SetMaxRows(Settings.GetStatusMaxRows());

					pBar->Invalidate();
				}
			}
		}

		LOGFONT lfStatus,lfCaption;
		Settings.GetStatusFont(&lfStatus);
		Settings.GetStatusCaptionFont(&lfCaption);
		m_TSTaskBarBase.SetFont(lfStatus,lfCaption);

		int OldInformationBarHeight=m_InformationBar.GetHeight();
		LOGFONT lf;
		if (Settings.GetInformationBarFont(&lf))
			m_InformationBar.SetFont(lf);
		CMainBoardSettings::InformationBarItemList ItemList;
		if (Settings.GetInformationBarItemList(&ItemList)) {
			m_InformationBar.DeleteAllItems();
			for (int i=0;i<(int)ItemList.size();i++) {
				const CMainBoardSettings::InformationBarItemInfo &Item=ItemList[i];
				InformationBarItems::CCustomInformationItem *pCustomItem=
					new InformationBarItems::CCustomInformationItem(i);
				pCustomItem->SetFormat(Item.Format.c_str());
				if (Item.Width>=0)
					pCustomItem->SetWidth(Item.Width);
				if (Item.TextAlign>=0)
					pCustomItem->SetTextAlign(CInformationBar::CItem::TextAlign(Item.TextAlign));
				m_InformationBar.AddItem(pCustomItem);
			}
			if (m_hwnd!=nullptr && m_fShowInformationBar) {
				m_InformationBar.UpdateItems();
				::SetTimer(m_hwnd,TIMER_ID_UPDATE_INFORMATION_BAR,
						   Settings.GetInformationBarUpdateInterval(),nullptr);
			}
		}

		if (m_hwnd!=nullptr) {
			if (TSTaskBarCount>0
					&& (m_TSTaskBarBase.GetHeight()!=OldTSTaskBarHeight
						|| m_InformationBar.GetHeight()!=OldInformationBarHeight))
				AdjustSize();
			else
				SendSizeMessage();
		}

		return true;
	}

	bool CMainBoard::GetSettings(CMainBoardSettings *pSettings) const
	{
		if (pSettings==nullptr)
			return false;

		Position Pos;
		GetPosition(&Pos);
		pSettings->SetPosition(Pos.Left,Pos.Top,Pos.Width,Pos.Height);
		pSettings->SetShowTrayIcon(m_fShowTrayIcon);
		pSettings->SetMinimizeToTray(m_fMinimizeToTray);
		pSettings->SetShowInTaskbar(m_fShowInTaskbar);
		pSettings->SetAlwaysOnTop(m_fAlwaysOnTop);
		pSettings->SetTranslucent(m_fTranslucent);
		pSettings->SetShowInformationBar(m_fShowInformationBar);

		return true;
	}

	bool CMainBoard::NotifyBonDriverLoaded(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_BONDRIVER_LOADED,ID,0);
	}

	bool CMainBoard::NotifyBonDriverUnloaded(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_BONDRIVER_LOADED,ID,0);
	}

	bool CMainBoard::NotifyTunerOpened(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_TUNER_OPENED,ID,0);
	}

	bool CMainBoard::NotifyTunerClosed(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_TUNER_CLOSED,ID,0);
	}

	bool CMainBoard::NotifyChannelChanged(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_CHANNEL_CHANGED,ID,0);
	}

	bool CMainBoard::NotifyServiceChanged(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_SERVICE_CHANGED,ID,0);
	}

	bool CMainBoard::NotifyRecordingStarted(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_RECORDING_STARTED,ID,0);
	}

	bool CMainBoard::NotifyRecordingStopped(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_RECORDING_STOPPED,ID,0);
	}

	bool CMainBoard::NotifyRecordingFileChanged(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_RECORDING_FILE_CHANGED,ID,0);
	}

	bool CMainBoard::NotifyStreamingStarted(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_STREAMING_STARTED,ID,0);
	}

	bool CMainBoard::NotifyStreamingStopped(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_STREAMING_STOPPED,ID,0);
	}

	bool CMainBoard::NotifyEventChanged(TSTask::TaskID ID)
	{
		return PostMessage(WM_APP_EVENT_CHANGED,ID,0);
	}

	LRESULT CMainBoard::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CREATE:
			{
				m_CaptionWidth=::GetSystemMetrics(SM_CYCAPTION);

				m_TSTaskBarBase.Create(hwnd);
				m_TSTaskBarBase.SetVisible(true);

				m_InformationBar.SetItemMargin(m_Core.ScaleDPI(m_InformationBar.GetDefaultItemMargin()));
				m_InformationBar.SetEventHandler(this);
				m_InformationBar.Create(hwnd);
				if (m_fShowInformationBar) {
					m_InformationBar.UpdateItems();
					m_InformationBar.SetVisible(true);
				}

				if (m_WindowPosition.Width<=0 || m_WindowPosition.Height<=0) {
					RECT rc;

					rc.left=0;
					rc.top=0;
					rc.right=m_Core.ScaleDPI(640);
					rc.bottom=m_Core.ScaleDPI(32);
					if (m_fShowInformationBar)
						rc.bottom+=m_InformationBar.GetHeight();
					::AdjustWindowRect(&rc,WS_POPUP | WS_THICKFRAME,FALSE);
					if (m_WindowPosition.Width<=0)
						m_WindowPosition.Width=rc.right-rc.left;
					if (m_WindowPosition.Height<=0)
						m_WindowPosition.Height=rc.bottom-rc.top;
					SetPosition(m_WindowPosition);
				}

				m_fInTray=false;
				m_TaskTrayManager.Initialize(hwnd,WM_APP_TASK_TRAY,
											 ::LoadIcon(m_hinst,MAKEINTRESOURCE(IDI_MAIN)));
				const bool fInTray=m_fMinimizeToTray && ::IsIconic(hwnd);
				if (m_fShowTrayIcon || fInTray) {
					if (m_TaskTrayManager.AddTrayIcon()) {
						if (fInTray)
							m_fInTray=true;
					}
				}

				if (m_fTranslucent)
					::SetLayeredWindowAttributes(hwnd,0,
						m_Core.GetSettings().MainBoard.GetTranslucentOpacity8(),LWA_ALPHA);

				UpdateTransparent();

				::SetTimer(m_hwnd,TIMER_ID_UPDATE_STATUS,
						   m_Core.GetSettings().MainBoard.GetStatusUpdateInterval(),nullptr);
				if (m_fShowInformationBar)
					::SetTimer(m_hwnd,TIMER_ID_UPDATE_INFORMATION_BAR,
							   m_Core.GetSettings().MainBoard.GetInformationBarUpdateInterval(),nullptr);
			}
			return 0;

		case WM_SIZE:
			{
				if (wParam==SIZE_MINIMIZED) {
					if (m_fMinimizeToTray && !m_fInTray) {
						if (m_TaskTrayManager.AddTrayIcon()) {
							SetVisible(false);
							m_fInTray=true;
						}
					}

					DeleteChildrenOffscreen();
					DeleteOffscreen();
				} else {
					int Width=LOWORD(lParam),Height=HIWORD(lParam);

					if (m_fShowInformationBar) {
						int InfoBarHeight=m_InformationBar.GetHeight();
						Height-=InfoBarHeight;
						if (Height<0)
							Height=0;
						m_InformationBar.SetPosition(m_CaptionWidth,Height,max(Width-m_CaptionWidth,0),InfoBarHeight);
					}

					m_TSTaskBarBase.SetPosition(m_CaptionWidth,0,max(Width-m_CaptionWidth,0),Height);

					if (wParam!=SIZE_MAXIMIZED) {
						if (m_fAdjustSizeOnRestore) {
							m_fAdjustSizeOnRestore=false;
							AdjustSize();
						}
					}

					if (m_fMinimizeToTray && m_fInTray) {
						SetVisible(true);
						if (!m_fShowTrayIcon)
							m_TaskTrayManager.RemoveTrayIcon();
						m_fInTray=false;
					}
				}
			}
			return 0;

		case WM_NCHITTEST:
			{
				const bool fFixedHeight=
					m_fFixedHeight && m_TSTaskBarBase.GetTSTaskBarCount()>0;
				POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
				RECT rc;

				::ScreenToClient(hwnd,&pt);
				::GetClientRect(hwnd,&rc);
				if (pt.x<rc.left) {
					if (!fFixedHeight) {
						if (pt.y<rc.top)
							return HTTOPLEFT;
						if (pt.y>=rc.bottom)
							return HTBOTTOMLEFT;
					}
					return HTLEFT;
				}
				if (pt.x>=rc.right) {
					if (!fFixedHeight) {
						if (pt.y<rc.top)
							return HTTOPRIGHT;
						if (pt.y>=rc.bottom)
							return HTBOTTOMRIGHT;
					}
					return HTRIGHT;
				}
				if (pt.y<rc.top) {
					if (fFixedHeight)
						return HTCAPTION;
					return HTTOP;
				}
				if (pt.y>=rc.bottom) {
					if (fFixedHeight)
						return HTCAPTION;
					return HTBOTTOM;
				}

				GetCaptionRect(&rc);
				if (::PtInRect(&rc,pt))
					return HTCAPTION;
			}
			return HTCLIENT;

		case WM_NCRBUTTONUP:
			if (wParam==HTCAPTION) {
				ShowMainMenu(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				return 0;
			}
			break;

		case WM_NCLBUTTONDBLCLK:
			if (wParam==HTCAPTION) {
				SendMessage(WM_COMMAND,CM_MINIMIZE,0);
				return 0;
			}
			break;

		case WM_NCACTIVATE:
			if (!m_fTransparentCaption) {
				RECT rc;

				GetCaptionRect(&rc);
				Invalidate(rc);
			}
			break;

		case WM_GETMINMAXINFO:
			if (m_fFixedHeight && m_TSTaskBarBase.GetTSTaskBarCount()>0) {
				MINMAXINFO *pmmi=reinterpret_cast<MINMAXINFO*>(lParam);
				RECT rc={0,0,0,m_TSTaskBarBase.CalcHeight()};

				if (m_fShowInformationBar)
					rc.bottom+=m_InformationBar.GetHeight();
				::AdjustWindowRectEx(&rc,GetStyle(),FALSE,GetExStyle());
				pmmi->ptMinTrackSize.y=rc.bottom-rc.top;
				pmmi->ptMaxTrackSize.y=rc.bottom-rc.top;
				return 0;
			}
			break;

		case WM_COMMAND:
			OnCommand(LOWORD(wParam));
			return 0;

		case WM_TIMER:
			switch (wParam) {
			case TIMER_ID_UPDATE_STATUS:
				{
					TSTask::TaskID ID;
					for (int i=0;(ID=m_TSTaskBarBase.GetTaskID(i))!=TSTask::INVALID_TASK_ID;i++) {
						UpdateStatistics(ID);
						UpdateRecordingTime(ID);
						UpdateClockStatus(ID);
					}
				}
				break;

			case TIMER_ID_UPDATE_INFORMATION_BAR:
				if (m_fShowInformationBar)
					m_InformationBar.UpdateItems();
				break;
			}
			return 0;

		case WM_DWMCOMPOSITIONCHANGED:
			UpdateTransparent();
			return 0;

		case WM_APP_CREATE_TS_TASK_BAR:
			TRACE(L"WM_APP_CREATE_TS_TASK_BAR(%Iu)\n",wParam);
			return CreateTSTaskBar(TSTask::TaskID(wParam));

		case WM_APP_REMOVE_TS_TASK_BAR:
			TRACE(L"WM_APP_REMOVE_TS_TASK_BAR(%Iu)\n",wParam);
			return RemoveTSTaskBar(TSTask::TaskID(wParam));

		case WM_APP_BONDRIVER_LOADED:
			TRACE(L"WM_APP_BONDRIVER_LOADED(%Iu)\n",wParam);
			UpdateTunerStatus(TSTask::TaskID(wParam));
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_BonDriverLoaded);
			return 0;

		case WM_APP_BONDRIVER_UNLOADED:
			TRACE(L"WM_APP_BONDRIVER_UNLOADED(%Iu)\n",wParam);
			UpdateTunerStatus(TSTask::TaskID(wParam));
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_BonDriverUnloaded);
			return 0;

		case WM_APP_TUNER_OPENED:
			TRACE(L"WM_APP_TUNER_OPENED(%Iu)\n",wParam);
			UpdateTunerStatus(TSTask::TaskID(wParam));
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_TunerOpened);
			return 0;

		case WM_APP_TUNER_CLOSED:
			TRACE(L"WM_APP_TUNER_CLOSED(%Iu)\n",wParam);
			UpdateTunerStatus(TSTask::TaskID(wParam));
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_TunerClosed);
			return 0;

		case WM_APP_CHANNEL_CHANGED:
			TRACE(L"WM_APP_CHANNEL_CHANGED(%Iu)\n",wParam);
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_ChannelChanged);
			return 0;

		case WM_APP_SERVICE_CHANGED:
			TRACE(L"WM_APP_SERVICE_CHANGED(%Iu)\n",wParam);
			UpdateChannelStatus(TSTask::TaskID(wParam));
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_ServiceChanged);
			return 0;

		case WM_APP_RECORDING_STARTED:
			TRACE(L"WM_APP_RECORDING_STARTED(%Iu)\n",wParam);
			UpdateRecordingStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_RecordingStarted);
			return 0;

		case WM_APP_RECORDING_STOPPED:
			TRACE(L"WM_APP_RECORDING_STOPPED(%Iu)\n",wParam);
			UpdateRecordingStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_RecordingStopped);
			return 0;

		case WM_APP_STREAMING_STARTED:
			TRACE(L"WM_APP_STREAMING_STARTED(%Iu)\n",wParam);
			UpdateStreamingStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_StreamingStarted);
			return 0;

		case WM_APP_STREAMING_STOPPED:
			TRACE(L"WM_APP_STREAMING_STOPPED(%Iu)\n",wParam);
			UpdateStreamingStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_StreamingStopped);
			return 0;

		case WM_APP_EVENT_CHANGED:
			TRACE(L"WM_APP_EVENT_CHANGED(%Iu)\n",wParam);
			UpdateEventStatus(TSTask::TaskID(wParam));
			m_Core.ExecuteTaskToolOnEvent(TSTask::TaskID(wParam),TSTask::MESSAGE_EVENT_EventChanged);
			return 0;

		case WM_APP_ERROR_LOG:
			TRACE(L"WM_APP_ERROR_LOG(%Iu)\n",wParam);
			if (m_fShowInformationBar
					&& m_Core.GetCurSettings().MainBoard.GetInformationBarShowError()) {
				TSTask::LogInfo Log;
				if (m_Core.GetLogger().GetLogByNumber((unsigned int)wParam,&Log)) {
					m_InformationBar.SetSingleText(
						Log.Type==TSTask::LOG_WARNING?
							CInformationBar::TEXT_TYPE_WARNING:
							CInformationBar::TEXT_TYPE_ERROR,
						Log.Text.c_str(),
						m_Core.GetCurSettings().MainBoard.GetInformationBarErrorDuration());
				}
			}
			return 0;

		case WM_APP_TASK_TRAY:
			switch (lParam) {
			case WM_RBUTTONUP:
				{
					HMENU hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_TRAY));
					POINT pt;

					::GetCursorPos(&pt);
					// お約束が必要な理由は以下を参照
					// http://support.microsoft.com/kb/135788/en-us
					::SetForegroundWindow(hwnd);		// お約束
					::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,hwnd,NULL);
					::PostMessage(hwnd,WM_NULL,0,0);	// お約束
					::DestroyMenu(hmenu);
				}
				break;

			case WM_LBUTTONUP:
				SendMessage(WM_COMMAND,CM_SHOW,0);
				break;
			}
			return 0;

		case WM_CLOSE:
			if (!ConfirmClose(false))
				return 0;
			break;

		case WM_DESTROY:
			m_GraphicSystem.ClearImagePool();

			m_TaskTrayManager.Finalize();

			::PostQuitMessage(0);
			break;
		}

		return CLayeredWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CMainBoard::Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);
		RECT rc;

		if (PaintRect.right>m_CaptionWidth) {
			GetClientRect(&rc);
			rc.left=m_CaptionWidth;
			StylePainter.DrawBackground(m_Theme.Background,rc);
		}

		if (!m_fTransparentCaption
				&& PaintRect.left<m_CaptionWidth) {
			GetCaptionRect(&rc);

			StylePainter.DrawBackground(
				::GetForegroundWindow()==m_hwnd?
					m_Theme.Caption.Active.Background:m_Theme.Caption.Inactive.Background,rc);
		}
	}

	bool CMainBoard::IsTransparentBackground() const
	{
		return m_Theme.Background.IsTransparent();
	}

	int CMainBoard::ShowMessage(CMessageDialog::MessageType Type,
								LPCWSTR pszText,LPCWSTR pszCaption,UINT Style,
								bool *pDontAskAgain) const
	{
		if (TSTask::IsStringEmpty(pszText))
			return 0;

		CMessageDialog Dialog;

		return Dialog.Show(m_hwnd,m_hinst,Type,pszText,pszCaption,Style,pDontAskAgain);
	}

	bool CMainBoard::ConfirmClose(bool fEndTasks)
	{
		const int TaskCount=m_TSTaskBarBase.GetTSTaskBarCount();
		bool fConfirmClose=m_Core.GetSettings().MainBoard.GetConfirmClose()
				&& (!m_Core.GetSettings().MainBoard.GetConfirmCloseTaskExists() || TaskCount>0);
		bool fConfirmEndTasks=
			fEndTasks && m_Core.GetSettings().MainBoard.GetConfirmTaskExit() && TaskCount>0;

		if (fConfirmClose || fConfirmEndTasks) {
			bool fDontAskAgain=false;
			TSTask::String Message;
			if (TaskCount>0)
				TSTask::StringUtility::Format(Message,L"%d個のタスクが動作中です。\r\n",TaskCount);
			if (fEndTasks) {
				if (TaskCount>0)
					Message+=APP_NAME_W L" と全てのタスクを終了させますか?";
				else
					Message+=APP_NAME_W L" を終了しますか?";
			} else {
				Message+=APP_NAME_W L" を終了しますか?";
				if (TaskCount>0)
					Message+=L"\r\n(" APP_NAME_W L" を終了しても、タスクは動作したままになります)";
			}
			int Result=ShowMessage(CMessageDialog::TYPE_QUESTION,
								   Message.c_str(),L"終了の確認",
								   CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL,
								   &fDontAskAgain);
			if (fDontAskAgain) {
				if (fConfirmClose)
					m_Core.GetSettings().MainBoard.SetConfirmClose(false);
				else
					m_Core.GetSettings().MainBoard.SetConfirmTaskExit(false);
			}
			if (Result!=IDOK)
				return false;
		}

		return true;
	}

	bool CMainBoard::ConfirmRecordingChannelChange(TSTask::TaskID ID)
	{
		if (m_Core.GetSettings().Recording.GetConfirmChannelChange()) {
			TSTask::RecordingInfo RecInfo;

			if (m_Core.GetTSTaskManager().GetRecordingInfo(ID,&RecInfo)
					&& RecInfo.State==TSTask::RECORDING_STATE_RECORDING) {
				bool fDontAskAgain=false;
				int Result=
					ShowMessage(CMessageDialog::TYPE_QUESTION,
								L"現在録画中です。チャンネルを変更しますか?",
								L"チャンネル変更の確認",
								CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL,
								&fDontAskAgain);
				if (fDontAskAgain)
					m_Core.GetSettings().Recording.SetConfirmChannelChange(false);
				if (Result!=IDOK)
					return false;
			}
		}

		return true;
	}

	void CMainBoard::GetCaptionRect(RECT *pRect) const
	{
		if (GetClientRect(pRect))
			pRect->right=m_CaptionWidth;
		else
			::SetRectEmpty(pRect);
	}

	void CMainBoard::UpdateTransparent()
	{
		m_fTransparentCaption=false;

		BOOL fCompositionEnabled;
		if (::DwmIsCompositionEnabled(&fCompositionEnabled)==S_OK && fCompositionEnabled) {
			MARGINS Margins={m_CaptionWidth,0,0,0};

			if (m_Theme.Background.IsTransparent())
				Margins.cxLeftWidth=-1;
			if (::DwmExtendFrameIntoClientArea(m_hwnd,&Margins)==S_OK)
				m_fTransparentCaption=true;
		}
	}

	void CMainBoard::OnCommand(int Command)
	{
		switch (Command) {
		case CM_MINIMIZE:
			::ShowWindow(m_hwnd,SW_MINIMIZE);
			return;

		case CM_ALWAYS_ON_TOP:
			SetAlwaysOnTop(!m_fAlwaysOnTop);
			return;

		case CM_TRANSLUCENT:
			SetTranslucent(!m_fTranslucent);
			return;

		case CM_INFORMATION_BAR:
			ShowInformationBar(!m_fShowInformationBar);
			return;

		case CM_CLOSE:
			SendMessage(WM_CLOSE,0,0);
			return;

		case CM_EXIT:
			Destroy();
			return;

		case CM_SHOW:
			if (::IsIconic(m_hwnd))
				::ShowWindow(m_hwnd,SW_RESTORE);
			::SetForegroundWindow(m_hwnd);
			return;

		case CM_SETTINGS:
			m_SettingsDialog.Create(m_hwnd,m_hinst);
			return;

		case CM_ABOUT:
			m_AboutDialog.Create(m_hwnd,m_hinst);
			return;

		case CM_NEW_TASK:
			m_Core.NewTask();
			return;

		case CM_END_ALL_TASKS_AND_EXIT:
		case CM_END_ALL_TASKS:
			{
				const bool fExit=Command==CM_END_ALL_TASKS_AND_EXIT;

				if (fExit && !ConfirmClose(true))
					return;

				m_fEndAllTaskByUser=true;
				bool fSuccess=true;

				for (int i=m_TSTaskBarBase.GetTSTaskBarCount()-1;i>=0;i--) {
					TSTask::TaskID ID=m_TSTaskBarBase.GetTaskID(i);

					if (ID!=TSTask::INVALID_TASK_ID) {
						if (!EndTask(ID,!fExit))
							fSuccess=false;
					}
				}

				if (!fSuccess) {
					m_fEndAllTaskByUser=false;
					return;
				}

				if (fExit)
					Destroy();
			}
			return;
		}
	}

	bool CMainBoard::ShowMainMenu(int x,int y) const
	{
		HMENU hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_MAIN));

		::CheckMenuItem(hmenu,CM_ALWAYS_ON_TOP,
						MF_BYCOMMAND | (m_fAlwaysOnTop?MF_CHECKED:MF_UNCHECKED));
		::CheckMenuItem(hmenu,CM_TRANSLUCENT,
						MF_BYCOMMAND | (m_fTranslucent?MF_CHECKED:MF_UNCHECKED));
		::CheckMenuItem(hmenu,CM_INFORMATION_BAR,
						MF_BYCOMMAND | (m_fShowInformationBar?MF_CHECKED:MF_UNCHECKED));
		::EnableMenuItem(hmenu,CM_END_ALL_TASKS,
						 MF_BYCOMMAND | (m_TSTaskBarBase.GetTSTaskBarCount()>0?MF_ENABLED:MF_GRAYED));
		::EnableMenuItem(hmenu,CM_END_ALL_TASKS_AND_EXIT,
						 MF_BYCOMMAND | (m_TSTaskBarBase.GetTSTaskBarCount()>0?MF_ENABLED:MF_GRAYED));

		::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,x,y,0,m_hwnd,nullptr);
		::DestroyMenu(hmenu);

		return true;
	}

	static void FormatMenuString(LPCWSTR pszSrcString,TSTask::String *pMenuString)
	{
		pMenuString->clear();

		for (LPCWSTR p=pszSrcString;*p!=L'\0';p++) {
			if (*p==L'&')
				pMenuString->push_back(L'&');
			pMenuString->push_back(*p);
		}
	}

	static void FormatMenuString(const TSTask::String &SrcString,TSTask::String *pMenuString)
	{
		pMenuString->clear();

		for (auto Src=SrcString.begin();Src!=SrcString.end();Src++) {
			if (*Src==L'&')
				pMenuString->push_back(L'&');
			pMenuString->push_back(*Src);
		}
	}

	void CMainBoard::OnBaseLButtonDown(int x,int y)
	{
		POINT pt={x,y};
		::ClientToScreen(m_TSTaskBarBase.GetHandle(),&pt);
		SendMessage(WM_SYSCOMMAND,SC_MOVE | 2,MAKELPARAM(pt.x,pt.y));
	}

	void CMainBoard::OnBaseRButtonUp(int x,int y)
	{
		POINT pt={x,y};
		::ClientToScreen(m_TSTaskBarBase.GetHandle(),&pt);
		ShowMainMenu(pt.x,pt.y);
	}

	void CMainBoard::OnCaptionLButtonDown(TSTask::TaskID ID,int x,int y)
	{
		CTSTaskBar *pBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pBar->GetMaxRows()>1)
			pBar->SetMaxRows(1);
		else
			pBar->SetMaxRows(m_Core.GetSettings().MainBoard.GetStatusMaxRows());
		AdjustSize();
		SendSizeMessage();
	}

	void CMainBoard::OnCaptionRButtonUp(TSTask::TaskID ID,int x,int y)
	{
		HMENU hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_TASK));
		HMENU hmenuPopup=::GetSubMenu(hmenu,0);
		POINT pt={x,y};

		const CToolsSettings &ToolsSettings=m_Core.GetSettings().Tools;
		size_t ToolCount=ToolsSettings.GetToolCount();
		if (ToolCount>0) {
			if (ToolCount>CM_TASK_TOOL_LAST-CM_TASK_TOOL_FIRST+1)
				ToolCount=CM_TASK_TOOL_LAST-CM_TASK_TOOL_FIRST+1;

			unsigned int ItemCount=0;
			TSTask::String Text;

			for (size_t i=0;i<ToolCount;i++) {
				CToolsSettings::ToolInfo Info;

				ToolsSettings.GetToolInfo(i,&Info);
				if (Info.fShowInMenu) {
					FormatMenuString(Info.Name,&Text);
					::InsertMenu(hmenuPopup,ItemCount,
								 MF_BYPOSITION | MF_STRING | MF_ENABLED,
								 CM_TASK_TOOL_FIRST+i,
								 Text.c_str());
					ItemCount++;
				}
			}

			if (ItemCount>0) {
				::InsertMenu(hmenuPopup,ItemCount,
							 MF_BYPOSITION | MF_SEPARATOR,0,nullptr);
			}
		}

		::ClientToScreen(m_TSTaskBarBase.GetHandle(),&pt);
		int Result=::TrackPopupMenu(hmenuPopup,
									TPM_RIGHTBUTTON | TPM_RETURNCMD,
									pt.x,pt.y,0,m_hwnd,nullptr);
		::DestroyMenu(hmenu);

		if (Result!=0) {
			switch (Result) {
			case CM_TASK_END:
				EndTask(ID);
				break;

			case CM_TASK_SETTINGS:
				{
					CTSTaskManager &TSTaskManager=m_Core.GetTSTaskManager();
					TSTask::String Path;

					if (!TSTaskManager.GetSetting(ID,L"General.IniFilePath",&Path)) {
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"INI ファイルのパスを取得できません。",L"エラー");
						break;
					}

					TSTask::CTSTaskSettings TSTaskSettings;
					TSTask::CSettings Settings;

					if (TSTask::PathUtility::IsExists(Path)) {
						TSTask::OutLog(TSTask::LOG_VERBOSE,L"TSTask の設定を \"%s\" から読み込みます。",Path.c_str());
						if (Settings.Open(Path.c_str(),TSTask::CSettings::OPEN_READ)) {
							TSTaskSettings.Load(Settings);
							Settings.Close();
						}
					} else {
						TSTask::String DefaultIniPath;
						TSTask::GetModuleDirectory(nullptr,&DefaultIniPath);
						TSTask::PathUtility::Append(&DefaultIniPath,L"TSTask.default.ini");
						TSTask::OutLog(TSTask::LOG_INFO,
										L"INI ファイル \"%s\" が存在しないため、デフォルトの設定を \"%s\" からコピーします。",
										Path.c_str(),DefaultIniPath.c_str());
						if (!::CopyFile(DefaultIniPath.c_str(),Path.c_str(),TRUE)) {
							TSTask::OutSystemErrorLog(::GetLastError(),L"ファイルをコピーできません。");
						}
					}

					CTSTaskSettingsDialog Dialog(m_Core,TSTaskSettings);

					if (Dialog.Show(m_hwnd,m_hinst)) {
						TSTask::OutLog(TSTask::LOG_INFO,L"TSTask の設定を \"%s\" に保存します。",Path.c_str());
						if (Settings.Open(Path.c_str(),TSTask::CSettings::OPEN_WRITE)) {
							TSTaskSettings.Save(Settings);
							Settings.Flush();
							Settings.Close();

							TSTaskManager.SendIniSettingsChanged(ID,Path);
						}
					}
				}
				break;

			case CM_TASK_LOG:
				{
					CLogDialog Dialog(m_Core);

					Dialog.Show(m_hwnd,m_hinst,ID);
				}
				break;

			case CM_TASK_CHANNEL_LIST:
				{
					CChannelListDialog Dialog(m_Core.GetTSTaskManager());

					Dialog.Show(m_hwnd,m_hinst,ID);
				}
				break;

			case CM_TASK_INFO:
				{
					CTaskInfoDialog Dialog(m_Core.GetTSTaskManager());

					Dialog.Show(m_hwnd,m_hinst,ID);
				}
				break;

			case CM_TASK_SEND_MESSAGE:
				{
					CSendMessageDialog Dialog(m_Core.GetTSTaskManager());

					Dialog.Show(m_hwnd,m_hinst,ID);
				}
				break;

			default:
				if (Result>=CM_TASK_TOOL_FIRST && Result<=CM_TASK_TOOL_LAST) {
					CToolsSettings::ToolInfo Info;
					ToolsSettings.GetToolInfo(Result-CM_TASK_TOOL_FIRST,&Info);
					if (!m_Core.ExecuteTaskTool(ID,Info))
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"プログラムの実行ができません。",L"エラー");
					break;
				}
				break;
			}
		}
	}

	void CMainBoard::OnItemLButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y)
	{
		CTSTaskManager &TSTaskManager=m_Core.GetTSTaskManager();

		switch (ItemID) {
		case CTSTaskBar::ITEM_ID_TUNER:
			{
				static const int MENU_ID_SCANNED_CHANNEL=1,MENU_ID_CHANNEL=10000,MENU_ID_BONDRIVER=20000;

				HMENU hmenu=::CreatePopupMenu();
				TSTask::String Text;

				TSTask::CTuningSpaceList TuningSpaceList;
				TSTask::BonDriverTuningSpaceList BonDriverChannelList;

				if (TSTaskManager.GetScannedChannelList(ID,&TuningSpaceList)
						&& TuningSpaceList.GetSpaceCount()>0) {
					TSTask::ChannelInfo CurChannel;
					if (!TSTaskManager.GetChannel(ID,&CurChannel))
						CurChannel.Space=-1;
					int MenuID=MENU_ID_SCANNED_CHANNEL;

					for (size_t i=0;i<TuningSpaceList.GetSpaceCount();i++) {
						const TSTask::CChannelList *pChannelList=TuningSpaceList.GetChannelList(i);

						if (pChannelList->GetChannelCount()>0) {
							HMENU hmenuSub=::CreatePopupMenu();

							for (size_t j=0;j<pChannelList->GetChannelCount();j++) {
								if (pChannelList->IsChannelEnabled(j)) {
									FormatMenuString(pChannelList->GetChannelName(j),&Text);
									::AppendMenu(hmenuSub,MF_STRING | MF_ENABLED |
												 (int(i)==CurChannel.Space && int(j)==CurChannel.ScannedChannel?MF_CHECKED:MF_UNCHECKED),
												 MenuID,Text.c_str());
								}
								MenuID++;
							}

							TSTask::StringUtility::Format(Text,L"&%d: ",(int)i);
							TSTask::String TuningSpaceName;
							FormatMenuString(TuningSpaceList.GetTuningSpaceName(i),&TuningSpaceName);
							Text+=TuningSpaceName;
							::AppendMenu(hmenu,MF_POPUP | MF_ENABLED,
										 reinterpret_cast<UINT_PTR>(hmenuSub),Text.c_str());
						}
					}

					::AppendMenu(hmenu,MF_SEPARATOR,0,nullptr);
				} else if (TSTaskManager.GetBonDriverChannelList(ID,&BonDriverChannelList)
						&& !BonDriverChannelList.empty()) {
					TSTask::ChannelInfo CurChannel;
					if (!TSTaskManager.GetChannel(ID,&CurChannel))
						CurChannel.Space=-1;
					int MenuID=MENU_ID_CHANNEL;
					TSTask::String Temp;

					for (size_t i=0;i<BonDriverChannelList.size();i++) {
						const std::vector<TSTask::String> &ChannelList=BonDriverChannelList[i].ChannelList;
						if (!ChannelList.empty()) {
							HMENU hmenuSub=::CreatePopupMenu();

							for (size_t j=0;j<ChannelList.size();j++) {
								TSTask::StringUtility::Format(Text,L"&%d: ",(int)j+1);
								FormatMenuString(ChannelList[j],&Temp);
								Text+=Temp;
								::AppendMenu(hmenuSub,MF_STRING | MF_ENABLED |
											 (int(i)==CurChannel.Space && int(j)==CurChannel.Channel?MF_CHECKED:MF_UNCHECKED),
											 MenuID,Text.c_str());
								MenuID++;
							}

							TSTask::StringUtility::Format(Text,L"&%d: ",(int)i);
							FormatMenuString(BonDriverChannelList[i].Name,&Temp);
							Text+=Temp;
							::AppendMenu(hmenu,MF_POPUP | MF_ENABLED,
										 reinterpret_cast<UINT_PTR>(hmenuSub),Text.c_str());
						}
					}

					::AppendMenu(hmenu,MF_SEPARATOR,0,nullptr);
				}

				TSTask::String CurBonDriverPath;
				TSTaskManager.GetBonDriver(ID,&CurBonDriverPath,nullptr);

				TSTask::String BonDriverDir;
				CBonDriverManager::BonDriverFileList FileList;
				int CurBonDriverItem=-1;
				TSTask::String FileName;

				if (TSTaskManager.GetSetting(ID,L"BonDriver.LoadDirectory",&BonDriverDir)
						&& m_Core.GetBonDriverFileList(BonDriverDir.c_str(),&FileList)) {
					for (size_t i=0;i<FileList.size();i++) {
						if (CurBonDriverItem<0 && !CurBonDriverPath.empty()
								&& TSTask::StringUtility::CompareNoCase(FileList[i],CurBonDriverPath)==0)
							CurBonDriverItem=(int)i;
						TSTask::PathUtility::GetFileName(FileList[i],&FileName);
						FormatMenuString(FileName,&Text);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED | ((int)i==CurBonDriverItem?MF_CHECKED:MF_UNCHECKED),
									 MENU_ID_BONDRIVER+i,Text.c_str());
					}
				}

				if (CurBonDriverItem<0 && !CurBonDriverPath.empty()) {
					TSTask::PathUtility::AppendDelimiter(&BonDriverDir);
					if (TSTask::StringUtility::CompareNoCase(BonDriverDir,CurBonDriverPath,BonDriverDir.length())==0) {
						TSTask::PathUtility::GetFileName(CurBonDriverPath,&FileName);
						FormatMenuString(FileName,&Text);
					} else {
						WCHAR szPath[80];
						if (::PathCompactPathEx(szPath,CurBonDriverPath.c_str(),_countof(szPath),0))
							FormatMenuString(szPath,&Text);
						else
							FormatMenuString(CurBonDriverPath,&Text);
					}
					::AppendMenu(hmenu,MF_STRING | MF_CHECKED,0,Text.c_str());
				}

				int Result=ShowTSTaskBarItemMenu(hmenu,pTSTaskBar,ItemID);

				::DestroyMenu(hmenu);

				if (Result>0) {
					if (Result>=MENU_ID_BONDRIVER) {
						size_t Index=Result-MENU_ID_BONDRIVER;
						if (Index<FileList.size() && (int)Index!=CurBonDriverItem) {
							if (m_Core.GetSettings().Recording.GetConfirmChannelChange()) {
								TSTask::RecordingInfo RecInfo;

								if (TSTaskManager.GetRecordingInfo(ID,&RecInfo)
										&& RecInfo.State==TSTask::RECORDING_STATE_RECORDING) {
									if (ShowMessage(CMessageDialog::TYPE_QUESTION,
													L"現在録画中です。BonDriverを変更しますか?",
													L"BonDriver変更の確認",
													CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL)!=IDOK)
										return;
								}
							}

							if (!TSTaskManager.LoadBonDriver(ID,FileList[Index].c_str())) {
								ShowMessage(CMessageDialog::TYPE_ERROR,
											L"BonDriverをロードできません。",L"エラー");
							} else {
								if (!TSTaskManager.OpenTuner(ID)) {
									TSTaskManager.UnloadBonDriver(ID);
									ShowMessage(CMessageDialog::TYPE_ERROR,
												L"チューナーを開けません。",L"エラー");
								}
							}
						}
					} else if (Result>=MENU_ID_CHANNEL) {
						if (!ConfirmRecordingChannelChange(ID))
							return;

						int MenuID=MENU_ID_CHANNEL;

						for (size_t i=0;i<BonDriverChannelList.size();i++) {
							const std::vector<TSTask::String> &ChannelList=BonDriverChannelList[i].ChannelList;

							if (Result>=MenuID && Result<MenuID+(int)ChannelList.size()) {
								TSTaskManager.SetChannelByIndex(ID,(int)i,Result-MenuID,0);
								break;
							}

							MenuID+=(int)ChannelList.size();
						}
					} else if (Result>=MENU_ID_SCANNED_CHANNEL) {
						if (!ConfirmRecordingChannelChange(ID))
							return;

						int MenuID=MENU_ID_SCANNED_CHANNEL;

						for (size_t i=0;i<TuningSpaceList.GetSpaceCount();i++) {
							const TSTask::CChannelList *pChannelList=TuningSpaceList.GetChannelList(i);

							if (Result>=MenuID && Result<MenuID+(int)pChannelList->GetChannelCount()) {
								TSTaskManager.SetChannelByScanned(ID,(int)i,Result-MenuID,0);
								break;
							}

							MenuID+=(int)pChannelList->GetChannelCount();
						}
					}

					UpdateTSTaskBar(ID);
				}
			}
			break;

		case CTSTaskBar::ITEM_ID_CHANNEL:
			{
				static const int MENU_ID_SCANNED_CHANNEL=1,MENU_ID_CHANNEL=10000;
				TSTask::ChannelInfo CurChannel;
				bool fCurChannel=TSTaskManager.GetChannel(ID,&CurChannel);

				TSTask::CTuningSpaceList TuningSpaceList;
				TSTask::BonDriverTuningSpaceList BonDriverChannelList;

				HMENU hmenu=::CreatePopupMenu();
				TSTask::String Text;

				if (TSTaskManager.GetScannedChannelList(ID,&TuningSpaceList)
						&& TuningSpaceList.GetSpaceCount()>0) {
					int MenuID=MENU_ID_SCANNED_CHANNEL;

					if (fCurChannel && CurChannel.ScannedChannel>=0) {
						const TSTask::CChannelList *pChannelList=
							TuningSpaceList.GetChannelList(CurChannel.Space);

						if (pChannelList!=nullptr) {
							for (size_t i=0;i<pChannelList->GetChannelCount();i++) {
								if (pChannelList->IsChannelEnabled(i)) {
									FormatMenuString(pChannelList->GetChannelName(i),&Text);
									::AppendMenu(hmenu,MF_STRING | MF_ENABLED |
												 (fCurChannel && (int)i==CurChannel.ScannedChannel?MF_CHECKED:MF_UNCHECKED),
												 MenuID,Text.c_str());
								}
								MenuID++;
							}
						}
					} else {
						for (size_t i=0;i<TuningSpaceList.GetSpaceCount();i++) {
							const TSTask::CChannelList *pChannelList=TuningSpaceList.GetChannelList(i);

							for (size_t j=0;j<pChannelList->GetChannelCount();j++) {
								if (pChannelList->IsChannelEnabled(j)) {
									FormatMenuString(pChannelList->GetChannelName(j),&Text);
									::AppendMenu(hmenu,MF_STRING | MF_ENABLED,
												 MenuID,Text.c_str());
								}
								MenuID++;
							}
						}
					}
				} else if (TSTaskManager.GetBonDriverChannelList(ID,&BonDriverChannelList)) {
					int MenuID=MENU_ID_CHANNEL;
					TSTask::String Temp;

					if (fCurChannel && CurChannel.Space>=0 && CurChannel.Space<(int)BonDriverChannelList.size()) {
						const std::vector<TSTask::String> &ChannelList=BonDriverChannelList[CurChannel.Space].ChannelList;
						for (size_t i=0;i<ChannelList.size();i++) {
							TSTask::StringUtility::Format(Text,L"&%d: ",(int)i+1);
							FormatMenuString(ChannelList[i],&Temp);
							Text+=Temp;
							::AppendMenu(hmenu,MF_STRING | MF_ENABLED |
										 ((int)i==CurChannel.Channel?MF_CHECKED:MF_UNCHECKED),
										 MenuID,Text.c_str());
							MenuID++;
						}
					} else {
						for (size_t i=0;i<BonDriverChannelList.size();i++) {
							const std::vector<TSTask::String> &ChannelList=BonDriverChannelList[i].ChannelList;
							if (!ChannelList.empty()) {
								HMENU hmenuSub=::CreatePopupMenu();

								for (size_t j=0;j<ChannelList.size();j++) {
									TSTask::StringUtility::Format(Text,L"&%d: ",(int)j+1);
									FormatMenuString(ChannelList[j],&Temp);
									Text+=Temp;
									::AppendMenu(hmenuSub,MF_STRING | MF_ENABLED,MenuID,Text.c_str());
									MenuID++;
								}

								TSTask::StringUtility::Format(Text,L"&%d: ",(int)i);
								FormatMenuString(BonDriverChannelList[i].Name,&Temp);
								Text+=Temp;
								::AppendMenu(hmenu,MF_POPUP | MF_ENABLED,
											 reinterpret_cast<UINT_PTR>(hmenuSub),Text.c_str());
							}
						}
					}
				} else {
					::DestroyMenu(hmenu);
					break;
				}

				int Result=ShowTSTaskBarItemMenu(hmenu,pTSTaskBar,ItemID);

				::DestroyMenu(hmenu);

				if (Result>0) {
					if (!ConfirmRecordingChannelChange(ID))
						return;

					if (Result>=MENU_ID_CHANNEL) {
						if (fCurChannel && CurChannel.Space>=0 && CurChannel.Space<(int)BonDriverChannelList.size()) {
							TSTaskManager.SetChannelByIndex(ID,CurChannel.Space,Result-MENU_ID_CHANNEL,0);
						} else {
							int MenuID=MENU_ID_CHANNEL;

							for (size_t i=0;i<BonDriverChannelList.size();i++) {
								const std::vector<TSTask::String> &ChannelList=BonDriverChannelList[i].ChannelList;

								if (Result>=MenuID && Result<MenuID+(int)ChannelList.size()) {
									TSTaskManager.SetChannelByIndex(ID,(int)i,Result-MenuID,0);
									break;
								}

								MenuID+=(int)ChannelList.size();
							}
						}
					} else {
						if (fCurChannel && CurChannel.ScannedChannel>=0) {
							TSTaskManager.SetChannelByScanned(ID,CurChannel.Space,Result-MENU_ID_SCANNED_CHANNEL,0);
						} else {
							int MenuID=MENU_ID_SCANNED_CHANNEL;

							for (size_t i=0;i<TuningSpaceList.GetSpaceCount();i++) {
								const TSTask::CChannelList *pChannelList=TuningSpaceList.GetChannelList(i);

								if (Result>=MenuID && Result<MenuID+(int)pChannelList->GetChannelCount()) {
									TSTaskManager.SetChannelByScanned(ID,(int)i,Result-MenuID,0);
									break;
								}

								MenuID+=(int)pChannelList->GetChannelCount();
							}
						}
					}

					UpdateTSTaskBar(ID);
				}
			}
			break;

		case CTSTaskBar::ITEM_ID_ERROR:
			TSTaskManager.ResetErrorStatistics(ID);
			UpdateStatistics(ID);
			break;

		case CTSTaskBar::ITEM_ID_RECORDING:
			{
				TSTask::RecordingInfo RecInfo;

				if (!TSTaskManager.GetRecordingInfo(ID,&RecInfo)) {
					ShowMessage(CMessageDialog::TYPE_ERROR,
								L"録画の状態を取得できません。",L"エラー");
					return;
				}

				if (RecInfo.State==TSTask::RECORDING_STATE_NOT_RECORDING) {
#if 0
					TSTask::RecordingSettings Settings;
					m_Core.GetSettings().Recording.GetRecordingSettings(&Settings);

					if (!TSTaskManager.StartRecording(ID,Settings)) {
#else
					if (!TSTaskManager.StartRecording(ID)) {
#endif
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"録画を開始できません。",L"エラー");
					}
				} else {
					if (m_Core.GetSettings().Recording.GetConfirmStop()) {
						bool fDontAskAgain=false;
						int Result=
							ShowMessage(CMessageDialog::TYPE_QUESTION,
										L"録画を停止しますか?",L"録画停止の確認",
										CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL,
										&fDontAskAgain);
						if (fDontAskAgain)
							m_Core.GetSettings().Recording.SetConfirmStop(false);
						if (Result!=IDOK)
							return;
					}

					if (!TSTaskManager.StopRecording(ID)) {
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"録画を停止できません。",L"エラー");
					}
				}

				UpdateRecordingStatus(ID);
			}
			break;

		case CTSTaskBar::ITEM_ID_STREAMING:
			{
				TSTask::StreamingInfo StreamingInfo;

				if (TSTaskManager.GetStreamingInfo(ID,&StreamingInfo)) {
					if (!TSTaskManager.StopStreaming(ID)) {
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"ストリーミングを停止できません。",L"エラー");
					}
				} else {
#if 0
					TSTask::StreamingInfo Streaming;

					m_Core.GetSettings().Streaming.GetStreamingSettings(&Streaming);
					if (!TSTaskManager.StartStreaming(ID,Streaming)) {
#else
					if (!TSTaskManager.StartStreaming(ID)) {
#endif
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"ストリーミングを開始できません。",L"エラー");
					}
				}

				UpdateStreamingStatus(ID);
			}
			break;
		}
	}

	void CMainBoard::OnItemRButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y)
	{
		CTSTaskManager &TSTaskManager=m_Core.GetTSTaskManager();

		switch (ItemID) {
		case CTSTaskBar::ITEM_ID_CHANNEL:
			{
				TSTask::ServiceList ServiceList;
				if (!TSTaskManager.GetServiceList(ID,&ServiceList))
					return;

				TSTask::ServiceInfo CurService;
				bool fCurService=TSTaskManager.GetService(ID,&CurService);

				HMENU hmenu=::CreatePopupMenu();
				TSTask::String Text,Temp;

				for (size_t i=0;i<ServiceList.size();i++) {
					const TSTask::ServiceInfo &Info=ServiceList[i];
					TSTask::StringUtility::Format(Text,L"&%d: ",int(i)+1);
					FormatMenuString(Info.ServiceName,&Temp);
					Text+=Temp;
					TSTask::StringUtility::Format(Temp,L" (%d)",Info.ServiceID);
					Text+=Temp;
					if (Info.ServiceType==SERVICE_TYPE_DIGITALAUDIO)
						Text+=L" [音声]";
					else if (Info.ServiceType==SERVICE_TYPE_DATA)
						Text+=L" [データ/ワンセグ]";
					::AppendMenu(hmenu,MF_STRING | MF_ENABLED | (fCurService && Info.ServiceID==CurService.ServiceID?MF_CHECKED:MF_UNCHECKED),
								 i+1,Text.c_str());
				}

				int Result=ShowTSTaskBarItemMenu(hmenu,pTSTaskBar,ItemID);

				::DestroyMenu(hmenu);

				if (Result>0 && size_t(Result)<=ServiceList.size()) {
					TSTaskManager.SetService(ID,ServiceList[Result-1].ServiceID);
				}
			}
			break;
		}
	}

	void CMainBoard::OnInformationBarLButtonDown(CInformationBar *pBar,int x,int y)
	{
		POINT pt={x,y};
		::ClientToScreen(pBar->GetHandle(),&pt);
		SendMessage(WM_SYSCOMMAND,SC_MOVE | 2,MAKELPARAM(pt.x,pt.y));
	}

	void CMainBoard::OnInformationBarRButtonUp(CInformationBar *pBar,int x,int y)
	{
		POINT pt={x,y};
		::ClientToScreen(pBar->GetHandle(),&pt);
		ShowMainMenu(pt.x,pt.y);
	}

	void CMainBoard::OnLog(const TSTask::LogInfo &Info)
	{
		if (Info.Type==TSTask::LOG_WARNING || Info.Type==TSTask::LOG_ERROR) {
			PostMessage(WM_APP_ERROR_LOG,Info.Number,0);
		}
	}

	bool CMainBoard::EndTask(TSTask::TaskID ID,bool fConfirm)
	{
		CTSTaskManager &TSTaskManager=m_Core.GetTSTaskManager();

		if (fConfirm && m_Core.GetSettings().MainBoard.GetConfirmTaskExit()) {
			TSTask::String Message;
			TSTask::StringUtility::Format(Message,
										   L"タスク%uを終了させてもいいですか?",
										   ID);
			bool fDontAskAgain=false;
			int Result=
				ShowMessage(CMessageDialog::TYPE_QUESTION,
							Message.c_str(),L"終了の確認",
							CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL,
							&fDontAskAgain);
			if (fDontAskAgain)
				m_Core.GetSettings().MainBoard.SetConfirmTaskExit(false);
			if (Result!=IDOK)
				return false;
		} else if (m_Core.GetSettings().Recording.GetConfirmExit()) {
			TSTask::RecordingInfo RecInfo;

			if (TSTaskManager.GetRecordingInfo(ID,&RecInfo)
					&& RecInfo.State==TSTask::RECORDING_STATE_RECORDING) {
				TSTask::String Message;
				TSTask::StringUtility::Format(Message,
											   L"現在録画中です。タスク%uを終了させてもいいですか?",
											   ID);
				bool fDontAskAgain=false;
				int Result=
					ShowMessage(CMessageDialog::TYPE_QUESTION,
								Message.c_str(),L"終了の確認",
								CMessageDialog::STYLE_OK_CANCEL | CMessageDialog::STYLE_DEFAULT_CANCEL,
								&fDontAskAgain);
				if (fDontAskAgain)
					m_Core.GetSettings().Recording.SetConfirmExit(false);
				if (Result!=IDOK)
					return false;
			}
		}

		if (!TSTaskManager.EndTask(ID)) {
			if (TSTask::TaskUtility::IsServerTaskExists(ID)) {
				TSTask::String Message;
				TSTask::StringUtility::Format(Message,
											   L"タスク%uを終了できません。強制終了させますか?",
											   ID);
				if (ShowMessage(CMessageDialog::TYPE_WARNING,
								Message.c_str(),L"タスクの終了",
								CMessageDialog::STYLE_OK_CANCEL)==IDOK) {
					if (TSTaskManager.TerminateTask(ID)) {
						RemoveTSTaskBar(ID);
					} else {
						ShowMessage(CMessageDialog::TYPE_ERROR,
									L"タスクを強制終了できません。",L"タスクの強制終了",
									CMessageDialog::STYLE_OK);
					}
				}
			} else {
				RemoveTSTaskBar(ID);
				TSTaskManager.RemoveTask(ID);
			}
		}

		return true;
	}

	bool CMainBoard::UpdateTSTaskBar(TSTask::TaskID ID)
	{
		UpdateTunerStatus(ID);
		UpdateChannelStatus(ID);
		UpdateStatistics(ID);
		UpdateRecordingStatus(ID);
		UpdateStreamingStatus(ID);
		UpdateEventStatus(ID);

		return true;
	}

	bool CMainBoard::UpdateTunerStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::String Text;
		if (m_Core.GetTSTaskManager().GetBonDriver(ID,nullptr,&Text,true))
			pTSTaskBar->SetTunerName(Text.c_str());
		else
			pTSTaskBar->SetTunerName(nullptr);

		return true;
	}

	bool CMainBoard::UpdateChannelStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::ChannelInfo Channel;
		if (m_Core.GetTSTaskManager().GetChannel(ID,&Channel)) {
			pTSTaskBar->SetTuningSpaceName(Channel.SpaceName.c_str());

			TSTask::ServiceInfo Service;
			if (((Channel.ServiceID!=0 && Channel.ServiceID!=Channel.ScannedServiceID)
						|| Channel.ChannelName.empty())
					&& m_Core.GetTSTaskManager().GetService(ID,&Service)
					&& !Service.ServiceName.empty()) {
				pTSTaskBar->SetChannelName(Service.ServiceName.c_str());
			} else {
				pTSTaskBar->SetChannelName(Channel.ChannelName.c_str());
			}
		} else {
			pTSTaskBar->SetTuningSpaceName(nullptr);
			pTSTaskBar->SetChannelName(nullptr);
		}

		return true;
	}

	bool CMainBoard::UpdateStatistics(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::StreamStatistics StreamStats;
		if (m_Core.GetTSTaskManager().GetStreamStatistics(ID,&StreamStats)) {
			pTSTaskBar->SetSignalStatus(StreamStats.SignalLevel,StreamStats.BitRate);
			pTSTaskBar->SetErrorStatistics(StreamStats.ErrorPacketCount,
											StreamStats.DiscontinuityCount,
											StreamStats.ScramblePacketCount);
		} else {
			pTSTaskBar->SetSignalStatus(0.0f,0);
			pTSTaskBar->SetErrorStatistics(0,0,0);
		}

		return true;
	}

	bool CMainBoard::UpdateRecordingStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::RecordingInfo RecInfo;
		if (m_Core.GetTSTaskManager().GetRecordingInfo(ID,&RecInfo))
			pTSTaskBar->SetRecordingInfo(RecInfo);

		return true;
	}

	bool CMainBoard::UpdateRecordingTime(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		pTSTaskBar->UpdateRecordingTime();

		return true;
	}

	bool CMainBoard::UpdateStreamingStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::StreamingInfo Info;
		if (m_Core.GetTSTaskManager().GetStreamingInfo(ID,&Info))
			pTSTaskBar->SetStreamingInfo(&Info);
		else
			pTSTaskBar->SetStreamingInfo(nullptr);

		return true;
	}

	bool CMainBoard::UpdateEventStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		TSTask::EventInfo Info;
		if (m_Core.GetTSTaskManager().GetEventInfo(ID,&Info))
			pTSTaskBar->SetEventInfo(Info.EventName.c_str(),
									  Info.StartTime.wYear!=0?&Info.StartTime:nullptr,
									  Info.Duration);
		else
			pTSTaskBar->SetEventInfo(nullptr,nullptr,0);

		return true;
	}

	bool CMainBoard::UpdateClockStatus(TSTask::TaskID ID)
	{
		CTSTaskBar *pTSTaskBar=m_TSTaskBarBase.GetTSTaskBar(ID);

		if (pTSTaskBar==nullptr)
			return false;

		SYSTEMTIME Time;
		if (m_Core.GetTSTaskManager().GetTotTime(ID,&Time)) {
			pTSTaskBar->SetClockTime(CTSTaskBar::CLOCK_TOT,Time);
		} else {
			pTSTaskBar->SetClockTime(CTSTaskBar::CLOCK_INVALID,Time);
		}

		return true;
	}

	int CMainBoard::ShowTSTaskBarItemMenu(HMENU hmenu,CTSTaskBar *pTSTaskBar,int ItemID)
	{
		RECT rc;
		pTSTaskBar->GetItemRect(ItemID,&rc);
		::MapWindowPoints(pTSTaskBar->GetHandle(),nullptr,
						  reinterpret_cast<POINT*>(&rc),2);

		TPMPARAMS tpm;
		tpm.cbSize=sizeof(tpm);
		tpm.rcExclude=rc;

		return ::TrackPopupMenuEx(
			hmenu,TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_VERTICAL,
			rc.left,rc.bottom,m_hwnd,&tpm);
	}


	const LPCTSTR CTSTaskBarBase::m_pszWindowClass=APP_NAME_W TEXT("_TSTaskBarBase");
	HINSTANCE CTSTaskBarBase::m_hinst=nullptr;

	bool CTSTaskBarBase::Initialize(HINSTANCE hinst)
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

		if (!CTSTaskBar::Initialize(hinst))
			return false;

		return true;
	}

	bool CTSTaskBarBase::GetDefaultTheme(Theme::ItemStyle *pStyle)
	{
		if (pStyle==nullptr)
			return false;

		pStyle->Background.Fill.Type=Theme::FILL_GRADIENT;
		pStyle->Background.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
		pStyle->Background.Fill.Gradient.Color1.Set(64,64,64);
		pStyle->Background.Fill.Gradient.Color2.Set(0,0,0);
		pStyle->Background.Border.Type=Theme::BORDER_NONE;
		pStyle->Foreground.Fill.Type=Theme::FILL_SOLID;
		pStyle->Foreground.Fill.Solid.Color.Set(255,255,255);
		pStyle->Foreground.Glow.Type=Theme::GLOW_FADEOUT;
		pStyle->Foreground.Glow.Color.Set(224,240,255,160);
		pStyle->Foreground.Glow.Radius=4;
		pStyle->Foreground.AntiAliasing=Theme::ANTIALIASING_ANTIALIAS;

		return true;
	}

	CTSTaskBarBase::CTSTaskBarBase(CTSTaskCentreCore &Core)
		: CLayeredWidget(Core.GetGraphicSystem())
		, m_Core(Core)
		, m_pEventHandler(nullptr)
	{
		GetDefaultTheme(&m_Style);

		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_CAPTION,&m_CaptionFont);
		m_CaptionFont.lfHeight=m_CaptionFont.lfHeight*6/5;
		m_CaptionFont.lfWeight=FW_BOLD;
		m_CaptionFont.lfItalic=1;
	}

	CTSTaskBarBase::~CTSTaskBarBase()
	{
		Destroy();
	}

	bool CTSTaskBarBase::Create(HWND hwndParent,int ID)
	{
		if (m_hwnd!=nullptr)
			return false;

		if (!CreateBasicWindow(hwndParent,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,
							   ID,m_pszWindowClass,nullptr,m_hinst))
			return false;

		return true;
	}

	bool CTSTaskBarBase::CreateTSTaskBar(TSTask::TaskID ID,LPCWSTR pszCaption,const CTSTaskBar::ThemeInfo &Theme)
	{
		if (m_hwnd==nullptr)
			return false;

		if (m_TSTaskBarList.find(ID)!=m_TSTaskBarList.end())
			return false;

		CTSTaskBar *pTSTaskBar=new CTSTaskBar(m_Core.GetGraphicSystem());
		if (!pTSTaskBar->Create(m_hwnd)) {
			delete pTSTaskBar;
			return false;
		}

		const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

		pTSTaskBar->SetTheme(Theme);

		LOGFONT lf;
		Settings.MainBoard.GetStatusFont(&lf);
		pTSTaskBar->SetFont(lf);
		Settings.MainBoard.GetStatusCaptionFont(&lf);
		pTSTaskBar->SetCaptionFont(lf);

		pTSTaskBar->SetCaptionWidth(pTSTaskBar->GetItemHeight()*2);
		pTSTaskBar->SetCaptionText(pszCaption);

		pTSTaskBar->SetItemMargin(m_Core.ScaleDPI(pTSTaskBar->GetDefaultItemMargin()));

		if (Settings.MainBoard.GetStatusMultiRow())
			pTSTaskBar->SetMaxRows(Settings.MainBoard.GetStatusMaxRows());

		pTSTaskBar->SetEventHandler(this);

		CMainBoardSettings::StatusItemList ItemList;
		m_Core.GetSettings().MainBoard.GetStatusItemListSortedByOrder(&ItemList);
		std::vector<int> OrderList;
		OrderList.resize(ItemList.size());
		for (size_t i=0;i<ItemList.size();i++)
			OrderList[i]=ItemList[i].ID;
		for (size_t j=0;j<ItemList.size();j++) {
			pTSTaskBar->SetItemWidth(ItemList[j].ID,ItemList[j].Width);
			pTSTaskBar->SetItemVisible(ItemList[j].ID,ItemList[j].fVisible);
		}
		pTSTaskBar->SetItemOrder(&OrderList[0]);

		m_TSTaskBarList.insert(std::pair<TSTask::TaskID,CTSTaskBar*>(ID,pTSTaskBar));
		m_TSTaskBarOrder.push_back(ID);

		Layout();

		if (m_TSTaskBarList.size()==1)
			Invalidate();

		pTSTaskBar->SetVisible(true);

		return true;
	}

	bool CTSTaskBarBase::RemoveTSTaskBar(TSTask::TaskID ID)
	{
		TSTaskBarList::iterator i=m_TSTaskBarList.find(ID);
		if (i==m_TSTaskBarList.end())
			return false;

		i->second->Destroy();
		delete i->second;
		m_TSTaskBarList.erase(i);

		auto itrID=std::find(m_TSTaskBarOrder.begin(),m_TSTaskBarOrder.end(),ID);
		if (itrID!=m_TSTaskBarOrder.end())
			m_TSTaskBarOrder.erase(itrID);

		Layout();

		if (m_TSTaskBarList.empty())
			Invalidate();

		return true;
	}

	CTSTaskBar *CTSTaskBarBase::GetTSTaskBar(TSTask::TaskID ID)
	{
		TSTaskBarList::iterator i=m_TSTaskBarList.find(ID);
		if (i==m_TSTaskBarList.end())
			return nullptr;

		return i->second;
	}

	TSTask::TaskID CTSTaskBarBase::GetTaskID(int Index) const
	{
		if (Index<0 || (size_t)Index>=m_TSTaskBarOrder.size())
			return TSTask::INVALID_TASK_ID;

		return m_TSTaskBarOrder[Index];
	}

	int CTSTaskBarBase::CalcHeight() const
	{
		SIZE sz;
		int Height;

		GetClientSize(&sz);
		Height=0;
		for (const auto &e:m_TSTaskBarList)
			Height+=e.second->CalcHeight(sz.cx);

		return Height;
	}

	void CTSTaskBarBase::SetEventHandler(CEventHandler *pEventHandler)
	{
		m_pEventHandler=pEventHandler;
	}

	bool CTSTaskBarBase::SetFont(const LOGFONT &StatusFont,const LOGFONT &CaptionFont)
	{
		for (auto &e:m_TSTaskBarList) {
			e.second->SetFont(StatusFont);
			e.second->SetCaptionFont(CaptionFont);
		}

		Layout();

		return true;
	}

	bool CTSTaskBarBase::SetTheme(const Theme::ItemStyle &Style)
	{
		m_Style=Style;

		if (m_hwnd!=nullptr)
			Invalidate();

		return true;
	}

	bool CTSTaskBarBase::SetTSTaskBarTheme(TSTask::TaskID ID,const CTSTaskBar::ThemeInfo &Theme)
	{
		TSTaskBarList::iterator i=m_TSTaskBarList.find(ID);
		if (i==m_TSTaskBarList.end())
			return false;

		return i->second->SetTheme(Theme);
	}

	LRESULT CTSTaskBarBase::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_SIZE:
			Layout(LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_LBUTTONDOWN:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnBaseLButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_LBUTTONUP:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnBaseLButtonUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONDOWN:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnBaseRButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONUP:
			if (m_pEventHandler!=nullptr)
				m_pEventHandler->OnBaseRButtonUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			return 0;

		case WM_DESTROY:
			for (auto &e:m_TSTaskBarList)
				delete e.second;
			m_TSTaskBarList.clear();
			break;
		}

		return CLayeredWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CTSTaskBarBase::OnCaptionLButtonDown(CTSTaskBar *pTSTaskBar,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			POINT pt={x,y};
			::MapWindowPoints(pTSTaskBar->GetHandle(),m_hwnd,&pt,1);
			m_pEventHandler->OnCaptionLButtonDown(i->first,pt.x,pt.y);
		}
	}

	void CTSTaskBarBase::OnCaptionLButtonUp(CTSTaskBar *pTSTaskBar,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			POINT pt={x,y};
			::MapWindowPoints(pTSTaskBar->GetHandle(),m_hwnd,&pt,1);
			m_pEventHandler->OnCaptionLButtonUp(i->first,pt.x,pt.y);
		}
	}

	void CTSTaskBarBase::OnCaptionRButtonDown(CTSTaskBar *pTSTaskBar,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			POINT pt={x,y};
			::MapWindowPoints(pTSTaskBar->GetHandle(),m_hwnd,&pt,1);
			m_pEventHandler->OnCaptionRButtonDown(i->first,pt.x,pt.y);
		}
	}

	void CTSTaskBarBase::OnCaptionRButtonUp(CTSTaskBar *pTSTaskBar,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			POINT pt={x,y};
			::MapWindowPoints(pTSTaskBar->GetHandle(),m_hwnd,&pt,1);
			m_pEventHandler->OnCaptionRButtonUp(i->first,pt.x,pt.y);
		}
	}

	void CTSTaskBarBase::OnItemLButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			m_pEventHandler->OnItemLButtonDown(i->first,pTSTaskBar,ID,x,y);
		}
	}

	void CTSTaskBarBase::OnItemRButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y)
	{
		if (m_pEventHandler!=nullptr) {
			auto i=FindTSTaskBar(pTSTaskBar);
			if (i==m_TSTaskBarList.end())
				return;

			m_pEventHandler->OnItemRButtonDown(i->first,pTSTaskBar,ID,x,y);
		}
	}

	void CTSTaskBarBase::Layout(int Width,int Height)
	{
		int y;

		y=0;
		for (auto e:m_TSTaskBarOrder) {
			CTSTaskBar *pBar=m_TSTaskBarList.find(e)->second;
			int BarHeight=pBar->CalcHeight(Width);

			pBar->SetPosition(0,y,Width,BarHeight);
			y+=BarHeight;
		}
	}

	void CTSTaskBarBase::Layout()
	{
		SIZE sz;

		GetClientSize(&sz);
		Layout(sz.cx,sz.cy);
	}

	void CTSTaskBarBase::Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);

		RECT rc;
		GetClientRect(&rc);

		StylePainter.DrawBackground(m_Style.Background,rc);

		if (m_TSTaskBarList.empty()) {
			Graphics::CFont *pFont=m_GraphicSystem.CreateFont(m_CaptionFont);
			m_Style.Background.CalcContentRect(&rc);
			StylePainter.DrawText(m_Style.Foreground,APP_NAME_W,
								  rc,rc,pFont,
								  Graphics::TEXT_FORMAT_HORIZONTAL_CENTER |
								  Graphics::TEXT_FORMAT_VERTICAL_CENTER |
								  Graphics::TEXT_FORMAT_NO_WRAP);
			delete pFont;
		}
	}

	bool CTSTaskBarBase::IsTransparentBackground() const
	{
		return m_Style.Background.IsTransparent();
	}

	CTSTaskBarBase::TSTaskBarList::iterator CTSTaskBarBase::FindTSTaskBar(const CTSTaskBar *pTSTaskBar)
	{
		return std::find_if(m_TSTaskBarList.begin(),m_TSTaskBarList.end(),
							[pTSTaskBar](std::pair<TSTask::TaskID,CTSTaskBar*> o) { return o.second==pTSTaskBar; });
	}

}
