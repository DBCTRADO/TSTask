// DtvEngine.cpp: CDtvEngine クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DtvEngine.h"
#include "../BonTsEngine/StdUtil.h"
#include "../Common/Debug.h"
#include "../Common/DebugDef.h"

#pragma comment(lib, "BonTsEngine.lib")


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)

	, m_CurTransportStreamID(TSID_INVALID)
	, m_CurServiceIndex(SERVICE_INVALID)
	, m_CurServiceID(SID_INVALID)
	, m_SpecServiceID(SID_INVALID)
	, m_SetChannelServiceID(SID_INVALID)

	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_TsPacketCounter(this)
	, m_TsRecorder(this)
	, m_TsGrabber(this)
	, m_FileTsSelector(this)
#ifdef DTVENGINE_NETWORK_SUPPORT
	, m_MediaTee(this)
	, m_NetworkTsSelector(this)
	, m_TsNetworkSender(this)
#endif

	, m_bBuiled(false)
	, m_bStartStreamingOnDriverOpen(false)

	, m_bWriteCurServiceOnly(false)
	, m_WriteStream(CTsSelector::STREAM_ALL)

#ifdef DTVENGINE_NETWORK_SUPPORT
	, m_bSendCurServiceOnly(false)
#ifdef BONTSENGINE_1SEG_SUPPORT
	, m_bSend1Seg(false)
#endif
	, m_SendStream(CTsSelector::STREAM_ALL)
#endif
{
#ifdef BONTSENGINE_1SEG_SUPPORT
	m_TsPacketParser.EnablePATGeneration(false);
#endif
}


CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}


bool CDtvEngine::BuildEngine(CEventHandler *pEventHandler)
{
	if (m_bBuiled)
		return true;

	/*
	グラフ構成図

	m_BonSrcDecoder
	    ↓
	m_TsPacketParser
	    ↓
	m_TsAnalyzer
	    ↓
	m_TsPacketCounter
	    ↓
	m_TsGrabber
	    ↓
	m_MediaTee──────┐
	    ↓                ↓
	m_FileTsSelector  m_NetworkTsSelector
	    ↓                ↓
	m_TsRecorder      m_TsNetworkSender
	*/

	Trace(CTracer::TYPE_INFORMATION, TEXT("デコーダグラフを構築しています..."));

	// デコーダグラフ構築
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	m_TsPacketParser.SetOutputDecoder(&m_TsAnalyzer);
	m_TsAnalyzer.SetOutputDecoder(&m_TsPacketCounter);
	m_TsPacketCounter.SetOutputDecoder(&m_TsGrabber);
#ifdef DTVENGINE_NETWORK_SUPPORT
	m_TsGrabber.SetOutputDecoder(&m_MediaTee);
	m_MediaTee.SetOutputDecoder(&m_FileTsSelector, 0);
	m_MediaTee.SetOutputDecoder(&m_NetworkTsSelector, 1);
	m_FileTsSelector.SetOutputDecoder(&m_TsRecorder);
	m_NetworkTsSelector.SetOutputDecoder(&m_TsNetworkSender);
#else
	m_TsGrabber.SetOutputDecoder(&m_FileTsSelector);
	m_FileTsSelector.SetOutputDecoder(&m_TsRecorder);
#endif

	// イベントハンドラ設定
	m_pEventHandler = pEventHandler;
	m_pEventHandler->m_pDtvEngine = this;

	m_bBuiled = true;

	return true;
}


bool CDtvEngine::CloseEngine(void)
{
	//if (!m_bBuiled)
	//	return true;

	Trace(CTracer::TYPE_INFORMATION, TEXT("DtvEngine を閉じています..."));

	ReleaseSrcFilter();

	// イベントハンドラ解除
	m_pEventHandler = NULL;

	m_bBuiled = false;

	Trace(CTracer::TYPE_INFORMATION, TEXT("DtvEngine を閉じました。"));

	return true;
}


bool CDtvEngine::ResetEngine(void)
{
	if (!m_bBuiled)
		return false;

	// デコーダグラフリセット
	m_BonSrcDecoder.ResetGraph();

	return true;
}


bool CDtvEngine::LoadBonDriver(LPCTSTR pszFileName)
{
	ReleaseSrcFilter();

	Trace(CTracer::TYPE_INFORMATION, TEXT("BonDriver を読み込んでいます..."));
	if (!m_BonSrcDecoder.LoadBonDriver(pszFileName)) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	return true;
}


bool CDtvEngine::UnloadBonDriver()
{
	if (m_BonSrcDecoder.IsBonDriverLoaded()) {
		m_BonSrcDecoder.UnloadBonDriver();
	}

	return true;
}


