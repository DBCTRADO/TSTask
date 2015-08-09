#include "stdafx.h"
#include "TsNetworkSender.h"
#include "StdUtil.h"
#include "../Common/DebugDef.h"

#pragma comment(lib,"Ws2_32.lib")


#ifndef TS_PACKETSIZE
#define TS_PACKETSIZE	(188U)
#endif


CTsNetworkSender::CTsNetworkSender(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_bWSAInitialized(false)
	, m_hSendThread(NULL)

	, m_SendSize(256)
	, m_SendWait(10)
	, m_bAdjustWait(true)
	, m_ConnectRetryInterval(2000)
	, m_MaxConnectRetries(15)
	, m_TcpMaxSendRetries(1)
	, m_bTcpPrependHeader(true)

	, m_bEnableQueueing(false)
	, m_QueueBlockSize((1024 * 1024) / TS_PACKETSIZE * TS_PACKETSIZE)
	, m_MaxQueueSize(32)
{
}


CTsNetworkSender::~CTsNetworkSender(void)
{
	Close();

	if (m_bWSAInitialized)
		::WSACleanup();
}


void CTsNetworkSender::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	ClearQueue();
}


const bool CTsNetworkSender::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_bEnableQueueing) {
		AddStream(pMediaData->GetData(), pMediaData->GetSize());
	}

	return true;
}


bool CTsNetworkSender::Open(const AddressInfo *pList, DWORD Length)
{
	CBlockLock Lock(&m_DecoderLock);

	if (IsOpen()) {
		SetError(TEXT("既に開かれています。"));
		return false;
	}

	if (pList == NULL || Length == 0) {
		SetError(TEXT("アドレスが指定されていません。"));
		return false;
	}

	if (!m_bWSAInitialized) {
		WSAData WSAData;
		int Error = ::WSAStartup(MAKEWORD(2,0), &WSAData);
		if (Error != 0) {
			TCHAR szText[64];
			StdUtil::snprintf(szText, _countof(szText), TEXT("Winsockの初期化ができません。(%d)"), Error);
			SetError(szText);
			return false;
		}
		m_bWSAInitialized = true;
	}

	try {
		for (DWORD i = 0; i < Length; i++) {
			SocketInfo Info;
			int Type;

			if (pList[i].Type == SOCKET_UDP) {
				Type = SOCK_DGRAM;
			} else if (pList[i].Type == SOCKET_TCP) {
				Type = SOCK_STREAM;
			} else {
				throw __LINE__;
			}
			Info.Type = pList[i].Type;
			TCHAR szPort[8];
			StdUtil::snprintf(szPort, _countof(szPort), TEXT("%d"), pList[i].Port);
			ADDRINFOT AddrHints;
			::ZeroMemory(&AddrHints, sizeof(ADDRINFOT));
			AddrHints.ai_socktype = Type;
			int Result = ::GetAddrInfo(pList[i].pszAddress, szPort, &AddrHints, &Info.AddrList);
			if (Result != 0) {
				SetError(TEXT("アドレスを取得できません。"));
				throw __LINE__;
			}

			// 非ブロッキングに設定
			u_long arg = 1;
			::ioctlsocket(Info.sock, FIONBIO, &arg);

			Info.bConnected = false;
			Info.addr = NULL;
			Info.sock = INVALID_SOCKET;
			Info.Event = WSA_INVALID_EVENT;
			Info.SentBytes = 0;

			if (Type == SOCK_DGRAM) {
				const DWORD LocalHost = ::inet_addr("127.0.0.1");
				::setsockopt(Info.sock, IPPROTO_IP, IP_MULTICAST_IF, (const char *)&LocalHost, sizeof(DWORD));
			}

			m_SockList.push_back(Info);
		}

		m_EndSendingEvent.Create(true);
		m_hSendThread = (HANDLE)::_beginthreadex(NULL, 0, SendThread, this, 0, NULL);
		if (m_hSendThread == NULL) {
			SetError(TEXT("スレッドを作成できません。"));
			throw __LINE__;
		}
	} catch (...) {
		Close();
		return false;
	}

	return true;
}


