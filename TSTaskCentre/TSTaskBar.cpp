#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskBar.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	static const CTSTaskBar::ItemInfo g_ItemList[] = {
		{L"Tuner",		L"チューナー"},
		{L"Channel",	L"チャンネル"},
		{L"Recording",	L"録画"},
		{L"Signal",		L"信号"},
		{L"Error",		L"エラー"},
		{L"Streaming",	L"ストリーミング"},
		{L"Event",		L"番組"},
		{L"Clock",		L"時計"},
	};

	static_assert(_countof(g_ItemList)==CTSTaskBar::ITEM_ID_TRAILER,
				  "項目の数が一致しません。");


	class CTSTaskBarItem : public CStatusBar::CItem
	{
	public:
		CTSTaskBarItem(CTSTaskBar *pTSTaskBar,int ID,int DefaultWidth,int MinWidth=0)
			: CItem(ID,DefaultWidth,MinWidth)
			, m_pTSTaskBar(pTSTaskBar)
		{
		}

	protected:
		virtual void OnLButtonDown(int x,int y) override;
		virtual void OnRButtonDown(int x,int y) override;

		CTSTaskBar *m_pTSTaskBar;
	};

	void CTSTaskBarItem::OnLButtonDown(int x,int y)
	{
		CTSTaskBar::CEventHandler *pEventHandler=m_pTSTaskBar->GetEventHandler();

		if (pEventHandler!=nullptr) {
			POINT pt={x,y};
			::MapWindowPoints(m_pStatusBar->GetHandle(),m_pTSTaskBar->GetHandle(),&pt,1);
			pEventHandler->OnItemLButtonDown(m_pTSTaskBar,m_ID,pt.x,pt.y);
		}
	}

	void CTSTaskBarItem::OnRButtonDown(int x,int y)
	{
		CTSTaskBar::CEventHandler *pEventHandler=m_pTSTaskBar->GetEventHandler();

		if (pEventHandler!=nullptr) {
			POINT pt={x,y};
			::MapWindowPoints(m_pStatusBar->GetHandle(),m_pTSTaskBar->GetHandle(),&pt,1);
			pEventHandler->OnItemRButtonDown(m_pTSTaskBar,m_ID,pt.x,pt.y);
		}
	}


	namespace StatusItems
	{

		class CTunerStatusItem : public CTSTaskBarItem
		{
		public:
			CTunerStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_TUNER,14*EM_UNIT) {}

			LPCTSTR GetName() const override { return TEXT("Tuner"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				if (!m_SpaceName.empty()) {
					WCHAR szText[256];

					TSTask::FormatString(szText,_countof(szText),
										 L"%s : %s",
										 m_TunerName.c_str(),m_SpaceName.c_str());
					DrawText(Info,szText,TextRect,ClipRect);
				} else if (!m_TunerName.empty()) {
					DrawText(Info,m_TunerName.c_str(),TextRect,ClipRect);
				} else {
					DrawText(Info,L"<Tuner>",TextRect,ClipRect);
				}
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"PT5 ISDB-T : 地上(UHF)",TextRect,ClipRect);
			}

			void SetTunerName(LPCWSTR pszTunerName)
			{
				if (TSTask::IsStringEmpty(pszTunerName))
					m_TunerName.clear();
				else
					m_TunerName=pszTunerName;
				Update();
			}

			void SetTuningSpaceName(LPCWSTR pszSpaceName)
			{
				if (TSTask::IsStringEmpty(pszSpaceName))
					m_SpaceName.clear();
				else
					m_SpaceName=pszSpaceName;
				Update();
			}

		private:
			TSTask::String m_TunerName;
			TSTask::String m_SpaceName;
		};

		class CChannelStatusItem : public CTSTaskBarItem
		{
		public:
			CChannelStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_CHANNEL,10*EM_UNIT) {}

			LPCTSTR GetName() const override { return TEXT("Channel"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				if (!m_ChannelName.empty()) {
					DrawText(Info,m_ChannelName.c_str(),TextRect,ClipRect);
				} else {
					DrawText(Info,L"<Channel>",TextRect,ClipRect);
				}
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"アフリカ中央テレビ",TextRect,ClipRect);
			}

			void SetChannelName(LPCWSTR pszChannelName)
			{
				if (TSTask::IsStringEmpty(pszChannelName))
					m_ChannelName.clear();
				else
					m_ChannelName=pszChannelName;
				Update();
			}

		private:
			TSTask::String m_ChannelName;
		};

		class CRecordingStatusItem : public CTSTaskBarItem
		{
		public:
			static bool GetDefaultCircleStyle(Theme::ForegroundStyle *pStyle)
			{
				if (pStyle==nullptr)
					return false;

				//pStyle->Fill.Type=Theme::FILL_SOLID;
				//pStyle->Fill.Solid.Color.Set(223,63,0);
				pStyle->Fill.Type=Theme::FILL_GRADIENT;
				pStyle->Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
				pStyle->Fill.Gradient.Color1.Set(255,72,0);
				pStyle->Fill.Gradient.Color2.Set(223,63,0);
				pStyle->Glow.Type=Theme::GLOW_NONE;

				return true;
			}

			CRecordingStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_RECORDING,7*EM_UNIT)
			{
				GetDefaultCircleStyle(&m_CircleStyle);

				m_RecordingInfo.State=TSTask::RECORDING_STATE_NOT_RECORDING;
			}

			LPCTSTR GetName() const override { return TEXT("Recording"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				if (m_RecordingInfo.State!=TSTask::RECORDING_STATE_NOT_RECORDING) {
					Info.pStylePainter->DrawText(m_CircleStyle,L"●",TextRect,ClipRect,
												 Info.pFont,
												 Graphics::TEXT_FORMAT_LEFT |
												 Graphics::TEXT_FORMAT_VERTICAL_CENTER);

					WCHAR szText[64];
					DWORD Time=(DWORD)(((m_CurTickCount-m_RecordingInfo.StartTickCount)+500ULL)/1000ULL);
					TSTask::FormatString(szText,_countof(szText),
										 L"%u:%02u:%02u",
										 Time/(60*60),(Time/60)%60,Time%60);
					RECT rc=TextRect;
					rc.left+=m_pStatusBar->GetFontHeight()+4;
					DrawText(Info,szText,rc,ClipRect);
				} else {
					DrawText(Info,L"■ <Rec>",TextRect,ClipRect);
				}
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				Info.pStylePainter->DrawText(m_CircleStyle,L"●",TextRect,ClipRect,Info.pFont,
											 Graphics::TEXT_FORMAT_LEFT |
											 Graphics::TEXT_FORMAT_VERTICAL_CENTER);
				RECT rc=TextRect;
				rc.left+=m_pStatusBar->GetFontHeight()+4;
				DrawText(Info,L"0:32:17",rc,ClipRect);
			}

			void SetRecordingInfo(const TSTask::RecordingInfo &Info)
			{
				m_RecordingInfo=Info;
				m_CurTickCount=::GetTickCount64();

				Update();
			}

			void UpdateTime()
			{
				if (m_RecordingInfo.State!=TSTask::RECORDING_STATE_NOT_RECORDING) {
					m_CurTickCount=::GetTickCount64();

					Update();
				}
			}

			void SetCircleStyle(const Theme::ForegroundStyle &Style)
			{
				m_CircleStyle=Style;
			}

		private:
			Theme::ForegroundStyle m_CircleStyle;
			TSTask::RecordingInfo m_RecordingInfo;
			ULONGLONG m_CurTickCount;
		};

		class CSignalStatusItem : public CTSTaskBarItem
		{
		public:
			CSignalStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_SIGNAL,12*EM_UNIT)
				, m_SignalLevel(0.0f)
				, m_BitRate(0)
			{
			}

			LPCTSTR GetName() const override { return TEXT("Signal"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				int SignalLevel=(int)(m_SignalLevel*100.0f);
				unsigned int BitRate=(unsigned int)::MulDiv(m_BitRate,100,1000*1000);
				WCHAR szText[64];

				TSTask::FormatString(szText,_countof(szText),
									 L"%d.%02d dB / %u.%02u Mbps",
									 SignalLevel/100,std::abs(SignalLevel)%100,
									 BitRate/100,BitRate%100);
				DrawText(Info,szText,TextRect,ClipRect);
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"24.37 dB / 16.25 Mbps",TextRect,ClipRect);
			}

			void SetSignalStatus(float SignalLevel,DWORD BitRate)
			{
				if (m_SignalLevel!=SignalLevel || m_BitRate!=BitRate) {
					m_SignalLevel=SignalLevel;
					m_BitRate=BitRate;

					Update();
				}
			}

		private:
			float m_SignalLevel;
			DWORD m_BitRate;
		};

		class CErrorStatusItem : public CTSTaskBarItem
		{
		public:
			CErrorStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_ERROR,14*EM_UNIT)
				, m_ErrorCount(0)
				, m_DiscontinuityCount(0)
				, m_ScrambleCount(0)
			{
			}

			LPCTSTR GetName() const override { return TEXT("Error"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				WCHAR szText[256];

				TSTask::FormatString(szText,_countof(szText),
									 L"D %llu / E %llu / S %llu",
									 m_DiscontinuityCount,m_ErrorCount,m_ScrambleCount);
				DrawText(Info,szText,TextRect,ClipRect);
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"D 0 / E 0 / S 102",TextRect,ClipRect);
			}

			void SetErrorStatistics(ULONGLONG ErrorCount,ULONGLONG DiscontinuityCount,ULONGLONG ScrambleCount)
			{
				if (m_ErrorCount!=ErrorCount
						|| m_DiscontinuityCount!=DiscontinuityCount
						|| m_ScrambleCount!=ScrambleCount) {
					m_ErrorCount=ErrorCount;
					m_DiscontinuityCount=DiscontinuityCount;
					m_ScrambleCount=ScrambleCount;

					Update();
				}
			}

		private:
			ULONGLONG m_ErrorCount;
			ULONGLONG m_DiscontinuityCount;
			ULONGLONG m_ScrambleCount;
		};

		class CStreamingStatusItem : public CTSTaskBarItem
		{
		public:
			CStreamingStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_STREAMING,7*EM_UNIT)
				, m_fStreaming(false)
			{
			}

			LPCTSTR GetName() const override { return TEXT("Streaming"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				if (m_fStreaming) {
					WCHAR szText[64];
					TSTask::FormatString(szText,_countof(szText),
										 L"%s %d",
										 TSTask::Streaming::GetProtocolText(m_StreamingInfo.Address.Protocol),
										 m_StreamingInfo.Address.Port);
					DrawText(Info,szText,TextRect,ClipRect);
				} else {
					DrawText(Info,L"<Network>",TextRect,ClipRect);
				}
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"UDP 1234",TextRect,ClipRect);
			}

			void SetStreamingInfo(const TSTask::StreamingInfo &Info)
			{
				m_StreamingInfo=Info;
				m_fStreaming=true;

				Update();
			}

			void ClearStreamingInfo()
			{
				m_fStreaming=false;

				Update();
			}

		private:
			bool m_fStreaming;
			TSTask::StreamingInfo m_StreamingInfo;
		};

		class CEventStatusItem : public CTSTaskBarItem
		{
		public:
			CEventStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_EVENT,20*EM_UNIT)
				, m_fStartTimeValid(false)
				, m_Duration(0)
			{
			}

			LPCTSTR GetName() const override { return TEXT("Event"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				WCHAR szText[256];
				int Length=0;

				if (m_fStartTimeValid) {
					Length+=TSTask::FormatString(&szText[Length],_countof(szText),
												 L"%d:%02d～",
												 m_StartTime.wHour,m_StartTime.wMinute);
					if (m_Duration>0) {
						SYSTEMTIME EndTime=m_StartTime;

						TSTask::OffsetSystemTime(&EndTime,m_Duration*1000);
						Length+=TSTask::FormatString(&szText[Length],_countof(szText)-Length,
													 L"%d:%02d",
													 EndTime.wHour,EndTime.wMinute);
					}
					szText[Length++]=L' ';
				}
				::lstrcpynW(&szText[Length],m_EventName.c_str(),_countof(szText)-Length);
				DrawText(Info,szText,TextRect,ClipRect);
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"1:30～2:00 今日のニュース",TextRect,ClipRect);
			}

			void SetEventInfo(LPCWSTR pszEventName,const SYSTEMTIME *pStartTime,DWORD Duration)
			{
				if (TSTask::IsStringEmpty(pszEventName))
					m_EventName.clear();
				else
					m_EventName=pszEventName;

				if (pStartTime!=nullptr && pStartTime->wYear!=0) {
					m_fStartTimeValid=true;
					m_StartTime=*pStartTime;
					m_Duration=Duration;
				} else {
					m_fStartTimeValid=false;
					m_Duration=0;
				}

				Update();
			}

			void ClearEventInfo()
			{
				m_EventName.clear();
				m_fStartTimeValid=false;
				m_Duration=0;
				Update();
			}

		private:
			TSTask::String m_EventName;
			bool m_fStartTimeValid;
			SYSTEMTIME m_StartTime;
			DWORD m_Duration;
		};

		class CClockStatusItem : public CTSTaskBarItem
		{
		public:
			CClockStatusItem(CTSTaskBar *pTSTaskBar)
				: CTSTaskBarItem(pTSTaskBar,CTSTaskBar::ITEM_ID_CLOCK,8*EM_UNIT)
				, m_Type(CTSTaskBar::CLOCK_INVALID) {}

			LPCTSTR GetName() const override { return TEXT("Clock"); }

			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				if (m_Type!=CTSTaskBar::CLOCK_INVALID) {
					WCHAR szText[64];

					TSTask::FormatString(szText,_countof(szText),
										 L"%s%d:%02d:%02d",
										 m_Type==CTSTaskBar::CLOCK_TOT?L"TOT ":L"",
										 m_Time.wHour,m_Time.wMinute,m_Time.wSecond);
					DrawText(Info,szText,TextRect,ClipRect);
				}
			}

			void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const override
			{
				DrawText(Info,L"TOT 12:45:23",TextRect,ClipRect);
			}

			void SetTime(CTSTaskBar::ClockType Type,const SYSTEMTIME &Time)
			{
				m_Type=Type;
				m_Time=Time;
				Update();
			}

		private:
			CTSTaskBar::ClockType m_Type;
			SYSTEMTIME m_Time;
		};

	}


	const LPCTSTR CTSTaskBar::m_pszWindowClass=APP_NAME_W TEXT("_TSTaskBar");
	HINSTANCE CTSTaskBar::m_hinst=nullptr;

	bool CTSTaskBar::Initialize(HINSTANCE hinst)
	{
		if (m_hinst==nullptr) {
			WNDCLASS wc;

			wc.style=0;
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

		if (!CStatusBar::Initialize(hinst))
			return false;

		return true;
	}

	bool CTSTaskBar::GetItemInfo(int ID,ItemInfo *pInfo)
	{
		if (ID<0 || ID>=ITEM_ID_TRAILER || pInfo==nullptr)
			return false;

		*pInfo=g_ItemList[ID];

		return true;
	}

	bool CTSTaskBar::GetDefaultTheme(ThemeInfo *pTheme)
	{
		if (pTheme==nullptr)
			return false;

		CStatusBar::GetDefaultTheme(&pTheme->Status);
		GetDefaultCaptionTheme(&pTheme->CaptionStyle);
		StatusItems::CRecordingStatusItem::GetDefaultCircleStyle(&pTheme->Recording.Circle);

		return true;
	}

	bool CTSTaskBar::GetDefaultCaptionTheme(Theme::ItemStyle *pStyle)
	{
		if (pStyle==nullptr)
			return false;

		pStyle->Background.Fill.Type=Theme::FILL_GRADIENT;
		pStyle->Background.Fill.Gradient.Direction=Graphics::DIRECTION_HORIZONTAL;
		pStyle->Background.Fill.Gradient.Color1.Set(128,152,204);
		pStyle->Background.Fill.Gradient.Color2.Set(96,128,192);
		pStyle->Background.Border.Type=Theme::BORDER_RAISED;
		pStyle->Background.Border.Color.Set(112,140,198);
		pStyle->Foreground.Fill.Type=Theme::FILL_SOLID;
		pStyle->Foreground.Fill.Solid.Color.Set(255,255,255);
		pStyle->Foreground.Glow.Type=Theme::GLOW_FADEOUT;
		pStyle->Foreground.Glow.Color.Set(0,0,0,96);
		pStyle->Foreground.Glow.Radius=3;
		pStyle->Foreground.AntiAliasing=Theme::ANTIALIASING_ANTIALIAS;

		return true;
	}

	CTSTaskBar::CTSTaskBar(Graphics::CSystem &GraphicSystem)
		: CLayeredWidget(GraphicSystem)
		, m_StatusBar(GraphicSystem)
		, m_CaptionWidth(0)
		, m_pEventHandler(nullptr)
	{
		GetDefaultCaptionTheme(&m_CaptionStyle);

		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_CAPTION,&m_CaptionFont);
		m_CaptionFont.lfWeight=FW_BOLD;

		m_StatusBar.SetMaxRows(1);

		m_StatusBar.AddItem(new StatusItems::CTunerStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CChannelStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CSignalStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CErrorStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CRecordingStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CStreamingStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CEventStatusItem(this));
		m_StatusBar.AddItem(new StatusItems::CClockStatusItem(this));
	}

	CTSTaskBar::~CTSTaskBar()
	{
		Destroy();
	}

	bool CTSTaskBar::Create(HWND hwndParent,int ID)
	{
		if (m_hwnd!=nullptr)
			return false;

		if (!CreateBasicWindow(hwndParent,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,0,
							   ID,m_pszWindowClass,nullptr,m_hinst))
			return false;

		return true;
	}

	int CTSTaskBar::CalcHeight(int Width) const
	{
		return m_StatusBar.CalcHeight(Width-m_CaptionWidth);
	}

	void CTSTaskBar::SetCaptionText(LPCWSTR pszText)
	{
		if (pszText!=nullptr)
			m_CaptionText=pszText;
		else
			m_CaptionText.clear();

		if (m_hwnd!=nullptr) {
			RedrawCaption();
		}
	}

	bool CTSTaskBar::SetCaptionWidth(int Width)
	{
		if (Width<0)
			return false;

		if (m_CaptionWidth!=Width) {
			m_CaptionWidth=Width;

			if (m_hwnd!=nullptr) {
				Layout();
				if (m_CaptionWidth>0)
					RedrawCaption();
			}
		}

		return true;
	}

	bool CTSTaskBar::SetCaptionFont(const LOGFONT &Font)
	{
		m_CaptionFont=Font;

		if (m_hwnd!=nullptr && !m_CaptionText.empty()) {
			RedrawCaption();
		}

		return true;
	}

	bool CTSTaskBar::SetFont(const LOGFONT &Font)
	{
		return m_StatusBar.SetFont(Font);
	}

	bool CTSTaskBar::SetTheme(const ThemeInfo &Theme)
	{
		m_StatusBar.SetTheme(Theme.Status);

		m_CaptionStyle=Theme.CaptionStyle;
		if (m_hwnd!=nullptr)
			RedrawCaption();

		StatusItems::CRecordingStatusItem *pRecordingItem=
			dynamic_cast<StatusItems::CRecordingStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_RECORDING));
		if (pRecordingItem!=nullptr)
			pRecordingItem->SetCircleStyle(Theme.Recording.Circle);

		return true;
	}

	void CTSTaskBar::SetEventHandler(CEventHandler *pEventHandler)
	{
		m_pEventHandler=pEventHandler;
	}

	bool CTSTaskBar::GetItemRect(int ID,RECT *pRect) const
	{
		if (!m_StatusBar.GetItemRectByID(ID,pRect))
			return false;

		::MapWindowPoints(m_StatusBar.GetHandle(),m_hwnd,reinterpret_cast<POINT*>(pRect),2);

		return true;
	}

	int CTSTaskBar::GetItemWidth(int ID) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return 0;

		return pItem->GetWidth();
	}

	bool CTSTaskBar::SetItemWidth(int ID,int Width)
	{
		CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return false;

		return pItem->SetWidth(Width);
	}

	int CTSTaskBar::GetItemDefaultWidth(int ID) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return 0;

		return pItem->GetDefaultWidth();
	}

	int CTSTaskBar::GetItemDefaultWidthPixels(int ID) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return 0;

		return pItem->GetDefaultWidthPixels();
	}

	int CTSTaskBar::GetItemMinWidth(int ID) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return 0;

		return pItem->GetMinWidth();
	}

	int CTSTaskBar::GetItemHeight() const
	{
		return m_StatusBar.GetItemHeight();
	}

	bool CTSTaskBar::SetItemMargin(const RECT &Margin)
	{
		return m_StatusBar.SetItemMargin(Margin);
	}

	RECT CTSTaskBar::GetItemMargin() const
	{
		return m_StatusBar.GetItemMargin();
	}

	RECT CTSTaskBar::GetDefaultItemMargin() const
	{
		return m_StatusBar.GetDefaultItemMargin();
	}

	bool CTSTaskBar::GetItemVisible(int ID) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return false;

		return pItem->GetVisible();
	}

	bool CTSTaskBar::SetItemVisible(int ID,bool fVisible)
	{
		CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return false;

		pItem->SetVisible(fVisible);

		return true;
	}

	bool CTSTaskBar::SetItemOrder(const int *pOrderList)
	{
		return m_StatusBar.SetItemOrder(pOrderList);
	}

	bool CTSTaskBar::SetMaxRows(int MaxRows)
	{
		if (MaxRows<1)
			return false;

		return m_StatusBar.SetMultiRow(MaxRows>1)
			&& m_StatusBar.SetMaxRows(MaxRows);
	}

	int CTSTaskBar::GetMaxRows() const
	{
		return m_StatusBar.GetMaxRows();
	}

	bool CTSTaskBar::DrawItemPreview(int ID,HDC hdc,const RECT &Rect,
									 bool fHighlight,const LOGFONT *pFont) const
	{
		const CStatusBar::CItem *pItem=m_StatusBar.GetItemByID(ID);
		if (pItem==nullptr)
			return 0;

		return m_StatusBar.DrawItemPreview(pItem,hdc,Rect,fHighlight,pFont);
	}

	void CTSTaskBar::SetTunerName(LPCWSTR pszTunerName)
	{
		StatusItems::CTunerStatusItem *pItem=
			dynamic_cast<StatusItems::CTunerStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_TUNER));
		if (pItem!=nullptr)
			pItem->SetTunerName(pszTunerName);
	}

	void CTSTaskBar::SetTuningSpaceName(LPCWSTR pszTuningSpaceName)
	{
		StatusItems::CTunerStatusItem *pItem=
			dynamic_cast<StatusItems::CTunerStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_TUNER));
		if (pItem!=nullptr)
			pItem->SetTuningSpaceName(pszTuningSpaceName);
	}

	void CTSTaskBar::SetChannelName(LPCWSTR pszChannelName)
	{
		StatusItems::CChannelStatusItem *pItem=
			dynamic_cast<StatusItems::CChannelStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_CHANNEL));
		if (pItem!=nullptr)
			pItem->SetChannelName(pszChannelName);
	}

	void CTSTaskBar::SetSignalStatus(float SignalLevel,DWORD BitRate)
	{
		StatusItems::CSignalStatusItem *pItem=
			dynamic_cast<StatusItems::CSignalStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_SIGNAL));
		if (pItem!=nullptr)
			pItem->SetSignalStatus(SignalLevel,BitRate);
	}

	void CTSTaskBar::SetErrorStatistics(ULONGLONG ErrorCount,ULONGLONG DiscontinuityCount,ULONGLONG ScrambleCount)
	{
		StatusItems::CErrorStatusItem *pItem=
			dynamic_cast<StatusItems::CErrorStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_ERROR));
		if (pItem!=nullptr)
			pItem->SetErrorStatistics(ErrorCount,DiscontinuityCount,ScrambleCount);
	}

	void CTSTaskBar::SetRecordingInfo(const TSTask::RecordingInfo &Info)
	{
		StatusItems::CRecordingStatusItem *pItem=
			dynamic_cast<StatusItems::CRecordingStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_RECORDING));
		if (pItem!=nullptr)
			pItem->SetRecordingInfo(Info);
	}

	void CTSTaskBar::UpdateRecordingTime()
	{
		StatusItems::CRecordingStatusItem *pItem=
			dynamic_cast<StatusItems::CRecordingStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_RECORDING));
		if (pItem!=nullptr)
			pItem->UpdateTime();
	}

	void CTSTaskBar::SetStreamingInfo(const TSTask::StreamingInfo *pInfo)
	{
		StatusItems::CStreamingStatusItem *pItem=
			dynamic_cast<StatusItems::CStreamingStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_STREAMING));
		if (pItem!=nullptr) {
			if (pInfo!=nullptr)
				pItem->SetStreamingInfo(*pInfo);
			else
				pItem->ClearStreamingInfo();
		}
	}

	void CTSTaskBar::SetEventInfo(LPCWSTR pszEventName,const SYSTEMTIME *pStartTime,DWORD Duration)
	{
		StatusItems::CEventStatusItem *pItem=
			dynamic_cast<StatusItems::CEventStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_EVENT));
		if (pItem!=nullptr)
			pItem->SetEventInfo(pszEventName,pStartTime,Duration);
	}

	void CTSTaskBar::SetClockTime(ClockType Type,const SYSTEMTIME &Time)
	{
		StatusItems::CClockStatusItem *pItem=
			dynamic_cast<StatusItems::CClockStatusItem*>(m_StatusBar.GetItemByID(ITEM_ID_CLOCK));
		if (pItem!=nullptr)
			pItem->SetTime(Type,Time);
	}

	LRESULT CTSTaskBar::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CREATE:
			{
				m_StatusBar.Create(hwnd);
				m_StatusBar.SetVisible(true);
			}
			return 0;

		case WM_SIZE:
			Layout(LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			if (m_pEventHandler!=nullptr) {
				POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
				RECT rc;

				GetCaptionRect(&rc);
				if (::PtInRect(&rc,pt)) {
					switch (uMsg) {
					case WM_LBUTTONDOWN:
						m_pEventHandler->OnCaptionLButtonDown(this,pt.x,pt.y);
						break;

					case WM_LBUTTONUP:
						m_pEventHandler->OnCaptionLButtonUp(this,pt.x,pt.y);
						break;

					case WM_RBUTTONDOWN:
						m_pEventHandler->OnCaptionRButtonDown(this,pt.x,pt.y);
						break;

					case WM_RBUTTONUP:
						m_pEventHandler->OnCaptionRButtonUp(this,pt.x,pt.y);
						break;
					}
				}
			}
			return 0;
		}

		return CLayeredWidget::OnMessage(hwnd,uMsg,wParam,lParam);
	}

	void CTSTaskBar::Layout(int Width,int Height)
	{
		m_StatusBar.SetPosition(m_CaptionWidth,0,max(Width-m_CaptionWidth,0),Height);
	}

	void CTSTaskBar::Layout()
	{
		SIZE sz;

		GetClientSize(&sz);
		Layout(sz.cx,sz.cy);
	}

	void CTSTaskBar::Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect)
	{
		RECT rc;
		GetCaptionRect(&rc);

		if (PaintRect.left<rc.right) {
			Theme::CStylePainter StylePainter(m_GraphicSystem,pCanvas);

			StylePainter.DrawBackground(m_CaptionStyle.Background,rc);

			if (!m_CaptionText.empty()) {
				Graphics::CFont *pFont=m_GraphicSystem.CreateFont(m_CaptionFont);

				m_CaptionStyle.Background.CalcContentRect(&rc);
				StylePainter.DrawText(m_CaptionStyle.Foreground,m_CaptionText.c_str(),
									  rc,rc,pFont,
									  Graphics::TEXT_FORMAT_HORIZONTAL_CENTER |
									  Graphics::TEXT_FORMAT_VERTICAL_CENTER |
									  Graphics::TEXT_FORMAT_NO_WRAP);
				delete pFont;
			}
		}
	}

	bool CTSTaskBar::IsTransparentBackground() const
	{
		//return m_CaptionStyle.Background.IsTransparent();
		return true;
	}

	void CTSTaskBar::GetCaptionRect(RECT *pRect) const
	{
		GetClientRect(pRect);
		pRect->right=m_CaptionWidth;
	}

	void CTSTaskBar::RedrawCaption() const
	{
		RECT rc;

		GetCaptionRect(&rc);
		Invalidate(rc);
	}

}
