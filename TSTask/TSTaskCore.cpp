#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskCore.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	static DWORD GetTsSelectorStreamFlags(DWORD Flags)
	{
		Flags &= STREAM_ALL;
		if (Flags == STREAM_ALL)
			return ::CTsSelector::STREAM_ALL;
		return Flags;
	}


	CTSTaskCore::CTSTaskCore()
		: m_fInitialized(false)
		, m_LockTimeout(5000)
		, m_StreamPoolCount(0)
		, m_MinDiskFreeSpace(100*1024*1024)
	{
		m_DtvEngine.SetTracer(this);
		m_DtvEngine.m_BonSrcDecoder.SetTracer(this);
		m_DtvEngine.m_TsRecorder.SetTracer(this);
		m_DtvEngine.m_TsNetworkSender.SetTracer(this);

		m_RecordingInfo.State=RECORDING_STATE_NOT_RECORDING;

		m_CurrentEvent.EventID=0;
		m_CurrentEvent.Duration=0;
	}

	CTSTaskCore::~CTSTaskCore()
	{
	}

	bool CTSTaskCore::Initialize()
	{
		if (!m_DtvEngine.BuildEngine(this))
			return false;

		m_fInitialized=true;

		return true;
	}

	bool CTSTaskCore::Finalize()
	{
		OutLog(LOG_VERBOSE,L"Coreの終了処理を行います。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			//return false;
		}

		m_fInitialized=false;

		StopRecording();

		StopStreaming();
		m_StreamingManager.Finalize();

		m_StreamPool.End();
		m_StreamPool.Close();

		CloseTuner();
		UnloadBonDriver();

		m_DtvEngine.CloseEngine();

		m_EventHandlerList.clear();

		return true;
	}

	bool CTSTaskCore::LoadBonDriver(LPCWSTR pszFileName)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		if (!m_fInitialized)
			return false;

		if (!UnloadBonDriver())
			return false;

		OutLog(LOG_IMPORTANT,L"BonDrvier(%s)をロードします。",pszFileName);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [pszFileName](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnBonDriverLoad(pszFileName);
							   })) {
			OutLog(LOG_INFO,L"BonDriverのロードがキャンセルされました。");
			return false;
		}

		if (!m_DtvEngine.LoadBonDriver(pszFileName)) {
			for (auto e:m_EventHandlerList)
				e->OnBonDriverLoadFailed(pszFileName);
			return false;
		}

		String ChannelFileName;
		if (GetModuleFilePath(m_DtvEngine.m_BonSrcDecoder.GetBonDriverModule(),&ChannelFileName)) {
			PathUtility::RenameExtension(&ChannelFileName,L".ch2");
			if (!PathUtility::IsFileExists(ChannelFileName)) {
				PathUtility::RenameExtension(&ChannelFileName,L".ch1");
				if (!PathUtility::IsFileExists(ChannelFileName)) {
					OutLog(LOG_WARNING,L"チャンネルファイル(%s/.ch2)が見付かりません。",ChannelFileName.c_str());
					PathUtility::RenameExtension(&ChannelFileName,L".ch2");
				}
			}
			if (!m_TuningSpaceList.LoadFromFile(ChannelFileName.c_str())) {
				OutLog(LOG_ERROR,L"チャンネルファイル \"%s\" の読み込みができません。",ChannelFileName.c_str());
			} else {
				OutLog(LOG_INFO,L"チャンネルファイル \"%s\" を読み込みました。",ChannelFileName.c_str());
				for (size_t i=0;i<m_TuningSpaceList.GetSpaceCount();i++) {
					if (IsStringEmpty(m_TuningSpaceList.GetTuningSpaceName(i))) {
						m_TuningSpaceList.GetTuningSpaceInfo(i)->SetName(
							m_DtvEngine.m_BonSrcDecoder.GetSpaceName((DWORD)i));
					}
				}

				for (auto e:m_EventHandlerList)
					e->OnChannelListLoaded(m_TuningSpaceList);
			}
		}

		OutLog(LOG_VERBOSE,L"BonDriver(%s)がロードされたことを通知します。",pszFileName);

		for (auto e:m_EventHandlerList)
			e->OnBonDriverLoaded(pszFileName);

		return true;
	}

	bool CTSTaskCore::UnloadBonDriver()
	{
		if (!IsBonDriverLoaded())
			return true;

		if (!CloseTuner())
			return false;

		OutLog(LOG_IMPORTANT,L"BonDrvierをアンロードします。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnBonDriverUnload();
							   })) {
			OutLog(LOG_INFO,L"BonDriverのアンロードがキャンセルされました。");
			return false;
		}

		m_DtvEngine.UnloadBonDriver();

		m_TuningSpaceList.Clear();

		OutLog(LOG_VERBOSE,L"BonDriverがアンロードされたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnBonDriverUnloaded();

		return true;
	}

	bool CTSTaskCore::IsBonDriverLoaded() const
	{
		return m_DtvEngine.m_BonSrcDecoder.IsBonDriverLoaded();
	}

	bool CTSTaskCore::GetBonDriverFilePath(String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		pFilePath->clear();

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!m_DtvEngine.m_BonSrcDecoder.IsBonDriverLoaded())
			return true;

		return GetModuleFilePath(m_DtvEngine.m_BonSrcDecoder.GetBonDriverModule(),pFilePath);
	}

	bool CTSTaskCore::OpenTuner()
	{
		if (!m_fInitialized)
			return false;

		if (!IsBonDriverLoaded())
			return false;

		if (!CloseTuner())
			return false;

		OutLog(LOG_IMPORTANT,L"チューナーを開きます。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnTunerOpen(); })) {
			OutLog(LOG_INFO,L"チューナーオープンがキャンセルされました。");
			return false;
		}

		if (!m_DtvEngine.OpenTuner()) {
			OutBonTsEngineErrorLog(m_DtvEngine);

			for (auto e:m_EventHandlerList)
				e->OnTunerOpenFailed();

			return false;
		}

		OutLog(LOG_VERBOSE,L"チューナーが開かれたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnTunerOpened();

		return true;
	}

	bool CTSTaskCore::CloseTuner()
	{
		if (!IsTunerOpened())
			return true;

		OutLog(LOG_IMPORTANT,L"チューナーを閉じます。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnTunerClose(); })) {
			OutLog(LOG_INFO,L"チューナークローズがキャンセルされました。");
			return false;
		}

		if (!m_DtvEngine.CloseTuner()) {
			OutBonTsEngineErrorLog(m_DtvEngine);
			return false;
		}

		m_CurrentChannel.Space=TuningChannelInfo::SPACE_INVALID;
		m_CurrentChannel.Channel=TuningChannelInfo::CHANNEL_INVALID;
		m_CurrentChannel.ServiceID=SERVICE_ID_INVALID;

		m_CurrentScannedChannel.Space=-1;
		m_CurrentScannedChannel.Channel=-1;
		m_CurrentScannedChannel.ServiceID=SERVICE_ID_INVALID;

		m_CurrentEvent.EventID=0;
		m_CurrentEvent.Duration=0;

		OutLog(LOG_VERBOSE,L"チューナーが閉じられたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnTunerClosed();

		return true;
	}

	bool CTSTaskCore::IsTunerOpened() const
	{
		return m_DtvEngine.IsSrcFilterOpen();
	}

	bool CTSTaskCore::GetTunerName(String *pName) const
	{
		if (pName==nullptr)
			return false;

		pName->clear();

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		LPCWSTR pszName=m_DtvEngine.m_BonSrcDecoder.GetTunerName();
		if (!IsStringEmpty(pszName))
			pName->assign(pszName);

		return true;
	}

	bool CTSTaskCore::SetFirstChannelSetDelay(unsigned int Delay)
	{
		OutLog(LOG_INFO,L"最初のチャンネル設定までの最小間隔を%umsに設定します。",Delay);

		return m_DtvEngine.m_BonSrcDecoder.SetFirstChannelSetDelay(Delay);
	}

	bool CTSTaskCore::SetMinChannelChangeInterval(unsigned int Interval)
	{
		OutLog(LOG_INFO,L"最小チャンネル変更間隔を%umsに設定します。",Interval);

		return m_DtvEngine.m_BonSrcDecoder.SetMinChannelChangeInterval(Interval);
	}

	bool CTSTaskCore::SetChannelByIndex(DWORD Space,DWORD Channel,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_IMPORTANT,L"チャンネルを設定します。(チューニング空間 %u / チャンネル %u / サービス %u / 強制変更 %s)",
			   Space,Channel,ServiceID,fForce?L"する":L"しない");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!fForce && m_CurrentChannel.Space==Space && m_CurrentChannel.Channel==Channel) {
			OutLog(LOG_VERBOSE,L"指定されたチャンネルは既に選ばれているため、サービスのみ設定します。");
			return SetService(ServiceID);
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Space,Channel,ServiceID](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnChannelChange(Space,Channel,ServiceID);
							   })) {
			OutLog(LOG_INFO,L"チャンネル設定がキャンセルされました。");
			return false;
		}

		if (!m_DtvEngine.SetChannel(Space,Channel,ServiceID)) {
			OutBonTsEngineErrorLog(m_DtvEngine);

			for (auto e:m_EventHandlerList)
				e->OnChannelChangeFailed(Space,Channel,ServiceID);

			return false;
		}

		m_CurrentChannel.Space=Space;
		m_CurrentChannel.Channel=Channel;
		m_CurrentChannel.ServiceID=ServiceID!=0 ? ServiceID : SERVICE_ID_INVALID;

		m_CurrentScannedChannel.Space=-1;
		m_CurrentScannedChannel.Channel=-1;
		m_CurrentScannedChannel.ServiceID=SERVICE_ID_INVALID;

		const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
		if (pChannelList!=nullptr) {
			int Index=pChannelList->Find((int)Space,(int)Channel,ServiceID);
			if (Index>=0) {
				OutLog(LOG_VERBOSE,L"指定されたチャンネルに一致するスキャン済みのチャンネル(%d)が見付かりました。",Index);
				m_CurrentScannedChannel.Space=(int)Space;
				m_CurrentScannedChannel.Channel=Index;
				m_CurrentScannedChannel.ServiceID=m_CurrentChannel.ServiceID;
			} else {
				OutLog(LOG_VERBOSE,L"指定されたチャンネルに一致するスキャン済みのチャンネルはありません。");
			}
		}

		m_CurrentEvent.EventID=0;
		m_CurrentEvent.Duration=0;

		OutLog(LOG_VERBOSE,L"チャンネルが変更されたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnChannelChanged(Space,Channel,ServiceID);

		return true;
	}

	bool CTSTaskCore::SetChannelByScanned(int Space,int Channel,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_VERBOSE,L"スキャン済みのチャンネルを選択します。(チューニング空間 %d / チャンネル %d / サービス %u)",
			   Space,Channel,ServiceID);

		if (Space<0 || Channel<0)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
		if (pChannelList!=nullptr) {
			const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(Channel);
			if (pChannelInfo!=nullptr) {
				if (!SetChannelByIndex(pChannelInfo->GetSpace(),pChannelInfo->GetChannel(),
									   ServiceID!=0?ServiceID:pChannelInfo->GetServiceID(),
									   fForce))
					return false;

				m_CurrentScannedChannel.Space=Space;
				m_CurrentScannedChannel.Channel=Channel;
				m_CurrentScannedChannel.ServiceID=pChannelInfo->GetServiceID();

				return true;
			} else {
				OutLog(LOG_ERROR,L"指定されたチャンネル(%d)が不正です。",Channel);
			}
		} else {
			OutLog(LOG_ERROR,L"指定されたチューニング空間(%d)が不正です。",Space);
		}

		return false;
	}

	bool CTSTaskCore::SetChannelByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_INFO,L"指定されたIDに一致するチャンネルを選択します。(NID 0x%x / TSID 0x%x / SID 0x%x)",
			   NetworkID,TransportStreamID,ServiceID);

		if (NetworkID==0 && TransportStreamID==0 && ServiceID==0)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		for (size_t i=0;i<m_TuningSpaceList.GetSpaceCount();i++) {
			const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(i);

			if (pChannelList!=nullptr) {
				int Index=pChannelList->FindByIDs(NetworkID,TransportStreamID,ServiceID);
				if (Index>=0) {
					return SetChannelByScanned((int)i,Index,fForce);
				}
			}
		}

		if (TransportStreamID!=0) {
			for (size_t i=0;i<m_TuningSpaceList.GetSpaceCount();i++) {
				const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(i);

				if (pChannelList!=nullptr) {
					for (size_t j=0;j<pChannelList->GetChannelCount();j++) {
						int Index=pChannelList->FindByIDs(0,TransportStreamID,ServiceID);
						if (Index<0 && ServiceID!=0)
							Index=pChannelList->FindByIDs(0,TransportStreamID,0);
						if (Index>=0) {
							return SetChannelByScanned((int)i,Index,ServiceID,fForce);
						}
					}
				}
			}
		}

		if (ServiceID!=0) {
			for (size_t i=0;i<m_TuningSpaceList.GetSpaceCount();i++) {
				const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(i);

				if (pChannelList!=nullptr) {
					for (size_t j=0;j<pChannelList->GetChannelCount();j++) {
						int Index=pChannelList->FindByIDs(NetworkID,0,ServiceID);
						if (Index<0 && NetworkID!=0)
							Index=pChannelList->FindByIDs(0,0,ServiceID);
						if (Index>=0) {
							return SetChannelByScanned((int)i,Index,ServiceID,fForce);
						}
					}
				}
			}
		}

		OutLog(LOG_ERROR,L"指定されたID(NID 0x%x / TSID 0x%x / SID 0x%x)に一致するチャンネルがありません。",
			   NetworkID,TransportStreamID,ServiceID);

		return false;
	}

	bool CTSTaskCore::GetChannel(ChannelInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_CurrentChannel.Space!=TuningChannelInfo::SPACE_INVALID) {
			pInfo->Space=m_CurrentChannel.Space;
			pInfo->Channel=m_CurrentChannel.Channel;
			LPCWSTR pszName=m_TuningSpaceList.GetTuningSpaceName(m_CurrentChannel.Space);
			if (!IsStringEmpty(pszName))
				pInfo->SpaceName=pszName;
			else
				pInfo->SpaceName.clear();
		} else {
			pInfo->Space=-1;
			pInfo->Channel=-1;
			pInfo->SpaceName.clear();
		}

		if (m_CurrentChannel.ServiceID!=SERVICE_ID_INVALID)
			pInfo->ServiceID=m_CurrentChannel.ServiceID;
		else
			pInfo->ServiceID=0;

		const CChannelInfo *pChannel=nullptr;

		if (m_CurrentScannedChannel.Space>=0 && m_CurrentScannedChannel.Channel>=0) {
			pChannel=m_TuningSpaceList.GetChannelInfo(m_CurrentScannedChannel.Space,
													  m_CurrentScannedChannel.Channel);
		}
		if (pChannel!=nullptr) {
			pInfo->ScannedChannel=m_CurrentScannedChannel.Channel;
			pInfo->RemoteControlKeyID=pChannel->GetRemoteControlKeyID();
			pInfo->ChannelName=pChannel->GetName();
			pInfo->ScannedServiceID=pChannel->GetServiceID();
			pInfo->NetworkID=pChannel->GetNetworkID();
			pInfo->TransportStreamID=pChannel->GetTransportStreamID();
		} else {
			pInfo->ScannedChannel=-1;
			pInfo->RemoteControlKeyID=0;
			pInfo->ChannelName.clear();
			if (pInfo->Space>=0) {
				LPCWSTR pszChannelName=m_DtvEngine.m_BonSrcDecoder.GetChannelName(pInfo->Space,pInfo->Channel);
				if (pszChannelName!=nullptr)
					pInfo->ChannelName=pszChannelName;
			}
			pInfo->ScannedServiceID=0;
			pInfo->NetworkID=0;
			pInfo->TransportStreamID=0;
		}

		return true;
	}

	bool CTSTaskCore::SetService(WORD ServiceID)
	{
		OutLog(LOG_IMPORTANT,L"サービスを設定します。(SID 0x%x)",ServiceID);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_CurrentChannel.Space==TuningChannelInfo::SPACE_INVALID) {
			OutLog(LOG_ERROR,L"チャンネルが選択されていません。");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [ServiceID](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnServiceChange(ServiceID);
							   })) {
			OutLog(LOG_INFO,L"サービス設定がキャンセルされました。");
			return false;
		}

		if (!m_DtvEngine.SetServiceByID(ServiceID)) {
			OutLog(LOG_ERROR,L"サービスの設定ができません。");
			return false;
		}

		m_CurrentChannel.ServiceID=ServiceID;

		const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(m_CurrentChannel.Space);
		if (pChannelList!=nullptr) {
			int Index=pChannelList->Find((int)m_CurrentChannel.Space,(int)m_CurrentChannel.Channel,ServiceID);
			if (Index>=0) {
				OutLog(LOG_VERBOSE,L"指定されたサービスに一致するスキャン済みのチャンネル(%d)が見付かりました。",Index);
				m_CurrentScannedChannel.Channel=Index;
				m_CurrentScannedChannel.ServiceID=ServiceID;
			} else {
				OutLog(LOG_VERBOSE,L"指定されたサービスに一致するスキャン済みのチャンネルはありません。");
			}
		}

		// CTSTaskCore::OnServiceChanged() が呼ばれるのを待つ
		/*
		for (auto e:m_EventHandlerList)
			e->OnServiceChanged(ServiceID);
		*/

		return true;
	}

	bool CTSTaskCore::GetServiceInfo(ServiceInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		WORD ServiceID;
		if (!m_DtvEngine.GetServiceID(&ServiceID))
			return false;
		const int ServiceIndex=m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
		if (ServiceIndex<0)
			return false;

		::CTsAnalyzer::ServiceInfo Service;
		if (!m_DtvEngine.m_TsAnalyzer.GetServiceInfo(ServiceIndex, &Service))
			return false;

		pInfo->ServiceName=Service.szServiceName;
		pInfo->ServiceID=Service.ServiceID;
		pInfo->ServiceType=Service.ServiceType;

		return true;
	}

	bool CTSTaskCore::GetServiceList(ServiceList *pList)
	{
		if (pList==nullptr)
			return false;

		pList->clear();

		::CTsAnalyzer::ServiceList ServiceList;
		if (!m_DtvEngine.m_TsAnalyzer.GetViewableServiceList(&ServiceList))
			return false;

		pList->reserve(ServiceList.size());
		for (auto i=ServiceList.begin();i!=ServiceList.end();i++) {
			ServiceInfo Service;
			Service.ServiceName=i->szServiceName;
			Service.ServiceID=i->ServiceID;
			Service.ServiceType=i->ServiceType;
			pList->push_back(Service);
		}

		return true;
	}

	bool CTSTaskCore::StartRecording(const RecordingSettings &Settings)
	{
		if (Settings.Directories.empty() || Settings.Directories.front().empty()
				|| Settings.FileName.empty()) {
			OutLog(LOG_ERROR,L"ファイルパスが指定されていないため録画を開始できません。");
			return false;
		}

		if (!m_fInitialized)
			return false;

		OutLog(LOG_IMPORTANT,L"録画を開始します。(%s)",Settings.FileName.c_str());

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State!=RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"既に録画中のため新しい録画を開始できません。");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Settings](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnRecordingStart(Settings);
							   })) {
			OutLog(LOG_INFO,L"録画開始がキャンセルされました。");
			return false;
		}

		bool fCurServiceOnly=Settings.ServiceSelect==SERVICE_SELECT_CURRENT;
