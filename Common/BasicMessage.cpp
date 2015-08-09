#include "stdafx.h"
#include "TSTaskBase.h"
#include "BasicMessage.h"
#include "MessageClient.h"
#include "DebugDef.h"


namespace TSTask
{

	bool BroadcastServerMessage(const CMessage &Message)
	{
		OutLog(LOG_VERBOSE,L"全てのクライアントにメッセージ(%s)を送信します。",Message.GetName());

		TaskUtility::TaskIDList TaskList;

		TaskUtility::GetClientTaskList(&TaskList);

		for (auto e:TaskList) {
			CLocalMessageClient MessageClient;
			String ServerName;

			TaskUtility::GetClientTaskLocalMessageServerName(e,&ServerName);
			MessageClient.SetServer(ServerName.c_str());
			MessageClient.SendMessage(&Message);
		}

		return true;
	}


	namespace BasicMessage
	{

		namespace TaskStarted
		{

			bool InitializeMessage(CMessage *pMessage,TaskID ID)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetName(MESSAGE_TaskStarted);
				pMessage->SetPropertyInt(MESSAGE_PROPERTY_TaskID,ID);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,TaskID *pID)
			{
				if (pMessage==nullptr || pID==nullptr)
					return false;

				CMessageProperty::IntType ID;
				if (!pMessage->GetProperty(MESSAGE_PROPERTY_TaskID,&ID))
					return false;

				*pID=(TaskID)ID;

				return true;
			}

		}

		namespace TaskEnded
		{