bool CTsNetworkSender::Close()
{
	if (m_hSendThread) {
		m_EndSendingEvent.Set();
		if (::WaitForSingleObject(m_hSendThread, 10000) == WAIT_TIMEOUT) {
			Trace(CTracer::TYPE_WARNING, TEXT("ネットワーク送信スレッドが応答しないため強制終了します。"));
			::TerminateThread(m_hSendThread, -1);
		}
		::CloseHandle(m_hSendThread);
		m_hSendThread=NULL;
	}
	m_EndSendingEvent.Close();

	CBlockLock Lock(&m_DecoderLock);

	ClearQueue();
	m_bEnableQueueing = false;

	for (auto i = m_SockList.begin(); i != m_SockList.end(); ++i) {
		if (i->Event != WSA_INVALID_EVENT) {
			if (i->sock != INVALID_SOCKET)
				::WSAEventSelect(i->sock, NULL, 0);
			::WSACloseEvent(i->Event);
		}
		if (i->sock != INVALID_SOCKET) {
			::shutdown(i->sock, SD_BOTH);
			::closesocket(i->sock);
		}
		::FreeAddrInfo(i->AddrList);
	}
	m_SockList.clear();

	return true;
}


bool CTsNetworkSender::IsOpen() const
{
	return m_SockList.size()>0;
}


bool CTsNetworkSender::SetSendSize(DWORD Size)
{
	if (Size == 0)
		return false;
	m_DecoderLock.Lock();
	m_SendSize = Size;
	m_DecoderLock.Unlock();
	return true;
}


DWORD CTsNetworkSender::GetSendSize() const
{
	return m_SendSize;
}


bool CTsNetworkSender::SetSendWait(DWORD Wait)
{
	m_SendWait = Wait;
	return true;
}


DWORD CTsNetworkSender::GetSendWait() const
{
	return m_SendWait;
}


bool CTsNetworkSender::SetAdjustWait(bool bAdjust)
{
	m_bAdjustWait = bAdjust;
	return true;
}


bool CTsNetworkSender::GetAdjustWait() const
{
	return m_bAdjustWait;
}


void CTsNetworkSender::SetConnectRetryInterval(DWORD Interval)
{
	m_ConnectRetryInterval = Interval;
}


DWORD CTsNetworkSender::GetConnectRetryInterval() const
{
	return m_ConnectRetryInterval;
}


bool CTsNetworkSender::SetMaxConnectRetries(int MaxRetries)
{
	if (MaxRetries < 0)
		return false;
	m_MaxConnectRetries = MaxRetries;
	return true;
}


int CTsNetworkSender::GetMaxConnectRetries() const
{
	return m_MaxConnectRetries;
}


bool CTsNetworkSender::SetTcpMaxSendRetries(int MaxRetries)
{
	if (MaxRetries < 0)
		return false;
	m_TcpMaxSendRetries = MaxRetries;
	return true;
}


int CTsNetworkSender::GetTcpMaxSendRetries() const
{
	return m_TcpMaxSendRetries;
}


void CTsNetworkSender::SetTcpPrependHeader(bool bPrependHeader)
{
	m_bTcpPrependHeader = bPrependHeader;
}


bool CTsNetworkSender::GetTcpPrependHeader() const
{
	return m_bTcpPrependHeader;
}


bool CTsNetworkSender::AddStream(const BYTE *pData, DWORD DataSize)
{
	if (!pData || DataSize == 0)
		return false;

	if (!m_SendQueue.empty()) {
		QueueBlock &Last = m_SendQueue.back();
		if (m_QueueBlockSize - Last.Size >= DataSize) {
			::CopyMemory(Last.pData + Last.Size, pData, DataSize);
			Last.Size += DataSize;
			return true;
		}
	}

	QueueBlock Block;

	if (m_SendQueue.size() < m_MaxQueueSize) {
		try {
			Block.pData = new BYTE[m_QueueBlockSize];
		} catch (...) {
			return false;
		}
	} else {
		Block = m_SendQueue.front();
		m_SendQueue.pop_front();
	}
	Block.Size = DataSize;
	Block.Offset = 0;
	::CopyMemory(Block.pData, pData, DataSize);
	m_SendQueue.push_back(Block);

	return true;
}


bool CTsNetworkSender::GetStream(CMediaData *pData, DWORD *pWait)
{
	CBlockLock Lock(&m_DecoderLock);

	if (m_SendQueue.empty() || m_SendQueue.front().Size == 0) {
		if (m_bAdjustWait) {
			if (*pWait < m_SendWait * 2)
				(*pWait) += 2;
		}
		return false;
	}

	DWORD RemainSize = m_SendSize * TS_PACKETSIZE;

	do {
		QueueBlock &Block = m_SendQueue.front();
		const DWORD Size = min(RemainSize, Block.Size - Block.Offset);
		pData->AddData(&Block.pData[Block.Offset], Size);
		RemainSize -= Size;
		Block.Offset += Size;
		if (Block.Offset == Block.Size) {
			if (m_SendQueue.size() > 1) {
				delete [] Block.pData;
				m_SendQueue.pop_front();
			} else {
				Block.Size = 0;
				Block.Offset = 0;
				break;
			}
		}
	} while (RemainSize > 0);

	if (m_bAdjustWait) {
		if (RemainSize) {
			if (*pWait < m_SendWait * 2)
				(*pWait)++;
		} else {
			if (*pWait > m_SendWait / 2)
				(*pWait)--;
		}
	}

	return true;
}


