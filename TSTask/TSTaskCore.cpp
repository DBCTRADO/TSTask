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
		OutLog(LOG_VERBOSE,L"Core�̏I���������s���܂��B");

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

		OutLog(LOG_IMPORTANT,L"BonDrvier(%s)�����[�h���܂��B",pszFileName);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [pszFileName](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnBonDriverLoad(pszFileName);
							   })) {
			OutLog(LOG_INFO,L"BonDriver�̃��[�h���L�����Z������܂����B");
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
					OutLog(LOG_WARNING,L"�`�����l���t�@�C��(%s/.ch2)�����t����܂���B",ChannelFileName.c_str());
					PathUtility::RenameExtension(&ChannelFileName,L".ch2");
				}
			}
			if (!m_TuningSpaceList.LoadFromFile(ChannelFileName.c_str())) {
				OutLog(LOG_ERROR,L"�`�����l���t�@�C�� \"%s\" �̓ǂݍ��݂��ł��܂���B",ChannelFileName.c_str());
			} else {
				OutLog(LOG_INFO,L"�`�����l���t�@�C�� \"%s\" ��ǂݍ��݂܂����B",ChannelFileName.c_str());
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

		OutLog(LOG_VERBOSE,L"BonDriver(%s)�����[�h���ꂽ���Ƃ�ʒm���܂��B",pszFileName);

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

		OutLog(LOG_IMPORTANT,L"BonDrvier���A�����[�h���܂��B");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnBonDriverUnload();
							   })) {
			OutLog(LOG_INFO,L"BonDriver�̃A�����[�h���L�����Z������܂����B");
			return false;
		}

		m_DtvEngine.UnloadBonDriver();

		m_TuningSpaceList.Clear();

		OutLog(LOG_VERBOSE,L"BonDriver���A�����[�h���ꂽ���Ƃ�ʒm���܂��B");

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

		OutLog(LOG_IMPORTANT,L"�`���[�i�[���J���܂��B");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnTunerOpen(); })) {
			OutLog(LOG_INFO,L"�`���[�i�[�I�[�v�����L�����Z������܂����B");
			return false;
		}

		if (!m_DtvEngine.OpenTuner()) {
			OutBonTsEngineErrorLog(m_DtvEngine);

			for (auto e:m_EventHandlerList)
				e->OnTunerOpenFailed();

			return false;
		}

		OutLog(LOG_VERBOSE,L"�`���[�i�[���J���ꂽ���Ƃ�ʒm���܂��B");

		for (auto e:m_EventHandlerList)
			e->OnTunerOpened();

		return true;
	}

	bool CTSTaskCore::CloseTuner()
	{
		if (!IsTunerOpened())
			return true;

		OutLog(LOG_IMPORTANT,L"�`���[�i�[����܂��B");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnTunerClose(); })) {
			OutLog(LOG_INFO,L"�`���[�i�[�N���[�Y���L�����Z������܂����B");
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

		OutLog(LOG_VERBOSE,L"�`���[�i�[������ꂽ���Ƃ�ʒm���܂��B");

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
		OutLog(LOG_INFO,L"�ŏ��̃`�����l���ݒ�܂ł̍ŏ��Ԋu��%ums�ɐݒ肵�܂��B",Delay);

		return m_DtvEngine.m_BonSrcDecoder.SetFirstChannelSetDelay(Delay);
	}

	bool CTSTaskCore::SetMinChannelChangeInterval(unsigned int Interval)
	{
		OutLog(LOG_INFO,L"�ŏ��`�����l���ύX�Ԋu��%ums�ɐݒ肵�܂��B",Interval);

		return m_DtvEngine.m_BonSrcDecoder.SetMinChannelChangeInterval(Interval);
	}

	bool CTSTaskCore::SetChannelByIndex(DWORD Space,DWORD Channel,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_IMPORTANT,L"�`�����l����ݒ肵�܂��B(�`���[�j���O��� %u / �`�����l�� %u / �T�[�r�X %u / �����ύX %s)",
			   Space,Channel,ServiceID,fForce?L"����":L"���Ȃ�");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!fForce && m_CurrentChannel.Space==Space && m_CurrentChannel.Channel==Channel) {
			OutLog(LOG_VERBOSE,L"�w�肳�ꂽ�`�����l���͊��ɑI�΂�Ă��邽�߁A�T�[�r�X�̂ݐݒ肵�܂��B");
			return SetService(ServiceID);
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Space,Channel,ServiceID](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnChannelChange(Space,Channel,ServiceID);
							   })) {
			OutLog(LOG_INFO,L"�`�����l���ݒ肪�L�����Z������܂����B");
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
				OutLog(LOG_VERBOSE,L"�w�肳�ꂽ�`�����l���Ɉ�v����X�L�����ς݂̃`�����l��(%d)�����t����܂����B",Index);
				m_CurrentScannedChannel.Space=(int)Space;
				m_CurrentScannedChannel.Channel=Index;
				m_CurrentScannedChannel.ServiceID=m_CurrentChannel.ServiceID;
			} else {
				OutLog(LOG_VERBOSE,L"�w�肳�ꂽ�`�����l���Ɉ�v����X�L�����ς݂̃`�����l���͂���܂���B");
			}
		}

		m_CurrentEvent.EventID=0;
		m_CurrentEvent.Duration=0;

		OutLog(LOG_VERBOSE,L"�`�����l�����ύX���ꂽ���Ƃ�ʒm���܂��B");

		for (auto e:m_EventHandlerList)
			e->OnChannelChanged(Space,Channel,ServiceID);

		return true;
	}

	bool CTSTaskCore::SetChannelByScanned(int Space,int Channel,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_VERBOSE,L"�X�L�����ς݂̃`�����l����I�����܂��B(�`���[�j���O��� %d / �`�����l�� %d / �T�[�r�X %u)",
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
				OutLog(LOG_ERROR,L"�w�肳�ꂽ�`�����l��(%d)���s���ł��B",Channel);
			}
		} else {
			OutLog(LOG_ERROR,L"�w�肳�ꂽ�`���[�j���O���(%d)���s���ł��B",Space);
		}

		return false;
	}

	bool CTSTaskCore::SetChannelByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fForce)
	{
		OutLog(LOG_INFO,L"�w�肳�ꂽID�Ɉ�v����`�����l����I�����܂��B(NID 0x%x / TSID 0x%x / SID 0x%x)",
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

		OutLog(LOG_ERROR,L"�w�肳�ꂽID(NID 0x%x / TSID 0x%x / SID 0x%x)�Ɉ�v����`�����l��������܂���B",
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
		OutLog(LOG_IMPORTANT,L"�T�[�r�X��ݒ肵�܂��B(SID 0x%x)",ServiceID);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_CurrentChannel.Space==TuningChannelInfo::SPACE_INVALID) {
			OutLog(LOG_ERROR,L"�`�����l�����I������Ă��܂���B");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [ServiceID](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnServiceChange(ServiceID);
							   })) {
			OutLog(LOG_INFO,L"�T�[�r�X�ݒ肪�L�����Z������܂����B");
			return false;
		}

		if (!m_DtvEngine.SetServiceByID(ServiceID)) {
			OutLog(LOG_ERROR,L"�T�[�r�X�̐ݒ肪�ł��܂���B");
			return false;
		}

		m_CurrentChannel.ServiceID=ServiceID;

		const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(m_CurrentChannel.Space);
		if (pChannelList!=nullptr) {
			int Index=pChannelList->Find((int)m_CurrentChannel.Space,(int)m_CurrentChannel.Channel,ServiceID);
			if (Index>=0) {
				OutLog(LOG_VERBOSE,L"�w�肳�ꂽ�T�[�r�X�Ɉ�v����X�L�����ς݂̃`�����l��(%d)�����t����܂����B",Index);
				m_CurrentScannedChannel.Channel=Index;
				m_CurrentScannedChannel.ServiceID=ServiceID;
			} else {
				OutLog(LOG_VERBOSE,L"�w�肳�ꂽ�T�[�r�X�Ɉ�v����X�L�����ς݂̃`�����l���͂���܂���B");
			}
		}

		// CTSTaskCore::OnServiceChanged() ���Ă΂��̂�҂�
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
			OutLog(LOG_ERROR,L"�t�@�C���p�X���w�肳��Ă��Ȃ����ߘ^����J�n�ł��܂���B");
			return false;
		}

		if (!m_fInitialized)
			return false;

		OutLog(LOG_IMPORTANT,L"�^����J�n���܂��B(%s)",Settings.FileName.c_str());

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State!=RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"���ɘ^�撆�̂��ߐV�����^����J�n�ł��܂���B");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Settings](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnRecordingStart(Settings);
							   })) {
			OutLog(LOG_INFO,L"�^��J�n���L�����Z������܂����B");
			return false;
		}

		bool fCurServiceOnly=Settings.ServiceSelect==SERVICE_SELECT_CURRENT;
