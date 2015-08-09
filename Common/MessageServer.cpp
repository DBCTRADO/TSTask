#include "stdafx.h"
#include "TSTaskBase.h"
#include "MessageServer.h"
#include "DebugDef.h"


namespace TSTask
{

	CMessageMap::CMessageMap()
	{
	}

	CMessageMap::~CMessageMap()
	{
		for (auto &e:m_HandlerMap)
			delete e.second;
	}

	bool CMessageMap::SetHandler(LPCWSTR pszMessage,CMessageFunctorBase *pFunctor)
	{
		if (IsStringEmpty(pszMessage) || pFunctor==nullptr)
			return false;

		auto Result=m_HandlerMap.insert(std::pair<String,CMessageFunctorBase*>(String(pszMessage),pFunctor));
		if (!Result.second)
			Result.first->second=pFunctor;

		return true;
	}

	bool CMessageMap::RemoveHandler(LPCWSTR pszMessage)
	{
		if (IsStringEmpty(pszMessage))
			return false;

		HandlerMap::iterator i=m_HandlerMap.find(String(pszMessage));
		if (i==m_HandlerMap.end())
			return false;
		m_HandlerMap.erase(i);

		return true;
	}

	bool CMessageMap::OnMessage(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		HandlerMap::iterator i=m_HandlerMap.find(String(pMessage->GetName()));
		if (i!=m_HandlerMap.end()) {
			return (*i->second)(pServer,pMessage,pResponse);
		}
		return false;
	}


	CMessageServer::CMessageServer(CMessageReceiver *pReceiver)
		: m_pReceiver(pReceiver)
		, m_fQuitSignal(false)
	{
		m_MessageEvent.Create();
		m_QuitEvent.Create(nullptr,true);
	}

	CMessageServer::~CMessageServer()
	{
	}

	bool CMessageServer::WaitMessage(DWORD Timeout)
	{
		HANDLE hHandles[2];
		hHandles[0]=m_QuitEvent.GetHandle();
		hHandles[1]=m_MessageEvent.GetHandle();
		DWORD Result=::WaitForMultipleObjects(2,hHandles,FALSE,Timeout);
		if (Result==WAIT_OBJECT_0) {
			OutLog(LOG_VERBOSE,L"���b�Z�[�W�T�[�o�[�̏I���C�x���g���V�O�i����ԂɂȂ�܂����B");
			return false;
		}

		if (m_fQuitSignal)
			m_QuitEvent.Set();

		return true;
	}

	CMessageServer::WaitMessageResult CMessageServer::WaitWindowMessage(DWORD Timeout)
	{
		HANDLE hHandles[2];

		hHandles[0]=m_MessageEvent.GetHandle();
		hHandles[1]=m_QuitEvent.GetHandle();
		DWORD Result=::MsgWaitForMultipleObjects(2,hHandles,FALSE,Timeout,QS_ALLINPUT);
		if (Result==WAIT_OBJECT_0+1) {
			OutLog(LOG_VERBOSE,L"���b�Z�[�W�T�[�o�[�̏I���C�x���g���V�O�i����ԂɂȂ�܂����B");
			return WAIT_RESULT_QUIT;
		}

		if (m_fQuitSignal)
			m_QuitEvent.Set();

		switch (Result) {
		case WAIT_OBJECT_0:
			return WAIT_RESULT_MESSAGE_RECEIVED;
		case WAIT_OBJECT_0+2:
			return WAIT_RESULT_WINDOW_MESSAGE;
		case WAIT_TIMEOUT:
			return WAIT_RESULT_TIMEOUT;
		}

		return WAIT_RESULT_ERROR;
	}

	bool CMessageServer::Quit()
	{
		OutLog(LOG_VERBOSE,L"���b�Z�[�W�T�[�o�[�̏I���C�x���g��ݒ肵�܂��B");
		return m_QuitEvent.Set();
	}

	void CMessageServer::SetQuitSignal()
	{
		OutLog(LOG_VERBOSE,L"���b�Z�[�W�T�[�o�[�̏I���V�O�i����ݒ肵�܂��B");
		m_fQuitSignal=true;
	}

