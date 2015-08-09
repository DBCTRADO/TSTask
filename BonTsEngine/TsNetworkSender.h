#pragma once


#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <deque>
#include "MediaDecoder.h"


class CTsNetworkSender : public CMediaDecoder
{
public:
	CTsNetworkSender(IEventHandler *pEventHandler = NULL);
	virtual ~CTsNetworkSender();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsNetwokSender
	enum SocketType {
		SOCKET_UDP,
		SOCKET_TCP
	};

	struct AddressInfo {
		SocketType Type;
		LPCTSTR pszAddress;
		WORD Port;
	};

	bool Open(const AddressInfo *pList, DWORD Length);
	bool Close();
	bool IsOpen() const;

	bool SetSendSize(DWORD Size);
	DWORD GetSendSize() const;
	bool SetSendWait(DWORD Wait);
	DWORD GetSendWait() const;
	bool SetAdjustWait(bool bAdjust);
	bool GetAdjustWait() const;
	void SetConnectRetryInterval(DWORD Interval);
	DWORD GetConnectRetryInterval() const;
	bool SetMaxConnectRetries(int MaxRetries);
	int GetMaxConnectRetries() const;
	bool SetTcpMaxSendRetries(int MaxRetries);
	int GetTcpMaxSendRetries() const;
	void SetTcpPrependHeader(bool bPrependHeader);
	bool GetTcpPrependHeader() const;

protected:
	bool AddStream(const BYTE *pData, DWORD DataSize);
	bool GetStream(CMediaData *pData, DWORD *pWait);
	void ClearQueue();
	bool ConnectTCP();
	void SendMain();
	void TraceAddress(CTracer::TraceType Type, LPCTSTR pszFormat, const ADDRINFOT *addr);
	void ConnectedTrace(const ADDRINFOT *addr);

	static unsigned int __stdcall WINAPI SendThread(LPVOID lpParameter);

	bool m_bWSAInitialized;

	HANDLE m_hSendThread;
	CLocalEvent m_EndSendingEvent;

	DWORD m_SendSize;
	DWORD m_SendWait;
	bool m_bAdjustWait;
	DWORD m_ConnectRetryInterval;
	int m_MaxConnectRetries;
	int m_TcpMaxSendRetries;
	bool m_bTcpPrependHeader;

	bool m_bEnableQueueing;
	DWORD m_QueueBlockSize;
	DWORD m_MaxQueueSize;
	struct QueueBlock {
		BYTE *pData;
		DWORD Size;
		DWORD Offset;
	};
	std::deque<QueueBlock> m_SendQueue;

	struct SocketInfo {
		SocketType Type;
		ADDRINFOT *AddrList;
		bool bConnected;
		const ADDRINFOT *addr;
		SOCKET sock;
		WSAEVENT Event;
		ULONGLONG SentBytes;
	};
	std::vector<SocketInfo> m_SockList;
};
