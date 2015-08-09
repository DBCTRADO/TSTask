#ifndef TSTASK_STREAMING_H
#define TSTASK_STREAMING_H


namespace TSTask
{

	enum NetworkProtocolType
	{
		PROTOCOL_UNDEFINED=-1,
		PROTOCOL_UDP,
		PROTOCOL_TCP,
		PROTOCOL_TRAILER
	};

	struct NetworkAddress
	{
		NetworkProtocolType Protocol = PROTOCOL_UNDEFINED;
		String Address;
		WORD Port = 0;
	};

	struct IPAddress
	{
		enum AddressType
		{
			TYPE_V4,
			TYPE_V6
		};

		struct V4Address
		{
			union
			{
				DWORD Value;
				BYTE Bytes[4];
			};
		};

		struct V6Address
		{
			union {
				UINT64 Value[2];
				BYTE Bytes[16];
			};
		};

		AddressType Type;
		V4Address V4;
		V6Address V6;
	};

	struct StreamingInfo
	{
		String Media;
		NetworkAddress Address;
		bool fFindUnusedPort;
		ServiceSelectType ServiceSelect;
		DWORD Streams;
	};

	class CStreamingManager
	{
	public:
		CStreamingManager();
		~CStreamingManager();
		bool Initialize();
		void Finalize();
		bool IsInitialized() const { return m_fInitialized; }

	private:
		bool m_fInitialized;
	};

	namespace Streaming
	{

		LPCWSTR GetProtocolText(NetworkProtocolType Protocol);
		NetworkProtocolType ParseProtocolText(LPCWSTR pszText);

		bool GetIPAddress(const NetworkAddress &Address,IPAddress *pIPAddress);
		bool GetAddressMutexName(NetworkProtocolType Protocol,const IPAddress &Address,WORD Port,String *pName);
		bool CreateAddressMutex(CMutex *pMutex,NetworkProtocolType Protocol,const IPAddress &Address,WORD Port);
		bool CreateAddressMutex(CMutex *pMutex,NetworkProtocolType Protocol,const IPAddress &Address,WORD *pPort);

	}

}


#endif
