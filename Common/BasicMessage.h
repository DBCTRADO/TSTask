#ifndef TSTASK_BASIC_MESSAGE_H
#define TSTASK_BASIC_MESSAGE_H


#include <vector>
#include "Message.h"
#include "TaskUtility.h"
#include "Channel.h"
#include "Streaming.h"


#define TSTASK_DEFINE_MESSAGE(name) \
	TSTASK_DEFINE_STRING(MESSAGE_##name,L#name)
#define TSTASK_DEFINE_MESSAGE_EVENT(name) \
	TSTASK_DEFINE_STRING(MESSAGE_EVENT_##name,L#name)
#define TSTASK_DEFINE_MESSAGE_PROPERTY(name) \
	TSTASK_DEFINE_STRING(MESSAGE_PROPERTY_##name,L#name)
#define TSTASK_DEFINE_MESSAGE_RESULT(name) \
	TSTASK_DEFINE_STRING(MESSAGE_RESULT_##name,L#name)


namespace TSTask
{

	TSTASK_DEFINE_MESSAGE(Response);
	TSTASK_DEFINE_MESSAGE(Event);
	TSTASK_DEFINE_MESSAGE(Hello);
	TSTASK_DEFINE_MESSAGE(RegisterClient);
	TSTASK_DEFINE_MESSAGE(UnregisterClient);
	TSTASK_DEFINE_MESSAGE(TaskStarted);
	TSTASK_DEFINE_MESSAGE(TaskEnded);
	TSTASK_DEFINE_MESSAGE(EndTask);
	TSTASK_DEFINE_MESSAGE(LoadBonDriver);
	TSTASK_DEFINE_MESSAGE(UnloadBonDriver);
	TSTASK_DEFINE_MESSAGE(GetBonDriver);
	TSTASK_DEFINE_MESSAGE(OpenTuner);
	TSTASK_DEFINE_MESSAGE(CloseTuner);
	TSTASK_DEFINE_MESSAGE(SetChannel);
	TSTASK_DEFINE_MESSAGE(GetChannel);
	TSTASK_DEFINE_MESSAGE(SetService);
	TSTASK_DEFINE_MESSAGE(GetService);
	TSTASK_DEFINE_MESSAGE(GetServiceList);
	TSTASK_DEFINE_MESSAGE(GetStreamStatistics);
	TSTASK_DEFINE_MESSAGE(ResetErrorStatistics);
	TSTASK_DEFINE_MESSAGE(StartRecording);
	TSTASK_DEFINE_MESSAGE(StopRecording);
	TSTASK_DEFINE_MESSAGE(ChangeRecordingFile);
	TSTASK_DEFINE_MESSAGE(GetRecordingInfo);
	TSTASK_DEFINE_MESSAGE(GetEventInfo);
	TSTASK_DEFINE_MESSAGE(GetLog);
	TSTASK_DEFINE_MESSAGE(GetChannelList);
	TSTASK_DEFINE_MESSAGE(GetBonDriverChannelList);
	TSTASK_DEFINE_MESSAGE(GetScannedChannelList);
	TSTASK_DEFINE_MESSAGE(CreateStreamPool);
	TSTASK_DEFINE_MESSAGE(CloseStreamPool);
	TSTASK_DEFINE_MESSAGE(GetTvRockInfo);
	TSTASK_DEFINE_MESSAGE(StartStreaming);
	TSTASK_DEFINE_MESSAGE(StopStreaming);
	TSTASK_DEFINE_MESSAGE(GetStreamingInfo);
	TSTASK_DEFINE_MESSAGE(GetStreamingSettings);
	TSTASK_DEFINE_MESSAGE(GetSetting);

	TSTASK_DEFINE_MESSAGE_EVENT(BonDriverLoaded);
	TSTASK_DEFINE_MESSAGE_EVENT(BonDriverUnloaded);
	TSTASK_DEFINE_MESSAGE_EVENT(TunerOpened);
	TSTASK_DEFINE_MESSAGE_EVENT(TunerClosed);
	TSTASK_DEFINE_MESSAGE_EVENT(ChannelChanged);
	TSTASK_DEFINE_MESSAGE_EVENT(ServiceChanged);
	TSTASK_DEFINE_MESSAGE_EVENT(RecordingStarted);
	TSTASK_DEFINE_MESSAGE_EVENT(RecordingStopped);
	TSTASK_DEFINE_MESSAGE_EVENT(RecordingFileChanged);
	TSTASK_DEFINE_MESSAGE_EVENT(StreamChanged);
	TSTASK_DEFINE_MESSAGE_EVENT(EventChanged);
	TSTASK_DEFINE_MESSAGE_EVENT(StreamingStarted);
	TSTASK_DEFINE_MESSAGE_EVENT(StreamingStopped);
	TSTASK_DEFINE_MESSAGE_EVENT(IniSettingsChanged);

	TSTASK_DEFINE_MESSAGE_PROPERTY(Result);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Message);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ErrorCode);
	TSTASK_DEFINE_MESSAGE_PROPERTY(TaskID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(FilePath);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Directory);
	TSTASK_DEFINE_MESSAGE_PROPERTY(FileName);
	TSTASK_DEFINE_MESSAGE_PROPERTY(TuningSpace);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Channel);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ChannelName);
	TSTASK_DEFINE_MESSAGE_PROPERTY(TuningSpaceName);
	TSTASK_DEFINE_MESSAGE_PROPERTY(NetworkID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(TransportStreamID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ServiceName);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ServiceID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ServiceType);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ScannedChannel);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ScannedServiceID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(RemoteControlKeyID);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Name);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Value);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Status);
	TSTASK_DEFINE_MESSAGE_PROPERTY(ServiceSelect);
	TSTASK_DEFINE_MESSAGE_PROPERTY(Streams);
	TSTASK_DEFINE_MESSAGE_PROPERTY(StartTime);
	TSTASK_DEFINE_MESSAGE_PROPERTY(StartTickCount);

	TSTASK_DEFINE_MESSAGE_RESULT(OK);
	TSTASK_DEFINE_MESSAGE_RESULT(Failed);
	TSTASK_DEFINE_MESSAGE_RESULT(NoProperty);

	struct SetChannelInfo
	{
		enum ChannelType
		{
			TYPE_INDEX,
			TYPE_SCANNED,
			TYPE_IDS
		};

		ChannelType Type;

		TuningChannelInfo Index;
		ScannedChannelInfo Scanned;
		ChannelStreamIDs IDs;

		bool fForceChange;
	};

	struct BonDriverTuningSpaceInfo
	{
		String Name;
		std::vector<String> ChannelList;
	};
	typedef std::vector<BonDriverTuningSpaceInfo> BonDriverTuningSpaceList;

	bool BroadcastServerMessage(const CMessage &Message);

	namespace BasicMessage
	{

		namespace TaskStarted
		{
			bool InitializeMessage(CMessage *pMessage,TaskID ID);
			bool GetProperties(const CMessage *pMessage,TaskID *pID);
		}

		namespace TaskEnded
		{
			bool InitializeMessage(CMessage *pMessage,TaskID ID);
			bool GetProperties(const CMessage *pMessage,TaskID *pID);
		}

		namespace LoadBonDriver
		{
			bool InitializeMessage(CMessage *pMessage,LPCWSTR pszBonDriverPath);
			bool GetProperties(const CMessage *pMessage,String *pBonDriverPath);
		}

		namespace SetChannel
		{
			bool InitializeMessage(CMessage *pMessage,const SetChannelInfo &Info);
			bool GetProperties(const CMessage *pMessage,SetChannelInfo *pInfo);
		}

		namespace GetChannel
		{
			bool SetResponse(CMessage *pMessage,const ChannelInfo &Info);
			bool GetProperties(const CMessage *pMessage,ChannelInfo *pInfo);
		}

		namespace GetService
		{
			bool SetResponse(CMessage *pMessage,const ServiceInfo &Info);
			bool GetProperties(const CMessage *pMessage,ServiceInfo *pInfo);
		}

		namespace GetServiceList
		{
			bool SetResponse(CMessage *pMessage,const ServiceList &List);
			bool GetProperties(const CMessage *pMessage,ServiceList *pList);
		}

		namespace StartRecording
		{
			bool InitializeMessage(CMessage *pMessage,const RecordingSettings &Settings);
			bool GetProperties(const CMessage *pMessage,RecordingSettings *pSettings);
		}

		namespace GetRecordingInfo
		{
			bool SetResponse(CMessage *pMessage,const RecordingInfo &Info);
			bool GetProperties(const CMessage *pMessage,RecordingInfo *pInfo);
		}

		namespace GetStreamStatistics
		{
			bool SetResponse(CMessage *pMessage,const StreamStatistics &Statistics);
			bool GetProperties(const CMessage *pMessage,StreamStatistics *pStatistics);
		}

		namespace GetEventInfo
		{
			bool SetResponse(CMessage *pMessage,const EventInfo &Info);
			bool GetProperties(const CMessage *pMessage,EventInfo *pInfo);
		}

		namespace GetLog
		{
			bool SetResponse(CMessage *pMessage,const LogList &List);
			bool GetProperties(const CMessage *pMessage,LogList *pList);
		}

		namespace GetBonDriverChannelList
		{
			bool SetResponse(CMessage *pMessage,const BonDriverTuningSpaceList &TuningSpaceList);
			bool GetProperties(const CMessage *pMessage,BonDriverTuningSpaceList *pTuningSpaceList);
		}

		namespace GetScannedChannelList
		{
			bool SetResponse(CMessage *pMessage,const CTuningSpaceList &TuningSpaceList);
			bool GetProperties(const CMessage *pMessage,CTuningSpaceList *pTuningSpaceList);
		}

		namespace StartStreaming
		{
			bool InitializeMessage(CMessage *pMessage,const StreamingInfo &Info);
			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo);
		}

		namespace GetStreamingInfo
		{
			bool SetResponse(CMessage *pMessage,const StreamingInfo &Info);
			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo);
		}

		namespace GetStreamingSettings
		{
			bool SetResponse(CMessage *pMessage,const StreamingInfo &Info);
			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo);
		}

	}

}


#endif
