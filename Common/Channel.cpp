#include "stdafx.h"
#include "TSTaskBase.h"
#include "Channel.h"
#include "DebugDef.h"


namespace TSTask
{

	CChannelInfo::CChannelInfo()
		: m_Space(0)
		, m_Channel(0)
		, m_RemoteControlKeyID(0)
		, m_NetworkID(0)
		, m_TransportStreamID(0)
		, m_ServiceID(0)
		, m_Frequency(0)
		, m_fEnabled(true)
	{
	}

	CChannelInfo::CChannelInfo(int Space,int Channel,int RemoteControlKeyID,LPCWSTR pszName)
		: m_Space(Space)
		, m_Channel(Channel)
		, m_RemoteControlKeyID(RemoteControlKeyID)
		, m_Name(pszName)
		, m_NetworkID(0)
		, m_TransportStreamID(0)
		, m_ServiceID(0)
		, m_Frequency(0)
		, m_fEnabled(true)
	{
	}

	bool CChannelInfo::SetSpace(int Space)
	{
		if (Space<0)
			return false;
		m_Space=Space;
		return true;
	}

	bool CChannelInfo::SetChannel(int Channel)
	{
		if (Channel<0)
			return false;
		m_Channel=Channel;
		return true;
	}

	bool CChannelInfo::SetRemoteControlKeyID(int RemoteControlKeyID)
	{
		if (RemoteControlKeyID<0)
			return false;
		m_RemoteControlKeyID=RemoteControlKeyID;
		return true;
	}

	bool CChannelInfo::SetName(LPCWSTR pszName)
	{
		if (!IsStringEmpty(pszName))
			m_Name=pszName;
		else
			m_Name.clear();
		return true;
	}

	bool CChannelInfo::SetNetworkID(WORD NetworkID)
	{
		m_NetworkID=NetworkID;
		return true;
	}

	bool CChannelInfo::SetTransportStreamID(WORD TransportStreamID)
	{
		m_TransportStreamID=TransportStreamID;
		return true;
	}

	bool CChannelInfo::SetServiceID(WORD ServiceID)
	{
		m_ServiceID=ServiceID;
		return true;
	}

	bool CChannelInfo::SetServiceType(BYTE ServiceType)
	{
		m_ServiceType=ServiceType;
		return true;
	}

	bool CChannelInfo::SetFrequency(int Frequency)
	{
		m_Frequency=Frequency;
		return true;
	}


	CChannelList::CChannelList()
	{
	}

	CChannelList::CChannelList(const CChannelList &Src)
	{
		*this=Src;
	}

	CChannelList::~CChannelList()
	{
		Clear();
	}

	CChannelList &CChannelList::operator=(const CChannelList &Src)
	{
		if (&Src!=this) {
			Clear();

			if (Src.m_List.size()>0) {
				m_List.reserve(Src.m_List.size());
				for (auto e:Src.m_List)
					m_List.push_back(new CChannelInfo(*e));
			}
		}
		return *this;
	}

	size_t CChannelList::GetEnabledChannelCount() const
	{
		size_t Count=0;

		for (auto e:m_List) {
			if (e->IsEnabled())
				Count++;
		}

		return Count;
	}

	bool CChannelList::AddChannel(CChannelInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		m_List.push_back(pInfo);

		return true;
	}

	bool CChannelList::AddChannel(const CChannelInfo &Info)
	{
		return AddChannel(new CChannelInfo(Info));
	}

	bool CChannelList::DeleteChannel(size_t Index)
	{
		if (Index>=m_List.size())
			return false;

		auto i=m_List.begin();
		std::advance(i,Index);
		delete *i;
		m_List.erase(i);

		return true;
	}

	void CChannelList::Clear()
	{
		for (auto e:m_List)
			delete e;
		m_List.clear();
	}

	CChannelInfo *CChannelList::GetChannelInfo(size_t Index)
	{
		if (Index>=m_List.size())
			return nullptr;

		return m_List[Index];
	}

	const CChannelInfo *CChannelList::GetChannelInfo(size_t Index) const
	{
		if (Index>=m_List.size())
			return nullptr;

		return m_List[Index];
	}

	int CChannelList::GetSpace(size_t Index) const
	{
		if (Index>=m_List.size())
			return -1;

		return m_List[Index]->GetSpace();
	}