void CTsNetworkSender::ClearQueue()
{
	while (!m_SendQueue.empty()) {
		delete [] m_SendQueue.front().pData;
		m_SendQueue.pop_front();
	}
}


bool CTsNetworkSender::ConnectTCP()
{
	for (auto i = m_SockList.begin(); i != m_SockList.end(); ++i) {
		if (m_EndSendingEvent.Wait(500) != WAIT_TIMEOUT)
			return false;

		if (i->Type == SOCKET_TCP && !i->bConnected) {
			for (const ADDRINFOT *addr = i->AddrList; addr != NULL; addr = addr->ai_next) {
				SOCKET sock = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
				if (sock == INVALID_SOCKET)
					continue;

				WSAEVENT Event = ::WSACreateEvent();
				if (Event == WSA_INVALID_EVENT)
					continue;
				if (::WSAEventSelect(sock, Event, FD_CONNECT | FD_WRITE) == SOCKET_ERROR) {
					::WSACloseEvent(Event);
					continue;
				}

				if (::connect(sock, addr->ai_addr, (int)addr->ai_addrlen) != SOCKET_ERROR) {
					i->bConnected = true;
					i->addr = addr;
					i->sock = sock;
					i->Event = Event;
					ConnectedTrace(addr);
					break;
				}

				int Error = ::WSAGetLastError();
				if (Error == WSAEWOULDBLOCK) {
					for (;;) {
						if (m_EndSendingEvent.Wait(500) != WAIT_TIMEOUT) {
							::WSAEventSelect(sock, NULL, 0);
							::WSACloseEvent(Event);
							::closesocket(sock);
							return false;
						}

						int Result = ::WSAWaitForMultipleEvents(1, &Event, FALSE, 0, FALSE);
						if (Result == WSA_WAIT_EVENT_0) {
							WSANETWORKEVENTS Events;

							Result = ::WSAEnumNetworkEvents(sock, Event, &Events);
							if (Result != SOCKET_ERROR) {
								if (Events.lNetworkEvents == 0)
									continue;
								if ((Events.lNetworkEvents & FD_CONNECT) != 0) {
									if (Events.iErrorCode[FD_CONNECT_BIT] == 0) {
										i->bConnected = true;
										i->addr = addr;
										i->sock = sock;
										i->Event = Event;
										ConnectedTrace(addr);
									}
								}
#ifdef _DEBUG
								else {
									TRACE(TEXT("WSAEnumNetworkEvents() Unexpected event %x\n"), Events.lNetworkEvents);
								}
#endif
								break;
							} else {
								TRACE(TEXT("WSAEnumNetworkEvents() error %d\n"), Result);
								break;
							}
						} else if (Result != WSA_WAIT_TIMEOUT) {
							TRACE(TEXT("WSAWaitForMultipleEvents() error %d\n"), Result);
							break;
						}
					}

					if (i->bConnected)
						break;
				}
#ifdef _DEBUG
				else {
					TRACE(TEXT("CTsNetworkSender::ConnectTCP() connect() error %d\n"),
						  Error);
				}
#endif

				::WSAEventSelect(sock, NULL, 0);
				::WSACloseEvent(Event);
				::closesocket(sock);
			}
		} else if (i->Type == SOCKET_UDP && i->sock == INVALID_SOCKET) {
			for (const ADDRINFOT *addr = i->AddrList; addr != NULL; addr = addr->ai_next) {
				SOCKET sock = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
				if (sock != INVALID_SOCKET) {
					i->addr = addr;
					i->sock = sock;
					break;
				}
			}
		}
	}

	for (auto i = m_SockList.begin(); i != m_SockList.end(); ++i) {
		if (!i->addr)
			return false;
	}

	return true;
}


