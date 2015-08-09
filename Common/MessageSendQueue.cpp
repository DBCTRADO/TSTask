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

		OutLog(LOG_VERBOSE,L"���b�Z�[�W���M�X���b�h���J�n���܂��B");

		m_EndEvent.Create();
		m_SendEvent.Create();

		CThread::CFunctorBase *pFunctor=
			new CThread::CFunctor<CMessageSendQueue>(this,&CMessageSendQueue::SendThread);
		if (!m_Thread.Begin(pFunctor)) {
			OutLog(LOG_ERROR,L"���b�Z�[�W���M�X���b�h���J�n�ł��܂���B");
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
			OutLog(LOG_VERBOSE,L"���b�Z�[�W���M�X���b�h���I�����܂��B");
			m_EndEvent.Set();
			if (m_Thread.Wait(10000)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"���b�Z�[�W���M�X���b�h�̉������������ߋ����I�����܂��B");
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

		OutLog(LOG_VERBOSE,L"���b�Z�[�W�L���[�Ƀ��b�Z�[�W��ǉ����܂��B(%s)",
			   Message.GetName());

		CTryBlockLock Lock(m_QueueLock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (!m_Thread.IsOpened()) {
			OutLog(LOG_ERROR,L"���b�Z�[�W���M�X���b�h�����s����Ă��܂���B");
			return false;
		}

		pSender->Refer();
		m_MessageQueue.push_back(MessageInfo(pSender,Message));

		m_SendEvent.Set();

		return true;
	}

	bool CMessageSendQueue::ClearQueue()
	{
		OutLog(LOG_VERBOSE,L"���b�Z�[�W�L���[���N���A���܂��B");

		CTryBlockLock Lock(m_QueueLock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
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
				OutLog(LOG_VERBOSE,L"�I���V�O�i���ɂ�胁�b�Z�[�W���M�X���b�h���I�����܂��B");
				break;
			} else if (Result==WAIT_OBJECT_0+1) {
				OutLog(LOG_VERBOSE,L"���b�Z�[�W���M�X���b�h���烁�b�Z�[�W�𑗐M���܂��B");
				while (true) {
					m_QueueLock.Lock();
					if (m_MessageQueue.empty()) {
						OutLog(LOG_VERBOSE,L"�L���[�ɂ���S�Ẵ��b�Z�[�W�����M����܂����B");
						m_QueueLock.Unlock();
						break;
					}
					MessageInfo Info(m_MessageQueue.front());
					m_MessageQueue.pop_front();
					m_QueueLock.Unlock();

					OutLog(LOG_VERBOSE,L"�L���[���烁�b�Z�[�W�𑗐M���܂��B(%s)",
						   Info.Message.GetName());
					Info.pSender->SendQueueMessage(Info.Message);
					Info.pSender->Release();
				}
			} else {
				OutLog(LOG_WARNING,L"���b�Z�[�W���M�X���b�h�̑ҋ@�ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",Result);
			}
		}

		return 0;
	}

}
