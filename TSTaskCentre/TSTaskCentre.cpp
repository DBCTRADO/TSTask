#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskCentreCore.h"
#include "MainBoard.h"
#include "TSTaskCentreCommandLine.h"
#include "ThemeSettings.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	class CTSTaskCentreApp : public CSettingsHandler, public CThemeManager::CApplier
	{
	public:
		CTSTaskCentreApp(HINSTANCE hInstance);
		~CTSTaskCentreApp();
		int Main(LPCWSTR pszCmdLine,int ShowCmd);

	private:
		void ApplyCommandLineSettings(const CTSTaskCentreCommandLine &CommandLine,
									  CTSTaskCentreSettings &Settings);
		bool SaveSettings() const;

		bool OnHello(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnTaskStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnTaskEnded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnBonDriverLoaded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnBonDriverUnloaded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnTunerOpened(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnTunerClosed(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnChannelChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnServiceChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnRecordingStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnRecordingStopped(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnRecordingFileChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnEventChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnStreamingStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		bool OnStreamingStopped(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);

	// CSettingsHandler
		bool OnSettingsChanged(const CTSTaskCentreSettings &Settings) override;

	// CThemeManager::CApplier
		bool ApplyTheme(const CThemeSettings &Theme) override;

		HINSTANCE m_hInstance;
		CTSTaskCentreCore m_Core;
		TSTask::CMessageMap m_MessageMap;
		TSTask::CLocalMessageServer m_MessageServer;
		CMainBoard m_MainBoard;
		TSTask::String m_IniFileName;
	};

	CTSTaskCentreApp::CTSTaskCentreApp(HINSTANCE hInstance)
		: m_hInstance(hInstance)
		, m_MessageServer(&m_MessageMap)
		, m_MainBoard(m_Core,this)
	{
		m_Core.GetThemeManager().SetApplier(this);
	}

	CTSTaskCentreApp::~CTSTaskCentreApp()
	{
	}

	int CTSTaskCentreApp::Main(LPCWSTR pszCmdLine,int ShowCmd)
	{
		TSTask::OutLog(TSTask::LOG_IMPORTANT,
					   APP_NAME_W L" ver." APP_VERSION_TEXT_W L" (" APP_PLATFORM_W L") �N��");

		TSTask::CMutex AppMutex;
		if (!AppMutex.Create(APP_NAME_W L"_Mutex",false)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"�v���O������Mutex���쐬�ł��܂���B");
		} else if (AppMutex.IsAlreadyExists()) {
			AppMutex.Close();
			TSTask::OutLog(TSTask::LOG_INFO,L"���ɋN�����Ă��邽�ߏI�����܂��B");
			return 1;
		}

		CTSTaskCentreCommandLine CommandLine;
		if (!TSTask::IsStringEmpty(pszCmdLine)) {
			TSTask::OutLog(TSTask::LOG_INFO,L"�R�}���h���C���I�v�V���� : %s",pszCmdLine);
			CommandLine.Parse(pszCmdLine);

			if (CommandLine.m_LoggingLevel>=0)
				m_Core.GetLogger().SetLoggingLevel(CGeneralSettings::LoggingLevelToType(CommandLine.m_LoggingLevel));
			if (CommandLine.m_DebugLog!=TSTask::BOOL_DEFAULT)
				m_Core.GetLogger().EnableDebugTrace(CommandLine.m_DebugLog==TSTask::BOOL_TRUE);
		}

		::CoInitialize(nullptr);

		if (!CMainBoard::Initialize(m_hInstance)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"�E�B���h�E�̏��������ł��܂���B");
			return 1;
		}

		m_Core.GetSettings().General.GetIniFilePath(&m_IniFileName);
		if (TSTask::PathUtility::IsFileExists(m_IniFileName)) {
			TSTask::OutLog(TSTask::LOG_VERBOSE,L"�ݒ�� \"%s\" ����ǂݍ��݂܂��B",m_IniFileName.c_str());
			TSTask::CSettings Settings;
			if (Settings.Open(m_IniFileName.c_str(),TSTask::CSettings::OPEN_READ)) {
				m_Core.GetSettings().Load(Settings);
				m_Core.GetCurSettings().Set(m_Core.GetSettings());
				Settings.Close();
				TSTask::OutLog(TSTask::LOG_INFO,L"�ݒ�� \"%s\" ����ǂݍ��݂܂����B",m_IniFileName.c_str());
			}
		}

		ApplyCommandLineSettings(CommandLine,m_Core.GetCurSettings());

		if (m_Core.GetCurSettings().General.GetProcessPriority()!=PROCESS_PRIORITY_INVALID) {
			DWORD Priority=m_Core.GetCurSettings().General.GetProcessPriorityClass();
			TSTask::OutLog(TSTask::LOG_INFO,L"�v���Z�X�̗D��x��ݒ肵�܂��B(%u)",Priority);
			::SetPriorityClass(::GetCurrentProcess(),Priority);
		}

		m_MainBoard.RestoreSettings(m_Core.GetCurSettings().MainBoard);

		TSTask::String ThemeFile;
		if (m_Core.GetCurSettings().MainBoard.GetThemeFile(&ThemeFile) && !ThemeFile.empty()) {
			m_Core.GetThemeManager().ApplyTheme(ThemeFile.c_str());
		}

		if (!m_Core.Initialize()) {
			return 1;
		}

		static const struct {
			LPCWSTR pszMessage;
			bool (CTSTaskCentreApp::*Member)(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse);
		} FunctorList[] = {
			{TSTask::MESSAGE_Hello,							&CTSTaskCentreApp::OnHello},
			{TSTask::MESSAGE_TaskStarted,					&CTSTaskCentreApp::OnTaskStarted},
			{TSTask::MESSAGE_TaskEnded,						&CTSTaskCentreApp::OnTaskEnded},
			{TSTask::MESSAGE_EVENT_BonDriverLoaded,			&CTSTaskCentreApp::OnBonDriverLoaded},
			{TSTask::MESSAGE_EVENT_BonDriverUnloaded,		&CTSTaskCentreApp::OnBonDriverUnloaded},
			{TSTask::MESSAGE_EVENT_TunerOpened,				&CTSTaskCentreApp::OnTunerOpened},
			{TSTask::MESSAGE_EVENT_TunerClosed,				&CTSTaskCentreApp::OnTunerClosed},
			{TSTask::MESSAGE_EVENT_ChannelChanged,			&CTSTaskCentreApp::OnChannelChanged},
			{TSTask::MESSAGE_EVENT_ServiceChanged,			&CTSTaskCentreApp::OnServiceChanged},
			{TSTask::MESSAGE_EVENT_RecordingStarted,		&CTSTaskCentreApp::OnRecordingStarted},
			{TSTask::MESSAGE_EVENT_RecordingStopped,		&CTSTaskCentreApp::OnRecordingStopped},
			{TSTask::MESSAGE_EVENT_RecordingFileChanged,	&CTSTaskCentreApp::OnRecordingFileChanged},
			{TSTask::MESSAGE_EVENT_EventChanged,			&CTSTaskCentreApp::OnEventChanged},
			{TSTask::MESSAGE_EVENT_StreamingStarted,		&CTSTaskCentreApp::OnStreamingStarted},
			{TSTask::MESSAGE_EVENT_StreamingStopped,		&CTSTaskCentreApp::OnStreamingStopped},
		};

		for (size_t i=0;i<_countof(FunctorList);i++) {
			m_MessageMap.SetHandler(FunctorList[i].pszMessage,
									new TSTask::CMessageFunctor<CTSTaskCentreApp>(this,FunctorList[i].Member));
		}

		TSTask::String ServerName;
		TSTask::TaskUtility::GetClientTaskLocalMessageServerName(m_Core.GetTaskID(),&ServerName);
		if (!m_MessageServer.Open(ServerName.c_str())) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"���[�J�����b�Z�[�W�T�[�o�[���J�n�ł��܂���B");
			return 1;
		}

		Graphics::CSystem::Initialize();

		m_MainBoard.Create();

		TSTask::TaskUtility::TaskIDList TaskList;
		TSTask::TaskUtility::GetServerTaskList(&TaskList);
		for (auto e:TaskList) {
			if (m_Core.AddTask(e))
				m_MainBoard.CreateTSTaskBar(e);
		}

		if (m_Core.GetCurSettings().MainBoard.GetMinimize()) {
			if (m_Core.GetCurSettings().MainBoard.GetMinimizeToTray())
				ShowCmd=SW_MINIMIZE;
			else
				ShowCmd=SW_SHOWMINNOACTIVE;
		}
		m_MainBoard.Show(ShowCmd);
		m_MainBoard.Update();

		m_Core.GetLogger().SetHandler(&m_MainBoard);

		MSG msg;
		BOOL Result;
		while ((Result=::GetMessage(&msg,nullptr,0,0))!=0) {
			if (Result==-1) {
				TSTask::OutSystemErrorLog(::GetLastError(),L"GetMessage() ���G���[��Ԃ��܂����B");
				break;
			}

			if (!m_Core.ProcessDialogMessage(&msg)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}

		Graphics::CSystem::Finalize();

		m_MessageServer.Close();

		m_MainBoard.GetSettings(&m_Core.GetSettings().MainBoard);
		SaveSettings();

		m_Core.Finalize();

		AppMutex.Close();

		::CoUninitialize();

		TSTask::OutLog(TSTask::LOG_IMPORTANT,APP_NAME_W L" �I��");

		return 0;
	}

	void CTSTaskCentreApp::ApplyCommandLineSettings(const CTSTaskCentreCommandLine &CommandLine,
													CTSTaskCentreSettings &Settings)
	{
		if (CommandLine.m_Minimize!=TSTask::BOOL_DEFAULT)
			Settings.MainBoard.SetMinimize(CommandLine.m_Minimize==TSTask::BOOL_TRUE);

		if (CommandLine.m_LoggingLevel>=0)
			Settings.General.SetLoggingLevel(CGeneralSettings::LoggingLevelToType(CommandLine.m_LoggingLevel));
		if (CommandLine.m_LogOutputToFile!=TSTask::BOOL_DEFAULT)
			Settings.General.SetLogOutputToFile(CommandLine.m_LogOutputToFile==TSTask::BOOL_TRUE);
		if (!CommandLine.m_LogFileName.empty())
			Settings.General.SetLogFileName(CommandLine.m_LogFileName);
		if (CommandLine.m_LogOverwrite!=TSTask::BOOL_DEFAULT)
			Settings.General.SetLogOverwrite(CommandLine.m_LogOverwrite==TSTask::BOOL_TRUE);
		if (CommandLine.m_DebugLog!=TSTask::BOOL_DEFAULT)
			Settings.General.SetDebugLog(CommandLine.m_DebugLog==TSTask::BOOL_TRUE);
	}

	bool CTSTaskCentreApp::SaveSettings() const
	{
		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�ݒ�� \"%s\" �ɕۑ����܂��B",m_IniFileName.c_str());

#if 0
		if (!TSTask::PathUtility::IsFileExists(m_IniFileName)) {
			HANDLE hFile=::CreateFile(m_IniFileName.c_str(),GENERIC_WRITE,0,nullptr,
									  CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
			if (hFile==INVALID_HANDLE_VALUE) {
				DWORD Error=::GetLastError();
				if (Error!=ERROR_ALREADY_EXISTS) {
					TSTask::OutSystemErrorLog(Error,L"INI�t�@�C��(%s)���쐬�ł��܂���B",
											   m_IniFileName.c_str());
				}
			} else {
				static const WORD BOM=0xFEFF;
				DWORD Write;
				::WriteFile(hFile,&BOM,sizeof(BOM),&Write,nullptr);
				::CloseHandle(hFile);
			}
		}
#endif

		TSTask::CSettings Settings;
		if (Settings.Open(m_IniFileName.c_str(),TSTask::CSettings::OPEN_WRITE)) {
			m_Core.GetSettings().Save(Settings);
			Settings.Close();
			TSTask::OutLog(TSTask::LOG_INFO,L"�ݒ�� \"%s\" �ɕۑ����܂����B",m_IniFileName.c_str());
		}

		return true;
	}

	bool CTSTaskCentreApp::OnHello(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::OutLog(TSTask::LOG_INFO,L"���A���b�Z�[�W����M���܂����B");

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Message,L"Hello! I'm " APP_NAME_W L" (ver." APP_VERSION_TEXT_W L" " APP_PLATFORM_W L").");
		pResponse->SetPropertyInt(TSTask::MESSAGE_PROPERTY_TaskID,m_Core.GetTaskID());
		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnTaskStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::TaskID TaskID;

		if (!TSTask::BasicMessage::TaskStarted::GetProperties(pMessage,&TaskID)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::OutLog(TSTask::LOG_INFO,L"�^�X�N(%u)�̊J�n�ʒm����M���܂����B",TaskID);

		if (m_Core.AddTask(TaskID))
			m_MainBoard.SendCreateTSTaskBar(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnTaskEnded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::TaskID TaskID;

		if (!TSTask::BasicMessage::TaskEnded::GetProperties(pMessage,&TaskID)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::OutLog(TSTask::LOG_INFO,L"�^�X�N(%u)�̏I���ʒm����M���܂����B",TaskID);

		m_MainBoard.SendRemoveTSTaskBar(TaskID);
		m_Core.RemoveTask(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnBonDriverLoaded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)��BonDriver���[�h�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyBonDriverLoaded(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnBonDriverUnloaded(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)��BonDriver�A�����[�h�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyBonDriverUnloaded(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnTunerOpened(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃`���[�i�[�I�[�v���ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyTunerOpened(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnTunerClosed(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃`���[�i�[�N���[�Y�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyTunerClosed(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnChannelChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃`�����l���ύX�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyChannelChanged(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnServiceChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃T�[�r�X�ύX�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyServiceChanged(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnRecordingStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̘^��J�n�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyRecordingStarted(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnRecordingStopped(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̘^���~�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyRecordingStopped(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnRecordingFileChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̘^��t�@�C���؂�ւ��ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyRecordingFileChanged(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnEventChanged(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̔ԑg�ω��ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyEventChanged(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnStreamingStarted(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃X�g���[�~���O�J�n�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyStreamingStarted(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnStreamingStopped(TSTask::CMessageServer *pServer,const TSTask::CMessage *pMessage,TSTask::CMessage *pResponse)
	{
		TSTask::CMessageProperty::IntType Value;

		if (!pMessage->GetProperty(TSTask::MESSAGE_PROPERTY_TaskID,&Value)) {
			pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_NoProperty);
			return true;
		}

		TSTask::TaskID TaskID=(TSTask::TaskID)Value;

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"�^�X�N(%u)�̃X�g���[�~���O��~�ʒm����M���܂����B",TaskID);

		m_MainBoard.NotifyStreamingStopped(TaskID);

		pResponse->SetProperty(TSTask::MESSAGE_PROPERTY_Result,TSTask::MESSAGE_RESULT_OK);

		return true;
	}

	bool CTSTaskCentreApp::OnSettingsChanged(const CTSTaskCentreSettings &Settings)
	{
		m_Core.ChangeSettings(Settings);

		m_MainBoard.ChangeSettings(Settings.MainBoard);

		SaveSettings();

		return true;
	}

	bool CTSTaskCentreApp::ApplyTheme(const CThemeSettings &Theme)
	{
		return m_MainBoard.SetTheme(Theme.GetMainBoardTheme());
	}

}


int APIENTRY wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR pszCmdLine, int ShowCmd)
{
	::SetDllDirectory(L"");

	TSTASK_DEBUG_INITIALIZE;

	TSTaskCentre::CTSTaskCentreApp App(hInstance);

	return App.Main(pszCmdLine,ShowCmd);
}
