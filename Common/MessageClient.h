#ifndef TSTASK_MESSAGE_CLIENT_H
#define TSTASK_MESSAGE_CLIENT_H


#include "Message.h"


namespace TSTask
{

	class CMessageClient
	{
	public:
		CMessageClient();
		virtual ~CMessageClient();
		bool SendMessage(const CMessage *pSendMessage,CMessage *pReceiveMessage=nullptr);
		bool SetTimeout(DWORD Timeout);

	protected:
		String m_ServerName;
		DWORD m_Timeout;

		virtual bool Send(const void *pSendData,size_t SendSize,CDataBlock *pReceiveData) = 0;
	};

	class CLocalMessageClient : public CMessageClient
	{
	public:
		CLocalMessageClient();
		~CLocalMessageClient();
		bool SetServer(const String &ServerName);

	private:
		bool Send(const void *pSendData,size_t SendSize,CDataBlock *pReceiveData) override;
		bool ReadPipe(HANDLE hPipe,void *pBuffer,DWORD Size,DWORD *pReadSize,OVERLAPPED *pOverlapped);
		bool ReadPipe(HANDLE hPipe,void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped);
		bool WritePipe(HANDLE hPipe,const void *pBuffer,DWORD Size,DWORD *pWrittenSize,OVERLAPPED *pOverlapped);
		bool WritePipe(HANDLE hPipe,const void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped);
	};

}


#endif
