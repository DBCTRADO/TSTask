#include "stdafx.h"
#include "TSTaskBase.h"
#include "MessageSendQueue.h"
#include "DebugDef.h"


namespace TSTask
{

	CMessageSendQueue::CMessageSendQueue()
		: m_LockTimeout(5000)
	{
	}

	CMessageSendQueue::~CMessageSendQueue()
	{
		End();
	}

	bool CMessageSendQueue::Start()
	{
		if (m_Thread.IsOpened())
			return true;

		OutLog(LOG_VERBOSE,L"メッセージ送信スレッドを開始します。");

		m_EndEvent.Create();
		m_SendEvent.Create();

		CThread::CFunctorBase *pFunctor=
			new CThread::CFunctor<CMessageSendQueue>(this,&CMessageSendQueue::SendThread);
		if (!m_Thread.Begin(pFunctor)) {
			OutLog(LOG_ERROR,L"メッセージ送信スレッドを開始できません。");
			delete pFunctor;
			End();
			return false;
		}

		return true;
	}

	void CMessageSendQueue::End()
	{
		ClearQueue();

		if (m_Thread.IsOpened()) {
			OutLog(LOG_VERBOSE,L"メッセージ送信スレッドを終了します。");
			m_EndEvent.Set();
			if (m_Thread.Wait(10000)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"メッセージ送信スレッドの応答が無いため強制終了します。");
				m_Thread.Terminate();
			}
			m_Thread.Close();
		}

		m_EndEvent.Close();
		m_SendEvent.Close();

		for (auto &e:m_MessageQueue)
			e.pSender->Release();
		m_MessageQueue.clear();
	}

	bool CMessageSendQueue::EnqueueMessage(CSender *pSender,const CMessage &Message)
	{
		if (pSender==nullptr)
			return false;

		OutLog(LOG_VERBOSE,L"メッセージキューにメッセージを追加します。(%s)",
			   Message.GetName());

		CTryBlockLock Lock(m_QueueLock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return false;
		}

		if (!m_Thread.IsOpened()) {
			OutLog(LOG_ERROR,L"メッセージ送信スレッドが実行されていません。");
			return false;
		}

		pSender->Refer();
		m_MessageQueue.push_back(MessageInfo(pSender,Message));

		m_SendEvent.Set();

		return true;
	}

	bool CMessageSendQueue::ClearQueue()
	{
		OutLog(LOG_VERBOSE,L"メッセージキューをクリアします。");

		CTryBlockLock Lock(m_QueueLock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"タイムアウトしました。");
			return false;
		}

		for (auto &e:m_MessageQueue)
			e.pSender->Release();

		m_MessageQueue.clear();

		return true;
	}

	unsigned int CMessageSendQueue::SendThread()
	{
		HANDLE EventList[2];
		EventList[0]=m_EndEvent.GetHandle();
		EventList[1]=m_SendEvent.GetHandle();

		while (true) {
			DWORD Result=::WaitForMultipleObjects(2,EventList,FALSE,INFINITE);
			if (Result==WAIT_OBJECT_0) {
				OutLog(LOG_VERBOSE,L"終了シグナルによりメッセージ送信スレッドを終了します。");
				break;
			} else if (Result==WAIT_OBJECT_0+1) {
				OutLog(LOG_VERBOSE,L"メッセージ送信スレッドからメッセージを送信します。");
				while (true) {
					m_QueueLock.Lock();
					if (m_MessageQueue.empty()) {
						OutLog(LOG_VERBOSE,L"キューにある全てのメッセージが送信されました。");
						m_QueueLock.Unlock();
						break;
					}
					MessageInfo Info(m_MessageQueue.front());
					m_MessageQueue.pop_front();
					m_QueueLock.Unlock();

					OutLog(LOG_VERBOSE,L"キューからメッセージを送信します。(%s)",
						   Info.Message.GetName());
					Info.pSender->SendQueueMessage(Info.Message);
					Info.pSender->Release();
				}
			} else {
				OutLog(LOG_WARNING,L"メッセージ送信スレッドの待機で予期しない結果が返されました。(0x%x)",Result);
			}
		}

		return 0;
	}

}
