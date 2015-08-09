// DtvEngine.h: CDtvEngine クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "../BonTsEngine/BonTsEngineConfig.h"
#include "../BonTsEngine/BonSrcDecoder.h"
#include "../BonTsEngine/TsPacketParser.h"
#include "../BonTsEngine/TsAnalyzer.h"
#include "../BonTsEngine/TsRecorder.h"
#include "../BonTsEngine/TsGrabber.h"
#include "../BonTsEngine/TsSelector.h"
#include "../BonTsEngine/TsPacketCounter.h"
#ifdef DTVENGINE_NETWORK_SUPPORT
#include "../BonTsEngine/MediaTee.h"
#include "../BonTsEngine/TsNetworkSender.h"
#endif


//////////////////////////////////////////////////////////////////////
// デジタルTVエンジンクラス
//////////////////////////////////////////////////////////////////////

class CDtvEngine : protected CMediaDecoder::IEventHandler, public CBonBaseClass
{
public:
	enum {
		TSID_INVALID	= 0x0000,
		SID_INVALID		= 0xFFFF,
#ifdef BONTSENGINE_1SEG_SUPPORT
		SID_1SEG		= 0xFFFE,
#endif
		SERVICE_INVALID	= 0xFFFF
	};

	class ABSTRACT_CLASS_DECL CEventHandler {
		friend CDtvEngine;
	public:
		virtual ~CEventHandler() = 0 {}
	protected:
		CDtvEngine *m_pDtvEngine;
		virtual void OnServiceChanged(WORD ServiceID) {}
		virtual void OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer, bool bStreamChanged) {}
		virtual void OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnEventUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnTotUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnFileWriteError(CTsRecorder *pTsRecoder, DWORD ErrorCode) {}
	};

	CDtvEngine(void);
	~CDtvEngine(void);

	bool BuildEngine(CEventHandler *pEventHandler);
	bool IsEngineBuild() const { return m_bBuiled; };
	bool CloseEngine(void);
	bool ResetEngine(void);

	bool LoadBonDriver(LPCTSTR pszFileName);
	bool UnloadBonDriver();
	bool OpenTuner();
	bool CloseTuner();
	bool ReleaseSrcFilter();
	bool IsSrcFilterOpen() const;
	void SetStartStreamingOnDriverOpen(bool bStart);

	bool SetChannel(const DWORD TuningSpace, const DWORD Channel, const WORD ServiceID = SID_INVALID);
	bool SetService(const WORD wService);
	bool GetServiceID(WORD *pServiceID);
	bool SetServiceByID(const WORD ServiceID, const bool bReserve = true);

	bool SetWriteStream(WORD ServiceID, DWORD Stream = CTsSelector::STREAM_ALL);
	bool GetWriteStream(WORD *pServiceID, DWORD *pStream = NULL);
	bool SetWriteCurServiceOnly(bool bOnly, DWORD Stream = CTsSelector::STREAM_ALL);
	bool GetWriteCurServiceOnly() const { return m_bWriteCurServiceOnly; }

#ifdef DTVENGINE_NETWORK_SUPPORT
	bool SetSendStream(WORD ServiceID, DWORD Stream = CTsSelector::STREAM_ALL);
	bool SetSendCurServiceOnly(bool bOnly, DWORD Stream = CTsSelector::STREAM_ALL);
#endif

	WORD GetEventID(bool bNext = false);
	int GetEventName(LPTSTR pszName, int MaxLength, bool bNext = false);
	int GetEventText(LPTSTR pszText, int MaxLength, bool bNext = false);
	int GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext = false);
	bool GetEventTime(SYSTEMTIME *pStartTime, SYSTEMTIME *pEndTime, bool bNext = false);
	DWORD GetEventDuration(bool bNext = false);
	bool GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext = false);
	bool GetEventInfo(CEventInfo *pInfo, bool bNext = false);
	bool GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex, bool bNext = false);

	unsigned __int64 GetPcrTimeStamp();

//protected:
	// CMediaDecoder から派生したメディアデコーダクラス
	CBonSrcDecoder m_BonSrcDecoder;
	CTsPacketParser m_TsPacketParser;
	CTsAnalyzer m_TsAnalyzer;
	CTsPacketCounter m_TsPacketCounter;
	CTsGrabber m_TsGrabber;
	CTsRecorder m_TsRecorder;
	CTsSelector m_FileTsSelector;
#ifdef DTVENGINE_NETWORK_SUPPORT
	CMediaTee m_MediaTee;
	CTsSelector m_NetworkTsSelector;
	CTsNetworkSender m_TsNetworkSender;
#endif

protected:
// CMediaDecoder::IEventHandler
	const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam) override;

	void ResetStatus();

#ifdef BONTSENGINE_1SEG_SUPPORT
	int Find1SegService(WORD *pServiceID = NULL);
#endif

	CCriticalLock m_EngineLock;
	CEventHandler *m_pEventHandler;

	WORD m_CurTransportStreamID;
	WORD m_CurServiceIndex;
	WORD m_CurServiceID;
	WORD m_SpecServiceID;
	WORD m_SetChannelServiceID;

	bool m_bBuiled;
	bool m_bStartStreamingOnDriverOpen;

	bool m_bWriteCurServiceOnly;
	DWORD m_WriteStream;

#ifdef DTVENGINE_NETWORK_SUPPORT
	bool m_bSendCurServiceOnly;
#ifdef BONTSENGINE_1SEG_SUPPORT
	bool m_bSend1Seg;
#endif
	DWORD m_SendStream;
#endif
};