	int CChannelList::GetChannel(size_t Index) const
	{
		if (Index>=m_List.size())
			return -1;

		return m_List[Index]->GetChannel();
	}

	int CChannelList::GetRemoteControlKeyID(size_t Index) const
	{
		if (Index>=m_List.size())
			return -1;

		return m_List[Index]->GetRemoteControlKeyID();
	}

	LPCWSTR CChannelList::GetChannelName(size_t Index) const
	{
		if (Index>=m_List.size())
			return L"";

		return m_List[Index]->GetName();
	}

	bool CChannelList::IsChannelEnabled(size_t Index) const
	{
		if (Index>=m_List.size())
			return false;

		return m_List[Index]->IsEnabled();
	}

	int CChannelList::Find(const CChannelInfo *pInfo) const
	{
		for (size_t i=0;i<m_List.size();i++) {
			if (m_List[i]==pInfo)
				return (int)i;
		}
		return -1;
	}

	int CChannelList::Find(int Space,int Channel,int ServiceID) const
	{
		for (size_t i=0;i<m_List.size();i++) {
			const CChannelInfo *pChInfo=m_List[i];

			if ((Space<0 || pChInfo->GetSpace()==Space)
					&& (Channel<0 || pChInfo->GetChannel()==Channel)
					&& (ServiceID<=0 || pChInfo->GetServiceID()==ServiceID))
				return (int)i;
		}
		return -1;
	}

	int CChannelList::FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
	{
		for (size_t i=0;i<m_List.size();i++) {
			const CChannelInfo *pChInfo=m_List[i];

			if ((NetworkID==0 || pChInfo->GetNetworkID()==NetworkID)
					&& (TransportStreamID==0 || pChInfo->GetTransportStreamID()==TransportStreamID)
					&& (ServiceID==0 || pChInfo->GetServiceID()==ServiceID))
				return (int)i;
		}
		return -1;
	}

	int CChannelList::FindRemoteControlKeyID(int RemoteControlKeyID) const
	{
		for (size_t i=0;i<m_List.size();i++) {
			if (m_List[i]->GetRemoteControlKeyID()==RemoteControlKeyID)
				return (int)i;
		}
		return -1;
	}

	int CChannelList::FindServiceID(WORD ServiceID) const
	{
		for (size_t i=0;i<m_List.size();i++) {
			if (m_List[i]->GetServiceID()==ServiceID)
				return (int)i;
		}
		return -1;
	}

	bool CChannelList::Sort(SortType Type,bool fDescending)
	{
		if (Type<0 || Type>=SORT_TRAILER)
			return false;

		if (m_List.size()>1) {
			CChannelInfo **ppTemp=new CChannelInfo*[m_List.size()];
			SortSub(Type,fDescending,0,m_List.size()-1,ppTemp);
			delete [] ppTemp;
		}

		return true;
	}

	void CChannelList::SortSub(SortType Type,bool fDescending,size_t First,size_t Last,CChannelInfo **ppTemp)
	{
		size_t Center,i,j,k;

		if (First>=Last)
			return;

		Center=(First+Last)/2;

		SortSub(Type,fDescending,First,Center,ppTemp);
		SortSub(Type,fDescending,Center+1,Last,ppTemp);

		for (i=First;i<=Center;i++)
			ppTemp[i]=m_List[i];
		for (j=Last;i<=Last;i++,j--)
			ppTemp[i]=m_List[j];

		i=First;
		j=Last;

		for (k=First;k<=Last;k++) {
			CChannelInfo *pCh1=ppTemp[i],*pCh2=ppTemp[j];
			int Cmp;

			switch (Type) {
			case SORT_SPACE:
				Cmp=pCh1->GetSpace()-pCh2->GetSpace();
				break;

			case SORT_CHANNEL:
				Cmp=pCh1->GetChannel()-pCh2->GetChannel();
				break;

			case SORT_REMOTE_CONTROL_KEY_ID:
				Cmp=pCh1->GetRemoteControlKeyID()-pCh2->GetRemoteControlKeyID();
				break;

			case SORT_NAME:
				Cmp=::lstrcmpiW(pCh1->GetName(),pCh2->GetName());
				if (Cmp==0)
					Cmp=::lstrcmpW(pCh1->GetName(),pCh2->GetName());
				break;

			case SORT_NETWORK_ID:
				Cmp=pCh1->GetNetworkID()-pCh2->GetNetworkID();
				break;

			case SORT_TRANSPORT_STREAM_ID:
				Cmp=pCh1->GetTransportStreamID()-pCh2->GetTransportStreamID();
				break;

			case SORT_SERVICE_ID:
				Cmp=pCh1->GetServiceID()-pCh2->GetServiceID();
				break;

			default:
				__assume(0);
			}

			if (fDescending)
				Cmp=-Cmp;
			if (Cmp<=0)
				m_List[k]=ppTemp[i++];
			else
				m_List[k]=ppTemp[j--];
		}
	}