			bool InitializeMessage(CMessage *pMessage,TaskID ID)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetName(MESSAGE_TaskEnded);
				pMessage->SetPropertyInt(MESSAGE_PROPERTY_TaskID,ID);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,TaskID *pID)
			{
				if (pMessage==nullptr || pID==nullptr)
					return false;

				CMessageProperty::IntType ID;
				if (!pMessage->GetProperty(MESSAGE_PROPERTY_TaskID,&ID))
					return false;

				*pID=(TaskID)ID;

				return true;
			}

		}

		namespace LoadBonDriver
		{

			bool InitializeMessage(CMessage *pMessage,LPCWSTR pszBonDriverPath)
			{
				if (pMessage==nullptr || IsStringEmpty(pszBonDriverPath))
					return false;

				pMessage->SetName(MESSAGE_LoadBonDriver);
				pMessage->SetProperty(MESSAGE_PROPERTY_FilePath,pszBonDriverPath);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,String *pBonDriverPath)
			{
				if (pMessage==nullptr || pBonDriverPath==nullptr)
					return false;

				return pMessage->GetProperty(MESSAGE_PROPERTY_FilePath,pBonDriverPath);
			}

		}

		namespace SetChannel
		{

			bool InitializeMessage(CMessage *pMessage,const SetChannelInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetName(MESSAGE_SetChannel);

				switch (Info.Type) {
				case SetChannelInfo::TYPE_INDEX:
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_TuningSpace,Info.Index.Space);
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_Channel,Info.Index.Channel);
					if (Info.Index.ServiceID!=0)
						pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceID,Info.Index.ServiceID);
					break;

				case SetChannelInfo::TYPE_SCANNED:
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_TuningSpace,Info.Scanned.Space);
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_ScannedChannel,Info.Scanned.Channel);
					if (Info.Scanned.ServiceID!=0)
						pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceID,Info.Scanned.ServiceID);
					break;

				case SetChannelInfo::TYPE_IDS:
					if (Info.IDs.NetworkID!=0)
						pMessage->SetPropertyInt(MESSAGE_PROPERTY_NetworkID,Info.IDs.NetworkID);
					if (Info.IDs.TransportStreamID!=0)
						pMessage->SetPropertyInt(MESSAGE_PROPERTY_TransportStreamID,Info.IDs.TransportStreamID);
					if (Info.IDs.ServiceID!=0)
						pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceID,Info.IDs.ServiceID);
					break;

				default:
					return false;
				}

				pMessage->SetProperty(L"ForceChange",Info.fForceChange);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,SetChannelInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				CMessageProperty::IntType Space,Channel,ServiceID=0;

				pMessage->GetProperty(MESSAGE_PROPERTY_ServiceID,&ServiceID);

				if (pMessage->GetProperty(MESSAGE_PROPERTY_ScannedChannel,&Channel)) {
					if (!pMessage->GetProperty(MESSAGE_PROPERTY_TuningSpace,&Space))
						return false;

					pInfo->Type=SetChannelInfo::TYPE_SCANNED;
					pInfo->Scanned.Space=(int)Space;
					pInfo->Scanned.Channel=(int)Channel;
					pInfo->Scanned.ServiceID=(WORD)ServiceID;
				} else if (pMessage->GetProperty(MESSAGE_PROPERTY_Channel,&Channel)) {
					if (!pMessage->GetProperty(MESSAGE_PROPERTY_TuningSpace,&Space))
						return false;

					pInfo->Type=SetChannelInfo::TYPE_INDEX;
					pInfo->Index.Space=(DWORD)Space;
					pInfo->Index.Channel=(DWORD)Channel;
					pInfo->Index.ServiceID=(WORD)ServiceID;
				} else {
					CMessageProperty::IntType NetworkID=0,TransportStreamID=0;

					pMessage->GetProperty(MESSAGE_PROPERTY_NetworkID,&NetworkID);
					pMessage->GetProperty(MESSAGE_PROPERTY_TransportStreamID,&TransportStreamID);
					if (NetworkID==0 && TransportStreamID==0 && ServiceID==0)
						return false;

					pInfo->Type=SetChannelInfo::TYPE_IDS;
					pInfo->IDs.NetworkID=(WORD)NetworkID;
					pInfo->IDs.TransportStreamID=(WORD)TransportStreamID;
					pInfo->IDs.ServiceID=(WORD)ServiceID;
				}

				if (!pMessage->GetProperty(L"ForceChange",&pInfo->fForceChange))
					pInfo->fForceChange=false;

				return true;
			}

		}

		namespace GetChannel
		{

			bool SetResponse(CMessage *pMessage,const ChannelInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(MESSAGE_PROPERTY_TuningSpace,Info.Space);
				pMessage->SetPropertyInt(MESSAGE_PROPERTY_Channel,Info.Channel);
				if (Info.ScannedChannel>=0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_ScannedChannel,Info.ScannedChannel);
				if (Info.RemoteControlKeyID>0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_RemoteControlKeyID,Info.RemoteControlKeyID);
				if (!Info.ChannelName.empty())
					pMessage->SetProperty(MESSAGE_PROPERTY_ChannelName,Info.ChannelName.c_str());
				if (!Info.SpaceName.empty())
					pMessage->SetProperty(MESSAGE_PROPERTY_TuningSpaceName,Info.SpaceName.c_str());
				if (Info.ServiceID!=0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceID,Info.ServiceID);
				if (Info.ScannedServiceID!=0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_ScannedServiceID,Info.ScannedServiceID);
				if (Info.NetworkID!=0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_NetworkID,Info.NetworkID);
				if (Info.TransportStreamID!=0)
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_TransportStreamID,Info.TransportStreamID);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,ChannelInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				CMessageProperty::IntType Value;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_TuningSpace,&Value))
					pInfo->Space=(int)Value;
				else
					pInfo->Space=-1;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_Channel,&Value))
					pInfo->Channel=(int)Value;
				else
					pInfo->Channel=-1;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_ScannedChannel,&Value))
					pInfo->ScannedChannel=(int)Value;
				else
					pInfo->ScannedChannel=-1;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_RemoteControlKeyID,&Value))
					pInfo->RemoteControlKeyID=(int)Value;
				else
					pInfo->RemoteControlKeyID=0;

				if (!pMessage->GetProperty(MESSAGE_PROPERTY_ChannelName,&pInfo->ChannelName))
					pInfo->ChannelName.clear();

				if (!pMessage->GetProperty(MESSAGE_PROPERTY_TuningSpaceName,&pInfo->SpaceName))
					pInfo->SpaceName.clear();

				if (pMessage->GetProperty(MESSAGE_PROPERTY_ServiceID,&Value))
					pInfo->ServiceID=(WORD)Value;
				else
					pInfo->ServiceID=0;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_ScannedServiceID,&Value))
					pInfo->ScannedServiceID=(WORD)Value;
				else
					pInfo->ScannedServiceID=0;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_NetworkID,&Value))
					pInfo->NetworkID=(WORD)Value;
				else
					pInfo->NetworkID=0;

				if (pMessage->GetProperty(MESSAGE_PROPERTY_TransportStreamID,&Value))
					pInfo->TransportStreamID=(WORD)Value;
				else
					pInfo->TransportStreamID=0;

				return true;
			}

		}

		namespace GetService
		{

			bool SetResponse(CMessage *pMessage,const ServiceInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetProperty(MESSAGE_PROPERTY_ServiceName,Info.ServiceName);
				pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceID,Info.ServiceID);
				pMessage->SetPropertyInt(MESSAGE_PROPERTY_ServiceType,Info.ServiceType);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,ServiceInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(MESSAGE_PROPERTY_ServiceName,&pInfo->ServiceName))
					pInfo->ServiceName.clear();

				if (!pMessage->GetProperty(MESSAGE_PROPERTY_ServiceID,&Value)
						|| Value<0 || Value>0xFFFF)
					return false;
				pInfo->ServiceID=WORD(Value);

				if (pMessage->GetProperty(MESSAGE_PROPERTY_ServiceType,&Value)) {
					if (Value<0 || Value>0xFF)
						return false;
					pInfo->ServiceType=BYTE(Value);
				} else {
					pInfo->ServiceType=SERVICE_TYPE_INVALID;
				}

				return true;
			}

		}

		namespace GetServiceList
		{

			bool SetResponse(CMessage *pMessage,const ServiceList &List)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"ServiceCount",List.size());

				String Keyword;
				for (size_t i=0;i<List.size();i++) {
					const ServiceInfo &Service=List[i];

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceName);
					pMessage->SetProperty(Keyword.c_str(),Service.ServiceName);

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceID);
					pMessage->SetPropertyInt(Keyword.c_str(),Service.ServiceID);

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceType);
					pMessage->SetPropertyInt(Keyword.c_str(),Service.ServiceType);
				}

				return true;
			}

			bool GetProperties(const CMessage *pMessage,ServiceList *pList)
			{
				if (pList==nullptr)
					return false;

				pList->clear();

				if (pMessage==nullptr)
					return false;

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(L"ServiceCount",&Value) || Value<0)
					return false;

				const size_t ServiceCount=size_t(Value);
				String Keyword;
				for (size_t i=0;i<ServiceCount;i++) {
					ServiceInfo Service;

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceName);
					pMessage->GetProperty(Keyword.c_str(),&Service.ServiceName);

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceID);
					if (!pMessage->GetProperty(Keyword.c_str(),&Value)
							|| Value<0 || Value>0xFFFF)
						return false;
					Service.ServiceID=WORD(Value);

					StringUtility::Format(Keyword,L"Service%u.%s",UINT(i),MESSAGE_PROPERTY_ServiceType);
					if (pMessage->GetProperty(Keyword.c_str(),&Value)) {
						if (Value<0 || Value>0xFF)
							return false;
						Service.ServiceType=BYTE(Value);
					} else {
						Service.ServiceType=0xFF;
					}

					pList->push_back(Service);
				}

				return true;
			}

		}

		static void SetRecordingSettingProperties(CMessage *pMessage,const RecordingSettings &Settings)
		{
			pMessage->SetPropertyInt(L"DirectoryCount",Settings.Directories.size());

			pMessage->SetProperty(MESSAGE_PROPERTY_FileName,Settings.FileName);

			String Keyword;
			for (size_t i=0;i<Settings.Directories.size();i++) {
				StringUtility::Format(Keyword,L"Directory%u",UINT(i));
				pMessage->SetProperty(Keyword.c_str(),Settings.Directories[i]);
			}

			pMessage->SetProperty(MESSAGE_PROPERTY_ServiceSelect,
								  CMessageProperty::IntType(Settings.ServiceSelect));
			pMessage->SetPropertyInt(MESSAGE_PROPERTY_Streams,
									 Settings.Streams);
		}

		static bool GetRecordingSettingProperties(const CMessage *pMessage,RecordingSettings *pSettings)
		{
			CMessageProperty::IntType Value;

			pMessage->GetProperty(MESSAGE_PROPERTY_FileName,&pSettings->FileName);

			if (pMessage->GetProperty(L"DirectoryCount",&Value) && Value>0) {
				const size_t DirectoryCount=(size_t)Value;
				String Keyword,Directory;
				for (size_t i=0;i<DirectoryCount;i++) {
					StringUtility::Format(Keyword,L"Directory%u",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&Directory))
						return false;
					pSettings->Directories.push_back(Directory);
				}
			}

			if (pMessage->GetProperty(MESSAGE_PROPERTY_ServiceSelect,&Value)
					&& Value>=0 && Value<SERVICE_SELECT_TRAILER)
				pSettings->ServiceSelect=ServiceSelectType(Value);

			if (pMessage->GetProperty(MESSAGE_PROPERTY_Streams,&Value))
				pSettings->Streams=DWORD(Value);

			return true;
		}

		namespace StartRecording
		{

			bool InitializeMessage(CMessage *pMessage,const RecordingSettings &Settings)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetName(MESSAGE_StartRecording);

				SetRecordingSettingProperties(pMessage,Settings);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,RecordingSettings *pSettings)
			{
				if (pMessage==nullptr || pSettings==nullptr)
					return false;

				if (!GetRecordingSettingProperties(pMessage,pSettings))
					return false;

				return true;
			}

		}

		namespace GetRecordingInfo
		{

			bool SetResponse(CMessage *pMessage,const RecordingInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetProperty(MESSAGE_PROPERTY_Status,(CMessageProperty::IntType)Info.State);

				if (Info.State!=RECORDING_STATE_NOT_RECORDING) {
					SetRecordingSettingProperties(pMessage,Info.Settings);

					pMessage->SetPropertyInt(L"FilePathCount",Info.FilePaths.size());

					String Keyword;
					for (size_t i=0;i<Info.FilePaths.size();i++) {
						StringUtility::Format(Keyword,L"FilePath%u",UINT(i));
						pMessage->SetProperty(Keyword.c_str(),Info.FilePaths[i]);
					}

					pMessage->SetPropertyInt(MESSAGE_PROPERTY_StartTime,
											 ((ULONGLONG)Info.StartTime.dwHighDateTime<<32) | (ULONGLONG)Info.StartTime.dwLowDateTime);
					pMessage->SetPropertyInt(MESSAGE_PROPERTY_StartTickCount,
											 Info.StartTickCount);
					pMessage->SetPropertyInt(L"CurDirectoryIndex",Info.CurDirectory);
				}

				return true;
			}

			bool GetProperties(const CMessage *pMessage,RecordingInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				pInfo->Settings.FileName.clear();
				pInfo->Settings.Directories.clear();
				pInfo->Settings.ServiceSelect=SERVICE_SELECT_ALL;
				pInfo->Settings.Streams=STREAM_ALL;

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(MESSAGE_PROPERTY_Status,&Value))
					return false;

				pInfo->State=RecordingState(Value);

				GetRecordingSettingProperties(pMessage,&pInfo->Settings);

				if (pMessage->GetProperty(L"FilePathCount",&Value) && Value>0) {
					const size_t FilePathCount=(size_t)Value;
					String Keyword,FilePath;
					for (size_t i=0;i<FilePathCount;i++) {
						StringUtility::Format(Keyword,L"FilePath%u",(UINT)i);
						if (!pMessage->GetProperty(Keyword.c_str(),&FilePath))
							return false;
						pInfo->FilePaths.push_back(FilePath);
					}
				} else {
					pInfo->FilePaths.clear();
				}

				if (pMessage->GetProperty(MESSAGE_PROPERTY_StartTime,&Value)) {
					pInfo->StartTime.dwLowDateTime=(DWORD)(Value&0xFFFFFFFFLL);
					pInfo->StartTime.dwHighDateTime=(DWORD)(Value>>32);
				} else {
					pInfo->StartTime=FILETIME_NULL;
				}

				if (pMessage->GetProperty(MESSAGE_PROPERTY_StartTickCount,&Value))
					pInfo->StartTickCount=Value;
				else
					pInfo->StartTickCount=0;

				if (pMessage->GetProperty(L"CurDirectoryIndex",&Value))
					pInfo->CurDirectory=int(Value);
				else
					pInfo->CurDirectory=0;

				return true;
			}

		}

		namespace GetStreamStatistics
		{

			bool SetResponse(CMessage *pMessage,const StreamStatistics &Statistics)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"SignalLevel",CMessageProperty::IntType(Statistics.SignalLevel*10000.0f));
				pMessage->SetPropertyInt(L"BitRate",Statistics.BitRate);
				pMessage->SetPropertyInt(L"InputPacketCount",Statistics.InputPacketCount);
				pMessage->SetPropertyInt(L"ErrorPacketCount",Statistics.ErrorPacketCount);
				pMessage->SetPropertyInt(L"DiscontinuityCount",Statistics.DiscontinuityCount);
				pMessage->SetPropertyInt(L"ScramblePacketCount",Statistics.ScramblePacketCount);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,StreamStatistics *pStatistics)
			{
				if (pMessage==nullptr || pStatistics==nullptr)
					return false;

				CMessageProperty::IntType Value;

				if (pMessage->GetProperty(L"SignalLevel",&Value))
					pStatistics->SignalLevel=(float)((double)Value/10000.0);
				else
					pStatistics->SignalLevel=0.0f;

				if (pMessage->GetProperty(L"BitRate",&Value))
					pStatistics->BitRate=(DWORD)Value;
				else
					pStatistics->BitRate=0;

				if (pMessage->GetProperty(L"InputPacketCount",&Value))
					pStatistics->InputPacketCount=Value;
				else
					pStatistics->InputPacketCount=0;

				if (pMessage->GetProperty(L"ErrorPacketCount",&Value))
					pStatistics->ErrorPacketCount=Value;
				else
					pStatistics->ErrorPacketCount=0;

				if (pMessage->GetProperty(L"DiscontinuityCount",&Value))
					pStatistics->DiscontinuityCount=Value;
				else
					pStatistics->DiscontinuityCount=0;

				if (pMessage->GetProperty(L"ScramblePacketCount",&Value))
					pStatistics->ScramblePacketCount=Value;
				else
					pStatistics->ScramblePacketCount=0;

				return true;
			}

		}

		namespace GetEventInfo
		{

			bool SetResponse(CMessage *pMessage,const EventInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"EventID",Info.EventID);
				if (Info.StartTime.wYear!=0) {
					FILETIME ft;
					if (::SystemTimeToFileTime(&Info.StartTime,&ft)) {
						pMessage->SetProperty(L"StartTime",
											  ((LONGLONG)ft.dwHighDateTime<<32) | (LONGLONG)ft.dwLowDateTime);
					}
				}
				if (Info.Duration!=0)
					pMessage->SetPropertyInt(L"Duration",Info.Duration);
				pMessage->SetProperty(L"EventName",Info.EventName);
				if (!Info.EventText.empty())
					pMessage->SetProperty(L"EventText",Info.EventText);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,EventInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(L"EventID",&Value))
					return false;
				pInfo->EventID=(WORD)Value;
				::ZeroMemory(&pInfo->StartTime,sizeof(SYSTEMTIME));
				if (pMessage->GetProperty(L"StartTime",&Value)) {
					FILETIME ft;
					SYSTEMTIME st;
					ft.dwLowDateTime=(DWORD)(Value&0xFFFFFFFFLL);
					ft.dwHighDateTime=(DWORD)(Value>>32);
					if (::FileTimeToSystemTime(&ft,&st))
						pInfo->StartTime=st;
				}
				if (pMessage->GetProperty(L"Duration",&Value))
					pInfo->Duration=(DWORD)Value;
				else
					pInfo->Duration=0;
				pMessage->GetProperty(L"EventName",&pInfo->EventName);
				pMessage->GetProperty(L"EventText",&pInfo->EventText);

				return true;
			}

		}

		namespace GetLog
		{

			bool SetResponse(CMessage *pMessage,const LogList &List)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"LogCount",List.size());

				String Keyword;

				for (size_t i=0;i<List.size();i++) {
					const LogInfo &Info=List[i];

					StringUtility::Format(Keyword,L"Log%u.Type",(UINT)i);
					pMessage->SetProperty(Keyword.c_str(),CMessageProperty::IntType(Info.Type));

					StringUtility::Format(Keyword,L"Log%u.Text",(UINT)i);
					pMessage->SetProperty(Keyword.c_str(),Info.Text);

					StringUtility::Format(Keyword,L"Log%u.Time",(UINT)i);
					LARGE_INTEGER Time;
					Time.LowPart=Info.Time.dwLowDateTime;
					Time.HighPart=Info.Time.dwHighDateTime;
					pMessage->SetProperty(Keyword.c_str(),Time.QuadPart);
				}

				return true;
			}

			bool GetProperties(const CMessage *pMessage,LogList *pList)
			{
				if (pMessage==nullptr || pList==nullptr)
					return false;

				pList->clear();

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(L"LogCount",&Value) || Value<0)
					return false;

				const size_t LogCount=(size_t)Value;
				String Keyword;

				pList->resize(LogCount);
				size_t i;
				for (i=0;i<LogCount;i++) {
					LogInfo &Info=(*pList)[i];

					StringUtility::Format(Keyword,L"Log%u.Type",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&Value))
						break;
					Info.Type=LogType(Value);

					StringUtility::Format(Keyword,L"Log%u.Text",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&Info.Text))
						break;

					StringUtility::Format(Keyword,L"Log%u.Time",(UINT)i);
					if (pMessage->GetProperty(Keyword.c_str(),&Value)) {
						Info.Time.dwLowDateTime=(DWORD)(Value&0xFFFFFFFFULL);
						Info.Time.dwHighDateTime=(DWORD)(Value>>32);
					} else {
						Info.Time=FILETIME_NULL;
					}
				}

				if (i<LogCount)
					pList->resize(i);

				return true;
			}

		}

		namespace GetBonDriverChannelList
		{

			bool SetResponse(CMessage *pMessage,const BonDriverTuningSpaceList &TuningSpaceList)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"TuningSpaceCount",TuningSpaceList.size());

				String Keyword;

				for (size_t i=0;i<TuningSpaceList.size();i++) {
					const BonDriverTuningSpaceInfo &SpaceInfo=TuningSpaceList[i];

					StringUtility::Format(Keyword,L"TuningSpace%u.Name",(UINT)i);
					pMessage->SetProperty(Keyword.c_str(),SpaceInfo.Name);
					StringUtility::Format(Keyword,L"TuningSpace%u.ChannelCount",(UINT)i);
					pMessage->SetPropertyInt(Keyword.c_str(),SpaceInfo.ChannelList.size());

					for (size_t j=0;j<SpaceInfo.ChannelList.size();j++) {
						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Name",(UINT)i,(UINT)j);
						pMessage->SetProperty(Keyword.c_str(),SpaceInfo.ChannelList[j]);
					}
				}

				return true;
			}

			bool GetProperties(const CMessage *pMessage,BonDriverTuningSpaceList *pTuningSpaceList)
			{
				if (pMessage==nullptr || pTuningSpaceList==nullptr)
					return false;

				pTuningSpaceList->clear();

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(L"TuningSpaceCount",&Value) || Value<0)
					return false;

				const size_t TuningSpaceCount=(size_t)Value;
				pTuningSpaceList->resize(TuningSpaceCount);

				String Keyword;

				for (size_t i=0;i<TuningSpaceCount;i++) {
					BonDriverTuningSpaceInfo &SpaceInfo=(*pTuningSpaceList)[i];

					StringUtility::Format(Keyword,L"TuningSpace%u.Name",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&SpaceInfo.Name))
						return false;
					StringUtility::Format(Keyword,L"TuningSpace%u.ChannelCount",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&Value) || Value<0)
						return false;

					SpaceInfo.ChannelList.resize((size_t)Value);

					for (size_t j=0;j<SpaceInfo.ChannelList.size();j++) {
						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Name",(UINT)i,(UINT)j);
						if (!pMessage->GetProperty(Keyword.c_str(),&SpaceInfo.ChannelList[j]))
							return false;
					}
				}

				return true;
			}

		}

		namespace GetScannedChannelList
		{

			bool SetResponse(CMessage *pMessage,const CTuningSpaceList &TuningSpaceList)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetPropertyInt(L"TuningSpaceCount",TuningSpaceList.GetSpaceCount());

				String Keyword;

				for (size_t i=0;i<TuningSpaceList.GetSpaceCount();i++) {
					const CTuningSpaceInfo *pSpaceInfo=TuningSpaceList.GetTuningSpaceInfo(i);
					const CChannelList &ChannelList=pSpaceInfo->GetChannelList();

					if (!IsStringEmpty(pSpaceInfo->GetName())) {
						StringUtility::Format(Keyword,L"TuningSpace%u.Name",(UINT)i);
						pMessage->SetProperty(Keyword.c_str(),pSpaceInfo->GetName());
					}
					StringUtility::Format(Keyword,L"TuningSpace%u.ChannelCount",(UINT)i);
					pMessage->SetPropertyInt(Keyword.c_str(),pSpaceInfo->GetChannelCount());

					for (size_t j=0;j<ChannelList.GetChannelCount();j++) {
						const CChannelInfo *pChannelInfo=ChannelList.GetChannelInfo(j);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Space",(UINT)i,(UINT)j);
						pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetSpace());
						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Channel",(UINT)i,(UINT)j);
						pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetChannel());
						if (pChannelInfo->GetRemoteControlKeyID()>0) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.RemoteControlKeyID",(UINT)i,(UINT)j);
							pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetRemoteControlKeyID());
						}
						if (!IsStringEmpty(pChannelInfo->GetName())) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Name",(UINT)i,(UINT)j);
							pMessage->SetProperty(Keyword.c_str(),pChannelInfo->GetName());
						}
						if (pChannelInfo->GetNetworkID()>0) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.NetworkID",(UINT)i,(UINT)j);
							pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetNetworkID());
						}
						if (pChannelInfo->GetTransportStreamID()>0) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.TransportStreamID",(UINT)i,(UINT)j);
							pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetTransportStreamID());
						}
						if (pChannelInfo->GetServiceID()>0) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.ServiceID",(UINT)i,(UINT)j);
							pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetServiceID());
						}
						if (!pChannelInfo->IsEnabled()) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Enabled",(UINT)i,(UINT)j);
							pMessage->SetProperty(Keyword.c_str(),false);
						}
						if (pChannelInfo->GetFrequency()!=0) {
							StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Frequency",(UINT)i,(UINT)j);
							pMessage->SetPropertyInt(Keyword.c_str(),pChannelInfo->GetFrequency());
						}
					}
				}

				return true;
			}

			bool GetProperties(const CMessage *pMessage,CTuningSpaceList *pTuningSpaceList)
			{
				if (pMessage==nullptr || pTuningSpaceList==nullptr)
					return false;

				pTuningSpaceList->Clear();

				CMessageProperty::IntType Value;

				if (!pMessage->GetProperty(L"TuningSpaceCount",&Value) || Value<0)
					return false;

				if (Value==0)
					return true;

				const size_t TuningSpaceCount=(size_t)Value;
				CChannelList AllChannelList;

				String Keyword,Text;

				for (size_t i=0;i<TuningSpaceCount;i++) {
					StringUtility::Format(Keyword,L"TuningSpace%u.ChannelCount",(UINT)i);
					if (!pMessage->GetProperty(Keyword.c_str(),&Value) || Value<0)
						return false;

					const size_t ChannelCount=(size_t)Value;

					for (size_t j=0;j<ChannelCount;j++) {
						CChannelInfo Channel;

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Space",(UINT)i,(UINT)j);
						if (!pMessage->GetProperty(Keyword.c_str(),&Value))
							return false;
						Channel.SetSpace((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Channel",(UINT)i,(UINT)j);
						if (!pMessage->GetProperty(Keyword.c_str(),&Value))
							return false;
						Channel.SetChannel((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.RemoteControlKeyID",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Value))
							Channel.SetRemoteControlKeyID((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Name",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Text))
							Channel.SetName(Text.c_str());

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.NetworkID",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Value))
							Channel.SetNetworkID((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.TransportStreamID",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Value))
							Channel.SetTransportStreamID((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.ServiceID",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Value))
							Channel.SetServiceID((int)Value);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Enabled",(UINT)i,(UINT)j);
						bool fEnabled;
						if (pMessage->GetProperty(Keyword.c_str(),&fEnabled))
							Channel.Enable(fEnabled);

						StringUtility::Format(Keyword,L"TuningSpace%u.Channel%u.Frequency",(UINT)i,(UINT)j);
						if (pMessage->GetProperty(Keyword.c_str(),&Value))
							Channel.SetFrequency((int)Value);

						AllChannelList.AddChannel(Channel);
					}
				}

				if (!pTuningSpaceList->Create(AllChannelList)) {
					return false;
				}

				for (size_t i=0;i<pTuningSpaceList->GetSpaceCount();i++) {
					StringUtility::Format(Keyword,L"TuningSpace%u.Name",(UINT)i);
					if (pMessage->GetProperty(Keyword.c_str(),&Text))
						pTuningSpaceList->GetTuningSpaceInfo(i)->SetName(Text.c_str());
				}

				return true;
			}

		}

		static void SetStreamingInfoProperties(CMessage *pMessage,const StreamingInfo &Info)
		{
			pMessage->SetProperty(L"Media",Info.Media);
			pMessage->SetProperty(L"Protocol",Streaming::GetProtocolText(Info.Address.Protocol));
			pMessage->SetProperty(L"Address",Info.Address.Address);
			pMessage->SetPropertyInt(L"Port",Info.Address.Port);
			pMessage->SetProperty(L"FindUnusedPort",Info.fFindUnusedPort);
			pMessage->SetProperty(MESSAGE_PROPERTY_ServiceSelect,
								  CMessageProperty::IntType(Info.ServiceSelect));
			pMessage->SetPropertyInt(MESSAGE_PROPERTY_Streams,
									 Info.Streams);
		}

		static bool GetStreamingInfoProperties(const CMessage *pMessage,StreamingInfo *pInfo)
		{
			pMessage->GetProperty(L"Media",&pInfo->Media);

			String Protocol;
			if (pMessage->GetProperty(L"Protocol",&Protocol)) {
				pInfo->Address.Protocol=Streaming::ParseProtocolText(Protocol.c_str());
				if (pInfo->Address.Protocol==PROTOCOL_UNDEFINED)
					return false;
			}

			pMessage->GetProperty(L"Address",&pInfo->Address.Address);

			CMessageProperty::IntType Value;

			if (pMessage->GetProperty(L"Port",&Value)) {
				if (Value<0 || Value>0xFFFF)
					return false;
				pInfo->Address.Port=WORD(Value);
			}

			pMessage->GetProperty(L"FindUnusedPort",&pInfo->fFindUnusedPort);

			if (pMessage->GetProperty(MESSAGE_PROPERTY_ServiceSelect,&Value)
					&& Value>=0 && Value<SERVICE_SELECT_TRAILER)
				pInfo->ServiceSelect=ServiceSelectType(Value);

			if (pMessage->GetProperty(MESSAGE_PROPERTY_Streams,&Value))
				pInfo->Streams=DWORD(Value);

			return true;
		}

		namespace StartStreaming
		{

			bool InitializeMessage(CMessage *pMessage,const StreamingInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				pMessage->SetName(MESSAGE_StartStreaming);

				SetStreamingInfoProperties(pMessage,Info);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				if (!GetStreamingInfoProperties(pMessage,pInfo))
					return false;

				return true;
			}

		}

		namespace GetStreamingInfo
		{

			bool SetResponse(CMessage *pMessage,const StreamingInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				SetStreamingInfoProperties(pMessage,Info);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				return GetStreamingInfoProperties(pMessage,pInfo);
			}

		}

		namespace GetStreamingSettings
		{

			bool SetResponse(CMessage *pMessage,const StreamingInfo &Info)
			{
				if (pMessage==nullptr)
					return false;

				SetStreamingInfoProperties(pMessage,Info);

				return true;
			}

			bool GetProperties(const CMessage *pMessage,StreamingInfo *pInfo)
			{
				if (pMessage==nullptr || pInfo==nullptr)
					return false;

				return GetStreamingInfoProperties(pMessage,pInfo);
			}

		}

	}

}
