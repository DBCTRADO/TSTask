#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskCentreCore.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTSTaskCentreCore::CTSTaskCentreCore()
		: m_DPI(USER_DEFAULT_SCREEN_DPI)
	{
		TSTask::SetGlobalLogger(&m_Logger);
	}

	CTSTaskCentreCore::~CTSTaskCentreCore()
	{
		TSTask::SetGlobalLogger(nullptr);
	}

	bool CTSTaskCentreCore::Initialize()
	{
		if (!m_TaskIdentity.Initialize(TSTask::TASK_TYPE_CLIENT)) {
			TSTask::OutLog(TSTask::LOG_ERROR,L"タスクのIDを割り当てられません。");
			return false;
		}

		m_Logger.SetMaxLog(m_CurSettings.General.GetMaxLog());
		m_Logger.SetLoggingLevel(m_CurSettings.General.GetLoggingLevel());
		if (m_CurSettings.General.GetLogOutputToFile()
				&& m_CurSettings.General.GetLoggingLevel()!=TSTask::LOG_NONE) {
			TSTask::String LogFilePath;
			if (m_CurSettings.General.GetLogFilePath(&LogFilePath)) {
				unsigned int Flags=TSTask::CBasicLogger::OPEN_WRITE_OLD_LOG;
				if (!m_CurSettings.General.GetLogOverwrite())
					Flags|=TSTask::CBasicLogger::OPEN_APPEND;
				m_Logger.OpenFile(LogFilePath.c_str(),Flags);
			}
		}
		m_Logger.EnableDebugTrace(m_CurSettings.General.GetDebugLog());

		HDC hdc=::CreateIC(TEXT("DISPLAY"),nullptr,nullptr,nullptr);
		if (hdc!=nullptr) {
			m_DPI=::GetDeviceCaps(hdc,LOGPIXELSX);
			TSTask::OutLog(TSTask::LOG_VERBOSE,L"ディスプレイ解像度を取得しました。(%d DPI)",m_DPI);
			::DeleteDC(hdc);
		}

		return true;
	}

	void CTSTaskCentreCore::Finalize()
	{
		m_TaskIdentity.Finalize();
	}

	bool CTSTaskCentreCore::ChangeSettings(const CTSTaskCentreSettings &Settings)
	{
		m_Logger.SetMaxLog(Settings.General.GetMaxLog());
		m_Logger.SetLoggingLevel(Settings.General.GetLoggingLevel());
		m_Logger.EnableDebugTrace(Settings.General.GetDebugLog());

		return true;
	}

	int CTSTaskCentreCore::GetTaskCount() const
	{
		return m_TSTaskManager.GetTaskCount();
	}

	bool CTSTaskCentreCore::GetTaskList(TSTask::TaskUtility::TaskIDList *pList) const
	{
		return m_TSTaskManager.GetTaskList(pList);
	}

	bool CTSTaskCentreCore::AddTask(TSTask::TaskID ID)
	{
		return m_TSTaskManager.AddTask(GetTaskID(),ID);
	}

	bool CTSTaskCentreCore::RemoveTask(TSTask::TaskID ID)
	{
		return m_TSTaskManager.RemoveTask(ID);
	}

	bool CTSTaskCentreCore::NewTask()
	{
		TSTask::String ExeFilePath,CommandLine,Options;

		if (!m_Settings.Task.GetExeFilePath(&ExeFilePath))
			return false;

		CommandLine=L'"';
		CommandLine+=ExeFilePath;
		CommandLine+=L'"';
		if (m_Settings.Task.GetCommandLineOptions(&Options)
				&& !Options.empty()) {
			CommandLine+=L' ';
			CommandLine+=Options;
		}
		CommandLine+=L'\0';

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		::ZeroMemory(&si,sizeof(si));
		si.cb=sizeof(si);
		::ZeroMemory(&pi,sizeof(pi));

		TSTask::OutLog(TSTask::LOG_INFO,L"新規タスク(%s)を作成します。",ExeFilePath.c_str());

		if (!::CreateProcess(nullptr,&CommandLine[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)) {
			TSTask::OutSystemErrorLog(::GetLastError(),L"プロセス(%s)を作成できません。",
									  ExeFilePath.c_str());
			return false;
		}

		TSTask::OutLog(TSTask::LOG_VERBOSE,L"プロセス(%s)を作成しました。(PID %u)",
					   ExeFilePath.c_str(),pi.dwProcessId);

		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);

		return true;
	}

	bool CTSTaskCentreCore::GetBonDriverFileList(LPCWSTR pszDirectory,CBonDriverManager::BonDriverFileList *pList)
	{
		TSTask::String Directory;

		if (pszDirectory==nullptr) {
			if (!m_Settings.BonDriver.GetLoadDirectoryAbsolute(&Directory))
				return false;
			pszDirectory=Directory.c_str();
		}

		return m_BonDriverManager.GetBonDriverFileList(pszDirectory,pList);
	}

	int CTSTaskCentreCore::ScaleDPI(int DPI,int Value) const
	{
		return ::MulDiv(Value,DPI,USER_DEFAULT_SCREEN_DPI);
	}

	RECT CTSTaskCentreCore::ScaleDPI(int DPI,const RECT &Rect) const
	{
		RECT rc;
		rc.left=ScaleDPI(DPI,Rect.left);
		rc.top=ScaleDPI(DPI,Rect.top);
		rc.right=ScaleDPI(DPI,Rect.right);
		rc.bottom=ScaleDPI(DPI,Rect.bottom);
		return rc;
	}

	bool CTSTaskCentreCore::AddModelessDialog(CDialog *pDialog)
	{
		if (pDialog==nullptr)
			return false;

		auto i=std::find(m_DialogList.begin(),m_DialogList.end(),pDialog);
		if (i!=m_DialogList.end())
			return false;

		m_DialogList.push_back(pDialog);

		return true;
	}

	bool CTSTaskCentreCore::RemoveModelessDialog(CDialog *pDialog)
	{
		auto i=std::find(m_DialogList.begin(),m_DialogList.end(),pDialog);
		if (i==m_DialogList.end())
			return false;

		m_DialogList.erase(i);

		return true;
	}

	bool CTSTaskCentreCore::ProcessDialogMessage(MSG *pMsg)
	{
		for (auto e:m_DialogList) {
			if (e->ProcessMessage(pMsg))
				return true;
		}

		return false;
	}

	bool CTSTaskCentreCore::CopyTextToClipboard(const TSTask::String &Text) const
	{
		if (Text.empty())
			return false;

		bool fOK=false;
		const size_t Size=(Text.length()+1)*sizeof(WCHAR);
		HGLOBAL hData=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,Size);
		if (hData!=nullptr) {
			LPTSTR pBuffer=static_cast<LPWSTR>(::GlobalLock(hData));
			if (pBuffer!=nullptr) {
				::CopyMemory(pBuffer,Text.c_str(),Size);
				::GlobalUnlock(hData);
				if (::OpenClipboard(nullptr)) {
					::EmptyClipboard();
					if (::SetClipboardData(CF_UNICODETEXT,hData)!=nullptr)
						fOK=true;
					::CloseClipboard();
				}
			}
			if (!fOK)
				::GlobalFree(hData);
		}

		return fOK;
	}

	bool CTSTaskCentreCore::ExecuteTaskTool(TSTask::TaskID ID,const CToolsSettings::ToolInfo &Info)
	{
		class CCommandVariableStringMap : public TSTask::CVariableStringMap
		{
			enum TaskInfoStatus
			{
				INFO_STATUS_NOT_READY,
				INFO_STATUS_SUCCEEDED,
				INFO_STATUS_FAILED
			};

		public:
			CCommandVariableStringMap(CTSTaskCentreCore &Core,TSTask::TaskID ID)
				: m_Core(Core)
				, m_TaskID(ID)
				, m_fParameterAcquireFailed(false)
				, m_fParameterNotAvailable(false)
				, m_RecordingInfoStatus(INFO_STATUS_NOT_READY)
				, m_StreamingInfoStatus(INFO_STATUS_NOT_READY)
				, m_ServiceInfoStatus(INFO_STATUS_NOT_READY)
			{
			}

			bool GetString(LPCWSTR pszKeyword,TSTask::String *pString) override
			{
				if (::lstrcmpiW(pszKeyword,L"file")==0) {
					if (GetRecordingInfo()) {
						if (!m_RecordingInfo.FilePaths.empty()) {
							*pString=m_RecordingInfo.FilePaths.front();
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"file-cur")==0) {
					if (GetRecordingInfo()) {
						if (!m_RecordingInfo.FilePaths.empty()) {
							*pString=m_RecordingInfo.FilePaths.back();
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"rec-dir")==0) {
					if (GetRecordingInfo()) {
						if (!m_RecordingInfo.FilePaths.empty()) {
							*pString=m_RecordingInfo.FilePaths.front();
							TSTask::PathUtility::RemoveFileName(pString);
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"rec-dir-cur")==0) {
					if (GetRecordingInfo()) {
						if (!m_RecordingInfo.FilePaths.empty()) {
							*pString=m_RecordingInfo.FilePaths.back();
							TSTask::PathUtility::RemoveFileName(pString);
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"port")==0) {
					if (GetStreamingInfo()) {
						if (m_StreamingInfo.Address.Port!=0) {
							TSTask::StringUtility::Format(*pString,L"%d",m_StreamingInfo.Address.Port);
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"protocol")==0) {
					if (GetStreamingInfo()) {
						LPCWSTR pszProtocol=TSTask::Streaming::GetProtocolText(m_StreamingInfo.Address.Protocol);
						if (pszProtocol!=nullptr) {
							*pString=pszProtocol;
						} else {
							m_fParameterNotAvailable=true;
						}
					}
				} else if (::lstrcmpiW(pszKeyword,L"task-id")==0) {
					TSTask::StringUtility::Format(*pString,L"%u",m_TaskID);
				} else if (::lstrcmpiW(pszKeyword,L"task-id-0")==0) {
					TSTask::StringUtility::Format(*pString,L"%u",m_TaskID-1);
				} else if (::lstrcmpiW(pszKeyword,L"task-id-13")==0) {
					TSTask::StringUtility::Format(*pString,L"%u",m_TaskID+12);
				} else if (::lstrcmpiW(pszKeyword,L"service-name")==0) {
					if (GetServiceInfo())
						*pString=m_ServiceInfo.ServiceName;
				} else if (::lstrcmpiW(pszKeyword,L"service-id")==0) {
					if (GetServiceInfo(false))
						TSTask::StringUtility::Format(*pString,L"%u",m_ServiceInfo.ServiceID);
					else
						*pString=L"0";
				} else {
					return false;
				}

				return true;
			}

			bool IsParameterNotAvailable() const
			{
				return m_fParameterNotAvailable;
			}

			bool IsParameterAcquireFailed() const
			{
				return m_fParameterAcquireFailed;
			}

		private:
			bool GetRecordingInfo()
			{
				if (m_RecordingInfoStatus==INFO_STATUS_NOT_READY) {
					if (m_Core.GetTSTaskManager().GetRecordingInfo(m_TaskID,&m_RecordingInfo)) {
						m_RecordingInfoStatus=INFO_STATUS_SUCCEEDED;
					} else {
						m_RecordingInfoStatus=INFO_STATUS_FAILED;
						m_fParameterAcquireFailed=true;
					}
				}
				return m_RecordingInfoStatus==INFO_STATUS_SUCCEEDED;
			}

			bool GetStreamingInfo()
			{
				if (m_StreamingInfoStatus==INFO_STATUS_NOT_READY) {
					if (m_Core.GetTSTaskManager().GetStreamingInfo(m_TaskID,&m_StreamingInfo)) {
						m_StreamingInfoStatus=INFO_STATUS_SUCCEEDED;
					} else if (m_Core.GetTSTaskManager().GetStreamingSettings(m_TaskID,&m_StreamingInfo)) {
						m_StreamingInfoStatus=INFO_STATUS_SUCCEEDED;
					} else {
						m_StreamingInfoStatus=INFO_STATUS_FAILED;
						m_fParameterAcquireFailed=true;
					}
				}
				return m_StreamingInfoStatus==INFO_STATUS_SUCCEEDED;
			}

			bool GetServiceInfo(bool fMust=true)
			{
				if (m_ServiceInfoStatus==INFO_STATUS_NOT_READY) {
					if (m_Core.GetTSTaskManager().GetService(m_TaskID,&m_ServiceInfo))
						m_ServiceInfoStatus=INFO_STATUS_SUCCEEDED;
					else
						m_ServiceInfoStatus=INFO_STATUS_FAILED;
				}
				if (m_ServiceInfoStatus==INFO_STATUS_FAILED && fMust)
					m_fParameterAcquireFailed=true;
				return m_ServiceInfoStatus==INFO_STATUS_SUCCEEDED;
			}

			CTSTaskCentreCore &m_Core;
			TSTask::TaskID m_TaskID;
			bool m_fParameterAcquireFailed;
			bool m_fParameterNotAvailable;
			TaskInfoStatus m_RecordingInfoStatus;
			TSTask::RecordingInfo m_RecordingInfo;
			TaskInfoStatus m_StreamingInfoStatus;
			TSTask::StreamingInfo m_StreamingInfo;
			TaskInfoStatus m_ServiceInfoStatus;
			TSTask::ServiceInfo m_ServiceInfo;
		};

		if (Info.Command.empty())
			return false;

		TSTask::OutLog(TSTask::LOG_INFO,L"ツールを実行します。(%s : %s)",
					   Info.Name.c_str(),Info.Command.c_str());

		TSTask::String FilePath,Parameters;
		TSTask::String::size_type Pos;
		if (Info.Command[0]==L'"') {
			Pos=Info.Command.find(L'"',1);
			if (Pos==TSTask::String::npos) {
				FilePath=Info.Command.substr(1);
			} else {
				FilePath=Info.Command.substr(1,Pos-1);
				Pos+=2;
			}
		} else {
			Pos=Info.Command.find(L' ');
			if (Pos==TSTask::String::npos) {
				FilePath=Info.Command;
			} else {
				FilePath=Info.Command.substr(0,Pos);
				Pos++;
			}
		}

		if (TSTask::StringUtility::CompareNoCase(FilePath,L"message")==0) {
			TSTask::OutLog(TSTask::LOG_INFO,L"メッセージを送信します。(%s)",Info.Command.c_str()+Pos);

			TSTask::CMessageTranslator Translator;
			std::vector<TSTask::CMessage> MessageList;

			if (!Translator.ParseSingleLine(&MessageList,Info.Command.c_str()+Pos))
				return false;
			for (auto &e:MessageList) {
				//SetMessageDefaultProperties(&e);
				if (!m_TSTaskManager.SendMessage(ID,&e))
					return false;
			}
		} else {
			if (TSTask::PathUtility::IsRelative(FilePath)) {
				TSTask::String Dir,Path;
				if (TSTask::GetModuleDirectory(nullptr,&Dir)
						&& TSTask::PathUtility::RelativeToAbsolute(&Path,Dir,FilePath)
						&& ::GetFileAttributes(Path.c_str())!=INVALID_FILE_ATTRIBUTES)
					FilePath=Path;
			}

			if (Pos<Info.Command.length()) {
				CCommandVariableStringMap StringMap(*this,ID);
				if (!TSTask::FormatVariableString(&Parameters,&StringMap,Info.Command.c_str()+Pos))
					return false;
				if (StringMap.IsParameterNotAvailable()) {
					TSTask::OutLog(TSTask::LOG_ERROR,L"パラメータが利用できません。");
					return false;
				}
				if (StringMap.IsParameterAcquireFailed()) {
					TSTask::OutLog(TSTask::LOG_ERROR,L"パラメータに必要な情報を取得できません。");
					return false;
				}
			}

			TSTask::OutLog(TSTask::LOG_INFO,L"シェルによるコマンド実行を行ないます。(%s : %s)",
						   FilePath.c_str(),Parameters.c_str());

			SHELLEXECUTEINFO sei;
			::ZeroMemory(&sei,sizeof(sei));
			sei.cbSize=sizeof(SHELLEXECUTEINFO);
			sei.fMask=0;
			sei.hwnd=nullptr;
			sei.lpVerb=nullptr;
			sei.lpFile=FilePath.c_str();
			sei.lpParameters=Parameters.empty()?nullptr:Parameters.c_str();
			sei.lpDirectory=Info.CurrentDirectory.empty()?nullptr:Info.CurrentDirectory.c_str();
			sei.nShow=Info.ShowCommand;
			if (!::ShellExecuteEx(&sei)) {
				TSTask::OutSystemErrorLog(::GetLastError(),L"ツールを実行できません。");
				return false;
			}
		}

		return true;
	}

	bool CTSTaskCentreCore::ExecuteTaskToolOnEvent(TSTask::TaskID ID,LPCWSTR pszEvent)
	{
		if (TSTask::IsStringEmpty(pszEvent))
			return false;

		int i=m_Settings.Tools.FindTrigger(pszEvent);
		if (i<0)
			return true;

		TSTask::OutLog(TSTask::LOG_INFO,L"イベントによりツールを実行します。(%s)",pszEvent);

		do {
			CToolsSettings::ToolInfo Info;
			if (!m_Settings.Tools.GetToolInfo(i,&Info)
					|| !ExecuteTaskTool(ID,Info))
				return false;
		} while ((i=m_Settings.Tools.FindTrigger(pszEvent,i+1))>=0);

		return true;
	}

	static void CopyMessageProperties(TSTask::CMessage *pMessage,const TSTask::CMessage &SrcMessage)
	{
		for (size_t i=0;i<SrcMessage.NumProperties();i++) {
			const TSTask::CMessageProperty *pProp=SrcMessage.GetPropertyByIndex(i);
			if (pProp!=nullptr
					&& !pMessage->HasProperty(pProp->GetName())) {
				TSTask::CMessageProperty *pCopyProp=pProp->Duplicate();
				if (!pMessage->SetProperty(pCopyProp))
					delete pCopyProp;
			}
		}
	}

#if 0
	bool CTSTaskCentreCore::SetMessageDefaultProperties(TSTask::CMessage *pMessage)
	{
		if (pMessage==nullptr)
			return false;

		if (::lstrcmpW(pMessage->GetName(),L"StartRecording")==0) {
			TSTask::RecordingSettings Settings;
			TSTask::CMessage DefaultMessage;

			m_Settings.Recording.GetRecordingSettings(&Settings);
			TSTask::BasicMessage::StartRecording::InitializeMessage(&DefaultMessage,Settings);
			// Directories の扱いが微妙
			CopyMessageProperties(pMessage,DefaultMessage);
		} else if (::lstrcmpW(pMessage->GetName(),L"StartStreaming")==0) {
			TSTask::StreamingInfo Info;
			TSTask::CMessage DefaultMessage;

			m_Settings.Streaming.GetStreamingSettings(&Info);
			TSTask::BasicMessage::StartStreaming::InitializeMessage(&DefaultMessage,Info);
			CopyMessageProperties(pMessage,DefaultMessage);
		} else {
			return false;
		}

		return true;
	}
#endif

}
