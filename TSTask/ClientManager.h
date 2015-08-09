#ifndef TSTASK_CLIENT_MANAGER_H
#define TSTASK_CLIENT_MANAGER_H


#include "EventHandler.h"


namespace TSTask
{

	class CClientManager : public CEventHandler
	{
	public:
		CClientManager();
		~CClientManager();
		bool Initialize(TaskID ID);
		void Finalize();
		bool AddClient(TaskID ID);
		bool RemoveClient(TaskID ID);

	private:
		bool BroadcastMessage(CMessage &Message);

	// CEventHandler
		void OnBonDriverLoaded(LPCWSTR pszFileName) override;
		void OnBonDriverUnloaded() override;
		void OnTunerOpened() override;
		void OnTunerClosed() override;
		void OnChannelChanged(DWORD Space,DWORD Channel,WORD ServiceID) override;
		void OnServiceChanged(WORD ServiceID) override;
		void OnRecordingStarted(const RecordingInfo &Info) override;
		void OnRecordingStopped() override;
		void OnRecordingFileChanged(LPCWSTR pszFileName) override;
		void OnStreamChanged() override;
		void OnEventChanged() override;
		void OnStreamingStarted(const StreamingInfo &Info) override;
		void OnStreamingStopped() override;

		class CClient : public CMessageSendQueue::CSender
		{
		public:
			CClient(TaskID ID);
			bool Open();
			void EndClient();

		private:
			~CClient();
			bool SendMessage(const CMessage *pSendMessage,CMessage *pReceiveMessage);

		// CMessageSendQueue::CSender
			bool SendQueueMessage(const CMessage &Message) override;

			TaskID m_TaskID;
			CLocalMessageClient m_MessageClient;
		};

		std::map<TaskID,CClient*> m_ClientList;
		CLocalLock m_Lock;
		DWORD m_LockTimeout;
		TaskID m_TaskID;
		CMessageSendQueue m_MessageSendQueue;
	};

}


#endif