#ifdef BONTSENGINE_1SEG_SUPPORT
		if ((Settings.Streams & STREAM_1SEG)!=0) {
			OutLog(LOG_INFO,L"ワンセグサービスを選択します。");
			m_DtvEngine.SetServiceByID(::CDtvEngine::SID_1SEG,true);
			fCurServiceOnly=true;
		} else
#endif
		if (m_CurrentChannel.ServiceID!=SERVICE_ID_INVALID) {
			OutLog(LOG_VERBOSE,L"サービスを選択します。(%d)",m_CurrentChannel.ServiceID);
			m_DtvEngine.SetServiceByID(m_CurrentChannel.ServiceID,true);
		} else {
			m_DtvEngine.SetService(::CDtvEngine::SERVICE_INVALID);
		}

		OutLog(LOG_VERBOSE,L"%sサービスを録画対象に設定します。",
			   fCurServiceOnly?L"指定":L"全ての");
		OutLog(LOG_VERBOSE,L"録画対象ストリームを設定します。(0x%x) 字幕: 保存%s / データ放送: 保存%s",
			   Settings.Streams & STREAM_ALL,
			   (Settings.Streams & STREAM_CAPTION)?L"する":L"しない",
			   (Settings.Streams & STREAM_DATA_CARROUSEL)?L"する":L"しない");
		m_DtvEngine.SetWriteCurServiceOnly(fCurServiceOnly,GetTsSelectorStreamFlags(Settings.Streams));

		for (size_t i=0;i<Settings.Directories.size();i++)
			OutLog(LOG_VERBOSE,L"録画先フォルダ%Iu : \"%s\"",i+1,Settings.Directories[i].c_str());

		String FilePath;
		size_t DirIndex;
		for (DirIndex=0;DirIndex<Settings.Directories.size();DirIndex++) {
			FilePath=Settings.Directories[DirIndex];

			if (!PathUtility::IsExists(FilePath)) {
				OutLog(LOG_INFO,L"フォルダ \"%s\" が存在しないため作成します。",FilePath.c_str());
				int Error=::SHCreateDirectory(nullptr,FilePath.c_str());
				if (Error!=ERROR_SUCCESS && Error!=ERROR_ALREADY_EXISTS) {
					OutSystemErrorLog(Error,L"フォルダ \"%s\" を作成できません。",
									  FilePath.c_str());
					// 一応ファイルのオープンは試してみる
				}
			}

#if 0
			if (m_MinDiskFreeSpace>0) {
				PathUtility::AppendDelimiter(&FilePath);
				ULARGE_INTEGER FreeSpace;
				if (::GetDiskFreeSpaceEx(FilePath.c_str(),&FreeSpace,NULL,NULL)) {
					if (FreeSpace.QuadPart<m_MinDiskFreeSpace) {
						OutLog(LOG_WARNING,L"フォルダ \"%s\" の空き容量が不足しています。(%llu MiB)",
							   FilePath.c_str(),FreeSpace.QuadPart/(1024*1024));
						continue;
					}
				} else {
					OutLog(LOG_WARNING,L"フォルダ \"%s\" の空き容量が取得できません。(Error 0x%x)",
						   FilePath.c_str(),::GetLastError());
					//continue;
				}
			}
#endif

			PathUtility::Append(&FilePath,Settings.FileName.c_str());

			OutLog(LOG_INFO,L"ファイル \"%s\" を開きます。",FilePath.c_str());

			if (m_DtvEngine.m_TsRecorder.OpenFile(FilePath.c_str()))
				break;

			OutBonTsEngineErrorLog(m_DtvEngine.m_TsRecorder);
		}

		if (DirIndex==Settings.Directories.size()) {
			for (auto e:m_EventHandlerList)
				e->OnRecordingStartFailed(Settings);

			return false;
		}

		m_RecordingInfo.State=RECORDING_STATE_RECORDING;
		m_RecordingInfo.Settings=Settings;
		m_RecordingInfo.FilePaths.clear();
		m_RecordingInfo.FilePaths.push_back(FilePath);
		::GetSystemTimeAsFileTime(&m_RecordingInfo.StartTime);
		m_RecordingInfo.StartTickCount=::GetTickCount64();
		m_RecordingInfo.CurDirectory=(int)DirIndex;

		ResetErrorStatistics();

		OutLog(LOG_VERBOSE,L"録画が開始されたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnRecordingStarted(m_RecordingInfo);

		return true;
	}

	bool CTSTaskCore::StopRecording()
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State==RECORDING_STATE_NOT_RECORDING)
			return true;

		OutLog(LOG_IMPORTANT,L"録画を停止します。");

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnRecordingStop(); })) {
			OutLog(LOG_INFO,L"録画停止がキャンセルされました。");
			return false;
		}

		m_DtvEngine.m_TsRecorder.CloseFile();

		m_RecordingInfo.State=RECORDING_STATE_NOT_RECORDING;

		::CTsRecorder::WriteStatistics Statistics;
		m_DtvEngine.m_TsRecorder.GetWriteStatistics(&Statistics);
		OutLog(LOG_INFO,L"録画を停止しました。(入力 %llu バイト / 書き出し %llu バイト / 書き出し回数 %llu / 書き出しエラー回数 %u)",
			   Statistics.InputSize,Statistics.OutputSize,Statistics.OutputCount,Statistics.WriteErrorCount);

		OutLog(LOG_VERBOSE,L"録画が停止したことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnRecordingStopped();

		return true;
	}

	bool CTSTaskCore::SetNextRecordingDirectory()
	{
		OutLog(LOG_INFO,L"録画を次のフォルダに切り替えます。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State==RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"録画中ではありません。");
			return false;
		}

		if (m_RecordingInfo.Settings.Directories.size()<=(size_t)(m_RecordingInfo.CurDirectory+1)) {
			OutLog(LOG_WARNING,L"次のフォルダが指定されていません。");
			return false;
		}

		String CurDirectory;
		CurDirectory=m_RecordingInfo.FilePaths.back();
		PathUtility::RemoveFileName(&CurDirectory);
		PathUtility::AppendDelimiter(&CurDirectory);

		String FilePath,Directory;
		size_t i;
		for (i=m_RecordingInfo.CurDirectory+1;i<m_RecordingInfo.Settings.Directories.size();i++) {
			FilePath=m_RecordingInfo.Settings.Directories[i];
			PathUtility::AppendDelimiter(&FilePath);
			if (StringUtility::CompareNoCase(FilePath,CurDirectory)!=0) {
				if (m_MinDiskFreeSpace==0)
					break;

				Directory=FilePath;
				while (!PathUtility::IsExists(Directory)) {
					if (Directory.length()<=3
							|| PathUtility::IsRoot(Directory))
						break;

					String::size_type Pos=Directory.rfind(L'\\',Directory.length()-2);
					if (Pos==String::npos || Pos<2)
						break;
					Directory.resize(Pos+1);
				}

				ULARGE_INTEGER FreeSpace;
				if (::GetDiskFreeSpaceEx(Directory.c_str(),&FreeSpace,NULL,NULL)) {
					if (FreeSpace.QuadPart>=m_MinDiskFreeSpace) {
						OutLog(LOG_INFO,L"フォルダ \"%s\" に空きが %llu MiB あるため録画先に設定します。",
							   Directory.c_str(),FreeSpace.QuadPart/(1024*1024));
						break;
					}
					OutLog(LOG_WARNING,L"フォルダ \"%s\" の空き容量が不足しています。(%llu MiB)",
						   Directory.c_str(),FreeSpace.QuadPart/(1024*1024));
				} else {
					OutLog(LOG_WARNING,L"フォルダ \"%s\" の空き容量が取得できません。(Error 0x%x)",
						   Directory.c_str(),::GetLastError());
				}
			}
		}
		if (i==m_RecordingInfo.Settings.Directories.size()) {
			OutLog(LOG_ERROR,L"利用可能な録画先がありません。");
			return false;
		}

		PathUtility::Append(&FilePath,m_RecordingInfo.Settings.FileName.c_str());

		String BasePath,Extension;
		BasePath=FilePath;
		PathUtility::RemoveExtension(&BasePath);
		PathUtility::GetExtension(FilePath,&Extension);
		const unsigned int PartNo=(unsigned int)m_RecordingInfo.FilePaths.size()+1;
		StringUtility::Format(FilePath,L"%s.part%u%s",
							  BasePath.c_str(),PartNo,Extension.c_str());

		if (PathUtility::IsExists(FilePath)) {
			OutLog(LOG_VERBOSE,L"パス \"%s\" が既に存在するため、ファイル名を変更します。",FilePath.c_str());
			for (unsigned int j=1;;j++) {
				StringUtility::Format(FilePath,L"%s.part%u(%u)%s",
									  BasePath.c_str(),PartNo,j,Extension.c_str());
				if (!PathUtility::IsExists(FilePath))
					break;
				if (j==1000) {
					OutLog(LOG_ERROR,L"ユニークなファイル名を生成できません。");
					return false;
				}
			}
		}

		m_RecordingInfo.CurDirectory=(int)i;

		if (!ChangeRecordingFile(FilePath.c_str())) {
			if (i+1<m_RecordingInfo.Settings.Directories.size())
				return SetNextRecordingDirectory();
			return false;
		}

		return true;
	}

	bool CTSTaskCore::ChangeRecordingFile(LPCWSTR pszFileName)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		OutLog(LOG_INFO,L"録画ファイルを切り替えます。(%s)",pszFileName);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State==RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"録画中ではありません。");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [pszFileName](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnRecordingFileChange(pszFileName);
							   })) {
			OutLog(LOG_INFO,L"録画ファイルの切り替えがキャンセルされました。");
			return false;
		}

		String Directory;
		Directory=pszFileName;
		PathUtility::RemoveFileName(&Directory);
		if (!PathUtility::IsExists(Directory)) {
			OutLog(LOG_INFO,L"フォルダ \"%s\" が存在しないため作成します。",Directory.c_str());
			int Error=::SHCreateDirectory(nullptr,Directory.c_str());
			if (Error!=ERROR_SUCCESS && Error!=ERROR_ALREADY_EXISTS) {
				OutSystemErrorLog(Error,L"フォルダ \"%s\" を作成できません。",
								  Directory.c_str());
			}
		}

		if (!m_DtvEngine.m_TsRecorder.RelayFile(pszFileName)) {
			OutBonTsEngineErrorLog(m_DtvEngine.m_TsRecorder);

			for (auto e:m_EventHandlerList)
				e->OnRecordingFileChangeFailed(pszFileName);

			return false;
		}

		m_RecordingInfo.FilePaths.push_back(String(pszFileName));

		OutLog(LOG_VERBOSE,L"録画ファイルが変更されたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnRecordingFileChanged(pszFileName);

		return true;
	}

	bool CTSTaskCore::GetRecordingInfo(RecordingInfo *pInfo) const
	{
		if (pInfo==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		*pInfo=m_RecordingInfo;

		return true;
	}

	bool CTSTaskCore::IsRecording() const
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		return m_RecordingInfo.State!=RECORDING_STATE_NOT_RECORDING;
	}

	bool CTSTaskCore::FormatRecordFileName(LPCWSTR pszFormat,String *pFileName,
											unsigned int UseNextEventInfoMargin)
	{
		if (IsStringEmpty(pszFormat) || pFileName==nullptr)
			return false;

		CEventVariableStringMap::EventInfo Info;
		if (!GetChannel(&Info.Channel))
			return false;

		if (GetEventInfo(&Info.Event)) {
			if (UseNextEventInfoMargin>0 && Info.Event.Duration>0) {
				SYSTEMTIME stEnd,stCurrent;

				stEnd=Info.Event.StartTime;
				OffsetSystemTime(&stEnd,Info.Event.Duration*1000);
				if (!m_DtvEngine.m_TsAnalyzer.GetTotTime(&stCurrent))
					::GetLocalTime(&stCurrent);
				if (SystemTimeSpan(stCurrent,stEnd)<=(LONGLONG)UseNextEventInfoMargin*1000LL) {
					EventInfo NextEventInfo;
					if (GetEventInfo(&NextEventInfo,true))
						Info.Event=NextEventInfo;
				}
			}
		} else {
			Info.Event.EventID=0;
			::ZeroMemory(&Info.Event.StartTime,sizeof(SYSTEMTIME));
			Info.Event.Duration=0;
			Info.Event.EventName.clear();
			Info.Event.EventText.clear();
		}

		CEventVariableStringMap StringMap(Info);
		if (!FormatVariableString(pFileName,&StringMap,pszFormat))
			return false;

		return true;
	}

	bool CTSTaskCore::SetWriteBufferSize(unsigned int Size)
	{
		if (Size<188)
			return false;

		OutLog(LOG_INFO,L"ファイルへの書き出しサイズを%uバイトに設定します。",Size);

		return m_DtvEngine.m_TsRecorder.SetBufferSize(Size);
	}

	bool CTSTaskCore::SetMaxWritePendingSize(unsigned int Size)
	{
		OutLog(LOG_INFO,L"ファイルへの書き出し待ち最大サイズを%uバイトに設定します。",Size);

		return m_DtvEngine.m_TsRecorder.SetMaxPendingSize(Size);
	}

	bool CTSTaskCore::SetWritePreAllocate(ULONGLONG PreAllocateSize)
	{
		if (PreAllocateSize>0)
			OutLog(LOG_INFO,L"ファイルの事前確保サイズを%lluバイトに設定します。",PreAllocateSize);
		else
			OutLog(LOG_INFO,L"ファイルの事前確保を無効にします。");

		return m_DtvEngine.m_TsRecorder.SetPreAllocationUnit(PreAllocateSize);
	}

	bool CTSTaskCore::SetMinDiskFreeSpace(ULONGLONG MinFreeSpace)
	{
		OutLog(LOG_INFO,L"最低空き容量を%lluバイトに設定します。",MinFreeSpace);

		m_MinDiskFreeSpace=MinFreeSpace;

		return true;
	}

	bool CTSTaskCore::GetStreamIDs(ChannelStreamIDs *pStreamIDs)
	{
		if (pStreamIDs==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		pStreamIDs->NetworkID=m_DtvEngine.m_TsAnalyzer.GetNetworkID();
		pStreamIDs->TransportStreamID=m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();
		//pStreamIDs->ServiceID=m_CurrentChannel.ServiceID!=SERVICE_ID_INVALID?m_CurrentChannel.ServiceID:0;
		if (!m_DtvEngine.GetServiceID(&pStreamIDs->ServiceID))
			pStreamIDs->ServiceID=0;

		return true;
	}

	bool CTSTaskCore::GetStreamStatistics(StreamStatistics *pStatistics)
	{
		if (pStatistics==nullptr)
			return false;

		pStatistics->SignalLevel=m_DtvEngine.m_BonSrcDecoder.GetSignalLevel();
		pStatistics->BitRate=m_DtvEngine.m_BonSrcDecoder.GetBitRate();
		pStatistics->InputPacketCount=m_DtvEngine.m_TsPacketParser.GetInputPacketCount();
		pStatistics->ErrorPacketCount=m_DtvEngine.m_TsPacketParser.GetErrorPacketCount();
		pStatistics->DiscontinuityCount=m_DtvEngine.m_TsPacketParser.GetContinuityErrorPacketCount();
		pStatistics->ScramblePacketCount=m_DtvEngine.m_TsPacketCounter.GetScrambledPacketCount();

		return true;
	}

	bool CTSTaskCore::GetTotTime(SYSTEMTIME *pTime)
	{
		if (pTime==nullptr)
			return false;

		return m_DtvEngine.m_TsAnalyzer.GetTotTime(pTime);
	}

	bool CTSTaskCore::ResetErrorStatistics()
	{
		OutLog(LOG_VERBOSE,L"エラーカウントをリセットします。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		m_DtvEngine.m_TsPacketParser.ResetErrorPacketCount();
		m_DtvEngine.m_TsPacketCounter.ResetScrambledPacketCount();

		for (auto e:m_EventHandlerList)
			e->OnErrorStatisticsReset();

		return true;
	}

	bool CTSTaskCore::GetEventInfo(EventInfo *pInfo,bool fNext)
	{
		if (pInfo==nullptr)
			return false;

		::CEventInfo Event;

		if (!m_DtvEngine.GetEventInfo(&Event,fNext))
			return false;

		pInfo->EventID=Event.m_EventID;
		if (Event.m_bValidStartTime)
			pInfo->StartTime=Event.m_StartTime;
		else
			::ZeroMemory(&pInfo->StartTime,sizeof(SYSTEMTIME));
		pInfo->Duration=Event.m_Duration;
		pInfo->EventName=Event.m_EventName;
		pInfo->EventText=Event.m_EventText;

		return true;
	}

	bool CTSTaskCore::GetBonDriverChannelList(BonDriverTuningSpaceList *pSpaceList)
	{
		if (pSpaceList==nullptr)
			return false;

		pSpaceList->clear();

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		LPCTSTR pszName;
		for (DWORD Space=0;(pszName=m_DtvEngine.m_BonSrcDecoder.GetSpaceName(Space))!=nullptr;Space++) {
			pSpaceList->resize(pSpaceList->size()+1);
			BonDriverTuningSpaceInfo &SpaceInfo=pSpaceList->back();
			SpaceInfo.Name=pszName;
			for (DWORD Channel=0;(pszName=m_DtvEngine.m_BonSrcDecoder.GetChannelName(Space,Channel))!=nullptr;Channel++)
				SpaceInfo.ChannelList.push_back(String(pszName));
		}

		return true;
	}

	bool CTSTaskCore::GetScannedChannelList(CTuningSpaceList *pSpaceList) const
	{
		if (pSpaceList==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		*pSpaceList=m_TuningSpaceList;

		return true;
	}

	bool CTSTaskCore::GetScannedChannelList(int Space,CChannelList *pChannelList) const
	{
		if (pChannelList==nullptr)
			return false;

		pChannelList->Clear();

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		const CChannelList *pList=m_TuningSpaceList.GetChannelList(Space);
		if (pList==nullptr)
			return false;

		*pChannelList=*pList;

		return true;
	}

	bool CTSTaskCore::StartStreaming(const StreamingInfo &Info)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		if ((Info.Address.Protocol!=PROTOCOL_UDP && Info.Address.Protocol!=PROTOCOL_TCP)
				|| Info.Address.Address.empty())
			return false;

		if (!m_fInitialized)
			return false;

		if (!StopStreaming())
			return false;

		OutLog(LOG_INFO,L"ストリーミングを開始します。(%s %s:%d)",
			   Streaming::GetProtocolText(Info.Address.Protocol),
			   Info.Address.Address.c_str(),Info.Address.Port);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!m_StreamingManager.Initialize()) {
			OutLog(LOG_ERROR,L"ストリーミングの初期化ができません。");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Info](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnStreamingStart(Info);
							   })) {
			OutLog(LOG_INFO,L"ストリーミングの開始がキャンセルされました。");
			return false;
		}

		IPAddress IPAddr;
		if (!Streaming::GetIPAddress(Info.Address,&IPAddr)) {
			OutLog(LOG_ERROR,L"IPアドレスを取得できません。");
			return false;
		}

		WORD Port=Info.Address.Port;

		if (Info.fFindUnusedPort) {
			if (!Streaming::CreateAddressMutex(&m_StreamingMutex,Info.Address.Protocol,IPAddr,&Port)) {
				OutLog(LOG_ERROR,L"Mutexを作成できません。");
				return false;
			}
			if (Port!=Info.Address.Port) {
				OutLog(LOG_INFO,L"ポート番号%uが既に使用されているため、%uに変更します。",Info.Address.Port,Port);
			}
		} else {
			if (!Streaming::CreateAddressMutex(&m_StreamingMutex,Info.Address.Protocol,IPAddr,Port)) {
				OutLog(LOG_ERROR,L"Mutexを作成できません。");
				return false;
			}
		}

		OutLog(LOG_VERBOSE,L"送信対象ストリームを設定します。(0x%x) 字幕: 送信%s / データ放送: 送信%s",
			   Info.Streams & STREAM_ALL,
			   (Info.Streams & STREAM_CAPTION)?L"する":L"しない",
			   (Info.Streams & STREAM_DATA_CARROUSEL)?L"する":L"しない");
