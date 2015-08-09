#ifndef TSTASK_EVENT_HANDLER_H
#define TSTASK_EVENT_HANDLER_H


namespace TSTask
{

	class CEventHandler
	{
	public:
		virtual bool OnBonDriverLoad(LPCWSTR pszFileName) { return true; }
		virtual void OnBonDriverLoaded(LPCWSTR pszFileName) {}
		virtual void OnBonDriverLoadFailed(LPCWSTR pszFileName) {}
		virtual bool OnBonDriverUnload() { return true; }
		virtual void OnBonDriverUnloaded() {}
		virtual bool OnTunerOpen() { return true; }
		virtual void OnTunerOpened() {}
		virtual void OnTunerOpenFailed() {}
		virtual bool OnTunerClose() { return true; }
		virtual void OnTunerClosed() {}
		virtual bool OnChannelChange(DWORD Space,DWORD Channel,WORD ServiceID) { return true; }
		virtual void OnChannelChanged(DWORD Space,DWORD Channel,WORD ServiceID) {}
		virtual void OnChannelChangeFailed(DWORD Space,DWORD Channel,WORD ServiceID) {}
		virtual bool OnServiceChange(WORD ServiceID) { return true; }
		virtual void OnServiceChanged(WORD ServiceID) {}
		virtual bool OnRecordingStart(const RecordingSettings &Settings) { return true; }
		virtual void OnRecordingStarted(const RecordingInfo &Info) {}
		virtual void OnRecordingStartFailed(const RecordingSettings &Settings) {}
		virtual bool OnRecordingStop() { return true; }
		virtual void OnRecordingStopped() {}
		virtual bool OnRecordingFileChange(LPCWSTR pszFileName) { return true; }
		virtual void OnRecordingFileChanged(LPCWSTR pszFileName) {}
		virtual void OnRecordingFileChangeFailed(LPCWSTR pszFileName) {}
		virtual void OnStreamChanged() {}
		virtual void OnEventChanged() {}
		virtual void OnTotChanged() {}
		virtual void OnErrorStatisticsReset() {}
		virtual bool OnStreamingStart(const StreamingInfo &Info) { return true; }
		virtual void OnStreamingStarted(const StreamingInfo &Info) {}
		virtual bool OnStreamingStop() { return true; }
		virtual void OnStreamingStopped() {}
		virtual bool OnChannelListLoaded(CTuningSpaceList &SpaceList) { return false; }
	};

}


#endif