#ifdef BONTSENGINE_1SEG_SUPPORT
		if ((Settings.Streams & STREAM_1SEG)!=0) {
			OutLog(LOG_INFO,L"�����Z�O�T�[�r�X��I�����܂��B");
			m_DtvEngine.SetServiceByID(::CDtvEngine::SID_1SEG,true);
			fCurServiceOnly=true;
		} else
#endif
		if (m_CurrentChannel.ServiceID!=SERVICE_ID_INVALID) {
			OutLog(LOG_VERBOSE,L"�T�[�r�X��I�����܂��B(%d)",m_CurrentChannel.ServiceID);
			m_DtvEngine.SetServiceByID(m_CurrentChannel.ServiceID,true);
		} else {
			m_DtvEngine.SetService(::CDtvEngine::SERVICE_INVALID);
		}

		OutLog(LOG_VERBOSE,L"%s�T�[�r�X��^��Ώۂɐݒ肵�܂��B",
			   fCurServiceOnly?L"�w��":L"�S�Ă�");
		OutLog(LOG_VERBOSE,L"�^��ΏۃX�g���[����ݒ肵�܂��B(0x%x) ����: �ۑ�%s / �f�[�^����: �ۑ�%s",
			   Settings.Streams & STREAM_ALL,
			   (Settings.Streams & STREAM_CAPTION)?L"����":L"���Ȃ�",
			   (Settings.Streams & STREAM_DATA_CARROUSEL)?L"����":L"���Ȃ�");
		m_DtvEngine.SetWriteCurServiceOnly(fCurServiceOnly,GetTsSelectorStreamFlags(Settings.Streams));

		for (size_t i=0;i<Settings.Directories.size();i++)
			OutLog(LOG_VERBOSE,L"�^���t�H���_%Iu : \"%s\"",i+1,Settings.Directories[i].c_str());

		String FilePath;
		size_t DirIndex;
		for (DirIndex=0;DirIndex<Settings.Directories.size();DirIndex++) {
			FilePath=Settings.Directories[DirIndex];

			if (!PathUtility::IsExists(FilePath)) {
				OutLog(LOG_INFO,L"�t�H���_ \"%s\" �����݂��Ȃ����ߍ쐬���܂��B",FilePath.c_str());
				int Error=::SHCreateDirectory(nullptr,FilePath.c_str());
				if (Error!=ERROR_SUCCESS && Error!=ERROR_ALREADY_EXISTS) {
					OutSystemErrorLog(Error,L"�t�H���_ \"%s\" ���쐬�ł��܂���B",
									  FilePath.c_str());
					// �ꉞ�t�@�C���̃I�[�v���͎����Ă݂�
				}
			}

#if 0
			if (m_MinDiskFreeSpace>0) {
				PathUtility::AppendDelimiter(&FilePath);
				ULARGE_INTEGER FreeSpace;
				if (::GetDiskFreeSpaceEx(FilePath.c_str(),&FreeSpace,NULL,NULL)) {
					if (FreeSpace.QuadPart<m_MinDiskFreeSpace) {
						OutLog(LOG_WARNING,L"�t�H���_ \"%s\" �̋󂫗e�ʂ��s�����Ă��܂��B(%llu MiB)",
							   FilePath.c_str(),FreeSpace.QuadPart/(1024*1024));
						continue;
					}
				} else {
					OutLog(LOG_WARNING,L"�t�H���_ \"%s\" �̋󂫗e�ʂ��擾�ł��܂���B(Error 0x%x)",
						   FilePath.c_str(),::GetLastError());
					//continue;
				}
			}
#endif

			PathUtility::Append(&FilePath,Settings.FileName.c_str());

			OutLog(LOG_INFO,L"�t�@�C�� \"%s\" ���J���܂��B",FilePath.c_str());

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

		OutLog(LOG_VERBOSE,L"�^�悪�J�n���ꂽ���Ƃ�ʒm���܂��B");

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

		OutLog(LOG_IMPORTANT,L"�^����~���܂��B");

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [](TSTask::CEventHandler *pHandler) { return pHandler->OnRecordingStop(); })) {
			OutLog(LOG_INFO,L"�^���~���L�����Z������܂����B");
			return false;
		}

		m_DtvEngine.m_TsRecorder.CloseFile();

		m_RecordingInfo.State=RECORDING_STATE_NOT_RECORDING;

		::CTsRecorder::WriteStatistics Statistics;
		m_DtvEngine.m_TsRecorder.GetWriteStatistics(&Statistics);
		OutLog(LOG_INFO,L"�^����~���܂����B(���� %llu �o�C�g / �����o�� %llu �o�C�g / �����o���� %llu / �����o���G���[�� %u)",
			   Statistics.InputSize,Statistics.OutputSize,Statistics.OutputCount,Statistics.WriteErrorCount);

		OutLog(LOG_VERBOSE,L"�^�悪��~�������Ƃ�ʒm���܂��B");

		for (auto e:m_EventHandlerList)
			e->OnRecordingStopped();

		return true;
	}

	bool CTSTaskCore::SetNextRecordingDirectory()
	{
		OutLog(LOG_INFO,L"�^������̃t�H���_�ɐ؂�ւ��܂��B");

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State==RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"�^�撆�ł͂���܂���B");
			return false;
		}

		if (m_RecordingInfo.Settings.Directories.size()<=(size_t)(m_RecordingInfo.CurDirectory+1)) {
			OutLog(LOG_WARNING,L"���̃t�H���_���w�肳��Ă��܂���B");
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
						OutLog(LOG_INFO,L"�t�H���_ \"%s\" �ɋ󂫂� %llu MiB ���邽�ߘ^���ɐݒ肵�܂��B",
							   Directory.c_str(),FreeSpace.QuadPart/(1024*1024));
						break;
					}
					OutLog(LOG_WARNING,L"�t�H���_ \"%s\" �̋󂫗e�ʂ��s�����Ă��܂��B(%llu MiB)",
						   Directory.c_str(),FreeSpace.QuadPart/(1024*1024));
				} else {
					OutLog(LOG_WARNING,L"�t�H���_ \"%s\" �̋󂫗e�ʂ��擾�ł��܂���B(Error 0x%x)",
						   Directory.c_str(),::GetLastError());
				}
			}
		}
		if (i==m_RecordingInfo.Settings.Directories.size()) {
			OutLog(LOG_ERROR,L"���p�\�Ș^��悪����܂���B");
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
			OutLog(LOG_VERBOSE,L"�p�X \"%s\" �����ɑ��݂��邽�߁A�t�@�C������ύX���܂��B",FilePath.c_str());
			for (unsigned int j=1;;j++) {
				StringUtility::Format(FilePath,L"%s.part%u(%u)%s",
									  BasePath.c_str(),PartNo,j,Extension.c_str());
				if (!PathUtility::IsExists(FilePath))
					break;
				if (j==1000) {
					OutLog(LOG_ERROR,L"���j�[�N�ȃt�@�C�����𐶐��ł��܂���B");
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

		OutLog(LOG_INFO,L"�^��t�@�C����؂�ւ��܂��B(%s)",pszFileName);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (m_RecordingInfo.State==RECORDING_STATE_NOT_RECORDING) {
			OutLog(LOG_ERROR,L"�^�撆�ł͂���܂���B");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [pszFileName](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnRecordingFileChange(pszFileName);
							   })) {
			OutLog(LOG_INFO,L"�^��t�@�C���̐؂�ւ����L�����Z������܂����B");
			return false;
		}

		String Directory;
		Directory=pszFileName;
		PathUtility::RemoveFileName(&Directory);
		if (!PathUtility::IsExists(Directory)) {
			OutLog(LOG_INFO,L"�t�H���_ \"%s\" �����݂��Ȃ����ߍ쐬���܂��B",Directory.c_str());
			int Error=::SHCreateDirectory(nullptr,Directory.c_str());
			if (Error!=ERROR_SUCCESS && Error!=ERROR_ALREADY_EXISTS) {
				OutSystemErrorLog(Error,L"�t�H���_ \"%s\" ���쐬�ł��܂���B",
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

		OutLog(LOG_VERBOSE,L"�^��t�@�C�����ύX���ꂽ���Ƃ�ʒm���܂��B");

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

		OutLog(LOG_INFO,L"�t�@�C���ւ̏����o���T�C�Y��%u�o�C�g�ɐݒ肵�܂��B",Size);

		return m_DtvEngine.m_TsRecorder.SetBufferSize(Size);
	}

	bool CTSTaskCore::SetMaxWritePendingSize(unsigned int Size)
	{
		OutLog(LOG_INFO,L"�t�@�C���ւ̏����o���҂��ő�T�C�Y��%u�o�C�g�ɐݒ肵�܂��B",Size);

		return m_DtvEngine.m_TsRecorder.SetMaxPendingSize(Size);
	}

	bool CTSTaskCore::SetWritePreAllocate(ULONGLONG PreAllocateSize)
	{
		if (PreAllocateSize>0)
			OutLog(LOG_INFO,L"�t�@�C���̎��O�m�ۃT�C�Y��%llu�o�C�g�ɐݒ肵�܂��B",PreAllocateSize);
		else
			OutLog(LOG_INFO,L"�t�@�C���̎��O�m�ۂ𖳌��ɂ��܂��B");

		return m_DtvEngine.m_TsRecorder.SetPreAllocationUnit(PreAllocateSize);
	}

	bool CTSTaskCore::SetMinDiskFreeSpace(ULONGLONG MinFreeSpace)
	{
		OutLog(LOG_INFO,L"�Œ�󂫗e�ʂ�%llu�o�C�g�ɐݒ肵�܂��B",MinFreeSpace);

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
		OutLog(LOG_VERBOSE,L"�G���[�J�E���g�����Z�b�g���܂��B");

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

		OutLog(LOG_INFO,L"�X�g���[�~���O���J�n���܂��B(%s %s:%d)",
			   Streaming::GetProtocolText(Info.Address.Protocol),
			   Info.Address.Address.c_str(),Info.Address.Port);

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		if (!m_StreamingManager.Initialize()) {
			OutLog(LOG_ERROR,L"�X�g���[�~���O�̏��������ł��܂���B");
			return false;
		}

		if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
							   [Info](TSTask::CEventHandler *pHandler) {
							       return pHandler->OnStreamingStart(Info);
							   })) {
			OutLog(LOG_INFO,L"�X�g���[�~���O�̊J�n���L�����Z������܂����B");
			return false;
		}

		IPAddress IPAddr;
		if (!Streaming::GetIPAddress(Info.Address,&IPAddr)) {
			OutLog(LOG_ERROR,L"IP�A�h���X���擾�ł��܂���B");
			return false;
		}

		WORD Port=Info.Address.Port;

		if (Info.fFindUnusedPort) {
			if (!Streaming::CreateAddressMutex(&m_StreamingMutex,Info.Address.Protocol,IPAddr,&Port)) {
				OutLog(LOG_ERROR,L"Mutex���쐬�ł��܂���B");
				return false;
			}
			if (Port!=Info.Address.Port) {
				OutLog(LOG_INFO,L"�|�[�g�ԍ�%u�����Ɏg�p����Ă��邽�߁A%u�ɕύX���܂��B",Info.Address.Port,Port);
			}
		} else {
			if (!Streaming::CreateAddressMutex(&m_StreamingMutex,Info.Address.Protocol,IPAddr,Port)) {
				OutLog(LOG_ERROR,L"Mutex���쐬�ł��܂���B");
				return false;
			}
		}

		OutLog(LOG_VERBOSE,L"���M�ΏۃX�g���[����ݒ肵�܂��B(0x%x) ����: ���M%s / �f�[�^����: ���M%s",
			   Info.Streams & STREAM_ALL,
			   (Info.Streams & STREAM_CAPTION)?L"����":L"���Ȃ�",
			   (Info.Streams & STREAM_DATA_CARROUSEL)?L"����":L"���Ȃ�");
#ifdef BONTSENGINE_1SEG_SUPPORT
		if ((Info.Streams & STREAM_1SEG)!=0) {
			OutLog(LOG_VERBOSE,L"�����Z�O�T�[�r�X�𑗐M�Ώۂɐݒ肵�܂��B");
			m_DtvEngine.SetSendStream(::CDtvEngine::SID_1SEG,GetTsSelectorStreamFlags(Info.Streams));
		} else
#endif
		{
			bool fCurServiceOnly=Info.ServiceSelect==SERVICE_SELECT_CURRENT;
			OutLog(LOG_VERBOSE,L"%s�T�[�r�X�𑗐M�Ώۂɐݒ肵�܂��B",
				   fCurServiceOnly?L"�w��":L"�S�Ă�");
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
			OutLog(LOG_ERROR,L"�X�g���[�~���O���J�n�ł��܂���B");
			m_StreamingMutex.Close();
			return false;
		}

		m_StreamingInfo=Info;
		m_StreamingInfo.Address.Port=Port;

		OutLog(LOG_VERBOSE,L"�X�g���[�~���O���J�n���ꂽ���Ƃ�ʒm���܂��B");

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
			OutLog(LOG_INFO,L"�X�g���[�~���O���~���܂��B");

			if (!ListUtility::Enum(m_EventHandlerList.begin(),m_EventHandlerList.end(),
								   [](TSTask::CEventHandler *pHandler) {
								       return pHandler->OnStreamingStop();
								   })) {
				OutLog(LOG_INFO,L"�X�g���[�~���O�̒�~���L�����Z������܂����B");
				return false;
			}

			m_DtvEngine.m_TsNetworkSender.Close();

			m_StreamingMutex.Close();

			OutLog(LOG_VERBOSE,L"�X�g���[�~���O����~�������Ƃ�ʒm���܂��B");

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

		OutLog(LOG_INFO,L"�X�g���[�����M�T�C�Y��%u�p�P�b�g(%u�o�C�g)�ɐݒ肵�܂��B",Size,Size*TS_PACKET_SIZE);

		return m_DtvEngine.m_TsNetworkSender.SetSendSize(Size);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingSendWait(unsigned int Wait,bool fAdjustWait)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"�X�g���[�����M�Ԋu��%ums(����%s)�ɐݒ肵�܂��B",Wait,fAdjustWait?L"����":L"�Ȃ�");

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
		OutLog(LOG_INFO,L"�ڑ����g���C�Ԋu��%ums�ɐݒ肵�܂��B",Interval);

		m_DtvEngine.m_TsNetworkSender.SetConnectRetryInterval(Interval);

		return true;
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingMaxConnectRetries(int MaxRetries)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"�ڑ����g���C�񐔂�%d��ɐݒ肵�܂��B",MaxRetries);

		return m_DtvEngine.m_TsNetworkSender.SetMaxConnectRetries(MaxRetries);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingTcpMaxSendRetries(int MaxRetries)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"TCP���M���g���C�񐔂�%d��ɐݒ肵�܂��B",MaxRetries);

		return m_DtvEngine.m_TsNetworkSender.SetTcpMaxSendRetries(MaxRetries);
