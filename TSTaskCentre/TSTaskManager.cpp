#include "stdafx.h"
#include <psapi.h>
#include "TSTaskCentre.h"
#include "TSTaskManager.h"
#include "../Common/DebugDef.h"

#pragma comment(lib, "psapi.lib")


namespace TSTaskCentre
{

	CTSTaskManager::CTSTaskManager()
		: m_LockTimeout(10000)
	{
	}

	CTSTaskManager::~CTSTaskManager()
	{
		for (auto &e:m_TaskList) {
			e.second->UnregisterClient();
			delete e.second;
		}
	}

	int CTSTaskManager::GetTaskCount() const
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return 0;
		}

		return (int)m_TaskList.size();
	}

	bool CTSTaskManager::GetTaskList(TSTask::TaskUtility::TaskIDList *pList) const
	{
		if (pList==nullptr)
			return false;

		pList->clear();

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		pList->reserve(m_TaskList.size());

		for (const auto &e:m_TaskList)
			pList->push_back(e.first);

		return true;
	}

	bool CTSTaskManager::AddTask(TSTask::TaskID ClientTaskID,TSTask::TaskID ID)
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_TaskList.find(ID)!=m_TaskList.end()) {
			TRACE(L"CTSTaskManager::AddTask() : タスク(%u)は既に登録されています。\n",UINT(ID));
			return false;
		}

		CTask *pTask=new CTask(ID);

		if (!pTask->Open(ClientTaskID)) {
			delete pTask;
			return false;
		}

		m_TaskList.insert(std::pair<TSTask::TaskID,CTask*>(ID,pTask));

		return true;
	}

	bool CTSTaskManager::RemoveTask(TSTask::TaskID ID)
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end()) {
			TRACE(L"CTSTaskManager::RemoveTask() : タスク(%u)が登録されていません。\n",UINT(ID));
			return false;
		}

		delete i->second;

		m_TaskList.erase(i);

		return true;
	}

	bool CTSTaskManager::GetTaskInfo(TSTask::TaskID ID,TSTask::TaskInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		return i->second->GetTaskInfo(pInfo);
	}

	bool CTSTaskManager::GetTaskProcessInfo(TSTask::TaskID ID,TaskProcessInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		const unsigned int Mask=pInfo->Mask;

		::ZeroMemory(pInfo,sizeof(TaskProcessInfo));

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		DWORD ProcessID=i->second->GetProcessID();
		if (ProcessID==0) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"タスク(%u)のプロセスIDを取得できません。",UINT(ID));
			return false;
		}

		pInfo->ProcessID=ProcessID;

		bool fLimited=false;
		HANDLE hProcess=::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE,ProcessID);
		if (hProcess==nullptr) {
			if (::GetLastError()==ERROR_ACCESS_DENIED)
				hProcess=::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,ProcessID);
			if (hProcess==nullptr) {
				TSTask::OutSystemErrorLog(::GetLastError(),L"プロセス(%u)を開けません。",ProcessID);
				return false;
			}
			fLimited=true;
			TSTask::OutLog(TSTask::LOG_WARNING,L"権限がないためプロセス(%u)の情報を全て取得できません。",ProcessID);
		}

		unsigned int ValidMask=0;

		if ((Mask & PROCESS_INFO_PROCESS_TIMES)!=0) {
			FILETIME ExitTime;
			if (::GetProcessTimes(hProcess,&pInfo->CreationTime,&ExitTime,
								  reinterpret_cast<LPFILETIME>(&pInfo->KernelTime),
								  reinterpret_cast<LPFILETIME>(&pInfo->UserTime)))
				ValidMask|=PROCESS_INFO_PROCESS_TIMES;
		}

		if ((Mask & PROCESS_INFO_CYCLE_TIME)!=0) {
			if (::QueryProcessCycleTime(hProcess,&pInfo->CycleTime))
				ValidMask|=PROCESS_INFO_CYCLE_TIME;
		}

		if ((Mask & PROCESS_INFO_IO_COUNTERS)!=0) {
			if (::GetProcessIoCounters(hProcess,&pInfo->IOCounters))
				ValidMask|=PROCESS_INFO_IO_COUNTERS;
		}

		if ((Mask & PROCESS_INFO_HANDLE_COUNT)!=0) {
			if (::GetProcessHandleCount(hProcess,&pInfo->HandleCount))
				ValidMask|=PROCESS_INFO_HANDLE_COUNT;
		}

		if ((Mask & PROCESS_INFO_GDI_OBJECTS)!=0 && !fLimited) {
			pInfo->GdiObjects=::GetGuiResources(hProcess,GR_GDIOBJECTS);
			ValidMask|=PROCESS_INFO_GDI_OBJECTS;
		}

		if ((Mask & PROCESS_INFO_USER_OBJECTS)!=0 && !fLimited) {
			pInfo->UserObjects=::GetGuiResources(hProcess,GR_USEROBJECTS);
			ValidMask|=PROCESS_INFO_USER_OBJECTS;
		}

		if ((Mask & PROCESS_INFO_MEMORY)!=0 && !fLimited) {
			auto pK32GetProcessMemoryInfo=
				static_cast<decltype(::GetProcessMemoryInfo)*>(
					TSTask::GetModuleFunction(L"kernel32.dll","K32GetProcessMemoryInfo"));
			PROCESS_MEMORY_COUNTERS_EX MemCounters;
			MemCounters.cb=sizeof(PROCESS_MEMORY_COUNTERS_EX);
			BOOL fResult;
			if (pK32GetProcessMemoryInfo!=nullptr)
				fResult=pK32GetProcessMemoryInfo(hProcess,(PROCESS_MEMORY_COUNTERS*)&MemCounters,MemCounters.cb);
			else
				fResult=::GetProcessMemoryInfo(hProcess,(PROCESS_MEMORY_COUNTERS*)&MemCounters,MemCounters.cb);
			if (fResult) {
				pInfo->Memory.PageFaultCount=MemCounters.PageFaultCount;
				pInfo->Memory.WorkingSetSize=MemCounters.WorkingSetSize;
				pInfo->Memory.PeakWorkingSetSize=MemCounters.PeakWorkingSetSize;
				pInfo->Memory.PageFileUsage=MemCounters.PagefileUsage;
				pInfo->Memory.PeakPageFileUsage=MemCounters.PeakPagefileUsage;
				pInfo->Memory.PrivateUsage=MemCounters.PrivateUsage;
				ValidMask|=PROCESS_INFO_MEMORY;
			}
		}

		if ((Mask & PROCESS_INFO_WOW64)!=0) {
			BOOL fWOW64;
			if (::IsWow64Process(hProcess,&fWOW64)) {
				pInfo->fWOW64=fWOW64!=FALSE;
				ValidMask|=PROCESS_INFO_WOW64;
			}
		}

		pInfo->Mask=ValidMask;

		::CloseHandle(hProcess);

		return true;
	}

	bool CTSTaskManager::GetTaskProcessPath(TSTask::TaskID ID,TSTask::String *pPath)
	{
		if (pPath==nullptr)
			return false;

		pPath->clear();

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		DWORD ProcessID=i->second->GetProcessID();
		if (ProcessID==0) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"タスク(%u)のプロセスIDを取得できません。",UINT(ID));
			return false;
		}

		HANDLE hProcess=::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,ProcessID);
		if (hProcess==nullptr) {
			TSTask::OutSystemErrorLog(::GetLastError(),L"プロセス(%u)を開けません。",ProcessID);
			return false;
		}

		bool fOK=false;
		WCHAR szPath[MAX_PATH];
		DWORD Length=_countof(szPath);
		if (::QueryFullProcessImageNameW(hProcess,0,szPath,&Length)) {
			*pPath=szPath;
			fOK=true;
		}

		::CloseHandle(hProcess);

		return fOK;
	}

	bool CTSTaskManager::SendMessage(TSTask::TaskID ID,
									 const TSTask::CMessage *pSendMessage,TSTask::CMessage *pReceiveMessage)
	{
		if (pSendMessage==nullptr)
			return false;

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);

		if (i==m_TaskList.end()) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージ(%s)の送信先のID(%u)が不正です。",
							pSendMessage->GetName(),ID);
			return false;
		}

		return i->second->SendMessage(pSendMessage,pReceiveMessage);
	}

	bool CTSTaskManager::BroadcastMessage(const TSTask::CMessage *pSendMessage)
	{
		if (pSendMessage==nullptr)
			return false;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"全てのタスクにメッセージ(%s)を送信します。",
					   pSendMessage->GetName());

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		for (auto &e:m_TaskList)
			e.second->SendMessage(pSendMessage,nullptr);

		return true;
	}

	bool CTSTaskManager::EndTask(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_EndTask);
	}

	bool CTSTaskManager::TerminateTask(TSTask::TaskID ID)
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		DWORD ProcessID=i->second->GetProcessID();

		if (ProcessID==0) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"タスク(%u)のプロセスIDを取得できません。",UINT(ID));
			return false;
		}

		HANDLE hProcess=::OpenProcess(PROCESS_TERMINATE,FALSE,ProcessID);
		if (hProcess==nullptr) {
			DWORD Error=::GetLastError();
			TSTask::OutSystemErrorLog(Error,L"プロセス(%u)を開けません。",ProcessID);
			if (Error==ERROR_INVALID_PARAMETER)
				return true;
			return false;
		}

		TSTask::OutLog(TSTask::LOG_INFO,L"タスク(%u)のプロセス(%u)を強制終了します。",UINT(ID),ProcessID);

		if (!::TerminateProcess(hProcess,-1)) {
			TSTask::OutSystemErrorLog(::GetLastError(),L"プロセス(%u)を強制終了できません。",ProcessID);
			::CloseHandle(hProcess);
			return false;
		}

		::CloseHandle(hProcess);

		delete i->second;
		m_TaskList.erase(i);

		return true;
	}

	bool CTSTaskManager::GetStreamStatistics(TSTask::TaskID ID,TSTask::StreamStatistics *pStatistics)
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		return i->second->GetStreamStatistics(pStatistics);
	}

	bool CTSTaskManager::GetTotTime(TSTask::TaskID ID,SYSTEMTIME *pTime)
	{
		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		return i->second->GetTotTime(pTime);
	}

	bool CTSTaskManager::LoadBonDriver(TSTask::TaskID ID,LPCWSTR pszFilePath)
	{
		if (TSTask::IsStringEmpty(pszFilePath))
			return false;

		TSTask::CMessage Message;

		TSTask::BasicMessage::LoadBonDriver::InitializeMessage(&Message,pszFilePath);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::UnloadBonDriver(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_UnloadBonDriver);
	}

	bool CTSTaskManager::GetBonDriver(TSTask::TaskID ID,TSTask::String *pFilePath,TSTask::String *pTunerName,bool fUpdate)
	{
		if (pFilePath==nullptr && pTunerName==nullptr)
			return false;

		if (pFilePath!=nullptr)
			pFilePath->clear();
		if (pTunerName!=nullptr)
			pTunerName->clear();

		TSTask::CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=m_TaskList.find(ID);
		if (i==m_TaskList.end())
			return false;

		return i->second->GetBonDriver(pFilePath,pTunerName,fUpdate);
	}

	bool CTSTaskManager::OpenTuner(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_OpenTuner);
	}

	bool CTSTaskManager::CloseTuner(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_CloseTuner);
	}

	bool CTSTaskManager::SetChannelByIndex(TSTask::TaskID ID,DWORD Space,DWORD Channel,WORD ServiceID,bool fForce)
	{
		TSTask::CMessage Message;
		TSTask::SetChannelInfo Info;

		Info.Type=TSTask::SetChannelInfo::TYPE_INDEX;
		Info.Index.Space=Space;
		Info.Index.Channel=Channel;
		Info.Index.ServiceID=ServiceID;
		Info.fForceChange=fForce;

		TSTask::BasicMessage::SetChannel::InitializeMessage(&Message,Info);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::SetChannelByScanned(TSTask::TaskID ID,int Space,int Channel,WORD ServiceID,bool fForce)
	{
		TSTask::CMessage Message;
		TSTask::SetChannelInfo Info;

		Info.Type=TSTask::SetChannelInfo::TYPE_SCANNED;
		Info.Scanned.Space=Space;
		Info.Scanned.Channel=Channel;
		Info.Scanned.ServiceID=ServiceID;
		Info.fForceChange=fForce;

		TSTask::BasicMessage::SetChannel::InitializeMessage(&Message,Info);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::SetChannelByIDs(TSTask::TaskID ID,WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fForce)
	{
		TSTask::CMessage Message;
		TSTask::SetChannelInfo Info;

		Info.Type=TSTask::SetChannelInfo::TYPE_IDS;
		Info.IDs.NetworkID=NetworkID;
		Info.IDs.TransportStreamID=TransportStreamID;
		Info.IDs.ServiceID=ServiceID;
		Info.fForceChange=fForce;

		TSTask::BasicMessage::SetChannel::InitializeMessage(&Message,Info);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::GetChannel(TSTask::TaskID ID,TSTask::ChannelInfo *pChannel)
	{
		if (pChannel==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetChannel),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetChannel::GetProperties(&Response,pChannel)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::SetService(TSTask::TaskID ID,WORD ServiceID)
	{
		TSTask::CMessage Message(TSTask::MESSAGE_SetService);

		Message.SetPropertyInt(TSTask::MESSAGE_PROPERTY_ServiceID,ServiceID);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::GetService(TSTask::TaskID ID,TSTask::ServiceInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetService),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetService::GetProperties(&Response,pInfo)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetServiceList(TSTask::TaskID ID,TSTask::ServiceList *pList)
	{
		if (pList==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetServiceList),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetServiceList::GetProperties(&Response,pList)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::StartRecording(TSTask::TaskID ID,const TSTask::RecordingSettings *pSettings)
	{
		TSTask::CMessage Message(TSTask::MESSAGE_StartRecording);

		if (pSettings!=nullptr)
			TSTask::BasicMessage::StartRecording::InitializeMessage(&Message,*pSettings);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::StopRecording(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_StopRecording);
	}

	bool CTSTaskManager::ChangeRecordingFile(TSTask::TaskID ID,LPCWSTR pszFileName)
	{
		if (TSTask::IsStringEmpty(pszFileName))
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_ChangeRecordingFile);

		Message.SetProperty(TSTask::MESSAGE_PROPERTY_FilePath,pszFileName);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::GetRecordingInfo(TSTask::TaskID ID,TSTask::RecordingInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetRecordingInfo),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetRecordingInfo::GetProperties(&Response,pInfo)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::ResetErrorStatistics(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_ResetErrorStatistics);
	}

	bool CTSTaskManager::GetEventInfo(TSTask::TaskID ID,TSTask::EventInfo *pInfo,bool fNext)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetEventInfo),Response;

		if (fNext)
			Message.SetProperty(L"Next",fNext);

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetEventInfo::GetProperties(&Response,pInfo)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetLog(TSTask::TaskID ID,TSTask::LogList *pList)
	{
		if (pList==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetLog),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetLog::GetProperties(&Response,pList)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetBonDriverChannelList(TSTask::TaskID ID,TSTask::BonDriverTuningSpaceList *pSpaceList)
	{
		if (pSpaceList==nullptr)
			return false;

		pSpaceList->clear();

		TSTask::CMessage Message(TSTask::MESSAGE_GetBonDriverChannelList),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetBonDriverChannelList::GetProperties(&Response,pSpaceList)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"チャンネルのリストを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetScannedChannelList(TSTask::TaskID ID,TSTask::CTuningSpaceList *pSpaceList)
	{
		if (pSpaceList==nullptr)
			return false;

		pSpaceList->Clear();

		TSTask::CMessage Message(TSTask::MESSAGE_GetScannedChannelList),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetScannedChannelList::GetProperties(&Response,pSpaceList)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"チャンネルのリストを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetTvRockInfo(TSTask::TaskID ID,int *pDeviceID)
	{
		if (pDeviceID==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetTvRockInfo),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		TSTask::CMessageProperty::IntType Value;
		if (Response.GetProperty(L"DeviceID",&Value))
			*pDeviceID=(int)Value;
		else
			*pDeviceID=-1;

		return true;
	}

	bool CTSTaskManager::StartStreaming(TSTask::TaskID ID,const TSTask::StreamingInfo *pInfo)
	{
		TSTask::CMessage Message(TSTask::MESSAGE_StartStreaming);

		if (pInfo!=nullptr)
			TSTask::BasicMessage::StartStreaming::InitializeMessage(&Message,*pInfo);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::StopStreaming(TSTask::TaskID ID)
	{
		return SendBasicMessage(ID,TSTask::MESSAGE_StopStreaming);
	}

	bool CTSTaskManager::GetStreamingInfo(TSTask::TaskID ID,TSTask::StreamingInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetStreamingInfo),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetStreamingInfo::GetProperties(&Response,pInfo)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetStreamingSettings(TSTask::TaskID ID,TSTask::StreamingInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetStreamingSettings),Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!TSTask::BasicMessage::GetStreamingSettings::GetProperties(&Response,pInfo)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージのプロパティを取得できません。");
			return false;
		}

		return true;
	}

	bool CTSTaskManager::GetSetting(TSTask::TaskID ID,LPCWSTR pszName,TSTask::String *pValue)
	{
		if (pValue==nullptr)
			return false;

		pValue->clear();

		if (TSTask::IsStringEmpty(pszName))
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_GetSetting),Response;
		Message.SetProperty(TSTask::MESSAGE_PROPERTY_Name,pszName);

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Value,pValue))
			return false;

		return true;
	}

	bool CTSTaskManager::SendIniSettingsChanged(TSTask::TaskID ID,const TSTask::String &FilePath)
	{
		if (FilePath.empty())
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_EVENT_IniSettingsChanged);
		Message.SetProperty(TSTask::MESSAGE_PROPERTY_FilePath,FilePath);

		return SendBasicMessage(ID,Message);
	}

	bool CTSTaskManager::SendBasicMessage(TSTask::TaskID ID,const TSTask::CMessage &Message)
	{
		TSTask::CMessage Response;

		if (!SendMessage(ID,&Message,&Response))
			return false;

		TSTask::String Result;
		if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
			return false;
		}

		if (Result!=TSTask::MESSAGE_RESULT_OK)
			return false;

		return true;
	}

	bool CTSTaskManager::SendBasicMessage(TSTask::TaskID ID,LPCWSTR pszName)
	{
		TSTask::CMessage Message(pszName);

		return SendBasicMessage(ID,Message);
	}

	void CTSTaskManager::OutTimeoutErrorLog() const
	{
		TSTask::OutLog(TSTask::LOG_ERROR,L"タイムアウトしました。(%u ms)",m_LockTimeout);
	}


	CTSTaskManager::CTask::CTask(TSTask::TaskID ID)
		: m_TaskID(ID)
		, m_ClientTaskID(TSTask::INVALID_TASK_ID)
		, m_fBonDriverValid(false)
	{
		m_TaskInfo.TaskID=TSTask::INVALID_TASK_ID;
	}

	CTSTaskManager::CTask::~CTask()
	{
	}

	bool CTSTaskManager::CTask::Open(TSTask::TaskID ClientTaskID)
	{
		TSTask::String ServerName;
		if (!TSTask::TaskUtility::GetServerTaskLocalMessageServerName(m_TaskID,&ServerName)
				|| !m_MessageClient.SetServer(ServerName))
			return false;

		TSTask::CMessage Message(TSTask::MESSAGE_RegisterClient);
		Message.SetPropertyInt(TSTask::MESSAGE_PROPERTY_TaskID,ClientTaskID);
		TSTask::CMessage Response;
		if (m_MessageClient.SendMessage(&Message,&Response))
			m_ClientTaskID=ClientTaskID;

		if (!m_SharedInfoReader.Open(m_TaskID))
			return false;

		if (!m_SharedInfoReader.GetTaskInfo(&m_TaskInfo)) {
			m_SharedInfoReader.Close();
			return false;
		}

		return true;
	}

	bool CTSTaskManager::CTask::UnregisterClient()
	{
		if (m_ClientTaskID!=TSTask::INVALID_TASK_ID) {
			TSTask::CMessage Message(TSTask::MESSAGE_UnregisterClient);
			Message.SetPropertyInt(TSTask::MESSAGE_PROPERTY_TaskID,m_ClientTaskID);
			TSTask::CMessage Response;
			if (!m_MessageClient.SendMessage(&Message,&Response))
				return false;
		}

		return true;
	}

	bool CTSTaskManager::CTask::GetTaskInfo(TSTask::TaskInfo *pInfo) const
	{
		if (m_TaskInfo.TaskID==TSTask::INVALID_TASK_ID)
			return false;
		*pInfo=m_TaskInfo;
		return true;
	}

	DWORD CTSTaskManager::CTask::GetProcessID()
	{
		TSTask::TaskInfo TaskInfo;

		if (!m_SharedInfoReader.GetTaskInfo(&TaskInfo))
			return 0;

		return m_TaskInfo.ProcessID;
	}

	bool CTSTaskManager::CTask::SendMessage(const TSTask::CMessage *pSendMessage,TSTask::CMessage *pReceiveMessage)
	{
		return m_MessageClient.SendMessage(pSendMessage,pReceiveMessage);
	}

	bool CTSTaskManager::CTask::GetStreamStatistics(TSTask::StreamStatistics *pStatistics)
	{
		return m_SharedInfoReader.GetStreamStatistics(pStatistics);
	}

	bool CTSTaskManager::CTask::GetTotTime(SYSTEMTIME *pTime)
	{
		return m_SharedInfoReader.GetTotTime(pTime);
	}

	bool CTSTaskManager::CTask::GetBonDriver(TSTask::String *pFilePath,TSTask::String *pTunerName,bool fUpdate)
	{
		if (!m_fBonDriverValid || fUpdate) {
			TSTask::CMessage Message(TSTask::MESSAGE_GetBonDriver),Response;

			if (!SendMessage(&Message,&Response))
				return false;

			TSTask::String Result;
			if (!Response.GetProperty(TSTask::MESSAGE_PROPERTY_Result,&Result)) {
				TSTask::OutLog(TSTask::LOG_ERROR,L"メッセージの結果を取得できません。");
				return false;
			}
			if (Result!=TSTask::MESSAGE_RESULT_OK)
				return false;

			m_BonDriverFilePath.clear();
			Response.GetProperty(TSTask::MESSAGE_PROPERTY_FilePath,&m_BonDriverFilePath);
			m_TunerName.clear();
			Response.GetProperty(TSTask::MESSAGE_PROPERTY_Name,&m_TunerName);

			if (!m_BonDriverFilePath.empty())
				m_fBonDriverValid=true;
		}

		if (pFilePath!=nullptr)
			*pFilePath=m_BonDriverFilePath;
		if (pTunerName!=nullptr)
			*pTunerName=m_TunerName;

		return true;
	}

}