bool CDtvEngine::OpenTuner()
{
	if (!m_BonSrcDecoder.OpenTuner()) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	if (m_bStartStreamingOnDriverOpen) {
		Trace(CTracer::TYPE_INFORMATION, TEXT("ストリームの再生を開始しています..."));
		if (!m_BonSrcDecoder.Play()) {
			SetError(m_BonSrcDecoder.GetLastErrorException());
			return false;
		}
	}
	//ResetEngine();
	ResetStatus();

	return true;
}


bool CDtvEngine::CloseTuner()
{
	if (m_BonSrcDecoder.IsOpen()) {
		m_BonSrcDecoder.CloseTuner();
	}

	return true;
}


bool CDtvEngine::ReleaseSrcFilter()
{
	return UnloadBonDriver();
}


bool CDtvEngine::IsSrcFilterOpen() const
{
	return m_BonSrcDecoder.IsOpen();
}


void CDtvEngine::SetStartStreamingOnDriverOpen(bool bStart)
{
	m_bStartStreamingOnDriverOpen = bStart;
}


bool CDtvEngine::SetChannel(const DWORD TuningSpace, const DWORD Channel, const WORD ServiceID)
{
	TRACE(TEXT("CDtvEngine::SetChannel(%u, %u, %04x)\n"), TuningSpace, Channel, ServiceID);

	m_EngineLock.Lock();

	const WORD OldServiceID = m_SetChannelServiceID;

	// サービスはPATが来るまで保留
	m_SetChannelServiceID = ServiceID;

	m_EngineLock.Unlock();

	// チャンネル変更
	if (!m_BonSrcDecoder.SetChannelAndPlay(TuningSpace, Channel)) {
		m_SetChannelServiceID = OldServiceID;
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	return true;
}


bool CDtvEngine::SetService(const WORD wService)
{
	CBlockLock Lock(&m_EngineLock);

	// サービス変更(wService==0xFFFFならPAT先頭サービス)

	if (wService == 0xFFFF || wService < m_TsAnalyzer.GetServiceNum()) {
		WORD wServiceID;

		if (wService == 0xFFFF) {
			// 先頭PMTが到着するまで失敗にする
			if (!m_TsAnalyzer.GetFirstViewableServiceID(&wServiceID))
				return false;
		} else {
			if (!m_TsAnalyzer.GetServiceID(wService, &wServiceID))
				return false;
		}
		m_CurServiceIndex = m_TsAnalyzer.GetServiceIndexByID(wServiceID);

		m_CurServiceID = wServiceID;
		m_SpecServiceID = wServiceID;

		TRACE(TEXT("------- Service Select -------\n"));
		TRACE(TEXT("%d (ServiceID = %04X)\n"), m_CurServiceIndex, wServiceID);

		m_TsPacketCounter.SetActiveServiceID(wServiceID);

		if (m_bWriteCurServiceOnly)
			SetWriteStream(wServiceID, m_WriteStream);

#ifdef DTVENGINE_NETWORK_SUPPORT
#ifdef BONTSENGINE_1SEG_SUPPORT
		if (m_bSend1Seg)
			SetSendStream(SID_1SEG, m_SendStream);
		else
#endif
		if (m_bSendCurServiceOnly)
			SetSendStream(wServiceID, m_SendStream);
#endif

		if (m_pEventHandler)
			m_pEventHandler->OnServiceChanged(wServiceID);

		return true;
	}

	return false;
}


bool CDtvEngine::GetServiceID(WORD *pServiceID)
{
	// サービスID取得
	return m_TsAnalyzer.GetServiceID(m_CurServiceIndex, pServiceID);
}


bool CDtvEngine::SetServiceByID(const WORD ServiceID, const bool bReserve)
{
	CBlockLock Lock(&m_EngineLock);

	// bReserve == true の場合、まだPATが来ていなくてもエラーにしない

	int Index;
#ifdef BONTSENGINE_1SEG_SUPPORT
	if (ServiceID == SID_1SEG)
		Index = Find1SegService();
	else
#endif
		Index = m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	if (Index < 0) {
		if (!bReserve || m_CurTransportStreamID != TSID_INVALID)
			return false;
	} else {
		SetService(Index);
	}
	m_SpecServiceID = ServiceID;

	return true;
}


bool CDtvEngine::SetWriteStream(WORD ServiceID, DWORD Stream)
{
	m_WriteStream = Stream;

	return m_FileTsSelector.SetTargetServiceID(ServiceID, Stream);
}


bool CDtvEngine::GetWriteStream(WORD *pServiceID, DWORD *pStream)
{
	if (pServiceID)
		*pServiceID = m_FileTsSelector.GetTargetServiceID();
	if (pStream)
		*pStream = m_FileTsSelector.GetTargetStream();
	return true;
}


bool CDtvEngine::SetWriteCurServiceOnly(bool bOnly, DWORD Stream)
{
	if (m_bWriteCurServiceOnly != bOnly || m_WriteStream != Stream) {
		m_bWriteCurServiceOnly = bOnly;

		WORD ServiceID = 0;
		if (bOnly)
			GetServiceID(&ServiceID);
		SetWriteStream(ServiceID, Stream);
	}
	return true;
}


#ifdef DTVENGINE_NETWORK_SUPPORT

bool CDtvEngine::SetSendStream(WORD ServiceID, DWORD Stream)
{
#ifdef BONTSENGINE_1SEG_SUPPORT
	if (ServiceID == SID_1SEG) {
		m_bSend1Seg = true;
		if (Find1SegService(&ServiceID) < 0)
			ServiceID = 0xFFFF;
	} else {
		m_bSend1Seg = false;
	}
#endif

	m_SendStream = Stream;

	return m_NetworkTsSelector.SetTargetServiceID(ServiceID, Stream);
}


bool CDtvEngine::SetSendCurServiceOnly(bool bOnly, DWORD Stream)
{
	if (m_bSendCurServiceOnly != bOnly || m_SendStream != Stream) {
		m_bSendCurServiceOnly = bOnly;

		WORD ServiceID = 0;
		if (bOnly)
			GetServiceID(&ServiceID);
		SetSendStream(ServiceID, Stream);
	}
	return true;
}

#endif	// DTVENGINE_NETWORK_SUPPORT


WORD CDtvEngine::GetEventID(bool bNext)
{
	return m_TsAnalyzer.GetEventID(m_CurServiceIndex, bNext);
}


int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventName(m_CurServiceIndex, pszName, MaxLength, bNext);
}


