#ifndef TSTASKCENTRE_TS_TASK_MANAGER_H
#define TSTASKCENTRE_TS_TASK_MANAGER_H


#include <map>


namespace TSTaskCentre
{

	class CTSTaskManager
	{
	public:
		struct TaskProcessInfo
		{
			unsigned int Mask;
			DWORD ProcessID;
			FILETIME CreationTime;
			ULONGLONG KernelTime;
			ULONGLONG UserTime;
			ULONG64 CycleTime;
			IO_COUNTERS IOCounters;
			DWORD HandleCount;
			DWORD GdiObjects;
			DWORD UserObjects;
			struct {
				DWORD PageFaultCount;
				size_t WorkingSetSize;
				size_t PeakWorkingSetSize;
				size_t PageFileUsage;
				size_t PeakPageFileUsage;
				size_t PrivateUsage;
			} Memory;
			bool fWOW64;
		};

		enum
		{
			PROCESS_INFO_PROCESS_TIMES	=0x0001U,
			PROCESS_INFO_CYCLE_TIME		=0x0002U,
			PROCESS_INFO_IO_COUNTERS	=0x0004U,
			PROCESS_INFO_HANDLE_COUNT	=0x0008U,
			PROCESS_INFO_GDI_OBJECTS	=0x0010U,
			PROCESS_INFO_USER_OBJECTS	=0x0020U,
			PROCESS_INFO_MEMORY			=0x0040U,
			PROCESS_INFO_WOW64			=0x0080U,
			PROCESS_INFO_ALL			=0x00FFU
		};

		CTSTaskManager();
		~CTSTaskManager();
		int GetTaskCount() const;
		bool GetTaskList(TSTask::TaskUtility::TaskIDList *pList) const;
		bool AddTask(TSTask::TaskID ClientTaskID,TSTask::TaskID ID);
		bool RemoveTask(TSTask::TaskID ID);
		bool GetTaskInfo(TSTask::TaskID ID,TSTask::TaskInfo *pInfo);
		bool GetTaskProcessInfo(TSTask::TaskID ID,TaskProcessInfo *pInfo);
		bool GetTaskProcessPath(TSTask::TaskID ID,TSTask::String *pPath);
		bool SendMessage(TSTask::TaskID ID,
						 const TSTask::CMessage *pSendMessage,TSTask::CMessage *pReceiveMessage=nullptr);
		bool BroadcastMessage(const TSTask::CMessage *pSendMessage);
		bool EndTask(TSTask::TaskID ID);
		bool TerminateTask(TSTask::TaskID ID);

		bool GetStreamStatistics(TSTask::TaskID ID,TSTask::StreamStatistics *pStatistics);
		bool GetTotTime(TSTask::TaskID ID,SYSTEMTIME *pTime);

		bool LoadBonDriver(TSTask::TaskID ID,LPCWSTR pszFilePath);
		bool UnloadBonDriver(TSTask::TaskID ID);
		bool GetBonDriver(TSTask::TaskID ID,TSTask::String *pFilePath,TSTask::String *pTunerName,bool fUpdate=false);
		bool OpenTuner(TSTask::TaskID ID);
		bool CloseTuner(TSTask::TaskID ID);
		bool SetChannelByIndex(TSTask::TaskID ID,DWORD Space,DWORD Channel,WORD ServiceID,bool fForce=false);
		bool SetChannelByScanned(TSTask::TaskID ID,int Space,int Channel,WORD ServiceID,bool fForce=false);
		bool SetChannelByIDs(TSTask::TaskID ID,WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fForce=false);
		bool GetChannel(TSTask::TaskID ID,TSTask::ChannelInfo *pChannel);
		bool SetService(TSTask::TaskID ID,WORD ServiceID);
		bool GetService(TSTask::TaskID ID,TSTask::ServiceInfo *pInfo);
		bool GetServiceList(TSTask::TaskID ID,TSTask::ServiceList *pList);
		bool StartRecording(TSTask::TaskID ID,const TSTask::RecordingSettings *pSettings=nullptr);
		bool StopRecording(TSTask::TaskID ID);
		bool ChangeRecordingFile(TSTask::TaskID ID,LPCWSTR pszFileName);
		bool GetRecordingInfo(TSTask::TaskID ID,TSTask::RecordingInfo *pInfo);
		bool ResetErrorStatistics(TSTask::TaskID ID);
		bool GetEventInfo(TSTask::TaskID ID,TSTask::EventInfo *pInfo,bool fNext=false);
		bool GetLog(TSTask::TaskID ID,TSTask::LogList *pList);
		bool GetBonDriverChannelList(TSTask::TaskID ID,TSTask::BonDriverTuningSpaceList *pSpaceList);
		bool GetScannedChannelList(TSTask::TaskID ID,TSTask::CTuningSpaceList *pSpaceList);
		bool GetTvRockInfo(TSTask::TaskID ID,int *pDeviceID);
		bool StartStreaming(TSTask::TaskID ID,const TSTask::StreamingInfo *pInfo=nullptr);
		bool StopStreaming(TSTask::TaskID ID);
		bool GetStreamingInfo(TSTask::TaskID ID,TSTask::StreamingInfo *pInfo);
		bool GetStreamingSettings(TSTask::TaskID ID,TSTask::StreamingInfo *pInfo);
		bool GetSetting(TSTask::TaskID ID,LPCWSTR pszName,TSTask::String *pValue);
		bool SendIniSettingsChanged(TSTask::TaskID ID,const TSTask::String &FilePath);

	private:
		bool SendBasicMessage(TSTask::TaskID ID,const TSTask::CMessage &Message);
		bool SendBasicMessage(TSTask::TaskID ID,LPCWSTR pszName);
		void OutTimeoutErrorLog() const;

		class CTask
		{
		public:
			CTask(TSTask::TaskID ID);
			~CTask();
			bool Open(TSTask::TaskID ClientTaskID);
			bool UnregisterClient();
			bool GetTaskInfo(TSTask::TaskInfo *pInfo) const;
			DWORD GetProcessID();
			bool SendMessage(const TSTask::CMessage *pSendMessage,TSTask::CMessage *pReceiveMessage);
			bool GetStreamStatistics(TSTask::StreamStatistics *pStatistics);
			bool GetTotTime(SYSTEMTIME *pTime);
			bool GetBonDriver(TSTask::String *pFilePath,TSTask::String *pTunerName,bool fUpdate);

		private:
			TSTask::TaskID m_TaskID;
			TSTask::TaskID m_ClientTaskID;
			TSTask::TaskInfo m_TaskInfo;
			TSTask::CLocalMessageClient m_MessageClient;
			TSTask::CServerTaskSharedInfoReader m_SharedInfoReader;
			bool m_fBonDriverValid;
			TSTask::String m_BonDriverFilePath;
			TSTask::String m_TunerName;
		};

		std::map<TSTask::TaskID,CTask*> m_TaskList;
		mutable TSTask::CLocalLock m_Lock;
		DWORD m_LockTimeout;
	};

}


#endif
