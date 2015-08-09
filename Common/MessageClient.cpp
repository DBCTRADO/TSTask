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

		OutLog(LOG_VERBOSE,L"メッセージ(%s)を送信します。",pSendMessage->GetName());

		CMessageTranslator Translator;
		CDataBlock SendData,ReceiveData;

		if (!Translator.Format(pSendMessage,&SendData)) {
			OutLog(LOG_ERROR,L"メッセージを変換できません。");
			return false;
		}

		if (SendData.GetSize()==0) {
			OutLog(LOG_ERROR,L"送信するデータが空です。");
			return false;
		}

		if (!Send(SendData.GetBuffer(),SendData.GetSize(),&ReceiveData))
			return false;

		if (pReceiveMessage!=nullptr) {
			if (ReceiveData.GetSize()>0) {
				if (!Translator.Parse(pReceiveMessage,ReceiveData.GetBuffer(),ReceiveData.GetSize())) {
					OutLog(LOG_ERROR,L"メッセージのレスポンスの解析でエラーが発生しました。");
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
			OutLog(LOG_ERROR,L"送信先サーバーが指定されていません。");
			return false;
		}

		if (SendSize>MESSAGE_PIPE_MAX_SEND_SIZE) {
			OutLog(LOG_ERROR,L"パイプ(%s)へ送信するサイズ(%Iu bytes)が大きすぎます。",
				   m_ServerName.c_str(),SendSize);
			return false;
		}

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		HANDLE hPipe;

		const DWORD StartTime=::GetTickCount();

		do {
			OutLog(LOG_VERBOSE,L"パイプ(%s)へ接続可能になるまで最大 %u ms 待機します。",
				   m_ServerName.c_str(),m_Timeout);

			if (!::WaitNamedPipe(m_ServerName.c_str(),m_Timeout)) {
				DWORD Error=::GetLastError();
				if (Error==ERROR_SEM_TIMEOUT) {
					OutLog(LOG_ERROR,L"パイプ(%s)が待機時間(%u ms)内に接続可能になりません。",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutSystemErrorLog(Error,L"パイプ(%s)の接続待機でエラーが発生しました。",
									  m_ServerName.c_str());
				}
				return false;
			}

			OutLog(LOG_VERBOSE,L"パイプ(%s)を開きます。",m_ServerName.c_str());

			hPipe=::CreateFile(m_ServerName.c_str(),
							   GENERIC_READ | GENERIC_WRITE,
							   0,
							   &SecurityAttributes,
							   OPEN_EXISTING,
							   FILE_FLAG_OVERLAPPED,
							   nullptr);
			if (hPipe!=INVALID_HANDLE_VALUE) {
				OutLog(LOG_VERBOSE,L"パイプ(%s)を開きました。(%p)",m_ServerName.c_str(),hPipe);
				break;
			}

			DWORD Error=::GetLastError();
			if (Error!=ERROR_PIPE_BUSY) {
				OutSystemErrorLog(Error,L"パイプ(%s)を開けません。",m_ServerName.c_str());
				return false;
			}
		} while (::GetTickCount()-StartTime<m_Timeout);

		if (hPipe==INVALID_HANDLE_VALUE) {
			OutLog(LOG_ERROR,L"パイプ(%s)が利用可能になりません。",m_ServerName.c_str());
			return false;
		}

		OVERLAPPED Overlapped;
		::ZeroMemory(&Overlapped,sizeof(Overlapped));
		Overlapped.hEvent=::CreateEvent(nullptr,TRUE,FALSE,nullptr);

		try {
			DWORD Size=(DWORD)SendSize;
			if (!WritePipe(hPipe,&Size,sizeof(DWORD),&Overlapped)) {
				//OutLog(LOG_ERROR,L"パイプ(%s)への書き出しができません。",m_ServerName.c_str());
				throw __LINE__;
			}
			if (!WritePipe(hPipe,pSendData,Size,&Overlapped)) {
				//OutLog(LOG_ERROR,L"パイプ(%s)への書き出しができません。",m_ServerName.c_str());
				throw __LINE__;
			}

			DWORD ReceiveSize=0;
			if (!ReadPipe(hPipe,&ReceiveSize,sizeof(DWORD),&Overlapped)) {
				//OutLog(LOG_ERROR,L"パイプ(%s)からの読み込みができません。",m_ServerName.c_str());
				throw __LINE__;
			}
			if (ReceiveSize>MESSAGE_PIPE_MAX_SEND_SIZE) {
				OutLog(LOG_ERROR,L"パイプ(%s)からの応答サイズ(%lu bytes)が大きすぎます。",
					   m_ServerName.c_str(),(unsigned long)ReceiveSize);
				throw __LINE__;
			}
			if (!pReceiveData->SetSize(ReceiveSize)) {
				OutLog(LOG_ERROR,L"パイプ(%s)からの応答受信用のメモリ(%lu bytes)を確保できません。",
					   m_ServerName.c_str(),(unsigned long)ReceiveSize);
				throw __LINE__;
			}
			if (!ReadPipe(hPipe,pReceiveData->GetBuffer(),ReceiveSize,&Overlapped)) {
				//OutLog(LOG_ERROR,L"パイプ(%s)からの応答の読み込みができません。",m_ServerName.c_str());
				throw __LINE__;
			}
		} catch (...) {
			::CloseHandle(hPipe);
			::CloseHandle(Overlapped.hEvent);
			return false;
		}

		::CloseHandle(hPipe);
		::CloseHandle(Overlapped.hEvent);

		OutLog(LOG_VERBOSE,L"パイプ(%s)を閉じました。(%p)",m_ServerName.c_str(),hPipe);

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
				OutSystemErrorLog(Result,L"パイプ(%s)からの応答の読み込みができません。",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"パイプ(%s)からの応答の読み込み待ちがタイムアウトしました。(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"パイプ(%s)からの応答の読み込み待ちで予期しない結果が返されました。(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"パイプ(%s)からの応答の読み込みをキャンセルします。",
					   m_ServerName.c_str());
				::CancelIo(hPipe);
				OutLog(LOG_VERBOSE,L"パイプ(%s)からの応答の読み込みキャンセルの完了を待機します。",
					   m_ServerName.c_str());
				::GetOverlappedResult(hPipe,pOverlapped,&Read,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(hPipe,pOverlapped,&Read,FALSE)) {
				OutSystemErrorLog(::GetLastError(),L"パイプ(%s)からの応答の読み込み結果を取得できません。",
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
				OutSystemErrorLog(Result,L"パイプ(%s)への書き出しができません。",
								  m_ServerName.c_str());
				return false;
			}

			Result=::WaitForSingleObject(pOverlapped->hEvent,m_Timeout);
			if (Result!=WAIT_OBJECT_0) {
				if (Result==WAIT_TIMEOUT) {
					OutLog(LOG_ERROR,L"パイプ(%s)への書き出し待ちがタイムアウトしました。(%u ms)",
						   m_ServerName.c_str(),m_Timeout);
				} else {
					OutLog(LOG_ERROR,L"パイプ(%s)への書き出し待ちで予期しない結果が返されました。(0x%x)",
						   m_ServerName.c_str(),Result);
				}

				OutLog(LOG_VERBOSE,L"パイプ(%s)への書き出しをキャンセルします。",
					   m_ServerName.c_str());
				::CancelIo(hPipe);
				OutLog(LOG_VERBOSE,L"パイプ(%s)への書き出しキャンセルの完了を待機します。",
					   m_ServerName.c_str());
				::GetOverlappedResult(hPipe,pOverlapped,&Write,TRUE);
				return false;
			}

			if (!::GetOverlappedResult(hPipe,pOverlapped,&Write,FALSE)) {
				OutSystemErrorLog(::GetLastError(),L"パイプ(%s)への書き出し結果を取得できません。",
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