	bool CChannelList::HasRemoteControlKeyID() const
	{
		for (size_t i=0;i<m_List.size();i++) {
			if (m_List[i]->GetRemoteControlKeyID()!=0)
				return true;
		}
		return false;
	}

	bool CChannelList::HasMultiService() const
	{
		for (size_t i=0;i<m_List.size()-1;i++) {
			const CChannelInfo *pChannelInfo1=m_List[i];

			for (size_t j=i+1;j<m_List.size();j++) {
				const CChannelInfo *pChannelInfo2=m_List[j];

				if (pChannelInfo1->GetNetworkID()==pChannelInfo2->GetNetworkID()
						&& pChannelInfo1->GetTransportStreamID()==pChannelInfo2->GetTransportStreamID()
						&& pChannelInfo1->GetServiceID()!=pChannelInfo2->GetServiceID())
					return true;
			}
		}
		return false;
	}


	bool CTuningSpaceInfo::Create(const CChannelList &List,LPCWSTR pszName)
	{
		m_ChannelList=List;

		SetName(pszName);

		return true;
	}

	bool CTuningSpaceInfo::SetName(LPCWSTR pszName)
	{
		if (!IsStringEmpty(pszName))
			m_Name=pszName;
		else
			m_Name.clear();

		return true;
	}


	CTuningSpaceList::CTuningSpaceList()
	{
	}

	CTuningSpaceList::CTuningSpaceList(const CTuningSpaceList &Src)
	{
		*this=Src;
	}

	CTuningSpaceList::~CTuningSpaceList()
	{
		Clear();
	}

	CTuningSpaceList &CTuningSpaceList::operator=(const CTuningSpaceList &Src)
	{
		if (&Src!=this) {
			Clear();

			if (Src.m_TuningSpaceList.size()>0) {
				m_TuningSpaceList.reserve(Src.m_TuningSpaceList.size());

				for (auto e:Src.m_TuningSpaceList)
					m_TuningSpaceList.push_back(new CTuningSpaceInfo(*e));
			}

			m_AllChannelList=Src.m_AllChannelList;
		}

		return *this;
	}

	CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(size_t Space)
	{
		if (Space>=m_TuningSpaceList.size())
			return nullptr;

		return m_TuningSpaceList[Space];
	}

	const CTuningSpaceInfo *CTuningSpaceList::GetTuningSpaceInfo(size_t Space) const
	{
		if (Space>=m_TuningSpaceList.size())
			return nullptr;

		return m_TuningSpaceList[Space];
	}

	CChannelList *CTuningSpaceList::GetChannelList(size_t Space)
	{
		if (Space>=m_TuningSpaceList.size())
			return nullptr;

		return &m_TuningSpaceList[Space]->GetChannelList();
	}

	const CChannelList *CTuningSpaceList::GetChannelList(size_t Space) const
	{
		if (Space>=m_TuningSpaceList.size())
			return nullptr;

		return &m_TuningSpaceList[Space]->GetChannelList();
	}

	LPCWSTR CTuningSpaceList::GetTuningSpaceName(size_t Space) const
	{
		if (Space>=m_TuningSpaceList.size())
			return nullptr;

		return m_TuningSpaceList[Space]->GetName();
	}

	const CChannelInfo *CTuningSpaceList::GetChannelInfo(size_t Space,size_t Channel) const
	{
		const CChannelList *pChannelList=GetChannelList(Space);
		if (pChannelList==nullptr)
			return nullptr;

		return pChannelList->GetChannelInfo(Channel);
	}

	bool CTuningSpaceList::Create(const CChannelList &List,int Spaces)
	{
		Clear();

		if (!MakeTuningSpaceList(List,Spaces))
			return false;

		m_AllChannelList=List;

		return true;
	}

