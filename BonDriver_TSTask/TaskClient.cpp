#include "stdafx.h"
#include "../Common/TSTaskCommon.h"
#include "TaskClient.h"
#include "../Common/DebugDef.h"


CTaskClient::CTaskClient()
	: m_MessageServer(&m_MessageMap)
	, m_ServerTaskID(TSTask::INVALID_TASK_ID)
{
}

CTaskClient::~CTaskClient()
{
}

bool CTaskClient::InitializeClient()
{
	TSTask::CBlockLock Lock(m_Lock);

	if (!m_TaskIdentity.Initialize(TSTask::TASK_TYPE_CLIENT)) {
		TSTask::OutLog(TSTask::LOG_ERROR,L"クライアントのタスクIDを割り当てられません。");
		return false;
	}

	m_MessageMap.SetHandler(TSTask::MESSAGE_TaskStarted,
							new TSTask::CMessageFunctor<CTaskClient>(this,&CTaskClient::OnTaskStarted));
	m_MessageMap.SetHandler(TSTask::MESSAGE_TaskEnded,
							new TSTask::CMessageFunctor<CTaskClient>(this,&CTaskClient::OnTaskEnded));

	TSTask::String ServerName;
	TSTask::TaskUtility::GetClientTaskLocalMessageServerName(m_TaskIdentity.GetTaskID(),&ServerName);
	if (!m_MessageServer.Open(ServerName.c_str())) {
		TSTask::OutLog(TSTask::LOG_ERROR,L"ローカルメッセージサーバーを開始できません。");
		m_TaskIdentity.Finalize();
		return false;
	}

	if (m_ServerTaskID!=TSTask::INVALID_TASK_ID)
		RegisterClient(m_ServerTaskID);

	return true;
}

void CTaskClient::FinalizeClient()
{
	TSTask::CBlockLock Lock(m_Lock);

	if (m_ServerTaskID!=TSTask::INVALID_TASK_ID)
		UnregisterClient(m_ServerTaskID);

	m_MessageServer.Close();
	m_TaskIdentity.Finalize();
}

bool CTaskClient::SetServerTask(TSTask::TaskID TaskID)
{
	TSTask::CBlockLock Lock(m_Lock);

	if (m_ServerTaskID==TaskID)
		return true;

	if (m_ServerTaskID!=TSTask::INVALID_TASK_ID)
		UnregisterClient(m_ServerTaskID);
	if (TaskID!=TSTask::INVALID_TASK_ID)
		RegisterClient(TaskID);

	m_ServerTaskID=TaskID;

	return true;
}

bool CTaskClient::RegisterClient(TSTask::TaskID TaskID)
{
	if (m_TaskIdentity.GetTaskID()==TSTask::INVALID_TASK_ID)
		return false;

	TSTask::String ServerName;
	if (!TSTask::TaskUtility::GetServerTaskLocalMessageServerName(TaskID,&ServerName))
		return false;

	TSTask::CLocalMessageClient MessageClient;
	if (!MessageClient.SetServer(ServerName))
		return false;

	TSTask::CMessage Message(TSTask::MESSAGE_RegisterClient);
	Message.SetPropertyInt(TSTask::MESSAGE_PROPERTY_TaskID,m_TaskIdentity.GetTaskID());
	TSTask::CMessage Response;

	return MessageClient.SendMessage(&Message,&Response);
}

bool CTaskClient::UnregisterClient(TSTask::TaskID TaskID)
{
	if (m_TaskIdentity.GetTaskID()==TSTask::INVALID_TASK_ID)
		return false;

	TSTask::String ServerName;
	if (!TSTask::TaskUtility::GetServerTaskLocalMessageServerName(TaskID,&ServerName))
		return false;

	TSTask::CLocalMessageClient MessageClient;
	if (!MessageClient.SetServer(ServerName))
		return false;

	TSTask::CMessage Message(TSTask::MESSAGE_UnregisterClient);
	Message.SetPropertyInt(TSTask::MESSAGE_PROPERTY_TaskID,m_TaskIdentity.GetTaskID());
	TSTask::CMessage Response;

	return MessageClient.SendMessage(&Message,&Response);
}

bool CTaskClient::OnTaskStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
{
	TSTask::TaskID TaskID;

	if (!TSTask::BasicMessage::TaskStarted::GetProperties(pMessage,&TaskID)) {
		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
		return true;
	}

	TSTask::OutLog(TSTask::LOG_INFO,L"タスク(%u)の開始通知を受信しました。",TaskID);

	m_Lock.Lock();
	if (m_ServerTaskID==TaskID)
		RegisterClient(TaskID);
	m_Lock.Unlock();

	OnTaskStarted(TaskID);

	pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

	return true;
}

bool CTaskClient::OnTaskEnded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
{
	TSTask::TaskID TaskID;

	if (!TSTask::BasicMessage::TaskEnded::GetProperties(pMessage,&TaskID)) {
		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
		return true;
	}

	TSTask::OutLog(TSTask::LOG_INFO,L"タスク(%u)の終了通知を受信しました。",TaskID);

	m_Lock.Lock();
	if (m_ServerTaskID==TaskID)
		m_ServerTaskID=TSTask::INVALID_TASK_ID;
	m_Lock.Unlock();

	OnTaskEnded(TaskID);

	pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

	return true;
}
