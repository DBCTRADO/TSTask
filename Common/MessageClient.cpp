#include "stdafx.h"
#include "TSTaskBase.h"
#include "MessageClient.h"
#include "DebugDef.h"


namespace TSTask
{

	CMessageClient::CMessageClient()
		: m_Timeout(10000)
	{
	}

	CMessageClient::~CMessageClient()
	{
	}

	bool CMessageClient::SendMessage(const CMessage *pSendMessage,CMessage *pReceiveMessage)
	{
		if (pSendMessage==nullptr)
			return false;

		OutLog(LOG_VERBOSE,L"���b�Z�[�W(%s)�𑗐M���܂��B",pSendMessage->GetName());

		CMessageTranslator Translator;
		CDataBlock SendData,ReceiveData;

		if (!Translator.Format(pSendMessage,&SendData)) {
			OutLog(LOG_ERROR,L"���b�Z�[�W��ϊ��ł��܂���B");
			return false;
		}

		if (SendData.GetSize()==0) {
			OutLog(LOG_ERROR,L"���M����f�[�^����ł��B");
			return false;
		}

		if (!Send(SendData.GetBuffer(),SendData.GetSize(),&ReceiveData))
			return false;

		if (pReceiveMessage!=nullptr) {
			if (ReceiveData.GetSize()>0) {
				if (!Translator.Parse(pReceiveMessage,ReceiveData.GetBuffer(),ReceiveData.GetSize())) {
					OutLog(LOG_ERROR,L"���b�Z�[�W�̃��X�|���X�̉�͂ŃG���[���������܂����B");
				}
			} else {
				pReceiveMessage->Clear();
			}
		}

		return true;
	}

	bool CMessageClient::SetTimeout(DWORD Timeout)
	{
		m_Timeout=Timeout;

		return true;
	}


	CLocalMessageClient::CLocalMessageClient()
	{
	}

	CLocalMessageClient::~CLocalMessageClient()
	{
	}

	bool CLocalMessageClient::SetServer(const String &ServerName)
	{
		m_ServerName=ServerName;

		return true;
	}

	bool CLocalMessageClient::Send(const void *pSendData,size_t SendSize,CDataBlock *pReceiveData)
	{
		if (m_ServerName.empty()) {
			OutLog(LOG_ERROR,L"���M��T�[�o�[���w�肳��Ă��܂���B");
			return false;
		}

		if (SendSize>MESSAGE_PIPE_MAX_SEND_SIZE) {
			OutLog(LOG_ERROR,L"�p�C�v(%s)�֑��M����T�C�Y(%Iu bytes)���傫�����܂��B",
				   m_ServerName.c_str(),SendSize);
			return false;
		}

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		HANDLE hPipe;

		const DWORD StartTime=::GetTickCount();

		do {
			OutLog(LOG_VERBOSE,L"�p�C�v(%s)�֐ڑ��\�ɂȂ�܂ōő� %u ms �ҋ@���܂��B",
				   m_ServerName.c_str(),m_Timeout);

			if (!::WaitNamedPipe(m_ServerName.c_str(),m_Timeout)) {
				DWORD Error=::GetLastError();
				if (Error==ERROR_SEM_TIMEOUT) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)���ҋ@����(%u ms)���ɐڑ��\�ɂȂ�܂���B",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutSystemErrorLog(Error,L"�p�C�v(%s)�̐ڑ��ҋ@�ŃG���[���������܂����B",
									  m_ServerName.c_str());
				}
				return false;
			}

			OutLog(LOG_VERBOSE,L"�p�C�v(%s)���J���܂��B",m_ServerName.c_str());

			hPipe=::CreateFile(m_ServerName.c_str(),
							   GENERIC_READ | GENERIC_WRITE,
							   0,
							   &SecurityAttributes,
							   OPEN_EXISTING,
							   FILE_FLAG_OVERLAPPED,
							   nullptr);
			if (hPipe!=INVALID_HANDLE_VALUE) {
				OutLog(LOG_VERBOSE,L"�p�C�v(%s)���J���܂����B(%p)",m_ServerName.c_str(),hPipe);
				break;
			}

