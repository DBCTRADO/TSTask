#ifndef TSTASK_SERVER_SETTINGS_H
#define TSTASK_SERVER_SETTINGS_H


namespace TSTask
{

	enum ProcessPriority
	{
		PROCESS_PRIORITY_INVALID=-3,
		PROCESS_PRIORITY_LOWEST,
		PROCESS_PRIORITY_LOWER,
		PROCESS_PRIORITY_NORMAL,
		PROCESS_PRIORITY_HIGHER,
		PROCESS_PRIORITY_HIGHEST
	};

	class CGeneralSettings : public CSettingsBase
	{
	public:
		static const unsigned int STATISTICS_UPDATE_INTERVAL_MIN=200;

		CGeneralSettings(CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(CSettings &Settings) override;
		bool Save(CSettings &Settings) const override;

		bool GetIniFilePath(String *pFilePath) const;
		bool SetIniFilePath(const String &FilePath);

		bool GetClientFilePath(String *pFilePath) const;
		bool SetClientFilePath(const String &FilePath);
		bool GetClientOptions(String *pOptions) const;
		bool SetClientOptions(const String &Options);
		bool GetClientCommand(String *pCommand) const;
		int GetClientShowCommand() const;
		bool SetClientShowCommand(int Command);
		bool GetClientExecuteOnStart() const;
		bool SetClientExecuteOnStart(bool fExecute);

		TaskID GetTaskID() const;
		bool SetTaskID(TaskID ID);
		ProcessPriority GetProcessPriority() const;
		bool SetProcessPriority(ProcessPriority Priority);
		DWORD GetProcessPriorityClass() const;
		unsigned int GetStatisticsUpdateInterval() const;
		bool SetStatisticsUpdateInterval(unsigned int Interval);

		bool SetLoggingLevel(LogType Level);
		LogType GetLoggingLevel() const;
		static LogType LoggingLevelToType(int Level) { return LogType(LOG_NONE-Level); }
		bool SetMaxLog(unsigned int Max);
		unsigned int GetMaxLog() const;
		bool GetLogOutputToFile() const;
		void SetLogOutputToFile(bool fOutputToFile);
		bool GetLogFileName(String *pFileName) const;
		bool SetLogFileName(const String &FileName);
		bool GetLogFilePath(String *pPath) const;
		bool GetLogOverwrite() const;
		void SetLogOverwrite(bool fOverwrite);
		bool GetDebugLog() const;
		void SetDebugLog(bool fDebugLog);

	private:
		String m_IniFilePath;

		String m_ClientFilePath;
		String m_ClientOptions;
		int m_ClientShowCommand;
		bool m_fClientExecuteOnStart;

		TaskID m_TaskID;
		ProcessPriority m_ProcessPriority;
		unsigned int m_StatisticsUpdateInterval;

		LogType m_LoggingLevel;
		unsigned int m_MaxLog;
		bool m_fLogOutputToFile;
		String m_LogFileName;
		bool m_fLogOverwrite;
		bool m_fDebugLog;
	};

	class CBonDriverSettings : public CSettingsBase
	{
	public:
		CBonDriverSettings(CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(CSettings &Settings) override;
		bool Save(CSettings &Settings) const override;

		bool GetLoadDirectory(String *pDirectory) const;
		bool GetLoadDirectoryAbsolute(String *pDirectory) const;
		bool SetLoadDirectory(const String &Directory);

		unsigned int GetFirstChannelSetDelay() const;
		void SetFirstChannelSetDelay(unsigned int Delay);
		unsigned int GetMinChannelChangeInterval() const;
		void SetMinChannelChangeInterval(unsigned int Interval);

	private:
		String m_LoadDirectory;

		unsigned int m_FirstChannelSetDelay;
		unsigned int m_MinChannelChangeInterval;
	};

	class CRecordingSettings : public CSettingsBase
	{
	public:
		static const UINT WRITE_BUFFER_SIZE_MIN=1880;
		static const UINT WRITE_BUFFER_SIZE_MAX=0x1000000;

		static const UINT MAX_PENDING_SIZE_MIN=32*1024*1024;
		static const UINT MAX_PENDING_SIZE_MAX=1024*1024*1024;

		static const ULONGLONG PRE_ALLOCATE_SIZE_MAX=0x280000000ULL;

