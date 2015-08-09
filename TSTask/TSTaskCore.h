#ifndef TSTASK_CORE_H
#define TSTASK_CORE_H


#include <vector>
#include "DtvEngine.h"
#include "EventHandler.h"
#include "TSTaskStreamPool.h"


namespace TSTask
{

	class CTSTaskCore : public ::CDtvEngine::CEventHandler, public ::CTracer
	{
	public:
		CTSTaskCore();
		~CTSTaskCore();

		bool Initialize();
		bool Finalize();

		bool LoadBonDriver(LPCWSTR pszFileName);
		bool UnloadBonDriver();
		bool IsBonDriverLoaded() const;
		bool GetBonDriverFilePath(String *pFilePath) const;
		bool OpenTuner();
		bool CloseTuner();
		bool IsTunerOpened() const;
		bool GetTunerName(String *pName) const;

		bool SetFirstChannelSetDelay(unsigned int Delay);
		bool SetMinChannelChangeInterval(unsigned int Interval);

		bool SetChannelByIndex(DWORD Space,DWORD Channel,WORD ServiceID=0,bool fForce=false);
		bool SetChannelByScanned(int Space,int Channel,WORD ServiceID=0,bool fForce=false);
		bool SetChannelByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID=0,bool fForce=false);
		bool GetChannel(ChannelInfo *pInfo);
		bool SetService(WORD ServiceID);
		bool GetServiceInfo(ServiceInfo *pInfo);
		bool GetServiceList(ServiceList *pList);

		bool StartRecording(const RecordingSettings &Settings);
		bool StopRecording();
		bool SetNextRecordingDirectory();
		bool ChangeRecordingFile(LPCWSTR pszFileName);
		bool GetRecordingInfo(RecordingInfo *pInfo) const;
		bool IsRecording() const;
		bool FormatRecordFileName(LPCWSTR pszFormat,String *pFileName,unsigned int UseNextEventInfoMargin=0);
		bool SetWriteBufferSize(unsigned int Size);
		bool SetMaxWritePendingSize(unsigned int Size);
		bool SetWritePreAllocate(ULONGLONG PreAllocateSize);
		bool SetMinDiskFreeSpace(ULONGLONG MinFreeSpace);

		bool GetStreamIDs(ChannelStreamIDs *pStreamIDs);
		bool GetStreamStatistics(StreamStatistics *pStatistics);
		bool GetTotTime(SYSTEMTIME *pTime);

		bool ResetErrorStatistics();
		bool GetEventInfo(EventInfo *pInfo,bool fNext=false);
		bool GetBonDriverChannelList(BonDriverTuningSpaceList *pSpaceList);
		bool GetScannedChannelList(CTuningSpaceList *pSpaceList) const;
		bool GetScannedChannelList(int Space,CChannelList *pChannelList) const;

		bool StartStreaming(const StreamingInfo &Info);
		bool StopStreaming();
		bool GetStreamingInfo(StreamingInfo *pInfo) const;
		bool IsStreaming() const;
		bool SetStreamingSendSize(unsigned int Size);
		bool SetStreamingSendWait(unsigned int Wait,bool fAdjustWait);
		bool SetStreamingConnectRetryInterval(unsigned int Interval);
		bool SetStreamingMaxConnectRetries(int MaxRetries);
		bool SetStreamingTcpMaxSendRetries(int MaxRetries);
		bool SetStreamingTcpPrependHeader(bool fPrependHeader);

		bool CreateStreamPool(LPCWSTR pszName,DWORD BufferLength);
		bool CloseStreamPool();

		bool AddEventHandler(TSTask::CEventHandler *pEventHandler);
		bool RemoveEventHandler(TSTask::CEventHandler *pEventHandler);

		typedef ::CTsGrabber::ITsHandler ITsGrabber;
		bool AddTsGrabber(ITsGrabber *pGrabber);
		bool RemoveTsGrabber(ITsGrabber *pGrabber);

	private:
		void OutTimeoutErrorLog() const;
		void OutBonTsEngineErrorLog(const ::CBonErrorHandler &ErrorHandler) const;

	// CDtvEngine::CEventHandler
		void OnServiceChanged(WORD ServiceID) override;
		void OnServiceListUpdated(::CTsAnalyzer *pTsAnalyzer,bool bStreamChanged) override;
		void OnEventUpdated(::CTsAnalyzer *pTsAnalyzer) override;
		void OnTotUpdated(::CTsAnalyzer *pTsAnalyzer) override;
		void OnFileWriteError(::CTsRecorder *pTsRecorder,DWORD ErrorCode) override;

	// CTracer
		void OnTrace(::CTracer::TraceType Type,LPCTSTR pszOutput) override;

		typedef std::vector<TSTask::CEventHandler*> EventHandlerList;

		struct EventStatus
		{
			WORD EventID;
			DWORD Duration;

			bool operator==(const EventStatus &Op) const
			{
				return EventID==Op.EventID
					&& Duration==Op.Duration;
			}

			bool operator!=(const EventStatus &Op) const { return !(*this==Op); }
		};

		bool m_fInitialized;
		mutable CLocalLock m_Lock;
		DWORD m_LockTimeout;
		::CDtvEngine m_DtvEngine;
		EventHandlerList m_EventHandlerList;
		TuningChannelInfo m_CurrentChannel;
		ScannedChannelInfo m_CurrentScannedChannel;
		CTuningSpaceList m_TuningSpaceList;
		EventStatus m_CurrentEvent;

		RecordingInfo m_RecordingInfo;

		CStreamingManager m_StreamingManager;
		StreamingInfo m_StreamingInfo;
		CMutex m_StreamingMutex;

		CTSTaskStreamPool m_StreamPool;
		UINT m_StreamPoolCount;

		ULONGLONG m_MinDiskFreeSpace;
	};

}


#endif
