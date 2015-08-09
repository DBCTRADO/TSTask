#ifndef TSTASKCENTRE_MAIN_BOARD_H
#define TSTASKCENTRE_MAIN_BOARD_H


#include <map>
#include "TSTaskCentreCore.h"
#include "TSTaskBar.h"
#include "InformationBar.h"
#include "TaskTrayManager.h"
#include "TSTaskCentreSettings.h"
#include "TSTaskCentreSettingsDialog.h"
#include "MessageDialog.h"
#include "AboutDialog.h"


namespace TSTaskCentre
{

	class CTSTaskBarBase : public CLayeredWidget, protected CTSTaskBar::CEventHandler
	{
	public:
		class TSTASK_ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() {}
			virtual void OnBaseLButtonDown(int x,int y) {}
			virtual void OnBaseLButtonUp(int x,int y) {}
			virtual void OnBaseRButtonDown(int x,int y) {}
			virtual void OnBaseRButtonUp(int x,int y) {}
			virtual void OnCaptionLButtonDown(TSTask::TaskID ID,int x,int y) {}
			virtual void OnCaptionLButtonUp(TSTask::TaskID ID,int x,int y) {}
			virtual void OnCaptionRButtonDown(TSTask::TaskID ID,int x,int y) {}
			virtual void OnCaptionRButtonUp(TSTask::TaskID ID,int x,int y) {}
			virtual void OnItemLButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y) {}
			virtual void OnItemRButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y) {}
		};

		static bool Initialize(HINSTANCE hinst);
		static bool GetDefaultTheme(Theme::ItemStyle *pStyle);

		CTSTaskBarBase(CTSTaskCentreCore &Core);
		~CTSTaskBarBase();
		bool Create(HWND hwndParent,int ID=0) override;

		bool CreateTSTaskBar(TSTask::TaskID ID,LPCWSTR pszCaption,const CTSTaskBar::ThemeInfo &Theme);
		bool RemoveTSTaskBar(TSTask::TaskID ID);
		int GetTSTaskBarCount() const { return (int)m_TSTaskBarList.size(); }
		CTSTaskBar *GetTSTaskBar(TSTask::TaskID ID);
		TSTask::TaskID GetTaskID(int Index) const;
		int CalcHeight() const;
		void SetEventHandler(CEventHandler *pEventHandler);
		bool SetFont(const LOGFONT &StatusFont,const LOGFONT &CaptionFont);
		bool SetTheme(const Theme::ItemStyle &Style);
		bool SetTSTaskBarTheme(TSTask::TaskID ID,const CTSTaskBar::ThemeInfo &Theme);

	private:
	// CCustomWidget
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CTSTaskBar::CEventHandler
		void OnCaptionLButtonDown(CTSTaskBar *pTSTaskBar,int x,int y) override;
		void OnCaptionLButtonUp(CTSTaskBar *pTSTaskBar,int x,int y) override;
		void OnCaptionRButtonDown(CTSTaskBar *pTSTaskBar,int x,int y) override;
		void OnCaptionRButtonUp(CTSTaskBar *pTSTaskBar,int x,int y) override;
		void OnItemLButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y) override;
		void OnItemRButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y) override;

	// CLayeredWidget
		void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) override;
		bool IsTransparentBackground() const override;

		void Layout(int Width,int Height);
		void Layout();

		typedef std::map<TSTask::TaskID,CTSTaskBar*> TSTaskBarList;

		TSTaskBarList::iterator FindTSTaskBar(const CTSTaskBar *pTSTaskBar);

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		CTSTaskCentreCore &m_Core;
		TSTaskBarList m_TSTaskBarList;
		std::vector<TSTask::TaskID> m_TSTaskBarOrder;
		CEventHandler *m_pEventHandler;
		Theme::ItemStyle m_Style;
		LOGFONT m_CaptionFont;
	};

	class CMainBoard
		: public CLayeredWidget
		, protected CTSTaskBarBase::CEventHandler
		, protected CInformationBar::CEventHandler
		, public TSTask::CBasicLogger::CHandler
	{
	public:
		struct ThemeInfo
		{
			Theme::BackgroundStyle Background;
			Theme::ItemStyle TSTaskBarBase;
			struct
			{
				Theme::ItemStyle Active;
				Theme::ItemStyle Inactive;
			} Caption;
			CTSTaskBar::ThemeInfo TSTaskBar;
			Theme::ItemStyle TvRockCaptionStyles[8];
			CInformationBar::ThemeInfo InformationBar;
		};

		static bool Initialize(HINSTANCE hinst);
		static bool GetDefaultTheme(ThemeInfo *pTheme);

		CMainBoard(CTSTaskCentreCore &Core,
				   CSettingsHandler *pSettingsHandler);
		~CMainBoard();
		bool Create(HWND hwndParent=nullptr,int ID=0) override;
		bool Show(int CmdShow);

		bool CreateTSTaskBar(TSTask::TaskID ID);
		bool RemoveTSTaskBar(TSTask::TaskID ID);
		bool SendCreateTSTaskBar(TSTask::TaskID ID);
		bool SendRemoveTSTaskBar(TSTask::TaskID ID);
		bool AdjustSize();

		bool SetTheme(const ThemeInfo &Theme);

		bool SetShowTrayIcon(bool fShow);
		bool SetMinimizeToTray(bool fMinimizeToTray);
		bool SetShowInTaskbar(bool fShow);
		bool SetAlwaysOnTop(bool fAlwaysOnTop);
		bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
		bool SetTranslucent(bool fTranslucent);
		bool GetTranslucent() const { return m_fTranslucent; }
		bool ShowInformationBar(bool fShow);
		bool RestoreSettings(const CMainBoardSettings &Settings);
		bool ChangeSettings(const CMainBoardSettings &Settings);
		bool GetSettings(CMainBoardSettings *pSettings) const;

		bool NotifyBonDriverLoaded(TSTask::TaskID ID);
		bool NotifyBonDriverUnloaded(TSTask::TaskID ID);
		bool NotifyTunerOpened(TSTask::TaskID ID);
		bool NotifyTunerClosed(TSTask::TaskID ID);
		bool NotifyChannelChanged(TSTask::TaskID ID);
		bool NotifyServiceChanged(TSTask::TaskID ID);
		bool NotifyRecordingStarted(TSTask::TaskID ID);
		bool NotifyRecordingStopped(TSTask::TaskID ID);
		bool NotifyRecordingFileChanged(TSTask::TaskID ID);
		bool NotifyStreamingStarted(TSTask::TaskID ID);
		bool NotifyStreamingStopped(TSTask::TaskID ID);
		bool NotifyEventChanged(TSTask::TaskID ID);

	private:
	// CCustomWidget
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	// CLayeredWidget
		void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) override;
		bool IsTransparentBackground() const override;

	// CTSTaskBarBase::CEventHandler
		void OnCaptionLButtonDown(TSTask::TaskID ID,int x,int y) override;
		void OnCaptionRButtonUp(TSTask::TaskID ID,int x,int y) override;
		void OnBaseLButtonDown(int x,int y) override;
		void OnBaseRButtonUp(int x,int y) override;
		void OnItemLButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y) override;
		void OnItemRButtonDown(TSTask::TaskID ID,CTSTaskBar *pTSTaskBar,int ItemID,int x,int y) override;

	// CInformationBar::CEventHandler
		void OnInformationBarLButtonDown(CInformationBar *pBar,int x,int y) override;
		void OnInformationBarRButtonUp(CInformationBar *pBar,int x,int y) override;

	// CBasicLogger::CHandler
		void OnLog(const TSTask::LogInfo &Info) override;

		int ShowMessage(CMessageDialog::MessageType Type,
						LPCWSTR pszText,LPCWSTR pszCaption,UINT Style=CMessageDialog::STYLE_OK,
						bool *pDontAskAgain=nullptr) const;
		bool ConfirmClose(bool fEndTasks);
		bool ConfirmRecordingChannelChange(TSTask::TaskID ID);
		void GetCaptionRect(RECT *pRect) const;
		void UpdateTransparent();
		void OnCommand(int Command);
		bool ShowMainMenu(int x,int y) const;
		bool EndTask(TSTask::TaskID ID,bool fConfirm=true);
		bool UpdateTSTaskBar(TSTask::TaskID ID);
		bool UpdateTunerStatus(TSTask::TaskID ID);
		bool UpdateChannelStatus(TSTask::TaskID ID);
		bool UpdateStatistics(TSTask::TaskID ID);
		bool UpdateRecordingStatus(TSTask::TaskID ID);
		bool UpdateRecordingTime(TSTask::TaskID ID);
		bool UpdateStreamingStatus(TSTask::TaskID ID);
		bool UpdateEventStatus(TSTask::TaskID ID);
		bool UpdateClockStatus(TSTask::TaskID ID);
		int ShowTSTaskBarItemMenu(HMENU hmenu,CTSTaskBar *pTSTaskBar,int ItemID);

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		CTSTaskCentreCore &m_Core;
		ThemeInfo m_Theme;
		int m_CaptionWidth;
		bool m_fTransparentCaption;
		CTSTaskBarBase m_TSTaskBarBase;
		CInformationBar m_InformationBar;
		bool m_fShowInformationBar;
		bool m_fAdjustSize;
		bool m_fAdjustSizeOnRestore;

		CTaskTrayManager m_TaskTrayManager;
		bool m_fShowTrayIcon;
		bool m_fMinimizeToTray;
		bool m_fInTray;
		bool m_fShowInTaskbar;
		bool m_fAlwaysOnTop;
		bool m_fTranslucent;
		bool m_fFixedHeight;

		bool m_fEndAllTaskByUser;

		CTSTaskCentreSettingsDialog m_SettingsDialog;
		CAboutDialog m_AboutDialog;
	};

}


#endif
