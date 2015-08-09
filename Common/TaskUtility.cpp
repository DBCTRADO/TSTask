#include "stdafx.h"
#include "TSTaskBase.h"
#include "TaskUtility.h"
#include "DebugDef.h"


#define TSTASK_SERVER_MUTEX_NAME			L"Global\\TSTask_Server_Mutex_"
#define TSTASK_CLIENT_MUTEX_NAME			L"Global\\TSTask_Client_Mutex_"
#define TSTASK_SERVER_PIPE_NAME				L"\\\\.\\pipe\\TSTask_Server_Pipe_"
#define TSTASK_CLIENT_PIPE_NAME				L"\\\\.\\pipe\\TSTask_Client_Pipe_"
#define TSTASK_SERVER_SHARED_MEMORY_NAME	L"TSTask_Server_SharedMemory_"
#define TSTASK_CLIENT_SHARED_MEMORY_NAME	L"TSTask_Client_SharedMemory_"
#define TSTASK_STREAM_POOL_NAME				L"TSTask_StreamPool_"


namespace TSTask
{

	CTaskIdentity::CTaskIdentity()
		: m_TaskID(INVALID_TASK_ID)
	{
	}

	CTaskIdentity::~CTaskIdentity()
	{
		Finalize();
	}

	bool CTaskIdentity::Initialize(TaskType Type,TaskID ID)
	{
		if (Type!=TASK_TYPE_SERVER && Type!=TASK_TYPE_CLIENT)
			return false;

		if (m_TaskID!=INVALID_TASK_ID) {
			return ID==INVALID_TASK_ID || m_TaskID==ID;
		}

		OutLog(LOG_VERBOSE,L"%sタスクのIDを割り当てます。",Type==TASK_TYPE_SERVER?L"サーバー":L"クライアント");

		if (ID==INVALID_TASK_ID) {
			while (true) {
				if (Type==TASK_TYPE_SERVER)
					ID=TaskUtility::GetNextServerTaskID(ID);
				else
					ID=TaskUtility::GetNextClientTaskID(ID);
				if (ID==INVALID_TASK_ID) {
					OutLog(LOG_ERROR,L"利用可能なタスクIDが見付かりません。");
					return false;
				}

				String Name;
				if (Type==TASK_TYPE_SERVER)
					TaskUtility::GetServerTaskMutexName(ID,&Name);
				else
					TaskUtility::GetClientTaskMutexName(ID,&Name);
				if (m_Mutex.Create(Name.c_str())) {
					if (!m_Mutex.IsAlreadyExists()) {
						OutLog(LOG_VERBOSE,L"タスクのMutexを作成しました。(\"%s\")",Name.c_str());
						break;
					}
					m_Mutex.Close();
				}
			}
		} else {
			String Name;
			bool fResult;
			if (Type==TASK_TYPE_SERVER)
				fResult=TaskUtility::GetServerTaskMutexName(ID,&Name);
			else
				fResult=TaskUtility::GetClientTaskMutexName(ID,&Name);
			if (!fResult) {
				OutLog(LOG_ERROR,L"要求されたタスクID(%u)が不正です。",ID);
				return false;
			}
			if (!m_Mutex.Create(Name.c_str())) {
				OutLog(LOG_ERROR,L"タスクのMutexを作成できません。(\"%s\")",Name.c_str());
				return false;
			}
			if (m_Mutex.IsAlreadyExists()) {
				OutLog(LOG_ERROR,L"タスクのMutexが既に存在します。(\"%s\")",Name.c_str());
				m_Mutex.Close();
				return false;
			}
			OutLog(LOG_VERBOSE,L"タスクのMutexを作成しました。(\"%s\")",Name.c_str());
		}

		OutLog(LOG_INFO,L"タスクのIDを割り当てました。(%u)",ID);

		m_TaskID=ID;

		return true;
	}

	void CTaskIdentity::Finalize()
	{
		m_Mutex.Close();
		m_TaskID=INVALID_TASK_ID;
	}


	CServerTaskSharedInfo::CServerTaskSharedInfo()
		: m_pInfo(nullptr)
		, m_LockTimeout(10000)
	{
	}

	CServerTaskSharedInfo::~CServerTaskSharedInfo()
	{
		Close();
	}

