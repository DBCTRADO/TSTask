#ifndef TSTASK_MESSAGE_SERVER_H
#define TSTASK_MESSAGE_SERVER_H


#include <map>
#include "Message.h"


namespace TSTask
{

	class CMessageServer;

	class CMessageReceiver
	{
	public:
		virtual ~CMessageReceiver() {}
		virtual bool OnMessage(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse) = 0;
	};

	class CMessageFunctorBase
	{
	public:
		virtual ~CMessageFunctorBase() {}
		virtual bool operator()(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse) = 0;
	};

	template<typename TClass> class CMessageFunctor : public CMessageFunctorBase
	{
	public:
		typedef bool (TClass::*MessageHandler)(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);

		CMessageFunctor(TClass *pObject,MessageHandler pHandler)
			: m_pObject(pObject)
			, m_pHandler(pHandler)
		{
		}

		bool operator()(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse) override
		{
			return (m_pObject->*m_pHandler)(pServer,pMessage,pResponse);
		}

	private:
		TClass *m_pObject;
		MessageHandler m_pHandler;
	};

	class CMessageMap : public CMessageReceiver
	{
	public:
		CMessageMap();
		~CMessageMap();
		bool SetHandler(LPCWSTR pszMessage,CMessageFunctorBase *pFunctor);
		bool RemoveHandler(LPCWSTR pszMessage);

	private:
		bool OnMessage(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse) override;

		typedef std::map<String,CMessageFunctorBase*> HandlerMap;

		HandlerMap m_HandlerMap;
	};

	class CMessageServer
	{
	public:
		enum WaitMessageResult
		{
			WAIT_RESULT_ERROR,
			WAIT_RESULT_MESSAGE_RECEIVED,
			WAIT_RESULT_QUIT,
			WAIT_RESULT_WINDOW_MESSAGE,
			WAIT_RESULT_TIMEOUT
		};

		CMessageServer(CMessageReceiver *pReceiver);
		virtual ~CMessageServer();
		virtual void Close() = 0;
		bool WaitMessage(DWORD Timeout=INFINITE);
		WaitMessageResult WaitWindowMessage(DWORD Timeout=INFINITE);
		bool Quit();
		void SetQuitSignal();

	protected:
		bool Receive(const CDataBlock *pMessage,CDataBlock *pResponse);

		CMessageReceiver *m_pReceiver;
		CEvent m_MessageEvent;
		CEvent m_QuitEvent;
		bool m_fQuitSignal;
		String m_ServerName;
	};

	class CLocalMessageServer : public CMessageServer
	{
	public:
		CLocalMessageServer(CMessageReceiver *pReceiver);
		~CLocalMessageServer();
		bool Open(LPCWSTR pszName);
		void Close() override;
		bool SetTimeout(DWORD Timeout);

	private:
		unsigned int ServerThread();
		bool ReadPipe(void *pBuffer,DWORD Size,DWORD *pReadSize,OVERLAPPED *pOverlapped);
		bool ReadPipe(void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped);
		bool WritePipe(const void *pBuffer,DWORD Size,DWORD *pWrittenSize,OVERLAPPED *pOverlapped);
		bool WritePipe(const void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped);

		CThread m_Thread;
		HANDLE m_hPipe;
		DWORD m_OutBufferSize;
		DWORD m_InBufferSize;
		CEvent m_EndEvent;
		DWORD m_Timeout;
	};

}


#endif
