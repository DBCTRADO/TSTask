#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskCore.h"
#include "TSTaskAppCore.h"
#include "CoreMessageHandler.h"
#include "ClientManager.h"
#include "TSTaskWindow.h"
#include "TSTaskCommandLine.h"
#include "TvRock.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	class CTSTaskApp : public CTSTaskAppCore, public CEventHandler
	{
	public:
		CTSTaskApp(HINSTANCE hInstance);
		~CTSTaskApp();
		int Main(LPCWSTR pszCmdLine);
		bool Quit() override;
		bool GetLog(LogList *pList,size_t Max) override;
		HWND GetWindowHandle() const override;
		int GetTvRockDeviceID() const override;
		bool ReloadSettings() override;

	private:
		void ApplyCommandLineSettings(const CTSTaskCommandLine &CommandLine,CTSTaskSettings &Settings);

		bool OnRegisterClient(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);
		bool OnUnregisterClient(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse);

	// CEventHandler
		bool OnTunerOpen() override;
		bool OnRecordingStart(const RecordingSettings &Settings) override;
		void OnRecordingStarted(const RecordingInfo &Info) override;
		void OnRecordingStopped() override;
		void OnTotChanged() override;
		void OnErrorStatisticsReset() override;
		bool OnChannelListLoaded(CTuningSpaceList &SpaceList) override;
		bool OnStreamingStart(const StreamingInfo &Info) override;

		CMessageMap m_MessageMap;
		CLocalMessageServer m_MessageServer;
		CCoreMessageHandler m_CoreMessageHandler;
		CClientManager m_ClientManager;
		CBasicLogger m_Logger;
		CTSTaskWindow m_TSTaskWindow;
		CTvRockDTVTarget m_TvRockDTVTarget;
		CTSTaskCommandLine m_CommandLine;
	};

	CTSTaskApp::CTSTaskApp(HINSTANCE hInstance)
		: CTSTaskAppCore(hInstance)
		, m_MessageServer(&m_MessageMap)
		, m_CoreMessageHandler(m_Core,*this)
		, m_TSTaskWindow(*this)
		, m_TvRockDTVTarget(*this)
	{
		SetGlobalLogger(&m_Logger);
	}

	CTSTaskApp::~CTSTaskApp()
	{
		SetGlobalLogger(nullptr);
	}

	int CTSTaskApp::Main(LPCWSTR pszCmdLine)
	{
		OutLog(LOG_IMPORTANT,APP_NAME_W L" ver." APP_VERSION_TEXT_W L" (" APP_PLATFORM_W L") 起動");

		if (!IsStringEmpty(pszCmdLine)) {
			OutLog(LOG_INFO,L"コマンドラインオプション : %s",pszCmdLine);
			m_CommandLine.Parse(pszCmdLine);

			if (m_CommandLine.m_LoggingLevel>=0)
				m_Logger.SetLoggingLevel(CGeneralSettings::LoggingLevelToType(m_CommandLine.m_LoggingLevel));
			if (m_CommandLine.m_DebugLog!=BOOL_DEFAULT)
				m_Logger.EnableDebugTrace(m_CommandLine.m_DebugLog==BOOL_TRUE);
		}

		String IniFilePath;
		if (!m_CommandLine.m_IniFileName.empty()) {
			if (m_CommandLine.m_IniFileName.find(L'\\')==String::npos) {
				GetModuleDirectory(nullptr,&IniFilePath);
				PathUtility::Append(&IniFilePath,m_CommandLine.m_IniFileName.c_str());
			} else {
				IniFilePath=m_CommandLine.m_IniFileName;
			}
		} else {
			m_Settings.General.GetIniFilePath(&IniFilePath);
		}

		OutLog(LOG_VERBOSE,L"設定を \"%s\" から読み込みます。",IniFilePath.c_str());

		CSettings Settings;
		if (Settings.Open(IniFilePath.c_str(),CSettings::OPEN_READ)) {
			m_Settings.Load(Settings);
			m_CurSettings.Set(m_Settings);
			OutLog(LOG_INFO,L"設定を \"%s\" から読み込みました。",IniFilePath.c_str());
		}

		if (!IniFilePath.empty())
			m_CurSettings.General.SetIniFilePath(IniFilePath);

		ApplyCommandLineSettings(m_CommandLine,m_CurSettings);

		if (m_CurSettings.General.GetProcessPriority()!=PROCESS_PRIORITY_INVALID) {
			DWORD Priority=m_CurSettings.General.GetProcessPriorityClass();
			OutLog(LOG_INFO,L"プロセスの優先度を設定します。(%u)",Priority);
			::SetPriorityClass(::GetCurrentProcess(),Priority);
		}

		m_Logger.SetMaxLog(m_CurSettings.General.GetMaxLog());
		m_Logger.SetLoggingLevel(m_CurSettings.General.GetLoggingLevel());
		if (m_CurSettings.General.GetLogOutputToFile()
				&& m_CurSettings.General.GetLoggingLevel()!=LOG_NONE) {
			String LogFilePath;
			if (m_CurSettings.General.GetLogFilePath(&LogFilePath)) {
				unsigned int Flags=
					CBasicLogger::OPEN_WRITE_OLD_LOG | CBasicLogger::OPEN_AUTO_RENAME;
				if (!m_CurSettings.General.GetLogOverwrite())
					Flags|=CBasicLogger::OPEN_APPEND;
				m_Logger.OpenFile(LogFilePath.c_str(),Flags);
			}
		}
		m_Logger.EnableDebugTrace(m_CurSettings.General.GetDebugLog());

		m_Core.Initialize();

		m_TSTaskWindow.Initialize(m_hInstance);

		if (!m_TaskIdentity.Initialize(TASK_TYPE_SERVER,m_CurSettings.General.GetTaskID())) {
			OutLog(LOG_ERROR,L"タスクのIDを割り当てられません。");
			return 1;
		}

		if (!m_SharedInfo.Create(m_TaskIdentity.GetTaskID(),
								 PackVersion(APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION))) {
			OutLog(LOG_ERROR,L"共有情報を作成できません。");
			m_TaskIdentity.Finalize();

			if (m_CurSettings.General.GetTaskID()==INVALID_TASK_ID) {
				OutLog(LOG_VERBOSE,L"他のタスクIDで共有情報の作成を試行します。");
				TaskID ID=INVALID_TASK_ID;
				while ((ID=TaskUtility::GetNextServerTaskID(ID))!=INVALID_TASK_ID) {
					if (m_TaskIdentity.Initialize(TASK_TYPE_SERVER,ID)) {
						if (m_SharedInfo.Create(m_TaskIdentity.GetTaskID(),
												PackVersion(APP_VERSION_MAJOR,APP_VERSION_MINOR,APP_VERSION_REVISION)))
							break;
						m_TaskIdentity.Finalize();
					}
				}
				if (ID==INVALID_TASK_ID) {
					OutLog(LOG_ERROR,L"共有情報を作成可能なタスクIDがありません。");
					return 2;
				}
			} else {
				return 2;
			}
		}

		m_ClientManager.Initialize(m_TaskIdentity.GetTaskID());

		m_Core.AddEventHandler(this);
		m_Core.AddEventHandler(&m_ClientManager);

		m_CoreMessageHandler.SetHandler(&m_MessageMap);
		m_MessageMap.SetHandler(MESSAGE_RegisterClient,
								new CMessageFunctor<CTSTaskApp>(this,&CTSTaskApp::OnRegisterClient));
		m_MessageMap.SetHandler(MESSAGE_UnregisterClient,
								new CMessageFunctor<CTSTaskApp>(this,&CTSTaskApp::OnUnregisterClient));

		String ServerName;
		TaskUtility::GetServerTaskLocalMessageServerName(m_TaskIdentity.GetTaskID(),&ServerName);
		if (!m_MessageServer.Open(ServerName.c_str())) {
			OutLog(LOG_ERROR,L"メッセージサーバーを開始できません。");
			return 3;
		}

		m_SharedInfo.SetTaskState(TASK_STATE_RUNNING);

		if (m_CommandLine.m_TvRockDeviceID>=0)
			m_TvRockDTVTarget.Initialize(m_CommandLine.m_TvRockDeviceID,
										 Settings.IsOpen()?&Settings:nullptr);

		Settings.Close();

		m_TSTaskWindow.Create();

		CMessage StartedMessage;
		BasicMessage::TaskStarted::InitializeMessage(&StartedMessage,m_TaskIdentity.GetTaskID());
		BroadcastServerMessage(StartedMessage);

		if (m_CurSettings.General.GetClientExecuteOnStart()) {
			ExecuteClient();
		}

		if (!m_CommandLine.m_BonDriverFileName.empty()) {
			OutLog(LOG_VERBOSE,L"コマンドラインで指定された BonDriver \"%s\" をロードします。",
				   m_CommandLine.m_BonDriverFileName.c_str());

			String BonDriverPath;

			if (PathUtility::IsRelative(m_CommandLine.m_BonDriverFileName)) {
				String Dir;
				if (!m_CurSettings.BonDriver.GetLoadDirectoryAbsolute(&Dir)
						|| !PathUtility::RelativeToAbsolute(&BonDriverPath,Dir,m_CommandLine.m_BonDriverFileName))
					BonDriverPath=m_CommandLine.m_BonDriverFileName;
			} else {
				BonDriverPath=m_CommandLine.m_BonDriverFileName;
			}

			if (m_Core.LoadBonDriver(BonDriverPath.c_str())) {
				if (m_Core.OpenTuner()) {
					if (m_CommandLine.m_Channel>=0) {
						m_Core.SetChannelByIndex(m_CommandLine.m_TuningSpace>=0?m_CommandLine.m_TuningSpace:0,
												 m_CommandLine.m_Channel,
												 m_CommandLine.m_ServiceID);
					} else if (m_CommandLine.m_TuningSpace>=0) {
						if (m_CommandLine.m_RemoteControlKeyID!=0
								|| m_CommandLine.m_NetworkID!=0
								|| m_CommandLine.m_TransportStreamID!=0
								|| m_CommandLine.m_ServiceID!=0) {
							CChannelList ChannelList;

							if (m_Core.GetScannedChannelList(m_CommandLine.m_TuningSpace,&ChannelList)) {
								int Channel;

								if (m_CommandLine.m_RemoteControlKeyID!=0) {
									Channel=ChannelList.FindRemoteControlKeyID(m_CommandLine.m_RemoteControlKeyID);
								} else {
									Channel=ChannelList.FindByIDs(m_CommandLine.m_NetworkID,
																  m_CommandLine.m_TransportStreamID,
																  m_CommandLine.m_ServiceID);
								}
								if (Channel>=0) {
									m_Core.SetChannelByScanned(m_CommandLine.m_TuningSpace,Channel);
								} else {
									OutLog(LOG_ERROR,L"コマンドラインでの指定に一致するチャンネルがありません。");
								}
							} else {
								OutLog(LOG_ERROR,L"コマンドラインで指定されたチューニング空間(%d)のチャンネルがありません。",
									   m_CommandLine.m_TuningSpace);
							}
						} else {
							OutLog(LOG_ERROR,L"コマンドラインでチューニング空間のみの指定はできません。");
						}
					} else if (m_CommandLine.m_NetworkID!=0
							|| m_CommandLine.m_TransportStreamID!=0
							|| m_CommandLine.m_ServiceID!=0) {
						m_Core.SetChannelByIDs(m_CommandLine.m_NetworkID,
											   m_CommandLine.m_TransportStreamID,
											   m_CommandLine.m_ServiceID);
					} else if (m_CommandLine.m_RemoteControlKeyID!=0) {
						CChannelList ChannelList;
						bool fFound=false;

						for (int i=0;m_Core.GetScannedChannelList(i,&ChannelList);i++) {
							int Channel=ChannelList.FindRemoteControlKeyID(m_CommandLine.m_RemoteControlKeyID);
							if (Channel>=0) {
								m_Core.SetChannelByScanned(m_CommandLine.m_TuningSpace,Channel);
								fFound=true;
								break;
							}
						}
						if (!fFound) {
							OutLog(LOG_ERROR,L"コマンドラインで指定されたリモコン番号(%d)のチャンネルがありません。",
								   m_CommandLine.m_RemoteControlKeyID);
						}
					}
				} else {
					//m_Core.UnloadBonDriver();
				}
			}
		}

		if (m_CommandLine.m_Record==BOOL_TRUE) {
			OutLog(LOG_VERBOSE,L"コマンドラインで指定されているため録画登録を行います。");

			RecordingSettings RecSettings;
			ReserveSettings Reserve;

			m_CurSettings.Recording.GetRecordingSettings(&RecSettings);

			if (m_CommandLine.m_RecordingStartTime.dwLowDateTime!=0
					|| m_CommandLine.m_RecordingStartTime.dwHighDateTime!=0) {
				Reserve.StartTime.Type=ReserveSettings::START_TIME_DATE;
				::LocalFileTimeToFileTime(&m_CommandLine.m_RecordingStartTime,&Reserve.StartTime.Time);
				if (m_CommandLine.m_RecordingDelay!=0)
					OffsetFileTime(&Reserve.StartTime.Time,
								   m_CommandLine.m_RecordingDelay*FILETIME_SECOND);
			} else if (m_CommandLine.m_RecordingDelay>0) {
				Reserve.StartTime.Type=ReserveSettings::START_TIME_DELAY;
				Reserve.StartTime.Delay=m_CommandLine.m_RecordingDelay*1000;
			} else {
				Reserve.StartTime.Type=ReserveSettings::START_TIME_NOT_SPECIFIED;
			}

			if (m_CommandLine.m_RecordingDuration>0) {
				Reserve.EndTime.Type=ReserveSettings::END_TIME_DURATION;
				Reserve.EndTime.Duration=m_CommandLine.m_RecordingDuration*1000;
			} else {
				Reserve.EndTime.Type=ReserveSettings::END_TIME_NOT_SPECIFIED;
			}

			ReserveRecording(Reserve,RecSettings);
		}

		if (m_CommandLine.m_StreamingUDP==BOOL_TRUE
				|| m_CommandLine.m_StreamingTCP==BOOL_TRUE) {
			OutLog(LOG_VERBOSE,L"コマンドラインで指定されているためストリーミングを開始します。");
			StreamingInfo Info;
			m_CurSettings.Streaming.GetStreamingSettings(&Info);
			m_Core.StartStreaming(Info);
		}

		m_StatisticsUpdater.SetUpdateInterval(m_CurSettings.General.GetStatisticsUpdateInterval());
		m_StatisticsUpdater.StartThread();

		OutLog(LOG_VERBOSE,L"メッセージループに入ります。");

		while (true) {
			CMessageServer::WaitMessageResult WaitResult=m_MessageServer.WaitWindowMessage();

			if (WaitResult==CMessageServer::WAIT_RESULT_QUIT) {
				m_TSTaskWindow.Destroy();
			} else if (WaitResult==CMessageServer::WAIT_RESULT_WINDOW_MESSAGE) {
				MSG msg;
				bool fQuit=false;
				while (::PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) {
					if (msg.message==WM_QUIT) {
						fQuit=true;
						break;
					}
					::DispatchMessage(&msg);
				}
				if (fQuit)
					break;
			}
		}

		OutLog(LOG_VERBOSE,L"メッセージループを抜けました。");

		m_StatisticsUpdater.EndThread();

		m_SharedInfo.SetTaskState(TASK_STATE_ENDING);

		m_TvRockDTVTarget.Finalize();

		m_Core.Finalize();

		m_ClientManager.Finalize();

		CMessage EndedMessage;
		BasicMessage::TaskEnded::InitializeMessage(&EndedMessage,m_TaskIdentity.GetTaskID());
		BroadcastServerMessage(EndedMessage);

		m_MessageServer.Close();

		m_SharedInfo.Close();

		m_TaskIdentity.Finalize();

		OutLog(LOG_IMPORTANT,APP_NAME_W L" 終了");

		return 0;
	}

	bool CTSTaskApp::Quit()
	{
		return m_MessageServer.Quit();
	}

	bool CTSTaskApp::GetLog(LogList *pList,size_t Max)
	{
		return m_Logger.GetLog(pList,Max);
	}

	HWND CTSTaskApp::GetWindowHandle() const
	{
		return m_TSTaskWindow.GetHandle();
	}

	int CTSTaskApp::GetTvRockDeviceID() const
	{
		return m_TvRockDTVTarget.GetDeviceID();
	}

	bool CTSTaskApp::ReloadSettings()
	{
		String IniFilePath;
		m_CurSettings.General.GetIniFilePath(&IniFilePath);

		OutLog(LOG_VERBOSE,L"設定を再読み込みします。(%s)",IniFilePath.c_str());

		CSettings Settings;
		if (!Settings.Open(IniFilePath.c_str(),CSettings::OPEN_READ)) {
			OutLog(LOG_ERROR,L"設定の再読み込みができません。(%s)",IniFilePath.c_str());
			return false;
		}

		m_Settings.Load(Settings);

		Settings.Close();

		m_CurSettings.Set(m_Settings);
		m_CurSettings.General.SetIniFilePath(IniFilePath);

		ApplyCommandLineSettings(m_CommandLine,m_CurSettings);

		if (m_Core.IsStreaming()) {
			m_Core.SetStreamingSendSize(m_CurSettings.Streaming.GetSendSize());
			m_Core.SetStreamingSendWait(m_CurSettings.Streaming.GetSendWait(),
										m_CurSettings.Streaming.GetAdjustSendWait());
		}

		return true;
	}

	void CTSTaskApp::ApplyCommandLineSettings(const CTSTaskCommandLine &CommandLine,CTSTaskSettings &Settings)
	{
		if (CommandLine.m_ExecuteClient!=BOOL_DEFAULT)
			Settings.General.SetClientExecuteOnStart(CommandLine.m_ExecuteClient==BOOL_TRUE);
		if (CommandLine.m_TaskID!=INVALID_TASK_ID)
			Settings.General.SetTaskID(CommandLine.m_TaskID);
		if (CommandLine.m_ProcessPriority>=PROCESS_PRIORITY_LOWEST)
			Settings.General.SetProcessPriority(ProcessPriority(CommandLine.m_ProcessPriority));

		if (CommandLine.m_ClientShowCommand>=0)
			Settings.General.SetClientShowCommand(CommandLine.m_ClientShowCommand);

		if (CommandLine.m_LoggingLevel>=0)
			Settings.General.SetLoggingLevel(CGeneralSettings::LoggingLevelToType(CommandLine.m_LoggingLevel));
		if (CommandLine.m_LogOutputToFile!=BOOL_DEFAULT)
			Settings.General.SetLogOutputToFile(CommandLine.m_LogOutputToFile==BOOL_TRUE);
		if (!CommandLine.m_LogFileName.empty())
			Settings.General.SetLogFileName(CommandLine.m_LogFileName);
		if (CommandLine.m_LogOverwrite!=BOOL_DEFAULT)
			Settings.General.SetLogOverwrite(CommandLine.m_LogOverwrite==BOOL_TRUE);
		if (CommandLine.m_DebugLog!=BOOL_DEFAULT)
			Settings.General.SetDebugLog(CommandLine.m_DebugLog==BOOL_TRUE);

		if (!CommandLine.m_RecordingFileName.empty()) {
			if (CommandLine.m_RecordingFileName.find(L'\\')!=String::npos) {
				String Directory,FileName;
				PathUtility::Split(CommandLine.m_RecordingFileName,&Directory,&FileName);
				Settings.Recording.SetDefaultDirectory(Directory);
				Settings.Recording.SetFileName(FileName);
			} else {
				Settings.Recording.SetFileName(CommandLine.m_RecordingFileName);
			}
		}
		if (!CommandLine.m_RecordingDirectories.empty()) {
			Settings.Recording.SetDirectories(CommandLine.m_RecordingDirectories);
		}
		if (CommandLine.m_RecordingService>=0)
			Settings.Recording.SetServiceSelectType(ServiceSelectType(CommandLine.m_RecordingService));
		if (CommandLine.m_Record1Seg!=BOOL_DEFAULT)
			Settings.Recording.SetStreamFlag(STREAM_1SEG,CommandLine.m_Record1Seg==BOOL_TRUE);
		if (CommandLine.m_RecordingPreAllocateSize>=0) {
			Settings.Recording.SetPreAllocate(true);
			Settings.Recording.SetPreAllocateSize(CommandLine.m_RecordingPreAllocateSize);
		}
		if (CommandLine.m_ExitOnRecordingStop!=BOOL_DEFAULT)
			Settings.Recording.SetExitOnStop(CommandLine.m_ExitOnRecordingStop==BOOL_TRUE);

		if (CommandLine.m_StreamingUDP==BOOL_TRUE)
			Settings.Streaming.SetProtocol(PROTOCOL_UDP);
		else if (CommandLine.m_StreamingTCP==BOOL_TRUE)
			Settings.Streaming.SetProtocol(PROTOCOL_TCP);
		else if (CommandLine.m_StreamingProtocol!=PROTOCOL_UNDEFINED)
			Settings.Streaming.SetProtocol(CommandLine.m_StreamingProtocol);
		if (!CommandLine.m_StreamingAddress.empty())
			Settings.Streaming.SetAddress(CommandLine.m_StreamingAddress);
		if (CommandLine.m_StreamingPort>=0 && CommandLine.m_StreamingPort<=0xFFFF)
			Settings.Streaming.SetPort(WORD(CommandLine.m_StreamingPort));
		if (CommandLine.m_StreamingService>=0)
			Settings.Streaming.SetServiceSelectType(ServiceSelectType(CommandLine.m_StreamingService));
		if (CommandLine.m_Streaming1Seg!=BOOL_DEFAULT)
			Settings.Streaming.SetStreamFlag(STREAM_1SEG,CommandLine.m_Streaming1Seg==BOOL_TRUE);

		if (CommandLine.m_FirstChannelSetDelay>=0)
			Settings.BonDriver.SetFirstChannelSetDelay(CommandLine.m_FirstChannelSetDelay);
		if (CommandLine.m_MinChannelChangeInterval>=0)
			Settings.BonDriver.SetMinChannelChangeInterval(CommandLine.m_MinChannelChangeInterval);
	}

	bool CTSTaskApp::OnRegisterClient(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		CMessageProperty::IntType ID;

		if (!pMessage->GetProperty(MESSAGE_PROPERTY_TaskID,&ID)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return false;
		}

		OutLog(LOG_VERBOSE,L"クライアント(%u)を登録します。",(UINT)ID);

		m_ClientManager.AddClient((TaskID)ID);

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskApp::OnUnregisterClient(CMessageServer *pServer,const CMessage *pMessage,CMessage *pResponse)
	{
		CMessageProperty::IntType ID;

		if (!pMessage->GetProperty(MESSAGE_PROPERTY_TaskID,&ID)) {
			pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_NoProperty);
			return false;
		}

		OutLog(LOG_VERBOSE,L"クライアント(%u)を登録解除します。",(UINT)ID);

		m_ClientManager.RemoveClient((TaskID)ID);

		pResponse->SetProperty(MESSAGE_PROPERTY_Result,MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskApp::OnTunerOpen()
	{
		m_Core.SetFirstChannelSetDelay(
			m_CurSettings.BonDriver.GetFirstChannelSetDelay());
		m_Core.SetMinChannelChangeInterval(
			m_CurSettings.BonDriver.GetMinChannelChangeInterval());

		return true;
	}

	bool CTSTaskApp::OnRecordingStart(const RecordingSettings &Settings)
	{
		m_Core.SetWriteBufferSize(m_CurSettings.Recording.GetWriteBufferSize());
		m_Core.SetMaxWritePendingSize(m_CurSettings.Recording.GetMaxPendingSize());
		m_Core.SetWritePreAllocate(m_CurSettings.Recording.GetPreAllocate()?
								   m_CurSettings.Recording.GetPreAllocateSize():0);
		m_Core.SetMinDiskFreeSpace(m_CurSettings.Recording.GetMinFreeSpace());

		return true;
	}

	void CTSTaskApp::OnRecordingStarted(const RecordingInfo &Info)
	{
		m_TSTaskWindow.OnRecordingStarted();
	}

	void CTSTaskApp::OnRecordingStopped()
	{
		m_TSTaskWindow.OnRecordingStopped();
		if (m_CurSettings.Recording.GetExitOnStop()) {
			OutLog(LOG_INFO,L"録画が停止したのでプログラムを終了します。");
			Quit();
		}
	}

	void CTSTaskApp::OnTotChanged()
	{
		UpdateTotTime();
	}

	void CTSTaskApp::OnErrorStatisticsReset()
	{
		UpdateStreamStatistics();
	}

	bool CTSTaskApp::OnStreamingStart(const StreamingInfo &Info)
	{
		m_Core.SetStreamingSendSize(m_CurSettings.Streaming.GetSendSize());
		m_Core.SetStreamingSendWait(m_CurSettings.Streaming.GetSendWait(),
									m_CurSettings.Streaming.GetAdjustSendWait());
		if (Info.Address.Protocol==PROTOCOL_TCP) {
			m_Core.SetStreamingConnectRetryInterval(m_CurSettings.Streaming.GetConnectRetryInterval());
			m_Core.SetStreamingMaxConnectRetries(m_CurSettings.Streaming.GetMaxConnectRetries());
			m_Core.SetStreamingTcpMaxSendRetries(m_CurSettings.Streaming.GetTcpMaxSendRetries());
			m_Core.SetStreamingTcpPrependHeader(m_CurSettings.Streaming.GetTcpPrependHeader());
		}

		return true;
	}

	bool CTSTaskApp::OnChannelListLoaded(CTuningSpaceList &SpaceList)
	{
		if (m_TvRockDTVTarget.GetDeviceID()<0)
			return false;

		OutLog(LOG_VERBOSE,L"チャンネルリストに周波数を設定します。");

		for (size_t i=0;i<SpaceList.GetSpaceCount();i++) {
			CChannelList *pChannelList=SpaceList.GetChannelList(i);
			if (pChannelList==nullptr)
				continue;

			for (size_t j=0;j<pChannelList->GetChannelCount();j++) {
				CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(j);

				if (pChannelInfo!=nullptr) {
					int Frequency=m_TvRockDTVTarget.GetFrequencyByTSID(pChannelInfo->GetTransportStreamID());
					if (Frequency!=0)
						pChannelInfo->SetFrequency(Frequency);
				}
			}
		}

		return true;
	}

}


int APIENTRY wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR pszCmdLine, int ShowCmd)
{
	::SetDllDirectory(L"");

	TSTASK_DEBUG_INITIALIZE;

	TSTask::CTSTaskApp App(hInstance);

	return App.Main(pszCmdLine);
}
