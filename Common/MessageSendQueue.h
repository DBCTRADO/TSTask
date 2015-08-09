#ifndef TSTASK_MESSAGE_SEND_QUEUE
#define TSTASK_MESSAGE_SEND_QUEUE


#include <deque>
#include "Message.h"
#include "TaskUtility.h"


namespace TSTask
{

	class CMessageSendQueue
	{
	public:
		class TSTASK_ABSTRACT_CLASS(CSender) : public CReferenceObject
		{
		public:
			virtual ~CSender() {}
			virtual bool SendQueueMessage(const CMessage &Message) = 0;
		};

		CMessageSendQueue();
		~CMessageSendQueue();
		bool Start();
		void End();
		bool EnqueueMessage(CSender *pSender,const CMessage &Message);
		bool ClearQueue();

	private:
		unsigned int SendThread();

		struct MessageInfo
		{
			CSender *pSender;
			CMessage Message;

			MessageInfo(CSender *a_pSender,const CMessage &a_Message)
				: pSender(a_pSender)
				, Message(a_Message)
			{
			}
		};

		std::deque<MessageInfo> m_MessageQueue;
		CLocalLock m_QueueLock;
		DWORD m_LockTimeout;
		CThread m_Thread;
		CEvent m_EndEvent;
		CEvent m_SendEvent;
	};

}


#endif