int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventText(m_CurServiceIndex, pszText, MaxLength, bNext);
}


int CDtvEngine::GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventExtendedText(m_CurServiceIndex, pszText, MaxLength, true, bNext);
}


bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, SYSTEMTIME *pEndTime, bool bNext)
{
	SYSTEMTIME stStart;

	if (!m_TsAnalyzer.GetEventStartTime(m_CurServiceIndex, &stStart, bNext))
		return false;
	if (pStartTime)
		*pStartTime = stStart;
	if (pEndTime) {
		DWORD Duration = m_TsAnalyzer.GetEventDuration(m_CurServiceIndex, bNext);
		if (Duration == 0)
			return false;

		CDateTime Time(stStart);
		Time.Offset(CDateTime::SECONDS(Duration));
		Time.Get(pEndTime);
	}
	return true;
}


DWORD CDtvEngine::GetEventDuration(bool bNext)
{
	return m_TsAnalyzer.GetEventDuration(m_CurServiceIndex, bNext);
}


bool CDtvEngine::GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventSeriesInfo(m_CurServiceIndex, pInfo, bNext);
}


bool CDtvEngine::GetEventInfo(CEventInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventInfo(m_CurServiceIndex, pInfo, true, bNext);
}


bool CDtvEngine::GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex, bool bNext)
{
	return m_TsAnalyzer.GetEventAudioInfo(m_CurServiceIndex, AudioIndex, pInfo, bNext);
}


unsigned __int64 CDtvEngine::GetPcrTimeStamp()
{
	// PCRタイムスタンプ取得
	unsigned __int64 TimeStamp;
	if (m_TsAnalyzer.GetPcrTimeStamp(m_CurServiceIndex, &TimeStamp))
		return TimeStamp;
	return 0ULL;
}


