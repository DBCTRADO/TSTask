#include "stdafx.h"
#include "TSTask.h"
#include "CoreMessageHandler.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CCoreMessageHandler::CCoreMessageHandler(CTSTaskCore &Core,CTSTaskAppCore &AppCore)
		: m_Core(Core)
		, m_AppCore(AppCore)
	{
	}

	CCoreMessageHandler::~CCoreMessageHandler()
	{
	}

	bool CCoreMessageHandler::SetHandler(CMessageMap *pMessageMap)
	{
		static const struct {
			LPCWSTR pszMessage;
			bool (CCoreMessageHandler::*Member)(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		} FunctorList[] = {
			{MESSAGE_Hello,						&CCoreMessageHandler::OnHello},
			{MESSAGE_EndTask,					&CCoreMessageHandler::OnEndTask},
			{MESSAGE_LoadBonDriver,				&CCoreMessageHandler::OnLoadBonDriver},
			{MESSAGE_UnloadBonDriver,			&CCoreMessageHandler::OnUnloadBonDriver},
			{MESSAGE_GetBonDriver,				&CCoreMessageHandler::OnGetBonDriver},
			{MESSAGE_OpenTuner,					&CCoreMessageHandler::OnOpenTuner},
			{MESSAGE_CloseTuner,				&CCoreMessageHandler::OnCloseTuner},
			{MESSAGE_SetChannel,				&CCoreMessageHandler::OnSetChannel},
			{MESSAGE_GetChannel,				&CCoreMessageHandler::OnGetChannel},
			{MESSAGE_SetService,				&CCoreMessageHandler::OnSetService},
			{MESSAGE_GetService,				&CCoreMessageHandler::OnGetService},
			{MESSAGE_GetServiceList,			&CCoreMessageHandler::OnGetServiceList},
			{MESSAGE_StartRecording,			&CCoreMessageHandler::OnStartRecording},
			{MESSAGE_StopRecording,				&CCoreMessageHandler::OnStopRecording},
			{MESSAGE_ChangeRecordingFile,		&CCoreMessageHandler::OnChangeRecordingFile},
			{MESSAGE_GetRecordingInfo,			&CCoreMessageHandler::OnGetRecordingInfo},
			{MESSAGE_GetStreamStatistics,		&CCoreMessageHandler::OnGetStreamStatistics},
			{MESSAGE_ResetErrorStatistics,		&CCoreMessageHandler::OnResetErrorStatistics},
			{MESSAGE_GetEventInfo,				&CCoreMessageHandler::OnGetEventInfo},
			{MESSAGE_GetLog,					&CCoreMessageHandler::OnGetLog},
			{MESSAGE_GetBonDriverChannelList,	&CCoreMessageHandler::OnGetBonDriverChannelList},
			{MESSAGE_GetScannedChannelList,		&CCoreMessageHandler::OnGetScannedChannelList},
			{MESSAGE_CreateStreamPool,			&CCoreMessageHandler::OnCreateStreamPool},
			{MESSAGE_CloseStreamPool,			&CCoreMessageHandler::OnCloseStreamPool},
			{MESSAGE_GetTvRockInfo,				&CCoreMessageHandler::OnGetTvRockInfo},
			{MESSAGE_StartStreaming,			&CCoreMessageHandler::OnStartStreaming},
			{MESSAGE_StopStreaming,				&CCoreMessageHandler::OnStopStreaming},
			{MESSAGE_GetStreamingInfo,			&CCoreMessageHandler::OnGetStreamingInfo},
			{MESSAGE_GetStreamingSettings,		&CCoreMessageHandler::OnGetStreamingSettings},
			{MESSAGE_GetSetting,				&CCoreMessageHandler::OnGetSetting},
			{MESSAGE_EVENT_IniSettingsChanged,	&CCoreMessageHandler::OnIniSettingsChanged},
		};

		for (size_t i=0;i<_countof(FunctorList);i++) {
			pMessageMap->SetHandler(FunctorList[i].pszMessage,
									new CMessageFunctor<CCoreMessageHandler>(this,FunctorList[i].Member));
		}

		return true;
	}

	bool CCoreMessageHandler::OnHello(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"挨拶メッセージを受信しました。");

		pResponse->SetProperty(MESSAGE_PROPERTY_Message,L"Hello! I'm " APP_NAME_W L" (ver." APP_VERSION_TEXT_W L" " APP_PLATFORM_W L").");
		pResponse->SetPropertyInt(MESSAGE_PROPERTY_TaskID,m_AppCore.GetTaskID());
		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnEndTask(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"タスク終了メッセージを受信しました。");

		pServer->SetQuitSignal();

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnLoadBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"BonDriverロードメッセージを受信しました。");

		String BonDriverPath;

		if (!BasicMessage::LoadBonDriver::GetProperties(pMessage,&BonDriverPath)) {
			OutLog(LOG_ERROR,L"メッセージからBonDriverのパスを取得できません。");
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		if (PathUtility::IsRelative(BonDriverPath)) {
			String Dir,Path;
			if (m_AppCore.GetCurSettings().BonDriver.GetLoadDirectoryAbsolute(&Dir)
					&& PathUtility::RelativeToAbsolute(&Path,Dir,BonDriverPath)) {
				BonDriverPath=Path;
			}
		}

		if (!m_Core.LoadBonDriver(BonDriverPath.c_str())) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnUnloadBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"BonDriver解放メッセージを受信しました。");

		if (!m_Core.UnloadBonDriver()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetBonDriver(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"BonDriver取得メッセージを受信しました。");

		String Name;

		if (m_Core.GetBonDriverFilePath(&Name))
			pResponse->SetProperty(MESSAGE_PROPERTY_FilePath,Name);
		if (m_Core.GetTunerName(&Name))
			pResponse->SetProperty(MESSAGE_PROPERTY_Name,Name);

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnOpenTuner(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"チューナーオープンのメッセージを受信しました。");

		if (!m_Core.OpenTuner()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnCloseTuner(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"チューナークローズのメッセージを受信しました。");

		if (!m_Core.CloseTuner()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnSetChannel(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"チャンネル設定のメッセージを受信しました。");

		SetChannelInfo Info;

		if (!BasicMessage::SetChannel::GetProperties(pMessage,&Info)) {
			OutLog(LOG_ERROR,L"メッセージからプロパティを取得できません。");
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		bool fResult;

		switch (Info.Type) {
		case SetChannelInfo::TYPE_INDEX:
			fResult=m_Core.SetChannelByIndex(Info.Index.Space,Info.Index.Channel,Info.Index.ServiceID,
											 Info.fForceChange);
			break;

		case SetChannelInfo::TYPE_SCANNED:
			fResult=m_Core.SetChannelByScanned(Info.Scanned.Space,Info.Scanned.Channel,Info.Scanned.ServiceID,
											   Info.fForceChange);
			break;

		case SetChannelInfo::TYPE_IDS:
			fResult=m_Core.SetChannelByIDs(Info.IDs.NetworkID,Info.IDs.TransportStreamID,Info.IDs.ServiceID,
										   Info.fForceChange);
			break;

		default:
			fResult=false;
			break;
		}

		if (!fResult) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetChannel(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"チャンネル取得のメッセージを受信しました。");

		ChannelInfo Channel;

		if (!m_Core.GetChannel(&Channel)
				|| !BasicMessage::GetChannel::SetResponse(pResponse,Channel)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnSetService(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"サービス設定のメッセージを受信しました。");

		CMessageProperty::IntType ServiceID;

		if (!pMessage->GetProperty(MESSAGE_PROPERTY_ServiceID,&ServiceID)) {
			OutLog(LOG_ERROR,L"メッセージからサービスIDを取得できません。");
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		if (!m_Core.SetService((WORD)ServiceID)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetService(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"サービス取得のメッセージを受信しました。");

		ServiceInfo Info;
		if (!m_Core.GetServiceInfo(&Info)
				|| !BasicMessage::GetService::SetResponse(pResponse,Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetServiceList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"サービスリスト取得のメッセージを受信しました。");

		ServiceList List;
		if (!m_Core.GetServiceList(&List)
				|| !BasicMessage::GetServiceList::SetResponse(pResponse,List)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnStartRecording(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"録画開始のメッセージを受信しました。");

		RecordingSettings Settings;

		m_AppCore.GetCurSettings().Recording.GetRecordingSettings(&Settings);
		if (!BasicMessage::StartRecording::GetProperties(pMessage,&Settings)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		String FileName;
		if (m_AppCore.FormatRecordFileName(Settings.FileName.c_str(),&FileName))
			Settings.FileName=FileName;

		if (!m_Core.StartRecording(Settings)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnStopRecording(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"録画停止のメッセージを受信しました。");

		if (!m_Core.StopRecording()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnChangeRecordingFile(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_INFO,L"録画ファイル変更のメッセージを受信しました。");

		String FileName;

		if (!pMessage->GetProperty(MESSAGE_PROPERTY_FilePath,&FileName)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		if (!m_Core.ChangeRecordingFile(FileName.c_str())) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetRecordingInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"録画情報取得のメッセージを受信しました。");

		RecordingInfo Info;

		if (!m_Core.GetRecordingInfo(&Info)
				|| !BasicMessage::GetRecordingInfo::SetResponse(pResponse,Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetStreamStatistics(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリーム情報取得のメッセージを受信しました。");

		StreamStatistics Statistics;

		if (!m_Core.GetStreamStatistics(&Statistics)
				|| !BasicMessage::GetStreamStatistics::SetResponse(pResponse,Statistics)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnResetErrorStatistics(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"エラーカウントリセットのメッセージを受信しました。");

		if (!m_Core.ResetErrorStatistics()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetEventInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"番組情報取得のメッセージを受信しました。");

		bool fNext=false;
		pMessage->GetProperty(L"Next",&fNext);

		EventInfo Info;

		if (!m_Core.GetEventInfo(&Info,fNext)
				|| !BasicMessage::GetEventInfo::SetResponse(pResponse,Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetLog(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ログ取得のメッセージを受信しました。");

		LogList List;
		if (!m_AppCore.GetLog(&List)
				|| !BasicMessage::GetLog::SetResponse(pResponse,List)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetBonDriverChannelList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"BonDriverのチャンネルリスト取得のメッセージを受信しました。");

		BonDriverTuningSpaceList SpaceList;

		if (!m_Core.GetBonDriverChannelList(&SpaceList)
				|| !BasicMessage::GetBonDriverChannelList::SetResponse(pResponse,SpaceList)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetScannedChannelList(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"スキャンされたチャンネルリスト取得のメッセージを受信しました。");

		CTuningSpaceList TuningSpaceList;

		if (!m_Core.GetScannedChannelList(&TuningSpaceList)
				|| !BasicMessage::GetScannedChannelList::SetResponse(pResponse,TuningSpaceList)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnCreateStreamPool(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリームプール作成のメッセージを受信しました。");

		String Name;
		TaskUtility::GetStreamPoolName(m_AppCore.GetTaskID(),&Name);
		if (!m_Core.CreateStreamPool(Name.c_str(),m_AppCore.GetCurSettings().StreamPool.GetSize()/188)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnCloseStreamPool(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリームプールクローズのメッセージを受信しました。");

		if (!m_Core.CloseStreamPool()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetTvRockInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"TvRock情報取得のメッセージを受信しました。");

		int DeviceID=m_AppCore.GetTvRockDeviceID();
		if (DeviceID>=0)
			pResponse->SetPropertyInt(L"DeviceID",DeviceID);

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnStartStreaming(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリーミング開始メッセージを受信しました。");

		StreamingInfo Info;

		m_AppCore.GetCurSettings().Streaming.GetStreamingSettings(&Info);
		if (!BasicMessage::StartStreaming::GetProperties(pMessage,&Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		if (!m_Core.StartStreaming(Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnStopStreaming(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリーミング停止メッセージを受信しました。");

		if (!m_Core.StopStreaming()) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetStreamingInfo(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリーミング情報取得メッセージを受信しました。");

		StreamingInfo Info;

		if (!m_Core.GetStreamingInfo(&Info)
				|| !BasicMessage::GetStreamingInfo::SetResponse(pResponse,Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetStreamingSettings(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"ストリーミング設定取得メッセージを受信しました。");

		StreamingInfo Info;

		m_AppCore.GetCurSettings().Streaming.GetStreamingSettings(&Info);
		if (!BasicMessage::GetStreamingSettings::SetResponse(pResponse,Info)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnGetSetting(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"設定取得メッセージを受信しました。");

		String Name;
		if (!pMessage->GetProperty(MESSAGE_PROPERTY_Name,&Name)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return true;
		}

		if (StringUtility::CompareNoCase(Name,L"General.IniFilePath")==0) {
			String Path;
			m_AppCore.GetCurSettings().General.GetIniFilePath(&Path);
			pResponse->SetProperty(MESSAGE_PROPERTY_Value,Path);
		} else if (StringUtility::CompareNoCase(Name,L"BonDriver.LoadDirectory")==0) {
			String Directory;
			m_AppCore.GetCurSettings().BonDriver.GetLoadDirectoryAbsolute(&Directory);
			pResponse->SetProperty(MESSAGE_PROPERTY_Value,Directory);
		} else {
			OutLog(LOG_WARNING,L"未知の設定 \"%s\" が要求されました。",Name.c_str());
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_Failed);
			return true;
		}

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CCoreMessageHandler::OnIniSettingsChanged(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		OutLog(LOG_VERBOSE,L"INI ファイル更新メッセージを受信しました。");

		m_AppCore.ReloadSettings();

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

}
