#include "stdafx.h"
#include "TSTask.h"
#include "ClientManager.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CClientManager::CClientManager()
		: m_LockTimeout(10000)
		, m_TaskID(INVALID_TASK_ID)
	{
	}

	CClientManager::~CClientManager()
	{
		Finalize();
	}

	bool CClientManager::Initialize(TaskID ID)
	{
		m_TaskID=ID;

		if (!m_MessageSendQueue.Start())
			return false;

		return true;
	}

	void CClientManager::Finalize()
	{
		if (m_TaskID==INVALID_TASK_ID)
			return;

		OutLog(LOG_VERBOSE,L"クライアントマネージャーの終了処理を行います。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return;
		}

		m_MessageSendQueue.End();

		for (auto &e:m_ClientList)
			e.second->EndClient();
		m_ClientList.clear();

		m_TaskID=INVALID_TASK_ID;
	}

	bool CClientManager::AddClient(TaskID ID)
	{
		OutLog(LOG_VERBOSE,L"クライアント%uを追加します。",ID);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return false;
		}

		if (m_ClientList.find(ID)!=m_ClientList.end())
			return false;

		CClient *pClient=new CClient(ID);

		if (!pClient->Open()) {
			pClient->Release();
			return false;
		}

		m_ClientList.insert(std::pair<TaskID,CClient*>(ID,pClient));

		return true;
	}

	bool CClientManager::RemoveClient(TaskID ID)
	{
		OutLog(LOG_VERBOSE,L"クライアント%uを削除します。",ID);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return false;
		}

		auto i=m_ClientList.find(ID);
		if (i==m_ClientList.end())
			return false;

		i->second->EndClient();

		m_ClientList.erase(i);

		return true;
	}

	bool CClientManager::BroadcastMessage(CMessage &Message)
	{
		OutLog(LOG_VERBOSE,L"メッセージ(%s)を登録されたクライアントに送信します。",
			   Message.GetName());

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return false;
		}

		Message.SetPropertyInt(MESSAGE_PROPERTY_TaskID,m_TaskID);

		for (auto &e:m_ClientList) {
			m_MessageSendQueue.EnqueueMessage(e.second,Message);
		}

		return true;
	}

	void CClientManager::OnBonDriverLoaded(LPCWSTR pszFileName)
	{
		CMessage Message(MESSAGE_EVENT_BonDriverLoaded);

		Message.SetProperty(MESSAGE_PROPERTY_FilePath,pszFileName);

		BroadcastMessage(Message);
	}

	void CClientManager::OnBonDriverUnloaded()
	{
		CMessage Message(MESSAGE_EVENT_BonDriverUnloaded);

		BroadcastMessage(Message);
	}

	void CClientManager::OnTunerOpened()
	{
		CMessage Message(MESSAGE_EVENT_TunerOpened);

		BroadcastMessage(Message);
	}

	void CClientManager::OnTunerClosed()
	{
		CMessage Message(MESSAGE_EVENT_TunerClosed);

		BroadcastMessage(Message);
	}

	void CClientManager::OnChannelChanged(DWORD Space,DWORD Channel,WORD ServiceID)
	{
		CMessage Message(MESSAGE_EVENT_ChannelChanged);

		Message.SetPropertyInt(MESSAGE_PROPERTY_TuningSpace,Space);
		Message.SetPropertyInt(MESSAGE_PROPERTY_Channel,Channel);
		if (ServiceID!=0)
			Message.SetPropertyInt(MESSAGE_PROPERTY_ServiceID,ServiceID);

		BroadcastMessage(Message);
	}

	void CClientManager::OnServiceChanged(WORD ServiceID)
	{
		CMessage Message(MESSAGE_EVENT_ServiceChanged);

		Message.SetPropertyInt(MESSAGE_PROPERTY_ServiceID,ServiceID);

		BroadcastMessage(Message);
	}

	void CClientManager::OnRecordingStarted(const RecordingInfo &Info)
	{
		CMessage Message(MESSAGE_EVENT_RecordingStarted);

		String Directory,FileName;
		PathUtility::Split(Info.FilePaths.front(),&Directory,&FileName);

		Message.SetProperty(MESSAGE_PROPERTY_FileName,FileName);
		Message.SetProperty(MESSAGE_PROPERTY_Directory,Directory);
		Message.SetProperty(MESSAGE_PROPERTY_ServiceSelect,CMessageProperty::IntType(Info.Settings.ServiceSelect));

		BroadcastMessage(Message);
	}

	void CClientManager::OnRecordingStopped()
	{
		CMessage Message(MESSAGE_EVENT_RecordingStopped);

		BroadcastMessage(Message);
	}

	void CClientManager::OnRecordingFileChanged(LPCWSTR pszFileName)
	{
		CMessage Message(MESSAGE_EVENT_RecordingFileChanged);

		Message.SetProperty(MESSAGE_PROPERTY_FilePath,pszFileName);

		BroadcastMessage(Message);
	}

	void CClientManager::OnStreamChanged()
	{
		CMessage Message(MESSAGE_EVENT_StreamChanged);

		BroadcastMessage(Message);
	}

	void CClientManager::OnEventChanged()
	{
		CMessage Message(MESSAGE_EVENT_EventChanged);

		BroadcastMessage(Message);
	}

	void CClientManager::OnStreamingStarted(const StreamingInfo &Info)
	{
		CMessage Message(MESSAGE_EVENT_StreamingStarted);

		BroadcastMessage(Message);
	}

	void CClientManager::OnStreamingStopped()
	{
		CMessage Message(MESSAGE_EVENT_StreamingStopped);

		BroadcastMessage(Message);
	}


	CClientManager::CClient::CClient(TaskID ID)
		: m_TaskID(ID)
	{
	}

	CClientManager::CClient::~CClient()
	{
	}

	bool CClientManager::CClient::Open()
	{
		String ServerName;
		if (!TaskUtility::GetClientTaskLocalMessageServerName(m_TaskID,&ServerName)
				|| !m_MessageClient.SetServer(ServerName))
			return false;

		return true;
	}

	void CClientManager::CClient::EndClient()
	{
		m_TaskID=INVALID_TASK_ID;
		Release();
	}

	bool CClientManager::CClient::SendMessage(const CMessage *pSendMessage,CMessage *pReceiveMessage)
	{
		return m_MessageClient.SendMessage(pSendMessage,pReceiveMessage);
	}

	bool CClientManager::CClient::SendQueueMessage(const CMessage &Message)
	{
		if (m_TaskID==INVALID_TASK_ID) {
			OutLog(LOG_VERBOSE,L"タスクが既に存在しないためメッセージ(%s)は送信されません。",
				   Message.GetName());
			return false;
		}

		return SendMessage(&Message,nullptr);
	}

}