	bool CServerTaskSharedInfo::Create(TaskID ID,DWORD Version)
	{
		OutLog(LOG_VERBOSE,L"サーバータスクの共有情報を作成します。(%u : %d.%d.%d)",
			   ID,GetVersionMajor(Version),GetVersionMinor(Version),GetVersionRevision(Version));

		String Name;

		if (!TaskUtility::GetServerTaskSharedMemoryName(ID,&Name))
			return false;

		bool fExists;
		if (!m_SharedMemory.Create(Name.c_str(),sizeof(ServerTaskSharedInfo),
								   PAGE_READWRITE,&fExists)) {
			OutLog(LOG_ERROR,L"共有メモリ(%s)を作成できません。",Name.c_str());
			return false;
		}

		if (fExists) {
			OutLog(LOG_ERROR,L"共有メモリ(%s)は既に作成されています。",Name.c_str());
			m_SharedMemory.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"共有メモリ(%s)を作成しました。",Name.c_str());

		m_pInfo=static_cast<ServerTaskSharedInfo*>(m_SharedMemory.Map());
		if (m_pInfo==nullptr) {
			OutLog(LOG_ERROR,L"共有メモリ(%s)をマップできません。",Name.c_str());
			m_SharedMemory.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"共有メモリ(%s)をマップしました。(%p)",Name.c_str(),m_pInfo);

		::ZeroMemory(m_pInfo,sizeof(ServerTaskSharedInfo));
		m_pInfo->Header.Size=sizeof(ServerTaskSharedInfo);
		m_pInfo->Header.Version=SERVER_TASK_SHARED_INFO_VERSION;
		m_pInfo->Task.TaskID=ID;
		m_pInfo->Task.Type=TASK_TYPE_SERVER;
		m_pInfo->Task.ProcessID=::GetCurrentProcessId();
		m_pInfo->Task.Version=Version;
		m_pInfo->Task.State=TASK_STATE_STARTING;
		m_pInfo->Statistics.SignalLevel=0.0f;

		return true;
	}

	void CServerTaskSharedInfo::Close()
	{
		if (m_pInfo!=nullptr) {
			m_SharedMemory.Lock(m_LockTimeout);
			m_pInfo->Task.ProcessID=0;
			::ZeroMemory(&m_pInfo->Statistics,sizeof(m_pInfo->Statistics));
			m_pInfo->Statistics.SignalLevel=0.0f;
			m_SharedMemory.Unlock();

			m_SharedMemory.Unmap(m_pInfo);
			m_pInfo=nullptr;
		}

		m_SharedMemory.Close();
	}