	bool CTuningSpaceList::MakeTuningSpaceList(const CChannelList &List,int Spaces)
	{
		for (size_t i=0;i<List.GetChannelCount();i++) {
			int Space=List.GetSpace(i);
			if (Space+1>Spaces)
				Spaces=Space+1;
		}

		if (Spaces<1)
			return false;

		if (!Reserve(Spaces))
			return false;

		for (size_t i=0;i<List.GetChannelCount();i++) {
			const CChannelInfo *pChInfo=List.GetChannelInfo(i);

			m_TuningSpaceList[pChInfo->GetSpace()]->GetChannelList().AddChannel(*pChInfo);
		}

		return true;
	}

	bool CTuningSpaceList::Reserve(int Spaces)
	{
		if (Spaces<0)
			return false;

		if ((size_t)Spaces==m_TuningSpaceList.size())
			return true;

		if (Spaces==0) {
			Clear();
			return true;
		}

		if ((size_t)Spaces<m_TuningSpaceList.size()) {
			auto i=m_TuningSpaceList.begin();
			std::advance(i,Spaces);
			do {
				m_TuningSpaceList.erase(i);
			} while (i!=m_TuningSpaceList.end());
		} else {
			for (size_t i=m_TuningSpaceList.size();i<(size_t)Spaces;i++)
				m_TuningSpaceList.push_back(new CTuningSpaceInfo);
		}

		return true;
	}

	void CTuningSpaceList::Clear()
	{
		for (auto e:m_TuningSpaceList)
			delete e;
		m_TuningSpaceList.clear();
		m_AllChannelList.Clear();
	}

	bool CTuningSpaceList::MakeAllChannelList()
	{
		m_AllChannelList.Clear();

		for (auto e:m_TuningSpaceList) {
			CChannelList &List=e->GetChannelList();

			for (size_t j=0;j<List.GetChannelCount();j++)
				m_AllChannelList.AddChannel(*List.GetChannelInfo(j));
		}

		return true;
	}

	static const UINT CP_SHIFT_JIS=932;