	bool CMessageServer::Receive(const CDataBlock *pMessage,CDataBlock *pResponse)
	{
		CMessage ReceiveMessage,ResponseMessage;

		CMessageTranslator Translator;
		if (!Translator.Parse(&ReceiveMessage,pMessage->GetBuffer(),pMessage->GetSize())) {
			OutLog(LOG_ERROR,L"��M���b�Z�[�W��ϊ��ł��܂���B");
			return false;
		}

		OutLog(LOG_VERBOSE,L"���b�Z�[�W(%s)����M���܂����B",ReceiveMessage.GetName());

		bool fResult=m_pReceiver->OnMessage(this,&ReceiveMessage,&ResponseMessage);

		if (fResult && !ResponseMessage.IsEmpty()) {
			if (IsStringEmpty(ResponseMessage.GetName()))
				ResponseMessage.SetName(L"Response");
			Translator.Format(&ResponseMessage,pResponse);
		}

		m_MessageEvent.Set();

		return fResult;
	}


	CLocalMessageServer::CLocalMessageServer(CMessageReceiver *pReceiver)
		: CMessageServer(pReceiver)
		, m_hPipe(INVALID_HANDLE_VALUE)
		, m_OutBufferSize(MESSAGE_PIPE_RECEIVE_BUFFER_SIZE)
		, m_InBufferSize(MESSAGE_PIPE_SEND_BUFFER_SIZE)
		, m_Timeout(10000)
	{
	}

	CLocalMessageServer::~CLocalMessageServer()
	{
		Close();
	}

	bool CLocalMessageServer::Open(LPCWSTR pszName)
	{
		if (IsStringEmpty(pszName))
			return false;

		if (m_hPipe!=INVALID_HANDLE_VALUE)
			return false;

		OutLog(LOG_VERBOSE,L"���O�t���p�C�v(%s)���쐬���܂��B",pszName);

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		m_hPipe=::CreateNamedPipe(
			pszName,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_BYTE,
			1,
			m_OutBufferSize,
			m_InBufferSize,
			m_Timeout,
			&SecurityAttributes);
		if (m_hPipe==INVALID_HANDLE_VALUE) {
			OutSystemErrorLog(::GetLastError(),L"���O�t���p�C�v(%s)���쐬�ł��܂���B",pszName);
			return false;
		}

		m_EndEvent.Create();

		CThread::CFunctorBase *pFunctor=new CThread::CFunctor<CLocalMessageServer>(this,&CLocalMessageServer::ServerThread);
		if (!m_Thread.Begin(pFunctor)) {
			OutLog(LOG_ERROR,L"�T�[�o�[�X���b�h���J�n�ł��܂���B");
			delete pFunctor;
			Close();
			return false;
		}

		m_ServerName=pszName;

		return true;
	}

	void CLocalMessageServer::Close()
	{
		if (m_Thread.IsOpened()) {
			OutLog(LOG_VERBOSE,L"�T�[�o�[�X���b�h���I�����܂��B");
			m_EndEvent.Set();
			if (m_Thread.Wait(m_Timeout)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"�T�[�o�[�X���b�h�̉������������ߋ����I�����܂��B");
				m_Thread.Terminate();
			}
			m_Thread.Close();
		}

		m_EndEvent.Close();

		if (m_hPipe!=INVALID_HANDLE_VALUE) {
			::FlushFileBuffers(m_hPipe);
			::DisconnectNamedPipe(m_hPipe);
			::CloseHandle(m_hPipe);
			m_hPipe=INVALID_HANDLE_VALUE;
			OutLog(LOG_VERBOSE,L"���O�t���p�C�v(%s)����܂����B",m_ServerName.c_str());
		}

