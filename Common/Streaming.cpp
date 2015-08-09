#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "TSTaskBase.h"
#include "Streaming.h"
#include "DebugDef.h"

#pragma comment(lib,"ws2_32.lib")


namespace TSTask
{

	CStreamingManager::CStreamingManager()
		: m_fInitialized(false)
	{
	}

	CStreamingManager::~CStreamingManager()
	{
		Finalize();
	}

	bool CStreamingManager::Initialize()
	{
		if (!m_fInitialized) {
			WSAData WSAData;
			int Error=::WSAStartup(MAKEWORD(2,0),&WSAData);
			if (Error!=0) {
				OutLog(LOG_ERROR,L"Winsockの初期化ができません。(Error 0x%x)",Error);
				return false;
			}
			m_fInitialized=true;
		}

		return true;
	}

	void CStreamingManager::Finalize()
	{
		if (m_fInitialized) {
			::WSACleanup();
			m_fInitialized=false;
		}
	}


	namespace Streaming
	{

		static const LPCWSTR g_ProtocolList[] = {
			L"UDP",
			L"TCP",
		};
		static_assert(_countof(g_ProtocolList)==PROTOCOL_TRAILER,"プロトコルの数が一致しません。");

		LPCWSTR GetProtocolText(NetworkProtocolType Protocol)
		{
			if (Protocol<0 || Protocol>=PROTOCOL_TRAILER)
				return nullptr;

			return g_ProtocolList[Protocol];
		}

		NetworkProtocolType ParseProtocolText(LPCWSTR pszText)
		{
			if (IsStringEmpty(pszText))
				return PROTOCOL_UNDEFINED;

			for (int i=0;i<_countof(g_ProtocolList);i++) {
				if (::lstrcmpiW(pszText,g_ProtocolList[i])==0)
					return NetworkProtocolType(i);
			}

			return PROTOCOL_UNDEFINED;
		}

		bool GetIPAddress(const NetworkAddress &Address,IPAddress *pIPAddress)
		{
			if (Address.Address.empty() || pIPAddress==nullptr)
				return false;

			IN_ADDR Addr;
			int Result=::InetPtonW(AF_INET,Address.Address.c_str(),&Addr);
			if (Result==1) {
				pIPAddress->Type=IPAddress::TYPE_V4;
				pIPAddress->V4.Value=Addr.S_un.S_addr;
			} else {
				if (Result!=0)
					return false;
				IN6_ADDR Addr6;
				Result=::InetPtonW(AF_INET6,Address.Address.c_str(),&Addr6);
				if (Result!=1)
					return false;
				pIPAddress->Type=IPAddress::TYPE_V6;
				std::memcpy(pIPAddress->V6.Bytes,Addr6.u.Byte,16);
			}

			return true;
		}

		bool GetAddressMutexName(NetworkProtocolType Protocol,const IPAddress &Address,WORD Port,String *pName)
		{
			if (pName==nullptr)
				return false;

			pName->clear();

			if (Address.Type==IPAddress::TYPE_V4) {
				// EDCBと互換の名前を使用する

				switch (Protocol) {
				case PROTOCOL_UDP:
					StringUtility::Format(*pName,L"Global\\EpgDataCap_Bon_UDP_PORT_%d_%d",
										  (int)Address.V4.Value,Port);
					break;

				case PROTOCOL_TCP:
					StringUtility::Format(*pName,L"Global\\EpgDataCap_Bon_TCP_PORT_%d_%d",
										  (int)Address.V4.Value,Port);
					break;

				default:
					return false;
				}
			} else if (Address.Type==IPAddress::TYPE_V6) {
				WCHAR szAddress[32+1];

				for (int i=0;i<16;i++)
					FormatString(szAddress+i*2,3,L"%02x",Address.V6.Bytes[i]);

				switch (Protocol) {
				case PROTOCOL_UDP:
					StringUtility::Format(*pName,L"Global\\TSTask_UDP_%s_%u",szAddress,Port);
					break;

				case PROTOCOL_TCP:
					StringUtility::Format(*pName,L"Global\\TSTask_TCP_%s_%u",szAddress,Port);
					break;

				default:
					return false;
				}
			} else {
				return false;
			}

			return true;
		}

		bool CreateAddressMutex(CMutex *pMutex,NetworkProtocolType Protocol,const IPAddress &Address,WORD Port)
		{
			if (pMutex==nullptr)
				return false;

			String Name;
			if (!GetAddressMutexName(Protocol,Address,Port,&Name))
				return false;

			if (!pMutex->Create(Name.c_str()))
				return false;

			if (pMutex->IsAlreadyExists()) {
				pMutex->Close();
				OutLog(LOG_VERBOSE,L"ストリーミングのMutex(%s)が既に存在します。",Name.c_str());
				return false;
			}

			OutLog(LOG_VERBOSE,L"ストリーミングのMutex(%s)を作成しました。",Name.c_str());

			return true;
		}

		bool CreateAddressMutex(CMutex *pMutex,NetworkProtocolType Protocol,const IPAddress &Address,WORD *pPort)
		{
			if (pMutex==nullptr || pPort==nullptr)
				return false;

			String Name;

			for (WORD Port=*pPort;;Port++) {
				if (!GetAddressMutexName(Protocol,Address,Port,&Name))
					return false;

				if (pMutex->Create(Name.c_str())) {
					if (!pMutex->IsAlreadyExists()) {
						OutLog(LOG_VERBOSE,L"ストリーミングのMutexを作成しました。(%s)",Name.c_str());
						*pPort=Port;
						break;
					}
					pMutex->Close();
				}

				if (Port==0xFFFF)
					return false;
			}

			return true;
		}

	}

}
