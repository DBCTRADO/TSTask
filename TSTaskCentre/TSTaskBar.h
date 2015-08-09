#ifndef TSTASKCENTRE_TS_TASK_BAR_H
#define TSTASKCENTRE_TS_TASK_BAR_H


#include "StatusBar.h"


namespace TSTaskCentre
{

	class CTSTaskBar : public CLayeredWidget
	{
	public:
		enum {
			ITEM_ID_TUNER,
			ITEM_ID_CHANNEL,
			ITEM_ID_RECORDING,
			ITEM_ID_SIGNAL,
			ITEM_ID_ERROR,
			ITEM_ID_STREAMING,
			ITEM_ID_EVENT,
			ITEM_ID_CLOCK,
			ITEM_ID_TRAILER
		};

		struct ItemInfo
		{
			LPCWSTR pszName;
			LPCWSTR pszText;
		};

		class TSTASK_ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() {}
			virtual void OnCaptionLButtonDown(CTSTaskBar *pTSTaskBar,int x,int y) {}
			virtual void OnCaptionLButtonUp(CTSTaskBar *pTSTaskBar,int x,int y) {}
			virtual void OnCaptionRButtonDown(CTSTaskBar *pTSTaskBar,int x,int y) {}
			virtual void OnCaptionRButtonUp(CTSTaskBar *pTSTaskBar,int x,int y) {}
			virtual void OnItemLButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y) {}
			virtual void OnItemRButtonDown(CTSTaskBar *pTSTaskBar,int ID,int x,int y) {}
		};

		struct ThemeInfo
		{
			CStatusBar::ThemeInfo Status;
			Theme::ItemStyle CaptionStyle;
			struct
			{
				Theme::ForegroundStyle Circle;
			} Recording;
		};

		enum ClockType
		{
			CLOCK_INVALID=-1,
			CLOCK_SYSTEM,
			CLOCK_TOT,
			CLOCK_TRAILER
		};

		static bool Initialize(HINSTANCE hinst);
		static bool GetItemInfo(int ID,ItemInfo *pInfo);
		static bool GetDefaultTheme(ThemeInfo *pTheme);
		static bool GetDefaultCaptionTheme(Theme::ItemStyle *pStyle);

		CTSTaskBar(Graphics::CSystem &GraphicSystem);
		~CTSTaskBar();
		bool Create(HWND hwndParent,int ID=0) override;
		int CalcHeight(int Width) const;
		void SetCaptionText(LPCWSTR pszText);
		bool SetCaptionWidth(int Width);
		bool SetCaptionFont(const LOGFONT &Font);
		bool SetFont(const LOGFONT &Font);
		bool SetTheme(const ThemeInfo &Theme);
		void SetEventHandler(CEventHandler *pEventHandler);
		CEventHandler *GetEventHandler() const { return m_pEventHandler; }
		bool GetItemRect(int ID,RECT *pRect) const;
		int GetItemWidth(int ID) const;
		bool SetItemWidth(int ID,int Width);
		int GetItemDefaultWidth(int ID) const;
		int GetItemDefaultWidthPixels(int ID) const;
		int GetItemMinWidth(int ID) const;
		int GetItemHeight() const;
		bool SetItemMargin(const RECT &Margin);
		RECT GetItemMargin() const;
		RECT GetDefaultItemMargin() const;
		bool GetItemVisible(int ID) const;
		bool SetItemVisible(int ID,bool fVisible);
		bool SetItemOrder(const int *pOrderList);
		bool SetMaxRows(int MaxRows);
		int GetMaxRows() const;
		bool DrawItemPreview(int ID,HDC hdc,const RECT &Rect,
							 bool fHighlight=false,const LOGFONT *pFont=nullptr) const;

		void SetTunerName(LPCWSTR pszTunerName);
		void SetTuningSpaceName(LPCWSTR pszTuningSpaceName);
		void SetChannelName(LPCWSTR pszChannelName);
		void SetSignalStatus(float SignalLevel,DWORD BitRate);
		void SetErrorStatistics(ULONGLONG ErrorCount,ULONGLONG DiscontinuityCount,ULONGLONG ScrambleCount);
		void SetRecordingInfo(const TSTask::RecordingInfo &Info);
		void UpdateRecordingTime();
		void SetStreamingInfo(const TSTask::StreamingInfo *pInfo);
		void SetEventInfo(LPCWSTR pszEventName,const SYSTEMTIME *pStartTime,DWORD Duration);
		void SetClockTime(ClockType Type,const SYSTEMTIME &Time);

	private:
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) override;
		bool IsTransparentBackground() const override;

		void Layout(int Width,int Height);
		void Layout();
		void GetCaptionRect(RECT *pRect) const;
		void RedrawCaption() const;

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		CStatusBar m_StatusBar;
		int m_CaptionWidth;
		Theme::ItemStyle m_CaptionStyle;
		LOGFONT m_CaptionFont;
		TSTask::String m_CaptionText;
		CEventHandler *m_pEventHandler;
	};

}


#endif