#ifdef BONTSENGINE_1SEG_SUPPORT
		if ((Info.Streams & STREAM_1SEG)!=0) {
			OutLog(LOG_VERBOSE,L"ワンセグサービスを送信対象に設定します。");
			m_DtvEngine.SetSendStream(::CDtvEngine::SID_1SEG,GetTsSelectorStreamFlags(Info.Streams));
		} else
#endif
		{
			bool fCurServiceOnly=Info.ServiceSelect==SERVICE_SELECT_CURRENT;
			OutLog(LOG_VERBOSE,L"%sサービスを送信対象に設定します。",
				   fCurServiceOnly?L"指定":L"全ての");
			m_DtvEngine.SetSendCurServiceOnly(fCurServiceOnly,GetTsSelectorStreamFlags(Info.Streams));
		}

		::CTsNetworkSender::AddressInfo Address;
		Address.Type=
			Info.Address.Protocol==PROTOCOL_UDP?
				CTsNetworkSender::SOCKET_UDP:
				CTsNetworkSender::SOCKET_TCP;
		Address.pszAddress=Info.Address.Address.c_str();
		Address.Port=Port;

		if (!m_DtvEngine.m_TsNetworkSender.Open(&Address,1)) {
			OutBonTsEngineErrorLog(m_DtvEngine.m_TsNetworkSender);
			OutLog(LOG_ERROR,L"ストリーミングを開始できません。");
			m_StreamingMutex.Close();
			return false;
		}

		m_StreamingInfo=Info;
		m_StreamingInfo.Address.Port=Port;

		OutLog(LOG_VERBOSE,L"ストリーミングが開始されたことを通知します。");

		for (auto e:m_EventHandlerList)
			e->OnStreamingStarted(Info);

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::StopStreaming()
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_DtvEngine.m_TsNetworkSender.IsOpen()) {
			OutLog(LOG_INFO,L"ストリーミングを停止します。");

			if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
								   [](TSTask::CEventHandler *pHandler) {
								       return pHandler->OnStreamingStop();
								   })) {
				OutLog(LOG_INFO,L"ストリーミングの停止がキャンセルされました。");
				return false;
			}

			m_DtvEngine.m_TsNetworkSender.Close();

			m_StreamingMutex.Close();

			OutLog(LOG_VERBOSE,L"ストリーミングが停止したことを通知します。");

			for (auto e:m_EventHandlerList)
				e->OnStreamingStopped();
		}

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::GetStreamingInfo(StreamingInfo *pInfo) const
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		if (pInfo==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!m_DtvEngine.m_TsNetworkSender.IsOpen())
			return false;

		*pInfo=m_StreamingInfo;

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::IsStreaming() const
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		return m_DtvEngine.m_TsNetworkSender.IsOpen();
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingSendSize(unsigned int Size)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		if (Size==0)
			return false;

		OutLog(LOG_INFO,L"ストリーム送信サイズを%uパケット(%uバイト)に設定します。",Size,Size*TS_PACKET_SIZE);

		return m_DtvEngine.m_TsNetworkSender.SetSendSize(Size);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingSendWait(unsigned int Wait,bool fAdjustWait)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"ストリーム送信間隔を%ums(調整%s)に設定します。",Wait,fAdjustWait?L"あり":L"なし");

		m_DtvEngine.m_TsNetworkSender.SetSendWait(Wait);
		m_DtvEngine.m_TsNetworkSender.SetAdjustWait(fAdjustWait);

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingConnectRetryInterval(unsigned int Interval)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"接続リトライ間隔を%umsに設定します。",Interval);

		m_DtvEngine.m_TsNetworkSender.SetConnectRetryInterval(Interval);

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingMaxConnectRetries(int MaxRetries)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"接続リトライ回数を%d回に設定します。",MaxRetries);

		return m_DtvEngine.m_TsNetworkSender.SetMaxConnectRetries(MaxRetries);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingTcpMaxSendRetries(int MaxRetries)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"TCP送信リトライ回数を%d回に設定します。",MaxRetries);

		return m_DtvEngine.m_TsNetworkSender.SetTcpMaxSendRetries(MaxRetries);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingTcpPrependHeader(bool fPrependHeader)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"TCP送信でヘッダを付加%sように設定します。",fPrependHeader?L"する":L"しない");

		m_DtvEngine.m_TsNetworkSender.SetTcpPrependHeader(fPrependHeader);

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::CreateStreamPool(LPCWSTR pszName,DWORD BufferLength)
	{
		if (!m_fInitialized)
			return false;

		if (!m_StreamPool.IsCreated()) {
			if (!m_StreamPool.Create(pszName,BufferLength))
				return false;

			m_DtvEngine.m_TsGrabber.AddTsHandler(&m_StreamPool);
		}

		m_StreamPoolCount++;

		return true;
	}

	bool CTSTaskCore::CloseStreamPool()
	{
		if (m_StreamPoolCount>0) {
			m_StreamPoolCount--;
			if (m_StreamPoolCount==0) {
				m_DtvEngine.m_TsGrabber.RemoveTsHandler(&m_StreamPool);
				m_StreamPool.End();
				m_StreamPool.Close();
			}
		}

		return true;
	}

	bool CTSTaskCore::AddEventHandler(TSTask::CEventHandler *pEventHandler)
	{
		OutLog(LOG_VERBOSE,L"イベントハンドラを追加します。(%p)",pEventHandler);

		if (pEventHandler==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (std::find(m_EventHandlerList.begin(),m_EventHandlerList.end(),pEventHandler)==m_EventHandlerList.end())
			m_EventHandlerList.push_back(pEventHandler);
		else
			OutLog(LOG_WARNING,L"イベントハンドラ(%p)は既に登録されています。",pEventHandler);

		return true;
	}

	bool CTSTaskCore::RemoveEventHandler(TSTask::CEventHandler *pEventHandler)
	{
		OutLog(LOG_VERBOSE,L"イベントハンドラを削除します。(%p)",pEventHandler);

		if (pEventHandler==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=std::find(m_EventHandlerList.begin(),m_EventHandlerList.end(),pEventHandler);
		if (i==m_EventHandlerList.end()) {
			OutLog(LOG_WARNING,L"イベントハンドラ(%p)が登録されていません。",pEventHandler);
			return false;
		}

		m_EventHandlerList.erase(i);

		return true;
	}

	bool CTSTaskCore::AddTsGrabber(ITsGrabber *pGrabber)
	{
		OutLog(LOG_VERBOSE,L"TSグラバーを追加します。(%p)",pGrabber);

		if (pGrabber==nullptr)
			return false;

		return m_DtvEngine.m_TsGrabber.AddTsHandler(pGrabber);
	}

	bool CTSTaskCore::RemoveTsGrabber(ITsGrabber *pGrabber)
	{
		OutLog(LOG_VERBOSE,L"TSグラバーを削除します。(%p)",pGrabber);

		if (pGrabber==nullptr)
			return false;

		return m_DtvEngine.m_TsGrabber.RemoveTsHandler(pGrabber);
	}

	void CTSTaskCore::OutTimeoutErrorLog() const
	{
		OutLog(LOG_ERROR,L"タイムアウトしました。(%u ms)",m_LockTimeout);
	}

	void CTSTaskCore::OutBonTsEngineErrorLog(const ::CBonErrorHandler &ErrorHandler) const
	{
		LPCWSTR pszText=ErrorHandler.GetLastErrorText();
		if (!IsStringEmpty(pszText)) {
			LPCWSTR pszAdvise=ErrorHandler.GetLastErrorAdvise();
			if (!IsStringEmpty(pszAdvise))
				OutLog(LOG_ERROR,L"%s(%s)",pszText,pszAdvise);
			else
				OutLog(LOG_ERROR,L"%s",pszText);
		}
	}

	void CTSTaskCore::OnServiceChanged(WORD ServiceID)
	{
		OutLog(LOG_VERBOSE,L"サービスが変更されました。(SID 0x%x)",ServiceID);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return;
		}

		m_CurrentChannel.ServiceID=ServiceID;

		for (auto e:m_EventHandlerList)
			e->OnServiceChanged(ServiceID);
	}

	void CTSTaskCore::OnServiceListUpdated(::CTsAnalyzer *pTsAnalyzer,bool bStreamChanged)
	{
		if (bStreamChanged) {
			OutLog(LOG_VERBOSE,L"ストリームが変更されました。");

			CTryBlockLock Lock(m_Lock);
			if (!Lock.TryLock(m_LockTimeout)) {
				OutTimeoutErrorLog();
				return;
			}

			for (auto e:m_EventHandlerList)
				e->OnStreamChanged();
		}
	}

	void CTSTaskCore::OnEventUpdated(::CTsAnalyzer *pTsAnalyzer)
	{
		OutLog(LOG_VERBOSE,L"番組情報が更新されました。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return;
		}

		if (m_CurrentChannel.ServiceID==SERVICE_ID_INVALID)
			return;

		int Service=m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(m_CurrentChannel.ServiceID);
		if (Service<0)
			return;

		EventStatus Status;
		Status.EventID=m_DtvEngine.GetEventID();
		Status.Duration=m_DtvEngine.GetEventDuration();

		if (Status!=m_CurrentEvent) {
			m_CurrentEvent=Status;

			for (auto e:m_EventHandlerList)
				e->OnEventChanged();
		}
	}

	void CTSTaskCore::OnTotUpdated(::CTsAnalyzer *pTsAnalyzer)
	{
		//OutLog(LOG_VERBOSE,L"TOTが更新されました。");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return;
		}

		for (auto e:m_EventHandlerList)
			e->OnTotChanged();
	}

	void CTSTaskCore::OnFileWriteError(::CTsRecorder *pTsRecorder,DWORD ErrorCode)
	{
		OutSystemErrorLog(ErrorCode,L"ファイルの書き出しでエラーが発生しました。");

		SetNextRecordingDirectory();
	}

	void CTSTaskCore::OnTrace(::CTracer::TraceType Type,LPCTSTR pszOutput)
	{
		if (!IsStringEmpty(pszOutput)) {
			LogType Log;

			switch (Type) {
			case ::CTracer::TYPE_INFORMATION:
				Log=LOG_INFO;
				break;
			case ::CTracer::TYPE_WARNING:
				Log=LOG_WARNING;
				break;
			case ::CTracer::TYPE_ERROR:
				Log=LOG_ERROR;
				break;
			default:
				Log=LOG_VERBOSE;
				break;
			}

			OutLog(Log,L"%s",pszOutput);
		}
	}

}