	bool CServerTaskSharedInfo::SetTaskState(TaskState State)
	{
		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		m_pInfo->Task.State=State;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CServerTaskSharedInfo::SetStreamStatistics(const StreamStatistics &Statistics)
	{
		if (m_pInfo==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		m_pInfo->StatisticsUpdateCount++;
		m_pInfo->Statistics=Statistics;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CServerTaskSharedInfo::SetTotTime(ULONGLONG Time)
	{
		if (m_pInfo==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		m_pInfo->TotTime=Time;

		m_SharedMemory.Unlock();

		return true;
	}


	CServerTaskSharedInfoReader::CServerTaskSharedInfoReader()
		: m_pInfo(nullptr)
		, m_LockTimeout(10000)
	{
	}

	CServerTaskSharedInfoReader::~CServerTaskSharedInfoReader()
	{
		Close();
	}

	bool CServerTaskSharedInfoReader::Open(TaskID ID)
	{
		OutLog(LOG_VERBOSE,L"サーバータスクの共有情報を開きます。(%u)",ID);

		String Name;

		if (!TaskUtility::GetServerTaskSharedMemoryName(ID,&Name))
			return false;

		if (!m_SharedMemory.Open(Name.c_str(),FILE_MAP_READ)) {
			OutLog(LOG_ERROR,L"共有メモリ(%s)を開けません。",Name.c_str());
			return false;
		}

		OutLog(LOG_VERBOSE,L"共有メモリ(%s)を開きました。",Name.c_str());

		m_pInfo=static_cast<const ServerTaskSharedInfo*>(m_SharedMemory.Map(FILE_MAP_READ));
		if (m_pInfo==nullptr) {
			OutLog(LOG_ERROR,L"共有メモリ(%s)をマップできません。",Name.c_str());
			m_SharedMemory.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"共有メモリ(%s)をマップしました。(%p)",Name.c_str(),m_pInfo);

		if (m_pInfo->Header.Size!=sizeof(ServerTaskSharedInfo)
				|| m_pInfo->Header.Version!=SERVER_TASK_SHARED_INFO_VERSION
				|| m_pInfo->Task.TaskID!=ID
				|| m_pInfo->Task.ProcessID==0) {
			OutLog(LOG_ERROR,L"共有情報が不正です。(Version %u / ID %u / PID %u)",
				   m_pInfo->Header.Version,m_pInfo->Task.TaskID,m_pInfo->Task.ProcessID);
			Close();
			return false;
		}

		if (m_pInfo->Task.State==TASK_STATE_ENDING) {
			OutLog(LOG_WARNING,L"タスクが終了処理中です。");
			Close();
			return false;
		}

		return true;
	}

	void CServerTaskSharedInfoReader::Close()
	{
		if (m_pInfo!=nullptr) {
			m_SharedMemory.Unmap((void*)m_pInfo);
			m_pInfo=nullptr;
		}

		m_SharedMemory.Close();
	}

	bool CServerTaskSharedInfoReader::GetTaskInfo(TaskInfo *pInfo)
	{
		if (m_pInfo==nullptr || pInfo==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		*pInfo=m_pInfo->Task;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CServerTaskSharedInfoReader::IsTaskEnded()
	{
		if (m_pInfo==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		bool fEnded=m_pInfo->Task.ProcessID==0;

		m_SharedMemory.Unlock();

		return fEnded;
	}

	bool CServerTaskSharedInfoReader::GetStreamStatistics(StreamStatistics *pStatistics)
	{
		if (m_pInfo==nullptr || pStatistics==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		*pStatistics=m_pInfo->Statistics;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CServerTaskSharedInfoReader::GetTotTime(ULONGLONG *pTime)
	{
		if (m_pInfo==nullptr || pTime==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_LockTimeout))
			return false;

		*pTime=m_pInfo->TotTime;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CServerTaskSharedInfoReader::GetTotTime(SYSTEMTIME *pTime)
	{
		ULONGLONG Time;

		if (!GetTotTime(&Time) || Time==0)
			return false;

		FILETIME ft;
		ft.dwLowDateTime=DWORD(Time&0xFFFFFFFFULL);
		ft.dwHighDateTime=DWORD(Time>>32);

		return ::FileTimeToSystemTime(&ft,pTime)!=FALSE;
	}


	namespace TaskUtility
	{

		bool TaskIDToString(TaskID ID,String *pString)
		{
			if (pString==nullptr)
				return false;

			if (ID==INVALID_TASK_ID) {
				pString->clear();
				return false;
			}

			StringUtility::Format(*pString,L"%u",ID);

			return true;
		}

		bool IsServerTaskIDValid(TaskID ID)
		{
			return ID!=INVALID_TASK_ID && ID<=MAX_SERVER_TASKS;
		}

		bool IsClientTaskIDValid(TaskID ID)
		{
			return ID!=INVALID_TASK_ID && ID<=MAX_CLIENT_TASKS;
		}

		TaskID GetNextServerTaskID(TaskID ID)
		{
			if (ID>=MAX_SERVER_TASKS)
				return INVALID_TASK_ID;
			return ID+1;
		}

		TaskID GetNextClientTaskID(TaskID ID)
		{
			if (ID>=MAX_CLIENT_TASKS)
				return INVALID_TASK_ID;
			return ID+1;
		}

		bool GetServerTaskMutexName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_SERVER_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_SERVER_MUTEX_NAME L"%u",ID);
			return true;
		}

		bool GetClientTaskMutexName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_CLIENT_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_CLIENT_MUTEX_NAME L"%u",ID);
			return true;
		}

		bool IsServerTaskExists(TaskID ID)
		{
			String Name;
			if (!GetServerTaskMutexName(ID,&Name))
				return false;

			CMutex Mutex;
			if (!Mutex.Open(Name.c_str()))
				return false;

			return true;
		}

		bool IsClientTaskExists(TaskID ID)
		{
			String Name;
			if (!GetClientTaskMutexName(ID,&Name))
				return false;

			CMutex Mutex;
			if (!Mutex.Open(Name.c_str()))
				return false;

			return true;
		}

		bool GetServerTaskList(TaskIDList *pList)
		{
			if (pList==nullptr)
				return false;

			OutLog(LOG_VERBOSE,L"サーバータスクを列挙します。");

			pList->clear();

			TaskID ID=INVALID_TASK_ID;
			while ((ID=GetNextServerTaskID(ID))!=INVALID_TASK_ID) {
				if (IsServerTaskExists(ID)) {
					OutLog(LOG_VERBOSE,L"→サーバー%uが見付かりました。",ID);
					pList->push_back(ID);
				}
			}

			return true;
		}

		bool GetClientTaskList(TaskIDList *pList)
		{
			if (pList==nullptr)
				return false;

			OutLog(LOG_VERBOSE,L"クライアントタスクを列挙します。");

			pList->clear();

			TaskID ID=INVALID_TASK_ID;
			while ((ID=GetNextClientTaskID(ID))!=INVALID_TASK_ID) {
				if (IsClientTaskExists(ID)) {
					OutLog(LOG_VERBOSE,L"→クライアント%uが見付かりました。",ID);
					pList->push_back(ID);
				}
			}

			return true;
		}

		bool GetServerTaskLocalMessageServerName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_SERVER_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_SERVER_PIPE_NAME L"%u",ID);
			return true;
		}

		bool GetClientTaskLocalMessageServerName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_CLIENT_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_CLIENT_PIPE_NAME L"%u",ID);
			return true;
		}

		bool GetServerTaskSharedMemoryName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_SERVER_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_SERVER_SHARED_MEMORY_NAME L"%u",ID);
			return true;
		}

		bool GetClientTaskSharedMemoryName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_CLIENT_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_CLIENT_SHARED_MEMORY_NAME L"%u",ID);
			return true;
		}

		bool GetStreamPoolName(TaskID ID,String *pName)
		{
			if (ID==INVALID_TASK_ID || ID>MAX_SERVER_TASKS || pName==nullptr)
				return false;
			StringUtility::Format(*pName,TSTASK_STREAM_POOL_NAME L"%u",ID);
			return true;
		}

	}

}