#else
		return false;
#endif
	}

	bool CTSTaskCore::SetStreamingTcpPrependHeader(bool fPrependHeader)
	{
#ifdef DTVENGINE_NETWORK_SUPPORT
		OutLog(LOG_INFO,L"TCP���M�Ńw�b�_��t��%s�悤�ɐݒ肵�܂��B",fPrependHeader?L"����":L"���Ȃ�");

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
		OutLog(LOG_VERBOSE,L"�C�x���g�n���h����ǉ����܂��B(%p)",pEventHandler);

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
			OutLog(LOG_WARNING,L"�C�x���g�n���h��(%p)�͊��ɓo�^����Ă��܂��B",pEventHandler);

		return true;
	}

	bool CTSTaskCore::RemoveEventHandler(TSTask::CEventHandler *pEventHandler)
	{
		OutLog(LOG_VERBOSE,L"�C�x���g�n���h�����폜���܂��B(%p)",pEventHandler);

		if (pEventHandler==nullptr)
			return false;

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutTimeoutErrorLog();
			return false;
		}

		auto i=std::find(m_EventHandlerList.begin(),m_EventHandlerList.end(),pEventHandler);
		if (i==m_EventHandlerList.end()) {
			OutLog(LOG_WARNING,L"�C�x���g�n���h��(%p)���o�^����Ă��܂���B",pEventHandler);
			return false;
		}

		m_EventHandlerList.erase(i);

		return true;
	}

	bool CTSTaskCore::AddTsGrabber(ITsGrabber *pGrabber)
	{
		OutLog(LOG_VERBOSE,L"TS�O���o�[��ǉ����܂��B(%p)",pGrabber);

		if (pGrabber==nullptr)
			return false;

		return m_DtvEngine.m_TsGrabber.AddTsHandler(pGrabber);
	}

	bool CTSTaskCore::RemoveTsGrabber(ITsGrabber *pGrabber)
	{
		OutLog(LOG_VERBOSE,L"TS�O���o�[���폜���܂��B(%p)",pGrabber);

		if (pGrabber==nullptr)
			return false;

		return m_DtvEngine.m_TsGrabber.RemoveTsHandler(pGrabber);
	}

	void CTSTaskCore::OutTimeoutErrorLog() const
	{
		OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B(%u ms)",m_LockTimeout);
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
		OutLog(LOG_VERBOSE,L"�T�[�r�X���ύX����܂����B(SID 0x%x)",ServiceID);

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
			OutLog(LOG_VERBOSE,L"�X�g���[�����ύX����܂����B");

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
		OutLog(LOG_VERBOSE,L"�ԑg��񂪍X�V����܂����B");

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
		//OutLog(LOG_VERBOSE,L"TOT���X�V����܂����B");

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
		OutSystemErrorLog(ErrorCode,L"�t�@�C���̏����o���ŃG���[���������܂����B");

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
