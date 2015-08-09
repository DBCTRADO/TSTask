#ifndef TSTASK_TASK_UTILITY_H
#define TSTASK_TASK_UTILITY_H


#include <vector>
#include "SharedMemory.h"


namespace TSTask
{

	typedef UINT TaskID;
	enum { INVALID_TASK_ID = 0U };

	enum TaskType
	{
		TASK_TYPE_SERVER,
		TASK_TYPE_CLIENT
	};

	class CTaskIdentity
	{
	public:
		CTaskIdentity();
		~CTaskIdentity();
		bool Initialize(TaskType Type,TaskID ID=INVALID_TASK_ID);
		void Finalize();
		TaskID GetTaskID() const { return m_TaskID; }

	private:
		TaskID m_TaskID;
		CMutex m_Mutex;
	};

	struct TaskSharedInfoHeader
	{
		DWORD Size;
		DWORD Version;
	};

	enum {
		SERVER_TASK_SHARED_INFO_VERSION	= 0,
		CLIENT_TASK_SHARED_INFO_VERSION	= 0
	};

	enum TaskState
	{
		TASK_STATE_STARTING,
		TASK_STATE_RUNNING,
		TASK_STATE_ENDING
	};

	struct TaskInfo
	{
		TaskID TaskID;
		TaskType Type;
		DWORD ProcessID;
		DWORD Version;
		TaskState State;
	};

	struct ServerTaskSharedInfo
	{
		TaskSharedInfoHeader Header;
		TaskInfo Task;
		ULONGLONG StatisticsUpdateCount;
		StreamStatistics Statistics;
		ULONGLONG TotTime;
	};

	struct ClientTaskSharedInfo
	{
		TaskSharedInfoHeader Header;
		TaskInfo Task;
	};

	class CServerTaskSharedInfo
	{
	public:
		CServerTaskSharedInfo();
		~CServerTaskSharedInfo();
		bool Create(TaskID ID,DWORD Version);
		void Close();
		bool SetTaskState(TaskState State);
		bool SetStreamStatistics(const StreamStatistics &Statistics);
		bool SetTotTime(ULONGLONG Time);

	private:
		CSharedMemory m_SharedMemory;
		ServerTaskSharedInfo *m_pInfo;
		DWORD m_LockTimeout;
	};

	class CServerTaskSharedInfoReader
	{
	public:
		CServerTaskSharedInfoReader();
		~CServerTaskSharedInfoReader();
		bool Open(TaskID ID);
		void Close();
		bool GetTaskInfo(TaskInfo *pInfo);
		bool IsTaskEnded();
		bool GetStreamStatistics(StreamStatistics *pStatistics);
		bool GetTotTime(ULONGLONG *pTime);
		bool GetTotTime(SYSTEMTIME *pTime);

	private:
		CSharedMemory m_SharedMemory;
		const ServerTaskSharedInfo *m_pInfo;
		DWORD m_LockTimeout;
	};

	namespace TaskUtility
	{

		typedef std::vector<TaskID> TaskIDList;

		bool TaskIDToString(TaskID ID,String *pString);
		bool IsServerTaskIDValid(TaskID ID);
		bool IsClientTaskIDValid(TaskID ID);
		TaskID GetNextServerTaskID(TaskID ID);
		TaskID GetNextClientTaskID(TaskID ID);
		bool GetServerTaskMutexName(TaskID ID,String *pName);
		bool GetClientTaskMutexName(TaskID ID,String *pName);
		bool IsServerTaskExists(TaskID ID);
		bool IsClientTaskExists(TaskID ID);
		bool GetServerTaskList(TaskIDList *pList);
		bool GetClientTaskList(TaskIDList *pList);
		bool GetServerTaskLocalMessageServerName(TaskID ID,String *pName);
		bool GetClientTaskLocalMessageServerName(TaskID ID,String *pName);
		bool GetServerTaskSharedMemoryName(TaskID ID,String *pName);
		bool GetClientTaskSharedMemoryName(TaskID ID,String *pName);
		bool GetStreamPoolName(TaskID ID,String *pName);

	}

}


#endif