const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if (pDecoder == &m_BonSrcDecoder) {
		switch (dwEventID) {
		case CBonSrcDecoder::EVENT_GRAPH_RESET:
			// デコーダグラフがリセットされた
			ResetStatus();
			return 0;

		case CBonSrcDecoder::EVENT_CHANNEL_CHANGED:
			// チャンネルが変更された
			{
				CBlockLock Lock(&m_EngineLock);

				m_SpecServiceID = m_SetChannelServiceID;
				ResetStatus();
			}
			return 0;
		}
	} else if (pDecoder == &m_TsAnalyzer) {
		switch (dwEventID) {
		case CTsAnalyzer::EVENT_PAT_UPDATED:
		case CTsAnalyzer::EVENT_PMT_UPDATED:
			// サービスの構成が変化した
			{
				CBlockLock Lock(&m_EngineLock);
				const WORD TransportStreamID = m_TsAnalyzer.GetTransportStreamID();
				const bool bStreamChanged = m_CurTransportStreamID != TransportStreamID;

				if (bStreamChanged) {
					// ストリームIDが変わっているなら初期化
					TRACE(TEXT("CDtvEngine ■Stream Change!! %04X\n"), TransportStreamID);

					m_CurServiceIndex = SERVICE_INVALID;
					m_CurServiceID = SID_INVALID;
					m_CurTransportStreamID = TransportStreamID;

					bool bSetService = true;
					WORD Service = SERVICE_INVALID;
#ifdef BONTSENGINE_1SEG_SUPPORT
					if (m_SpecServiceID == SID_1SEG) {
						const int Service1Seg = Find1SegService();
						if (Service1Seg >= 0) {
							Service = (WORD)Service1Seg;
						} else {
							bSetService = false;
						}
					} else
#endif
					if (m_SpecServiceID != SID_INVALID) {
						// サービスが指定されている
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							if (m_TsAnalyzer.GetViewableServiceNum() > 0) {
								TRACE(TEXT("Service not found %04x\n"), m_SpecServiceID);
								m_SpecServiceID = SID_INVALID;
							} else {
								// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
								bSetService = false;
							}
						} else {
							if (m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								Service = (WORD)ServiceIndex;
							} else {
								// サービスはPATにあるが、まだPMTが来ていない
								bSetService = false;
							}
						}
					}
					if (bSetService)
						SetService(Service);
				} else {
					// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
					TRACE(TEXT("CDtvEngine ■Stream Updated!! %04X\n"), TransportStreamID);

					bool bSetService = true;
					WORD Service = SERVICE_INVALID;
#ifdef BONTSENGINE_1SEG_SUPPORT
					if (m_SpecServiceID == SID_1SEG) {
						const int Service1Seg = Find1SegService();
						if (Service1Seg >= 0) {
							Service = (WORD)Service1Seg;
						} else {
							bSetService = false;
						}
					} else
#endif
					if (m_SpecServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							if (m_TsAnalyzer.GetViewableServiceNum() > 0) {
								TRACE(TEXT("Service not found %04x\n"), m_SpecServiceID);
								m_SpecServiceID = SID_INVALID;
							} else {
								bSetService = false;
							}
						} else {
							if (m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								Service = (WORD)ServiceIndex;
							} else {
								bSetService = false;
							}
						}
					}
					if (bSetService && Service == SERVICE_INVALID && m_CurServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_CurServiceID);
						if (ServiceIndex < 0) {
							if (m_TsAnalyzer.GetViewableServiceNum() > 0) {
								TRACE(TEXT("Service not found %04x\n"), m_SpecServiceID);
								m_CurServiceID = SID_INVALID;
							} else {
								bSetService = false;
							}
						} else {
							if (m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								Service = (WORD)ServiceIndex;
							} else {
								bSetService = false;
							}
						}
					}
					if (bSetService)
						SetService(Service);
				}
				if (m_pEventHandler)
					m_pEventHandler->OnServiceListUpdated(&m_TsAnalyzer, bStreamChanged);
			}
			return 0UL;

		case CTsAnalyzer::EVENT_SDT_UPDATED:
			// サービスの情報が更新された
			if (m_pEventHandler)
				m_pEventHandler->OnServiceInfoUpdated(&m_TsAnalyzer);
			return 0UL;

		case CTsAnalyzer::EVENT_EIT_UPDATED:
			//  EITが更新された
			if (m_pEventHandler)
				m_pEventHandler->OnEventUpdated(&m_TsAnalyzer);
			return 0UL;

		case CTsAnalyzer::EVENT_TOT_UPDATED:
			//  TOTが更新された
			if (m_pEventHandler)
				m_pEventHandler->OnTotUpdated(&m_TsAnalyzer);
			return 0UL;
		}
	} else if (pDecoder == &m_TsRecorder) {
		switch (dwEventID) {
		case CTsRecorder::EVENT_WRITE_ERROR:
			// 書き込みエラーが発生した
			if (m_pEventHandler)
				m_pEventHandler->OnFileWriteError(&m_TsRecorder, *static_cast<const DWORD*>(pParam));
			return 0UL;
		}
	}

	return 0UL;
}


void CDtvEngine::ResetStatus()
{
	m_CurTransportStreamID = TSID_INVALID;
	m_CurServiceIndex = SERVICE_INVALID;
	m_CurServiceID = SID_INVALID;
}


#ifdef BONTSENGINE_1SEG_SUPPORT
int CDtvEngine::Find1SegService(WORD *pServiceID)
{
	const WORD NumServices = m_TsAnalyzer.GetServiceNum();
	WORD FirstPID = 0xFFFF;
	int FirstIndex = -1;

	for (WORD i = 0 ; i < NumServices ; i++) {
		WORD PmtPID;
		if (m_TsAnalyzer.GetPmtPID(i, &PmtPID)
				&& PmtPID >= 0x1FC8 && PmtPID <= 0x1FCF
				&& PmtPID < FirstPID) {
			FirstPID = PmtPID;
			FirstIndex = i;
		}
	}

	if (pServiceID) {
		if (!m_TsAnalyzer.GetServiceID(FirstIndex, pServiceID))
			return -1;
	}

	return FirstIndex;
}
#endif
