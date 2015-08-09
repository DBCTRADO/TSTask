#ifndef TSTASK_CHANNEL_H
#define TSTASK_CHANNEL_H


#include <vector>


namespace TSTask
{

	class CChannelInfo
	{
	public:
		CChannelInfo();
		CChannelInfo(int Space,int Channel,int RemoteControlKeyID,LPCWSTR pszName);
		virtual ~CChannelInfo() {}
		int GetSpace() const { return m_Space; }
		bool SetSpace(int Space);
		int GetChannel() const { return m_Channel; }
		bool SetChannel(int Channel);
		int GetRemoteControlKeyID() const { return m_RemoteControlKeyID; }
		bool SetRemoteControlKeyID(int RemoteControlKeyID);
		LPCWSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCWSTR pszName);
		WORD GetNetworkID() const { return m_NetworkID; }
		bool SetNetworkID(WORD NetworkID);
		WORD GetTransportStreamID() const { return m_TransportStreamID; }
		bool SetTransportStreamID(WORD TransportStreamID);
		WORD GetServiceID() const { return m_ServiceID; }
		bool SetServiceID(WORD ServiceID);
		BYTE GetServiceType() const { return m_ServiceType; }
		bool SetServiceType(BYTE ServiceType);
		int GetFrequency() const { return m_Frequency; }
		bool SetFrequency(int Frequency);
		void Enable(bool fEnable) { m_fEnabled=fEnable; }
		bool IsEnabled() const { return m_fEnabled; }

	protected:
		int m_Space;
		int m_Channel;
		int m_RemoteControlKeyID;
		String m_Name;
		WORD m_NetworkID;
		WORD m_TransportStreamID;
		WORD m_ServiceID;
		BYTE m_ServiceType;
		int m_Frequency;
		bool m_fEnabled;
	};

	class CChannelList
	{
	public:
		enum SortType
		{
			SORT_SPACE,
			SORT_CHANNEL,
			SORT_REMOTE_CONTROL_KEY_ID,
			SORT_NAME,
			SORT_NETWORK_ID,
			SORT_TRANSPORT_STREAM_ID,
			SORT_SERVICE_ID,
			SORT_TRAILER
		};

		CChannelList();
		CChannelList(const CChannelList &Src);
		~CChannelList();
		CChannelList &operator=(const CChannelList &Src);
		size_t GetChannelCount() const { return m_List.size(); }
		size_t GetEnabledChannelCount() const;
		bool AddChannel(CChannelInfo *pInfo);
		bool AddChannel(const CChannelInfo &Info);
		bool DeleteChannel(size_t Index);
		void Clear();
		CChannelInfo *GetChannelInfo(size_t Index);
		const CChannelInfo *GetChannelInfo(size_t Index) const;
		int GetSpace(size_t Index) const;
		int GetChannel(size_t Index) const;
		int GetRemoteControlKeyID(size_t Index) const;
		LPCWSTR GetChannelName(size_t Index) const;
		bool IsChannelEnabled(size_t Index) const;
		int Find(const CChannelInfo *pInfo) const;
		int Find(int Space,int Channel,int ServiceID=-1) const;
		int FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID=0) const;
		int FindRemoteControlKeyID(int RemoteControlKeyID) const;
		int FindServiceID(WORD ServiceID) const;
		bool Sort(SortType Type,bool fDescending=false);
		bool HasRemoteControlKeyID() const;
		bool HasMultiService() const;

	private:
		void SortSub(SortType Type,bool fDescending,size_t First,size_t Last,CChannelInfo **ppTemp);

		std::vector<CChannelInfo*> m_List;
	};

	class CTuningSpaceInfo
	{
	public:
		bool Create(const CChannelList &List,LPCWSTR pszName=nullptr);
		CChannelList &GetChannelList() { return m_ChannelList; }
		const CChannelList &GetChannelList() const { return m_ChannelList; }
		LPCWSTR GetName() const { return m_Name.c_str(); }
		bool SetName(LPCWSTR pszName);
		size_t GetChannelCount() const { return m_ChannelList.GetChannelCount(); }

	private:
		String m_Name;
		CChannelList m_ChannelList;
	};

	class CTuningSpaceList
	{
	public:
		CTuningSpaceList();
		CTuningSpaceList(const CTuningSpaceList &Src);
		~CTuningSpaceList();
		CTuningSpaceList &operator=(const CTuningSpaceList &Src);
		size_t GetSpaceCount() const { return m_TuningSpaceList.size(); }
		CTuningSpaceInfo *GetTuningSpaceInfo(size_t Space);
		const CTuningSpaceInfo *GetTuningSpaceInfo(size_t Space) const;
		CChannelList *GetChannelList(size_t Space);
		const CChannelList *GetChannelList(size_t Space) const;
		CChannelList &GetAllChannelList() { return m_AllChannelList; }
		const CChannelList &GetAllChannelList() const { return m_AllChannelList; }
		LPCWSTR GetTuningSpaceName(size_t Space) const;
		const CChannelInfo *GetChannelInfo(size_t Space,size_t Channel) const;
		bool Create(const CChannelList &List,int Spaces=0);
		bool Reserve(int Spaces);
		void Clear();
		bool MakeAllChannelList();
		bool SaveToFile(LPCWSTR pszFileName) const;
		bool LoadFromFile(LPCWSTR pszFileName);

	private:
		bool MakeTuningSpaceList(const CChannelList &List,int Spaces=0);

		std::vector<CTuningSpaceInfo*> m_TuningSpaceList;
		CChannelList m_AllChannelList;
	};

}


#endif