			DWORD Error=::GetLastError();
			if (Error!=ERROR_PIPE_BUSY) {
				OutSystemErrorLog(Error,L"�p�C�v(%s)���J���܂���B",m_ServerName.c_str());
				return false;
			}
		} while (::GetTickCount()-StartTime<m_Timeout);

		if (hPipe==INVALID_HANDLE_VALUE) {
			OutLog(LOG_ERROR,L"�p�C�v(%s)�����p�\�ɂȂ�܂���B",m_ServerName.c_str());
			return false;
		}

		OVERLAPPED Overlapped;
		::ZeroMemory(&Overlapped,sizeof(Overlapped));
		Overlapped.hEvent=::CreateEvent(nullptr,TRUE,FALSE,nullptr);

		try {
			DWORD Size=(DWORD)SendSize;
			if (!WritePipe(hPipe,&Size,sizeof(DWORD),&Overlapped)) {
				//OutLog(LOG_ERROR,L"�p�C�v(%s)�ւ̏����o�����ł��܂���B",m_ServerName.c_str());
				throw __LINE__;
			}
			if (!WritePipe(hPipe,pSendData,Size,&Overlapped)) {
				//OutLog(LOG_ERROR,L"�p�C�v(%s)�ւ̏����o�����ł��܂���B",m_ServerName.c_str());
				throw __LINE__;
			}

			DWORD ReceiveSize=0;
			if (!ReadPipe(hPipe,&ReceiveSize,sizeof(DWORD),&Overlapped)) {
				//OutLog(LOG_ERROR,L"�p�C�v(%s)����̓ǂݍ��݂��ł��܂���B",m_ServerName.c_str());
				throw __LINE__;
			}
			if (ReceiveSize>MESSAGE_PIPE_MAX_SEND_SIZE) {
				OutLog(LOG_ERROR,L"�p�C�v(%s)����̉����T�C�Y(%lu bytes)���傫�����܂��B",
					   m_ServerName.c_str(),(unsigned long)ReceiveSize);
				throw __LINE__;
			}
			if (!pReceiveData->SetSize(ReceiveSize)) {
				OutLog(LOG_ERROR,L"�p�C�v(%s)����̉�����M�p�̃�����(%lu bytes)���m�ۂł��܂���B",
					   m_ServerName.c_str(),(unsigned long)ReceiveSize);
				throw __LINE__;
			}
			if (!ReadPipe(hPipe,pReceiveData->GetBuffer(),ReceiveSize,&Overlapped)) {
				//OutLog(LOG_ERROR,L"�p�C�v(%s)����̉����̓ǂݍ��݂��ł��܂���B",m_ServerName.c_str());
				throw __LINE__;
			}
		} catch (...) {
			::CloseHandle(hPipe);
			::CloseHandle(Overlapped.hEvent);
			return false;
		}

		::CloseHandle(hPipe);
		::CloseHandle(Overlapped.hEvent);

		OutLog(LOG_VERBOSE,L"�p�C�v(%s)����܂����B(%p)",m_ServerName.c_str(),hPipe);

		return true;
	}

	bool CLocalMessageClient::ReadPipe(HANDLE hPipe,void *pBuffer,DWORD Size,DWORD *pReadSize,OVERLAPPED *pOverlapped)
	{
		*pReadSize=0;
		DWORD Read=0;
		if (::ReadFile(hPipe,pBuffer,Size,&Read,pOverlapped)) {
			*pReadSize=Read;
		} else {
			DWORD Result=::GetLastError();
			if (Result!=ERROR_IO_PENDING) {
				OutSystemErrorLog(Result,L"�p�C�v(%s)����̉����̓ǂݍ��݂��ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̉����̓ǂݍ��ݑ҂����^�C���A�E�g���܂����B(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"�p�C�v(%s)����̉����̓ǂݍ��ݑ҂��ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"�p�C�v(%s)����̉����̓ǂݍ��݂��L�����Z�����܂��B",
					   m_ServerName.c_str());
				::CancelIo(hPipe);
				OutLog(LOG_VERBOSE,L"�p�C�v(%s)����̉����̓ǂݍ��݃L�����Z���̊�����ҋ@���܂��B",
					   m_ServerName.c_str());
				::GetOverlappedResult(hPipe,pOverlapped,&Read,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(hPipe,pOverlapped,&Read,FALSE)) {
				OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)����̉����̓ǂݍ��݌��ʂ��擾�ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			::ResetEvent(pOverlapped->hEvent);

			*pReadSize=Read;
		}

		return true;
	}

	bool CLocalMessageClient::ReadPipe(HANDLE hPipe,void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped)
	{
		DWORD TotalReadSize=0;

		while (TotalReadSize<Size) {
			DWORD Read;
			if (!ReadPipe(hPipe,
						  static_cast<BYTE*>(pBuffer)+TotalReadSize,
						  min(Size-TotalReadSize,MESSAGE_PIPE_RECEIVE_BUFFER_SIZE),
						  &Read,
						  pOverlapped))
				return false;
			TotalReadSize+=Read;
		}

		return true;
	}

	bool CLocalMessageClient::WritePipe(HANDLE hPipe,const void *pBuffer,DWORD Size,DWORD *pWrittenSize,OVERLAPPED *pOverlapped)
	{
		*pWrittenSize=0;
		DWORD Write=0;
		if (::WriteFile(hPipe,pBuffer,Size,&Write,pOverlapped)) {
			*pWrittenSize=Write;
		} else {
			DWORD Result=::GetLastError();
			if (Result!=ERROR_IO_PENDING) {
				OutSystemErrorLog(Result,L"�p�C�v(%s)�ւ̏����o�����ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"�p�C�v(%s)�ւ̏����o���҂����^�C���A�E�g���܂����B(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"�p�C�v(%s)�ւ̏����o���҂��ŗ\�����Ȃ����ʂ��Ԃ���܂����B(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̏����o�����L�����Z�����܂��B",
					   m_ServerName.c_str());
				::CancelIo(hPipe);
				OutLog(LOG_VERBOSE,L"�p�C�v(%s)�ւ̏����o���L�����Z���̊�����ҋ@���܂��B",
					   m_ServerName.c_str());
				::GetOverlappedResult(hPipe,pOverlapped,&Write,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(hPipe,pOverlapped,&Write,FALSE)) {
				OutSystemErrorLog(::GetLastError(),L"�p�C�v(%s)�ւ̏����o�����ʂ��擾�ł��܂���B",
								  m_ServerName.c_str());
				return false;
			}

			::ResetEvent(pOverlapped->hEvent);

			*pWrittenSize=Write;
		}

		return true;
	}

	bool CLocalMessageClient::WritePipe(HANDLE hPipe,const void *pBuffer,DWORD Size,OVERLAPPED *pOverlapped)
	{
		DWORD TotalWroteSize=0;

		while (TotalWroteSize<Size) {
			DWORD Write;
			if (!WritePipe(hPipe,
						   static_cast<const BYTE*>(pBuffer)+TotalWroteSize,
						   min(Size-TotalWroteSize,MESSAGE_PIPE_SEND_BUFFER_SIZE),
						   &Write,
						   pOverlapped))
				return false;
			TotalWroteSize+=Write;
		}

		return true;
	}

}