		CRecordingSettings(CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(CSettings &Settings) override;
		bool Save(CSettings &Settings) const override;

		bool GetDefaultDirectory(String *pDirectory) const;
		bool SetDefaultDirectory(const String &Directory);
		bool GetFileName(String *pFileName) const;
		bool SetFileName(const String &FileName);
		bool GetDirectory(int Index,String *pDirectory) const;
		bool GetDirectories(std::vector<String> *pDirectories) const;
		bool SetDirectories(const std::vector<String> &Directories);
		ULONGLONG GetMinFreeSpace() const;
		bool SetMinFreeSpace(ULONGLONG Size);
		ServiceSelectType GetServiceSelectType() const;
		bool SetServiceSelectType(ServiceSelectType Type);
		DWORD GetStreams() const;
		bool SetStreams(DWORD Streams);
		bool SetStreamFlag(DWORD Flag,bool fSet=true);
		bool GetRecordingSettings(RecordingSettings *pSettings) const;

		bool GetSystemRequired() const;
		void SetSystemRequired(bool fRequired);
		bool GetAwayModeRequired() const;
		void SetAwayModeRequired(bool fRequired);
		bool GetExitOnStop() const;
		void SetExitOnStop(bool fExit);

		unsigned int GetWriteBufferSize() const;
		bool SetWriteBufferSize(unsigned int Size);
		unsigned int GetMaxPendingSize() const;
		bool SetMaxPendingSize(unsigned int Size);
		bool GetPreAllocate() const;
		void SetPreAllocate(bool fPreAllocate);
		ULONGLONG GetPreAllocateSize() const;
		bool SetPreAllocateSize(ULONGLONG Size);
		unsigned int GetUseNextEventInfoMargin() const;
		void SetUseNextEventInfoMargin(unsigned int Margin);

	private:
		void SetStreamFlagImpl(DWORD Flag,bool fSet);

		String m_DefaultDirectory;
		String m_FileName;
		std::vector<String> m_DirectoryList;
		ULONGLONG m_MinFreeSpace;
		ServiceSelectType m_ServiceSelect;
		DWORD m_Streams;

		bool m_fSystemRequired;
		bool m_fAwayModeRequired;
		bool m_fExitOnStop;

		unsigned int m_WriteBufferSize;
		unsigned int m_MaxPendingSize;
		bool m_fPreAllocate;
		ULONGLONG m_PreAllocateSize;
		unsigned int m_UseNextEventInfoMargin;
	};

	class CStreamPoolSettings : public CSettingsBase
	{
	public:
		CStreamPoolSettings(CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(CSettings &Settings) override;
		bool Save(CSettings &Settings) const override;

		unsigned int GetSize() const;
		bool SetSize(unsigned int Size);

	private:
		unsigned int m_Size;
	};

	class CStreamingSettings : public CSettingsBase
	{
	public:
		CStreamingSettings(CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(CSettings &Settings) override;
		bool Save(CSettings &Settings) const override;

		NetworkProtocolType GetProtocol() const;
		bool SetProtocol(NetworkProtocolType Protocol);
		bool GetAddress(String *pAddress) const;
		bool SetAddress(const String &Address);
		WORD GetPort() const;
		bool SetPort(WORD Port);
		bool GetFindUnusedPort() const;
		void SetFindUnusedPort(bool fFind);
		ServiceSelectType GetServiceSelectType() const;
		bool SetServiceSelectType(ServiceSelectType Type);
		DWORD GetStreams() const;
		bool SetStreams(DWORD Streams);
		bool SetStreamFlag(DWORD Flag,bool fSet=true);
		bool GetStreamingSettings(StreamingInfo *pInfo) const;

		unsigned int GetSendSize() const;
		bool SetSendSize(unsigned int Size);
		unsigned int GetSendWait() const;
		bool SetSendWait(unsigned int Wait);
		bool GetAdjustSendWait() const;
		void SetAdjustSendWait(bool fAdjust);
		void SetConnectRetryInterval(unsigned int Interval);
		unsigned int GetConnectRetryInterval() const;
		bool SetMaxConnectRetries(int MaxRetries);
		int GetMaxConnectRetries() const;
		int GetTcpMaxSendRetries() const;
		bool SetTcpMaxSendRetries(int MaxRetries);
		bool GetTcpPrependHeader() const;
		void SetTcpPrependHeader(bool fPrependHeader);

	private:
		void SetStreamFlagImpl(DWORD Flag,bool fSet);

		NetworkProtocolType m_Protocol;
		String m_Address;
		WORD m_Port;
		bool m_fFindUnusedPort;
		ServiceSelectType m_ServiceSelect;
		DWORD m_Streams;

		unsigned int m_SendSize;
		unsigned int m_SendWait;
		bool m_fAdjustSendWait;
		unsigned int m_ConnectRetryInterval;
		int m_MaxConnectRetries;
		int m_TcpMaxSendRetries;
		bool m_fTcpPrependHeader;
	};

	class CTSTaskSettings
	{
	public:
		CGeneralSettings General;
		CBonDriverSettings BonDriver;
		CRecordingSettings Recording;
		CStreamPoolSettings StreamPool;
		CStreamingSettings Streaming;

		CTSTaskSettings();
		void SetDefault();
		bool Load(CSettings &Settings);
		bool Save(CSettings &Settings) const;
		void Set(const CTSTaskSettings &Src);

	private:
		static CRWLock m_Lock;
	};

}


#endif
