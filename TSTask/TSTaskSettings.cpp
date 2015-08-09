#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskSettings.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CGeneralSettings::CGeneralSettings(CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CGeneralSettings::SetDefault()
	{
		CRWLockWrite Lock(*m_pLock);

		GetModuleFilePath(nullptr,&m_IniFilePath);
		PathUtility::RenameExtension(&m_IniFilePath,L".ini");

		m_TaskID=INVALID_TASK_ID;
		m_ProcessPriority=PROCESS_PRIORITY_INVALID;
		m_StatisticsUpdateInterval=1000;

		m_ClientFilePath=L"TSTaskCentre.exe";
		m_ClientOptions.clear();
		m_ClientShowCommand=-1;
		m_fClientExecuteOnStart=true;

		m_LoggingLevel=LOG_INFO;
		m_MaxLog=500;
		m_fLogOutputToFile=false;
		PathUtility::GetFileName(m_IniFilePath,&m_LogFileName);
		PathUtility::RenameExtension(&m_LogFileName,L".log");
		m_fLogOverwrite=true;
		m_fDebugLog=
#ifdef _DEBUG
			true;
#else
			false;
#endif
	}

	bool CGeneralSettings::Load(CSettings &Settings)
	{
		CRWLockWrite Lock(*m_pLock);

		int Value;

		Settings.Read(L"Client.FilePath",&m_ClientFilePath);
		Settings.Read(L"Client.Options",&m_ClientOptions);
		Settings.Read(L"Client.ShowCommand",&m_ClientShowCommand);
		Settings.Read(L"Client.ExecuteOnStart",&m_fClientExecuteOnStart);

		unsigned int TaskID;
		if (Settings.Read(L"General.TaskID",&TaskID))
			SetTaskID(TaskID);
		if (Settings.Read(L"General.ProcessPriority",&Value))
			SetProcessPriority(ProcessPriority(Value));
		unsigned int Interval;
		if (Settings.Read(L"General.StatisticsUpdateInterval",&Interval))
			SetStatisticsUpdateInterval(Interval);

		if (Settings.Read(L"Logging.Level",&Value))
			SetLoggingLevel(LogType(LOG_NONE-Value));
		Settings.Read(L"Logging.MaxCount",&m_MaxLog);
		Settings.Read(L"Logging.OutputToFile",&m_fLogOutputToFile);
		String FileName;
		if (Settings.Read(L"Logging.FileName",&FileName) && !FileName.empty())
			m_LogFileName=FileName;
		Settings.Read(L"Logging.Overwrite",&m_fLogOverwrite);
		Settings.Read(L"Logging.Debug",&m_fDebugLog);

		return true;
	}

	bool CGeneralSettings::Save(CSettings &Settings) const
	{
		CRWLockRead Lock(*m_pLock);

		Settings.Write(L"Client.FilePath",m_ClientFilePath);
		Settings.Write(L"Client.Options",m_ClientOptions);
		Settings.Write(L"Client.ShowCommand",m_ClientShowCommand);
		Settings.Write(L"Client.ExecuteOnStart",m_fClientExecuteOnStart);

		if (m_TaskID!=INVALID_TASK_ID)
			Settings.Write(L"General.TaskID",m_TaskID);
		else
			Settings.Write(L"General.TaskID",L"");
		if (m_ProcessPriority!=PROCESS_PRIORITY_INVALID)
			Settings.Write(L"General.ProcessPriority",(int)m_ProcessPriority);
		else
			Settings.Write(L"General.ProcessPriority",L"");
		Settings.Write(L"General.StatisticsUpdateInterval",m_StatisticsUpdateInterval);

		Settings.Write(L"Logging.Level",LOG_NONE-(int)m_LoggingLevel);
		Settings.Write(L"Logging.MaxCount",m_MaxLog);
		Settings.Write(L"Logging.OutputToFile",m_fLogOutputToFile);
		Settings.Write(L"Logging.FileName",m_LogFileName);
		Settings.Write(L"Logging.Overwrite",m_fLogOverwrite);
		Settings.Write(L"Logging.Debug",m_fDebugLog);

		return true;
	}

	bool CGeneralSettings::GetIniFilePath(String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pFilePath=m_IniFilePath;

		return true;
	}

	bool CGeneralSettings::SetIniFilePath(const String &FilePath)
	{
		if (FilePath.empty())
			return false;

		CRWLockWrite Lock(*m_pLock);

		m_IniFilePath=FilePath;

		return true;
	}

	bool CGeneralSettings::GetClientFilePath(String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pFilePath=m_ClientFilePath;

		return true;
	}

	bool CGeneralSettings::SetClientFilePath(const String &FilePath)
	{
		CRWLockWrite Lock(*m_pLock);

		m_ClientFilePath=FilePath;

		return true;
	}

	bool CGeneralSettings::GetClientOptions(String *pOptions) const
	{
		if (pOptions==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pOptions=m_ClientOptions;

		return true;
	}

	bool CGeneralSettings::SetClientOptions(const String &Options)
	{
		CRWLockWrite Lock(*m_pLock);

		m_ClientOptions=Options;

		return true;
	}

	int CGeneralSettings::GetClientShowCommand() const
	{
		return m_ClientShowCommand;
	}

	bool CGeneralSettings::SetClientShowCommand(int Command)
	{
		m_ClientShowCommand=Command;

		return true;
	}

	bool CGeneralSettings::GetClientCommand(String *pCommand) const
	{
		if (pCommand==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		if (m_ClientFilePath.empty())
			return false;

		pCommand->append(L"\"");
		if (PathUtility::IsRelative(m_ClientFilePath)) {
			String Dir,Path;
			if (GetModuleDirectory(nullptr,&Dir)
					&& PathUtility::RelativeToAbsolute(&Path,Dir,m_ClientFilePath)) {
				pCommand->append(Path);
			} else {
				pCommand->append(m_ClientFilePath);
			}
		} else {
			pCommand->append(m_ClientFilePath);
		}
		pCommand->append(L"\"");

		if (!m_ClientOptions.empty()) {
			pCommand->append(L" ");
			pCommand->append(m_ClientOptions);
		}

		return true;
	}

	bool CGeneralSettings::GetClientExecuteOnStart() const
	{
		return m_fClientExecuteOnStart;
	}

	bool CGeneralSettings::SetClientExecuteOnStart(bool fExecute)
	{
		m_fClientExecuteOnStart=fExecute;

		return true;
	}

	TaskID CGeneralSettings::GetTaskID() const
	{
		return m_TaskID;
	}

	bool CGeneralSettings::SetTaskID(TaskID ID)
	{
		if (ID!=INVALID_TASK_ID && !TaskUtility::IsServerTaskIDValid(ID))
			return false;

		m_TaskID=ID;

		return true;
	}

	ProcessPriority CGeneralSettings::GetProcessPriority() const
	{
		return m_ProcessPriority;
	}

	bool CGeneralSettings::SetProcessPriority(ProcessPriority Priority)
	{
		if (Priority<PROCESS_PRIORITY_INVALID || Priority>PROCESS_PRIORITY_HIGHEST)
			return false;

		m_ProcessPriority=Priority;

		return true;
	}

	DWORD CGeneralSettings::GetProcessPriorityClass() const
	{
		const ProcessPriority Priority=m_ProcessPriority;

		switch (Priority) {
		case PROCESS_PRIORITY_LOWEST:	return IDLE_PRIORITY_CLASS;
		case PROCESS_PRIORITY_LOWER:	return BELOW_NORMAL_PRIORITY_CLASS;
		case PROCESS_PRIORITY_HIGHER:	return ABOVE_NORMAL_PRIORITY_CLASS;
		case PROCESS_PRIORITY_HIGHEST:	return HIGH_PRIORITY_CLASS;
		}
		return NORMAL_PRIORITY_CLASS;
	}

	unsigned int CGeneralSettings::GetStatisticsUpdateInterval() const
	{
		return m_StatisticsUpdateInterval;
	}

	bool CGeneralSettings::SetStatisticsUpdateInterval(unsigned int Interval)
	{
		if (Interval<STATISTICS_UPDATE_INTERVAL_MIN)
			return false;

		m_StatisticsUpdateInterval=Interval;

		return true;
	}

	bool CGeneralSettings::SetLoggingLevel(LogType Level)
	{
		if (Level<0 || Level>LOG_NONE)
			return false;

		m_LoggingLevel=Level;

		return true;
	}

	LogType CGeneralSettings::GetLoggingLevel() const
	{
		return m_LoggingLevel;
	}

	bool CGeneralSettings::SetMaxLog(unsigned int Max)
	{
		m_MaxLog=Max;

		return true;
	}

	unsigned int CGeneralSettings::GetMaxLog() const
	{
		return m_MaxLog;
	}

	bool CGeneralSettings::GetLogOutputToFile() const
	{
		return m_fLogOutputToFile;
	}

	void CGeneralSettings::SetLogOutputToFile(bool fOutputToFile)
	{
		m_fLogOutputToFile=fOutputToFile;
	}

	bool CGeneralSettings::GetLogFileName(String *pFileName) const
	{
		if (pFileName==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pFileName=m_LogFileName;

		return true;
	}

	bool CGeneralSettings::SetLogFileName(const String &FileName)
	{
		if (FileName.empty())
			return false;

		CRWLockWrite Lock(*m_pLock);

		m_LogFileName=FileName;

		return true;
	}

	bool CGeneralSettings::GetLogFilePath(String *pPath) const
	{
		if (pPath==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		if (PathUtility::IsRelative(m_LogFileName)) {
			String Dir;
			if (!GetModuleDirectory(nullptr,&Dir)
					|| !PathUtility::RelativeToAbsolute(pPath,Dir,m_LogFileName))
				*pPath=m_LogFileName;
		} else {
			*pPath=m_LogFileName;
		}

		return true;
	}

	bool CGeneralSettings::GetLogOverwrite() const
	{
		return m_fLogOverwrite;
	}

	void CGeneralSettings::SetLogOverwrite(bool fOverwrite)
	{
		m_fLogOverwrite=fOverwrite;
	}

	bool CGeneralSettings::GetDebugLog() const
	{
		return m_fDebugLog;
	}

	void CGeneralSettings::SetDebugLog(bool fDebugLog)
	{
		m_fDebugLog=fDebugLog;
	}


	CBonDriverSettings::CBonDriverSettings(CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CBonDriverSettings::SetDefault()
	{
		CRWLockWrite Lock(*m_pLock);

		m_LoadDirectory.clear();

		m_FirstChannelSetDelay=0;
		m_MinChannelChangeInterval=0;
	}

	bool CBonDriverSettings::Load(CSettings &Settings)
	{
		CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"BonDriver.LoadDirectory",&m_LoadDirectory);

		Settings.Read(L"Tuner.FirstChannelSetDelay",&m_FirstChannelSetDelay);
		Settings.Read(L"Tuner.MinChannelChangeInterval",&m_MinChannelChangeInterval);

		return true;
	}

	bool CBonDriverSettings::Save(CSettings &Settings) const
	{
		CRWLockRead Lock(*m_pLock);

		Settings.Write(L"BonDriver.LoadDirectory",m_LoadDirectory);

		Settings.Write(L"Tuner.FirstChannelSetDelay",m_FirstChannelSetDelay);
		Settings.Write(L"Tuner.MinChannelChangeInterval",m_MinChannelChangeInterval);

		return true;
	}

	bool CBonDriverSettings::GetLoadDirectory(String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pDirectory=m_LoadDirectory;

		return true;
	}

	bool CBonDriverSettings::GetLoadDirectoryAbsolute(String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		if (m_LoadDirectory.empty())
			return GetModuleDirectory(nullptr,pDirectory);

		if (PathUtility::IsRelative(m_LoadDirectory)) {
			String Dir;
			if (GetModuleDirectory(nullptr,&Dir)
					&& PathUtility::RelativeToAbsolute(pDirectory,Dir,m_LoadDirectory))
				return true;
		}

		*pDirectory=m_LoadDirectory;

		return true;
	}

	bool CBonDriverSettings::SetLoadDirectory(const String &Directory)
	{
		CRWLockWrite Lock(*m_pLock);

		m_LoadDirectory=Directory;

		return true;
	}

	unsigned int CBonDriverSettings::GetFirstChannelSetDelay() const
	{
		return m_FirstChannelSetDelay;
	}

	void CBonDriverSettings::SetFirstChannelSetDelay(unsigned int Delay)
	{
		m_FirstChannelSetDelay=Delay;
	}

	unsigned int CBonDriverSettings::GetMinChannelChangeInterval() const
	{
		return m_MinChannelChangeInterval;
	}

	void CBonDriverSettings::SetMinChannelChangeInterval(unsigned int Interval)
	{
		m_MinChannelChangeInterval=Interval;
	}


	CRecordingSettings::CRecordingSettings(CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CRecordingSettings::SetDefault()
	{
		CRWLockWrite Lock(*m_pLock);

		GetModuleDirectory(nullptr,&m_DefaultDirectory);

		m_FileName=L"%event-title%_%date%-%time%.ts";

		m_DirectoryList.clear();
		m_MinFreeSpace=100*1024*1024;

		m_ServiceSelect=SERVICE_SELECT_ALL;
		m_Streams=STREAM_ALL;

		m_fSystemRequired=true;
		m_fAwayModeRequired=true;
		m_fExitOnStop=false;

		m_WriteBufferSize=0x200000;
		m_MaxPendingSize=512*1024*1024;

		m_fPreAllocate=false;
		m_PreAllocateSize=0x40000000LL;

		m_UseNextEventInfoMargin=120;
	}

	bool CRecordingSettings::Load(TSTask::CSettings &Settings)
	{
		CRWLockWrite Lock(*m_pLock);

		int Value;
		LONGLONG Size;

		Settings.Read(L"Recording.Directory",&m_DefaultDirectory);
		Settings.Read(L"Recording.FileName",&m_FileName);
		m_DirectoryList.clear();
		for (UINT i=0;;i++) {
			WCHAR szKey[32];
			String Directory;
			FormatString(szKey,_countof(szKey),L"Recording.Directory%u",(UINT)(i+1));
			if (!Settings.Read(szKey,&Directory) || Directory.empty())
				break;
			m_DirectoryList.push_back(Directory);
		}
		if (Settings.Read(L"Recording.MinFreeSpace",&Size) && Size>=0)
			m_MinFreeSpace=Size;
		if (Settings.Read(L"Recording.Service",&Value))
			SetServiceSelectType(ServiceSelectType(Value));
		bool f;
		if (Settings.Read(L"Recording.SaveCaption",&f))
			SetStreamFlagImpl(STREAM_CAPTION,f);
		if (Settings.Read(L"Recording.SaveDataCarrousel",&f))
			SetStreamFlagImpl(STREAM_DATA_CARROUSEL,f);
		if (Settings.Read(L"Recording.1SegOnly",&f))
			SetStreamFlagImpl(STREAM_1SEG,f);

		Settings.Read(L"Recording.SystemRequired",&m_fSystemRequired);
		Settings.Read(L"Recording.AwayModeRequired",&m_fAwayModeRequired);
		Settings.Read(L"Recording.ExitOnStop",&m_fExitOnStop);

		if (Settings.Read(L"Recording.WriteBufferSize",&Value)
				&& Value>=WRITE_BUFFER_SIZE_MIN && Value<=WRITE_BUFFER_SIZE_MAX)
			m_WriteBufferSize=Value;
		if (Settings.Read(L"Recording.MaxPendingSize",&Value)
				&& Value>=MAX_PENDING_SIZE_MIN && Value<=MAX_PENDING_SIZE_MAX)
			m_MaxPendingSize=Value;
		Settings.Read(L"Recording.PreAllocate",&m_fPreAllocate);
		if (Settings.Read(L"Recording.PreAllocateSize",&Size) && Size>=0)
			SetPreAllocateSize(Size);
		Settings.Read(L"Recording.UseNextEventInfoMargin",&m_UseNextEventInfoMargin);

		return true;
	}

	bool CRecordingSettings::Save(TSTask::CSettings &Settings) const
	{
		CRWLockRead Lock(*m_pLock);

		Settings.Write(L"Recording.Directory",m_DefaultDirectory);
		Settings.Write(L"Recording.FileName",m_FileName);
		for (size_t i=0;;i++) {
			WCHAR szKey[32];
			FormatString(szKey,_countof(szKey),L"Recording.Directory%u",(UINT)(i+1));
			if (i<m_DirectoryList.size()) {
				Settings.Write(szKey,m_DirectoryList[i]);
			} else {
				if (!Settings.IsKeyExists(szKey))
					break;
				Settings.DeleteKey(szKey);
			}
		}
		Settings.Write(L"Recording.MinFreeSpace",(LONGLONG)m_MinFreeSpace);
		Settings.Write(L"Recording.Service",(int)m_ServiceSelect);
		Settings.Write(L"Recording.SaveCaption",(m_Streams&STREAM_CAPTION)!=0);
		Settings.Write(L"Recording.SaveDataCarrousel",(m_Streams&STREAM_DATA_CARROUSEL)!=0);
		Settings.Write(L"Recording.1SegOnly",(m_Streams&STREAM_1SEG)!=0);

		Settings.Write(L"Recording.SystemRequired",m_fSystemRequired);
		Settings.Write(L"Recording.AwayModeRequired",m_fAwayModeRequired);
		Settings.Write(L"Recording.ExitOnStop",m_fExitOnStop);

		Settings.Write(L"Recording.WriteBufferSize",m_WriteBufferSize);
		Settings.Write(L"Recording.MaxPendingSize",m_MaxPendingSize);
		Settings.Write(L"Recording.PreAllocate",m_fPreAllocate);
		Settings.Write(L"Recording.PreAllocateSize",(LONGLONG)m_PreAllocateSize);
		Settings.Write(L"Recording.UseNextEventInfoMargin",m_UseNextEventInfoMargin);

		return true;
	}

	bool CRecordingSettings::GetDefaultDirectory(TSTask::String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pDirectory=m_DefaultDirectory;

		return true;
	}

	bool CRecordingSettings::SetDefaultDirectory(const String &Directory)
	{
		CRWLockWrite Lock(*m_pLock);

		m_DefaultDirectory=Directory;

		return true;
	}

	bool CRecordingSettings::GetFileName(String *pFileName) const
	{
		if (pFileName==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pFileName=m_FileName;

		return true;
	}

	bool CRecordingSettings::SetFileName(const String &FileName)
	{
		if (FileName.empty())
			return false;

		CRWLockWrite Lock(*m_pLock);

		m_FileName=FileName;

		return true;
	}

	bool CRecordingSettings::GetDirectory(int Index,String *pDirectory) const
	{
		CRWLockRead Lock(*m_pLock);

		if (Index<0 || (size_t)Index>=m_DirectoryList.size())
			return false;

		if (pDirectory!=nullptr)
			*pDirectory=m_DirectoryList[Index];

		return true;
	}

	bool CRecordingSettings::GetDirectories(std::vector<String> *pDirectories) const
	{
		if (pDirectories==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pDirectories=m_DirectoryList;

		return true;
	}

	bool CRecordingSettings::SetDirectories(const std::vector<String> &Directories)
	{
		CRWLockWrite Lock(*m_pLock);

		m_DirectoryList=Directories;

		return true;
	}

	ULONGLONG CRecordingSettings::GetMinFreeSpace() const
	{
		return AtomicGet64(&m_MinFreeSpace);
	}

	bool CRecordingSettings::SetMinFreeSpace(ULONGLONG Size)
	{
		AtomicSet64(&m_MinFreeSpace,Size);

		return true;
	}

	ServiceSelectType CRecordingSettings::GetServiceSelectType() const
	{
		return m_ServiceSelect;
	}

	bool CRecordingSettings::SetServiceSelectType(ServiceSelectType Type)
	{
		if (Type<0 || Type>=SERVICE_SELECT_TRAILER)
			return false;

		m_ServiceSelect=Type;

		return true;
	}

	DWORD CRecordingSettings::GetStreams() const
	{
		return m_Streams;
	}

	bool CRecordingSettings::SetStreams(DWORD Streams)
	{
		m_Streams=Streams;

		return true;
	}

	bool CRecordingSettings::SetStreamFlag(DWORD Flag,bool fSet)
	{
		if (Flag==0)
			return false;

		CRWLockWrite Lock(*m_pLock);

		SetStreamFlagImpl(Flag,fSet);

		return true;
	}

	void CRecordingSettings::SetStreamFlagImpl(DWORD Flag,bool fSet)
	{
		if (fSet)
			m_Streams|=Flag;
		else
			m_Streams&=~Flag;
	}

	bool CRecordingSettings::GetRecordingSettings(RecordingSettings *pSettings) const
	{
		if (pSettings==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		pSettings->FileName=m_FileName;
		pSettings->Directories.clear();
		if (!m_DefaultDirectory.empty())
			pSettings->Directories.push_back(m_DefaultDirectory);
		for (const auto &e:m_DirectoryList)
			pSettings->Directories.push_back(e);
		pSettings->ServiceSelect=m_ServiceSelect;
		pSettings->Streams=m_Streams;

		return true;
	}

	bool CRecordingSettings::GetSystemRequired() const
	{
		return m_fSystemRequired;
	}

	void CRecordingSettings::SetSystemRequired(bool fRequired)
	{
		m_fSystemRequired=fRequired;
	}

	bool CRecordingSettings::GetAwayModeRequired() const
	{
		return m_fAwayModeRequired;
	}

	void CRecordingSettings::SetAwayModeRequired(bool fRequired)
	{
		m_fAwayModeRequired=fRequired;
	}

	bool CRecordingSettings::GetExitOnStop() const
	{
		return m_fExitOnStop;
	}

	void CRecordingSettings::SetExitOnStop(bool fExit)
	{
		m_fExitOnStop=fExit;
	}

	unsigned int CRecordingSettings::GetWriteBufferSize() const
	{
		return m_WriteBufferSize;
	}

	bool CRecordingSettings::SetWriteBufferSize(unsigned int Size)
	{
		if (Size<WRITE_BUFFER_SIZE_MIN || Size>WRITE_BUFFER_SIZE_MAX) {
			OutLog(LOG_ERROR,L"ファイルへの書き出しサイズの指定が有効範囲外です。(%u Bytes : %u から %u まで)",
				   Size,WRITE_BUFFER_SIZE_MIN,WRITE_BUFFER_SIZE_MAX);
			return false;
		}

		m_WriteBufferSize=Size;

		return true;
	}

	unsigned int CRecordingSettings::GetMaxPendingSize() const
	{
		return m_MaxPendingSize;
	}

	bool CRecordingSettings::SetMaxPendingSize(unsigned int Size)
	{
		if (Size<MAX_PENDING_SIZE_MIN || Size>MAX_PENDING_SIZE_MAX) {
			OutLog(LOG_ERROR,L"ファイルへの書き出し待ち最大サイズの指定が有効範囲外です。(%u Bytes : %u から %u まで)",
				   Size,MAX_PENDING_SIZE_MIN,MAX_PENDING_SIZE_MAX);
			return false;
		}

		m_WriteBufferSize=Size;

		return true;
	}

	bool CRecordingSettings::GetPreAllocate() const
	{
		return m_fPreAllocate;
	}

	void CRecordingSettings::SetPreAllocate(bool fPreAllocate)
	{
		m_fPreAllocate=fPreAllocate;
	}

	ULONGLONG CRecordingSettings::GetPreAllocateSize() const
	{
		return AtomicGet64(&m_PreAllocateSize);
	}

	bool CRecordingSettings::SetPreAllocateSize(ULONGLONG Size)
	{
		if (Size>PRE_ALLOCATE_SIZE_MAX) {
			OutLog(LOG_ERROR,L"ファイル領域の事前確保サイズの指定が有効範囲外です。(%llu Bytes : %llu まで)",
				   Size,PRE_ALLOCATE_SIZE_MAX);
			return false;
		}

		AtomicSet64(&m_PreAllocateSize,Size);

		return true;
	}

	unsigned int CRecordingSettings::GetUseNextEventInfoMargin() const
	{
		return m_UseNextEventInfoMargin;
	}

	void CRecordingSettings::SetUseNextEventInfoMargin(unsigned int Margin)
	{
		m_UseNextEventInfoMargin=Margin;
	}


	CStreamPoolSettings::CStreamPoolSettings(CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CStreamPoolSettings::SetDefault()
	{
		CRWLockWrite Lock(*m_pLock);

		m_Size=10*1024*1024;
	}

	bool CStreamPoolSettings::Load(CSettings &Settings)
	{
		CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"StreamPool.Size",&m_Size);

		return true;
	}

	bool CStreamPoolSettings::Save(CSettings &Settings) const
	{
		CRWLockRead Lock(*m_pLock);

		Settings.Write(L"StreamPool.Size",m_Size);

		return true;
	}

	unsigned int CStreamPoolSettings::GetSize() const
	{
		return m_Size;
	}

	bool CStreamPoolSettings::SetSize(unsigned int Size)
	{
		if (Size<188)
			return false;

		m_Size=Size;

		return true;
	}


	CStreamingSettings::CStreamingSettings(CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CStreamingSettings::SetDefault()
	{
		CRWLockWrite Lock(*m_pLock);

		m_Protocol=PROTOCOL_UDP;
		m_Address=L"127.0.0.1";
		m_Port=1234;
		m_fFindUnusedPort=true;
		m_ServiceSelect=SERVICE_SELECT_CURRENT;
		m_Streams=STREAM_ALL & ~STREAM_DATA_CARROUSEL;

		m_SendSize=256;
		m_SendWait=10;
		m_fAdjustSendWait=true;
		m_ConnectRetryInterval=2000;
		m_MaxConnectRetries=15;
		m_TcpMaxSendRetries=1;
		m_fTcpPrependHeader=true;
	}

	bool CStreamingSettings::Load(CSettings &Settings)
	{
		CRWLockWrite Lock(*m_pLock);

		int Value;

		String Protocol;
		if (Settings.Read(L"Streaming.Protocol",&Protocol))
			SetProtocol(Streaming::ParseProtocolText(Protocol.c_str()));
		Settings.Read(L"Streaming.Address",&m_Address);
		if (Settings.Read(L"Streaming.Port",&Value)
				&& Value>=0 && Value<=0xFFFF)
			m_Port=WORD(Value);
		Settings.Read(L"Streaming.FindUnusedPort",&m_fFindUnusedPort);
		if (Settings.Read(L"Streaming.Service",&Value))
			SetServiceSelectType(ServiceSelectType(Value));
		bool f;
		if (Settings.Read(L"Streaming.SendCaption",&f))
			SetStreamFlagImpl(STREAM_CAPTION,f);
		if (Settings.Read(L"Streaming.SendDataCarrousel",&f))
			SetStreamFlagImpl(STREAM_DATA_CARROUSEL,f);
		if (Settings.Read(L"Streaming.1SegOnly",&f))
			SetStreamFlagImpl(STREAM_1SEG,f);

		Settings.Read(L"Streaming.SendSize",&m_SendSize);
		Settings.Read(L"Streaming.SendWait",&m_SendWait);
		Settings.Read(L"Streaming.AdjustSendWait",&m_fAdjustSendWait);
		Settings.Read(L"Streaming.ConnectRetryInterval",&m_ConnectRetryInterval);
		Settings.Read(L"Streaming.MaxConnectRetries",&m_MaxConnectRetries);
		if (Settings.Read(L"Streaming.TcpMaxSendRetries",&Value))
			SetTcpMaxSendRetries(Value);
		Settings.Read(L"Streaming.TcpPrependHeader",&m_fTcpPrependHeader);

		return true;
	}

	bool CStreamingSettings::Save(CSettings &Settings) const
	{
		CRWLockRead Lock(*m_pLock);

		Settings.Write(L"Streaming.Protocol",Streaming::GetProtocolText(m_Protocol));
		Settings.Write(L"Streaming.Address",m_Address);
		Settings.Write(L"Streaming.Port",int(m_Port));
		Settings.Write(L"Streaming.FindUnusedPort",m_fFindUnusedPort);
		Settings.Write(L"Streaming.Service",int(m_ServiceSelect));
		Settings.Write(L"Streaming.SendCaption",(m_Streams&STREAM_CAPTION)!=0);
		Settings.Write(L"Streaming.SendDataCarrousel",(m_Streams&STREAM_DATA_CARROUSEL)!=0);
		Settings.Write(L"Streaming.1SegOnly",(m_Streams&STREAM_1SEG)!=0);

		Settings.Write(L"Streaming.SendSize",m_SendSize);
		Settings.Write(L"Streaming.SendWait",m_SendWait);
		Settings.Write(L"Streaming.AdjustSendWait",m_fAdjustSendWait);
		Settings.Write(L"Streaming.ConnectRetryInterval",m_ConnectRetryInterval);
		Settings.Write(L"Streaming.MaxConnectRetries",m_MaxConnectRetries);
		Settings.Write(L"Streaming.TcpMaxSendRetries",m_TcpMaxSendRetries);
		Settings.Write(L"Streaming.TcpPrependHeader",m_fTcpPrependHeader);

		return true;
	}

	NetworkProtocolType CStreamingSettings::GetProtocol() const
	{
		return m_Protocol;
	}

	bool CStreamingSettings::SetProtocol(NetworkProtocolType Protocol)
	{
		if (Protocol<0 || Protocol>=PROTOCOL_TRAILER)
			return false;

		m_Protocol=Protocol;

		return true;
	}

	bool CStreamingSettings::GetAddress(String *pAddress) const
	{
		if (pAddress==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		*pAddress=m_Address;

		return true;
	}

	bool CStreamingSettings::SetAddress(const String &Address)
	{
		CRWLockWrite Lock(*m_pLock);

		m_Address=Address;

		return true;
	}

	WORD CStreamingSettings::GetPort() const
	{
		return m_Port;
	}

	bool CStreamingSettings::SetPort(WORD Port)
	{
		m_Port=Port;

		return true;
	}

	bool CStreamingSettings::GetFindUnusedPort() const
	{
		return m_fFindUnusedPort;
	}

	void CStreamingSettings::SetFindUnusedPort(bool fFind)
	{
		m_fFindUnusedPort=fFind;
	}

	ServiceSelectType CStreamingSettings::GetServiceSelectType() const
	{
		return m_ServiceSelect;
	}

	bool CStreamingSettings::SetServiceSelectType(ServiceSelectType Type)
	{
		if (Type<0 || Type>=SERVICE_SELECT_TRAILER)
			return false;

		m_ServiceSelect=Type;

		return true;
	}

	DWORD CStreamingSettings::GetStreams() const
	{
		return m_Streams;
	}

	bool CStreamingSettings::SetStreams(DWORD Streams)
	{
		m_Streams=Streams;

		return true;
	}

	bool CStreamingSettings::SetStreamFlag(DWORD Flag,bool fSet)
	{
		if (Flag==0)
			return false;

		CRWLockWrite Lock(*m_pLock);

		SetStreamFlagImpl(Flag,fSet);

		return true;
	}

	void CStreamingSettings::SetStreamFlagImpl(DWORD Flag,bool fSet)
	{
		if (fSet)
			m_Streams|=Flag;
		else
			m_Streams&=~Flag;
	}

	bool CStreamingSettings::GetStreamingSettings(StreamingInfo *pInfo) const
	{
		if (pInfo==nullptr)
			return false;

		CRWLockRead Lock(*m_pLock);

		pInfo->Media=L"TS";
		pInfo->Address.Protocol=m_Protocol;
		pInfo->Address.Address=m_Address;
		pInfo->Address.Port=m_Port;
		pInfo->fFindUnusedPort=m_fFindUnusedPort;
		pInfo->ServiceSelect=m_ServiceSelect;
		pInfo->Streams=m_Streams;

		return true;
	}

	unsigned int CStreamingSettings::GetSendSize() const
	{
		return m_SendSize;
	}

	bool CStreamingSettings::SetSendSize(unsigned int Size)
	{
		if (Size<TS_PACKET_SIZE)
			return false;

		m_SendSize=Size;

		return true;
	}

	unsigned int CStreamingSettings::GetSendWait() const
	{
		return m_SendWait;
	}

	bool CStreamingSettings::SetSendWait(unsigned int Wait)
	{
		if (Wait==0)
			return false;

		m_SendWait=Wait;

		return true;
	}

	bool CStreamingSettings::GetAdjustSendWait() const
	{
		return m_fAdjustSendWait;
	}

	void CStreamingSettings::SetAdjustSendWait(bool fAdjust)
	{
		m_fAdjustSendWait=fAdjust;
	}

	void CStreamingSettings::SetConnectRetryInterval(unsigned int Interval)
	{
		m_ConnectRetryInterval=Interval;
	}

	unsigned int CStreamingSettings::GetConnectRetryInterval() const
	{
		return m_ConnectRetryInterval;
	}

	bool CStreamingSettings::SetMaxConnectRetries(int MaxRetries)
	{
		if (MaxRetries<0)
			return false;
		m_MaxConnectRetries=MaxRetries;
		return true;
	}

	int CStreamingSettings::GetMaxConnectRetries() const
	{
		return m_MaxConnectRetries;
	}

	int CStreamingSettings::GetTcpMaxSendRetries() const
	{
		return m_TcpMaxSendRetries;
	}

	bool CStreamingSettings::SetTcpMaxSendRetries(int MaxRetries)
	{
		if (MaxRetries<0)
			return false;
		m_TcpMaxSendRetries=MaxRetries;
		return true;
	}

	bool CStreamingSettings::GetTcpPrependHeader() const
	{
		return m_fTcpPrependHeader;
	}

	void CStreamingSettings::SetTcpPrependHeader(bool fPrependHeader)
	{
		m_fTcpPrependHeader=fPrependHeader;
	}


	CRWLock CTSTaskSettings::m_Lock;

	CTSTaskSettings::CTSTaskSettings()
		: General(&m_Lock)
		, BonDriver(&m_Lock)
		, Recording(&m_Lock)
		, StreamPool(&m_Lock)
		, Streaming(&m_Lock)
	{
	}

	void CTSTaskSettings::SetDefault()
	{
		General.SetDefault();
		BonDriver.SetDefault();
		Recording.SetDefault();
		StreamPool.SetDefault();
		Streaming.SetDefault();
	}

	bool CTSTaskSettings::Load(CSettings &Settings)
	{
		if (!Settings.SetSection(L"Settings"))
			return false;

		General.Load(Settings);
		BonDriver.Load(Settings);
		Recording.Load(Settings);
		StreamPool.Load(Settings);
		Streaming.Load(Settings);

		return true;
	}

	bool CTSTaskSettings::Save(CSettings &Settings) const
	{
		if (!Settings.SetSection(L"Settings"))
			return false;

		General.Save(Settings);
		BonDriver.Save(Settings);
		Recording.Save(Settings);
		StreamPool.Save(Settings);
		Streaming.Save(Settings);

		return true;
	}

	void CTSTaskSettings::Set(const CTSTaskSettings &Src)
	{
		CRWLockWrite Lock(m_Lock);

		*this=Src;
	}

}