	bool CTuningSpaceList::SaveToFile(LPCWSTR pszFileName) const
	{
		if (IsStringEmpty(pszFileName))
			return false;

		String Buffer(
			L"; TVTest チャンネル設定ファイル\r\n"
			L"; 名称,チューニング空間,チャンネル,リモコン番号,サービスタイプ,サービスID,ネットワークID,TSID,状態\r\n");
		String Temp;

		for (size_t i=0;i<GetSpaceCount();i++) {
			const CChannelList &ChannelList=m_TuningSpaceList[i]->GetChannelList();

			if (ChannelList.GetChannelCount()==0)
				continue;

			if (GetTuningSpaceName(i)!=NULL) {
				StringUtility::Format(Temp,L";#SPACE(%d,%s)\r\n",static_cast<int>(i),GetTuningSpaceName(i));
				Buffer+=Temp;
			}

			for (size_t j=0;j<ChannelList.GetChannelCount();j++) {
				const CChannelInfo *pChInfo=ChannelList.GetChannelInfo(j);
				LPCWSTR pszName=pChInfo->GetName();
				String Name;

				// 必要に応じて " で囲む
				if (pszName[0]==L'#' || pszName[0]==L';'
						|| ::StrChrW(pszName,L',')!=NULL
						|| ::StrChrW(pszName,L'"')!=NULL) {
					LPCWSTR p=pszName;
					Name=L'"';
					while (*p!=L'\0') {
						if (*p==L'"') {
							Name+=L"\"\"";
							p++;
						} else {
							int SrcLength;
							for (SrcLength=1;p[SrcLength]!=L'"' && p[SrcLength]!=L'\0';SrcLength++);
							Name.append(p,SrcLength);
							p+=SrcLength;
						}
					}
					Name+=L'"';
				}

				StringUtility::Format(Temp,L"%s,%d,%d,%d,",
					Name.empty()?pszName:Name.c_str(),
					pChInfo->GetSpace(),
					pChInfo->GetChannel(),
					pChInfo->GetRemoteControlKeyID());
				Buffer+=Temp;
				TCHAR szText[32];
				if (pChInfo->GetServiceType()!=0) {
					FormatString(szText,_countof(szText),L"%d",pChInfo->GetServiceType());
					Buffer+=szText;
				}
				Buffer+=L',';
				if (pChInfo->GetServiceID()!=0) {
					FormatString(szText,_countof(szText),L"%d",pChInfo->GetServiceID());
					Buffer+=szText;
				}
				Buffer+=L',';
				if (pChInfo->GetNetworkID()!=0) {
					FormatString(szText,_countof(szText),L"%d",pChInfo->GetNetworkID());
					Buffer+=szText;
				}
				Buffer+=L',';
				if (pChInfo->GetTransportStreamID()!=0) {
					FormatString(szText,_countof(szText),L"%d",pChInfo->GetTransportStreamID());
					Buffer+=szText;
				}
				Buffer+=L',';
				Buffer+=pChInfo->IsEnabled()?L'1':L'0';
				Buffer+=L"\r\n";
			}
		}

		HANDLE hFile;
		DWORD Write;

		hFile=::CreateFileW(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;

		bool fUnicode=true;

		if (::GetACP()==CP_SHIFT_JIS) {
			BOOL fUsedDefaultChar=FALSE;
			int Length=::WideCharToMultiByte(
				CP_SHIFT_JIS,0,
				Buffer.data(),static_cast<int>(Buffer.length()),
				NULL,0,NULL,&fUsedDefaultChar);
			if (Length>0 && !fUsedDefaultChar) {
				char *pMBCSBuffer=new char[Length];
				Length=::WideCharToMultiByte(
					CP_SHIFT_JIS,0,
					Buffer.data(),static_cast<int>(Buffer.length()),
					pMBCSBuffer,Length,NULL,NULL);
				if (Length<1
						|| !::WriteFile(hFile,pMBCSBuffer,Length,&Write,NULL)
						|| Write!=static_cast<DWORD>(Length)) {
					delete [] pMBCSBuffer;
					::CloseHandle(hFile);
					return false;
				}
				delete [] pMBCSBuffer;
				fUnicode=false;
			}
		}

		if (fUnicode) {
			static const WCHAR BOM=0xFEFF;
			const DWORD Size=static_cast<DWORD>(Buffer.length())*sizeof(WCHAR);
			if (!::WriteFile(hFile,&BOM,sizeof(BOM),&Write,NULL)
						|| Write!=sizeof(BOM)
					|| !::WriteFile(hFile,Buffer.data(),Size,&Write,NULL)
						|| Write!=Size) {
				::CloseHandle(hFile);
				return false;
			}
		}

		::CloseHandle(hFile);

		return true;
	}

	static void SkipSpaces(LPWSTR *ppText)
	{
		LPWSTR p=*ppText;
		p+=::StrSpnW(p,L" \t");
		*ppText=p;
	}

	static bool NextToken(LPWSTR *ppText)
	{
		LPWSTR p=*ppText;

		SkipSpaces(&p);
		if (*p!=L',')
			return false;
		p++;
		SkipSpaces(&p);
		*ppText=p;
		return true;
	}

	bool inline IsDigit(WCHAR c)
	{
		return c>=L'0' && c<=L'9';
	}

	static int ParseDigits(LPWSTR *ppText)
	{
		LPWSTR pEnd;
		int Value=std::wcstol(*ppText,&pEnd,10);
		*ppText=pEnd;
		return Value;
	}

	bool CTuningSpaceList::LoadFromFile(LPCWSTR pszFileName)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		static const LONGLONG MAX_FILE_SIZE=8LL*1024*1024;

		HANDLE hFile;
		LARGE_INTEGER FileSize;
		DWORD Read;

		hFile=::CreateFileW(pszFileName,GENERIC_READ,FILE_SHARE_READ,nullptr,
							OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;
		if (!::GetFileSizeEx(hFile,&FileSize)
				|| FileSize.QuadPart<1
				|| FileSize.QuadPart>MAX_FILE_SIZE) {
			::CloseHandle(hFile);
			return false;
		}
		BYTE *pFileBuffer=new BYTE[FileSize.LowPart+sizeof(TCHAR)];
		if (!::ReadFile(hFile,pFileBuffer,FileSize.LowPart,&Read,nullptr) || Read!=FileSize.LowPart) {
			delete [] pFileBuffer;
			::CloseHandle(hFile);
			return false;
		}
		::CloseHandle(hFile);
		LPWSTR pszBuffer=nullptr,p;
		if (FileSize.LowPart>=2 && *reinterpret_cast<LPWSTR>(pFileBuffer)==0xFEFF) {
			p=reinterpret_cast<LPWSTR>(pFileBuffer)+1;
			p[FileSize.LowPart/2-1]=L'\0';
		} else {
			int Length=::MultiByteToWideChar(
				CP_SHIFT_JIS,0,
				reinterpret_cast<LPCSTR>(pFileBuffer),FileSize.LowPart,
				nullptr,0);
			if (Length<1) {
				delete [] pFileBuffer;
				return false;
			}
			pszBuffer=new WCHAR[Length+1];
			Length=::MultiByteToWideChar(
				CP_SHIFT_JIS,0,
				reinterpret_cast<LPCSTR>(pFileBuffer),FileSize.LowPart,
				pszBuffer,Length);
			pszBuffer[Length]=L'\0';
			p=pszBuffer;
		}

		m_AllChannelList.Clear();

		do {
			String Name;

			p+=::StrSpnW(p,L"\r\n \t");

			if (*p==L'#' || *p==L';') {	// コメント
				p++;
				if (*p==L'#') {
					p++;
					if (::StrCmpNIW(p,L"SPACE(",6)==0) {
						// チューニング空間名 #space(インデックス,名前)
						p+=6;
						SkipSpaces(&p);
						if (IsDigit(*p)) {
							int Space=ParseDigits(&p);
							if (Space>=0 && Space<100 && NextToken(&p)) {
								int Length=::StrCSpnW(p,L")\r\n");
								if (p[Length]==L')' && p[Length+1]==L')')
									Length++;
								if (Length>0) {
									if ((int)m_TuningSpaceList.size()<=Space) {
										Reserve(Space+1);
										Name.assign(p,Length);
										m_TuningSpaceList[Space]->SetName(Name.c_str());
									}
									p+=Length;
									if (*p==L'\0')
										break;
									p++;
								}
							}
						}
					}
				}
				goto Next;
			}
			if (*p==L'\0')
				break;

			{
				CChannelInfo ChInfo;

				// チャンネル名
				Name.clear();
				bool fQuote=false;
				if (*p==L'"') {
					fQuote=true;
					p++;
				}
				while (*p!=L'\0') {
					if (fQuote) {
						if (*p==L'"') {
							p++;
							if (*p!=L'"') {
								SkipSpaces(&p);
								break;
							}
						}
					} else {
						if (*p==L',')
							break;
					}
					Name+=*p++;
				}
				ChInfo.SetName(Name.c_str());
				if (!NextToken(&p))
					goto Next;

				// チューニング空間
				if (!IsDigit(*p))
					goto Next;
				ChInfo.SetSpace(ParseDigits(&p));
				if (!NextToken(&p))
					goto Next;

				// チャンネル
				if (!IsDigit(*p))
					goto Next;
				ChInfo.SetChannel(ParseDigits(&p));

				if (NextToken(&p)) {
					// リモコン番号(オプション)
					ChInfo.SetRemoteControlKeyID(ParseDigits(&p));
					if (NextToken(&p)) {
						// サービスタイプ(オプション)
						ChInfo.SetServiceType(static_cast<BYTE>(ParseDigits(&p)));
						if (NextToken(&p)) {
							// サービスID(オプション)
							ChInfo.SetServiceID(static_cast<WORD>(ParseDigits(&p)));
							if (NextToken(&p)) {
								// ネットワークID(オプション)
								ChInfo.SetNetworkID(static_cast<WORD>(ParseDigits(&p)));
								if (NextToken(&p)) {
									// トランスポートストリームID(オプション)
									ChInfo.SetTransportStreamID(static_cast<WORD>(ParseDigits(&p)));
									if (NextToken(&p)) {
										// 状態(オプション)
										if (IsDigit(*p)) {
											int Flags=ParseDigits(&p);
											ChInfo.Enable((Flags&1)!=0);
										}
									}
								}
							}
						}
					}
				}

				m_AllChannelList.AddChannel(ChInfo);
			}

		Next:
			p+=::StrCSpnW(p,L"\r\n");
		} while (*p!=L'\0');

		delete [] pszBuffer;
		delete [] pFileBuffer;

		return MakeTuningSpaceList(m_AllChannelList);
	}

}