/*
	接続ごとにスレッドを作るように改良した方がいいと思われます
*/
void CTsNetworkSender::SendMain()
{
	for (int i = 0; i <= m_MaxConnectRetries; i++) {
		if (ConnectTCP())
			break;
		if (m_EndSendingEvent.Wait(m_ConnectRetryInterval) != WAIT_TIMEOUT)
			return;
	}

	for (auto i = m_SockList.begin(); i != m_SockList.end(); ++i) {
		if (i->Type == SOCKET_TCP && !i->bConnected) {
			TraceAddress(CTracer::TYPE_ERROR, TEXT("%s:%d に接続できません。"), i->AddrList);
		}
	}

	CMediaData SendData;
	DWORD Count = 0;
	DWORD Wait = m_SendWait;

	m_bEnableQueueing = true;

	do {
		static const DWORD TCP_HEADER_SIZE = 2 * sizeof(DWORD);

		SendData.SetSize(TCP_HEADER_SIZE);

		if (GetStream(&SendData, &Wait)) {
			const DWORD DataOctets = SendData.GetSize() - TCP_HEADER_SIZE;

			DWORD *pHeader = (DWORD*)SendData.GetData();
			pHeader[0] = Count;
			pHeader[1] = DataOctets;

			for (auto i = m_SockList.begin(); i != m_SockList.end(); ++i) {
				if (i->Type == SOCKET_UDP) {
					if (i->sock != INVALID_SOCKET) {
						int Result = ::sendto(
							i->sock,
							(const char*)SendData.GetData() + TCP_HEADER_SIZE,
							DataOctets,
							0,
							i->addr->ai_addr,
							(int)i->addr->ai_addrlen);
						if (Result != SOCKET_ERROR) {
							i->SentBytes += Result;
						}
#ifdef _DEBUG
						else {
							TRACE(TEXT("sendto() error %d\n"), ::WSAGetLastError());
						}
#endif
					}
				} else {
					if (i->bConnected) {
						const char *pData = (const char*)SendData.GetData();
						int Size;

						if (m_bTcpPrependHeader) {
							Size = (int)SendData.GetSize();
						} else {
							pData += TCP_HEADER_SIZE;
							Size = DataOctets;
						}

						const int MaxRetries = m_TcpMaxSendRetries;

						for (int j = 0; j < 1 + MaxRetries; j++) {
							int Result = ::send(i->sock, pData, Size, 0);
							if (Result != SOCKET_ERROR) {
								i->SentBytes += Result;
								break;
							}
							if (j == MaxRetries) {
								TRACE(TEXT("send() error %d\n"), ::WSAGetLastError());
								break;
							}
							int Error = ::WSAGetLastError();
							if (Error != WSAEWOULDBLOCK) {
								TRACE(TEXT("send() error %d\n"), Error);
								break;
							}

							Result = ::WSAWaitForMultipleEvents(1, &i->Event, FALSE, 1000, FALSE);
							if (Result != WSA_WAIT_EVENT_0)
								break;
							WSANETWORKEVENTS Events;
							Result = ::WSAEnumNetworkEvents(i->sock, i->Event, &Events);
							if (Result == SOCKET_ERROR
									|| (Events.lNetworkEvents & FD_WRITE) == 0)
								break;
						}
						//Count++;
					}
				}
			}
			Count++;
		}
	} while (m_EndSendingEvent.Wait(Wait) == WAIT_TIMEOUT);
}


void CTsNetworkSender::TraceAddress(CTracer::TraceType Type, LPCTSTR pszFormat, const ADDRINFOT *addr)
{
	TCHAR szText[256];

	if (addr->ai_family == AF_INET) {
		const struct sockaddr_in *sockaddr = (const struct sockaddr_in*)addr->ai_addr;
		TCHAR szAddress[INET_ADDRSTRLEN];
		if (!::InetNtop(AF_INET, (void*)&sockaddr->sin_addr,
						szAddress, _countof(szAddress)))
			return;
		StdUtil::snprintf(szText, _countof(szText), pszFormat,
						  szAddress, ::ntohs(sockaddr->sin_port));
	} else if (addr->ai_family == AF_INET6) {
		const struct sockaddr_in6 *sockaddr = (const struct sockaddr_in6*)addr->ai_addr;
		TCHAR szAddress[INET6_ADDRSTRLEN];
		if (!::InetNtop(AF_INET, (void*)&sockaddr->sin6_addr,
						szAddress, _countof(szAddress)))
			return;
		StdUtil::snprintf(szText, _countof(szText), pszFormat,
						  szAddress, ::ntohs(sockaddr->sin6_port));
	} else {
		return;
	}

	Trace(Type, szText);
}


void CTsNetworkSender::ConnectedTrace(const ADDRINFOT *addr)
{
	TraceAddress(CTracer::TYPE_INFORMATION, TEXT("%s:%d に接続しました。"), addr);
}


unsigned int __stdcall CTsNetworkSender::SendThread(LPVOID lpParameter)
{
	CTsNetworkSender *pThis = static_cast<CTsNetworkSender*>(lpParameter);

	try {
		pThis->SendMain();
	} catch (...) {
		return 1;
	}

	return 0;
}