		m_ServerName.clear();
	}

	bool CLocalMessageServer::SetTimeout(DWORD Timeout)
	{
		m_Timeout=Timeout;

		return true;
	}

	unsigned int CLocalMessageServer::ServerThread()
	{
		OVERLAPPED Overlapped;
		::ZeroMemory(&Overlapped,sizeof(Overlapped));
		Overlapped.hEvent=::CreateEvent(nullptr,TRUE,FALSE,nullptr);

		HANDLE hEventList[2];
		hEventList[0]=m_EndEvent.GetHandle();
		hEventList[1]=Overlapped.hEvent;

		DWORD ConnectError=ERROR_SUCCESS;

		while (true) {
			OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̐ڑ���ҋ@���܂��B",m_ServerName.c_str());

			bool fConnected=false,fWait=false;

			if (::ConnectNamedPipe(m_hPipe,&Overlapped)) {
				TRACE(L"ConnectNamedPipe() return nonzero\n");
				fWait=true;
			} else {
				DWORD Error=::GetLastError();
				if (Error==ERROR_PIPE_CONNECTED) {
					TRACE(L"ConnectNamedPipe() ERROR_PIPE_CONNECTED\n");
					fConnected=true;
				} else if (Error==ERROR_IO_PENDING) {
					TRACE(L"ConnectNamedPipe() ERROR_IO_PENDING\n");
					DWORD Result=::WaitForMultipleObjects(2,hEventList,FALSE,INFINITE);
					if (Result==WAIT_OBJECT_0+1) {
						DWORD Transferred;
						if (::GetOverlappedResult(m_hPipe,&Overlapped,&Transferred,FALSE)) {
							fConnected=true;
						} else {
							OutSystemErrorLog(::GetLastError(),
											  L"�p�C�v(%s)�ւ̐ڑ��ҋ@������Ɋ������܂���ł����B",
											  m_ServerName.c_str());
						}
					} else {
						OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̐ڑ��ҋ@���L�����Z�����܂��B",
							   m_ServerName.c_str());
						::CancelIo(m_hPipe);
						if (Result==WAIT_OBJECT_0) {
							OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̐ڑ��ҋ@���I�����܂��B",m_ServerName.c_str());
							break;
						} else {
							OutLog(LOG_WARNING,L"�p�C�v(%s)�ւ̐ڑ��ҋ@�ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",
								   m_ServerName.c_str(),Result);
						}
					}
				} else {
					if (Error!=ERROR_PIPE_LISTENING) {
						if (ConnectError!=Error) {
							OutSystemErrorLog(Error,L"�p�C�v(%s)�ւ̐ڑ��ҋ@�ŃG���[���������܂����B",
											  m_ServerName.c_str());
							ConnectError=Error;
						}
						if (Error==ERROR_NO_DATA) {
							OutLog(LOG_WARNING,L"�p�C�v(%s)�ւ̐ڑ����ؒf���ꂸ�ɑҋ@�ɓ���܂����B",
								   m_ServerName.c_str());
							::DisconnectNamedPipe(m_hPipe);
						}
					}

					fWait=true;
				}
			}

			if (fConnected) {
				ULONG PID;
				if (::GetNamedPipeClientProcessId(m_hPipe,&PID)) {
					OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ɃN���C�A���g���ڑ����܂����B(PID %u)",
						   m_ServerName.c_str(),PID);
				} else {
					OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ɃN���C�A���g���ڑ����܂����B",
						   m_ServerName.c_str());
				}

				OVERLAPPED IoOverlapped;
				::ZeroMemory(&IoOverlapped,sizeof(IoOverlapped));
				IoOverlapped.hEvent=::CreateEvent(nullptr,TRUE,FALSE,nullptr);

				DWORD Size=0;
				CDataBlock ReceiveData,ResponseData;

				if (!ReadPipe(&Size,sizeof(DWORD),&IoOverlapped)) {
					//OutLog(LOG_ERROR,L"�p�C�v(%s)����̓ǂݍ��݂��ł��܂���B",m_ServerName.c_str());
				} else if (Size==0 || Size>MESSAGE_PIPE_MAX_SEND_SIZE) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̎�M�T�C�Y(%lu bytes)���s���ł��B",
						   m_ServerName.c_str(),(unsigned long)Size);
				} else if (!ReceiveData.SetSize(Size)) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̎�M�p������(%lu bytes)���m�ۂł��܂���B",
						   m_ServerName.c_str(),(unsigned long)Size);
				} else {
					if (ReadPipe(ReceiveData.GetBuffer(),Size,&IoOverlapped)) {
						if (!Receive(&ReceiveData,&ResponseData))
							ResponseData.Free();

						Size=(DWORD)ResponseData.GetSize();
						if (WritePipe(&Size,sizeof(DWORD),&IoOverlapped))
							WritePipe(ResponseData.GetBuffer(),Size,&IoOverlapped);
					}
				}

				::CloseHandle(IoOverlapped.hEvent);

				OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̐ڑ����I�����܂��B",m_ServerName.c_str());

				if (!::FlushFileBuffers(m_hPipe)) {
					OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)�̃o�b�t�@���t���b�V���ł��܂���B",
									  m_ServerName.c_str());
				}

				if (!::DisconnectNamedPipe(m_hPipe)) {
					OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)�ւ̐ڑ����I���ł��܂���B",
									  m_ServerName.c_str());
				}
			} else if (fWait) {
				if (m_EndEvent.Wait(100)!=WAIT_TIMEOUT) {
					OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̐ڑ��ҋ@���I�����܂��B",m_ServerName.c_str());
					break;
				}
			}
		}

		::CloseHandle(Overlapped.hEvent);

		return 0;
	}

	bool CLocalMessageServer::ReadPipe(void *pBuffer,DWORD Size,DWORD *pReadSize,OVERLAPPED *pOverlapped)
	{
		*pReadSize=0;
		DWORD Read=0;
		if (::ReadFile(m_hPipe,pBuffer,Size,&Read,pOverlapped)) {
			*pReadSize=Read;
		} else {
			DWORD Result=::GetLastError();
			if (Result!=ERROR_IO_PENDING) {
				OutSystemErrorLog(Result,L"�p�C�v(%s)����̓ǂݍ��݂��ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̓ǂݍ��ݑ҂����^�C���A�E�g���܂����B(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̓ǂݍ��ݑ҂��ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"�p�C�v(%s)����̓ǂݍ��݂��L�����Z�����܂��B",
					   m_ServerName.c_str());
				::CancelIo(m_hPipe);
				OutLog(LOG_VERBOSE,L"�p�C�v(%s)����̓ǂݍ��݃L�����Z���̊�����ҋ@���܂��B",
					   m_ServerName.c_str());
				::GetOverlappedResult(m_hPipe,pOverlapped,&Read,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(m_hPipe,pOverlapped,&Read,TRUE)) {
				OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)����̓ǂݍ��݌��ʂ��擾�ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			::ResetEvent(pOverlapped->hEvent);

			*pReadSize=Read;
		}

		return true;
	}

	bool CLocalMessageServer::ReadPipe(void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped)
	{
		DWORD TotalReadSize=0;

		while (TotalReadSize<Size) {
			DWORD Read;
			if (!ReadPipe(static_cast<BYTE*>(pBuffer)+TotalReadSize,
						  min(Size-TotalReadSize,m_InBufferSize),&Read,pOverlapped))
				return false;
			TotalReadSize+=Read;
		}

		return true;
	}

	bool CLocalMessageServer::WritePipe(const void *pBuffer,DWORD Size,DWORD *pWrittenSize,OVERLAPPED *pOverlapped)
	{
		*pWrittenSize=0;
		DWORD Write=0;
		if (::WriteFile(m_hPipe,pBuffer,Size,&Write,pOverlapped)) {
			*pWrittenSize=Write;
		} else {
			DWORD Result=::GetLastError();
			if (Result!=ERROR_IO_PENDING) {
				OutSystemErrorLog(Result,L"�p�C�v(%s)�̉����̏����o�����ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)�̉����̏����o���҂����^�C���A�E�g���܂����B(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"�p�C�v(%s)�̉����̏����o���҂��ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"�p�C�v(%s)�̉����̏����o�����L�����Z�����܂��B",
					   m_ServerName.c_str());
				::CancelIo(m_hPipe);
				OutLog(LOG_VERBOSE,L"�p�C�v(%s)�̉����̏����o���L�����Z���̊�����ҋ@���܂��B",
					   m_ServerName.c_str());
				::GetOverlappedResult(m_hPipe,pOverlapped,&Write,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(m_hPipe,pOverlapped,&Write,TRUE)) {
				OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)�̉����̏����o�����ʂ��擾�ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			::ResetEvent(pOverlapped->hEvent);

			*pWrittenSize=Write;
		}

		return true;
	}

	bool CLocalMessageServer::WritePipe(const void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped)
	{
		DWORD TotalWroteSize=0;

		while (TotalWroteSize<Size) {
			DWORD Write;
			if (!WritePipe(static_cast<const BYTE*>(pBuffer)+TotalWroteSize,
						   min(Size-TotalWroteSize,m_OutBufferSize),&Write,pOverlapped))
				return false;
			TotalWroteSize+=Write;
		}

		return true;
	}

}
