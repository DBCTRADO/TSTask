#ifndef BONDRIVER_TSTASK_TASK_CLIENT_H
#define BONDRIVER_TSTASK_TASK_CLIENT_H


class CTaskClient
{
public:
	CTaskClient();
	~CTaskClient();
	bool InitializeClient();
	void FinalizeClient();
	bool SetServerTask(TSTask::TaskID TaskID);

protected:
	virtual void OnTaskStarted(TSTask::TaskID TaskID) = 0;
	virtual void OnTaskEnded(TSTask::TaskID TaskID) = 0;

private:
	bool RegisterClient(TSTask::TaskID TaskID);
	bool UnregisterClient(TSTask::TaskID TaskID);
	bool OnTaskStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
	bool OnTaskEnded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);

	TSTask::CTaskIdentity m_TaskIdentity;
	TSTask::CMessageMap m_MessageMap;
	TSTask::CLocalMessageServer m_MessageServer;
	TSTask::TaskID m_ServerTaskID;
	TSTask::CLocalLock m_Lock;
};


#endif
