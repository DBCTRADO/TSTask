#ifndef TSTASKCENTER_SETTINGS_H
#define TSTASKCENTER_SETTINGS_H


#include "TSTaskBar.h"


namespace TSTaskCentre
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

	class CGeneralSettings : public TSTask::CSettingsBase
	{
	public:
		CGeneralSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		bool GetIniFilePath(TSTask::String *pFilePath) const;
		bool SetIniFilePath(const TSTask::String &FilePath);

		ProcessPriority GetProcessPriority() const;
		bool SetProcessPriority(ProcessPriority Priority);
		DWORD GetProcessPriorityClass() const;

		bool GetExitWhenAllTaskEnded() const;
		void SetExitWhenAllTaskEnded(bool fExit);

		bool SetLoggingLevel(TSTask::LogType Level);
		TSTask::LogType GetLoggingLevel() const;
		static TSTask::LogType LoggingLevelToType(int Level) { return TSTask::LogType(TSTask::LOG_NONE-Level); }
		static int LoggingTypeToLevel(TSTask::LogType Type) { return int(TSTask::LOG_NONE-Type); }
		void SetMaxLog(unsigned int Max);
		unsigned int GetMaxLog() const;
		bool GetLogOutputToFile() const;
		void SetLogOutputToFile(bool fOutputToFile);
		bool GetLogFilePath(TSTask::String *pPath) const;
		bool SetLogFileName(const TSTask::String &FileName);
		bool GetLogOverwrite() const;
		void SetLogOverwrite(bool fOverwrite);
		bool GetDebugLog() const;
		void SetDebugLog(bool fDebugLog);

	private:
		TSTask::String m_IniFilePath;
		ProcessPriority m_ProcessPriority;
		bool m_fExitWhenAllTaskEnded;
		TSTask::LogType m_LoggingLevel;
		unsigned int m_MaxLog;
		bool m_fLogOutputToFile;
		TSTask::String m_LogFileName;
		bool m_fLogOverwrite;
		bool m_fDebugLog;
	};

	class CTaskSettings : public TSTask::CSettingsBase
	{
	public:
		CTaskSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		bool GetDirectory(TSTask::String *pDirectory) const;
		bool SetDirectory(const TSTask::String &Directory);
		bool GetExeFileName(TSTask::String *pFileName) const;
		bool SetExeFileName(const TSTask::String &FileName);
		bool GetExeFilePath(TSTask::String *pFilePath) const;
		bool GetCommandLineOptions(TSTask::String *pOptions) const;
		bool SetCommandLineOptions(const TSTask::String &Options);

	private:
		TSTask::String m_Directory;
		TSTask::String m_ExeFileName;
		TSTask::String m_CommandLineOptions;
	};

	class CBonDriverSettings : public TSTask::CSettingsBase
	{
	public:
		CBonDriverSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		bool GetLoadDirectory(TSTask::String *pDirectory) const;
		bool GetLoadDirectoryAbsolute(TSTask::String *pDirectory) const;
		bool SetLoadDirectory(const TSTask::String &Directory);

	private:
		TSTask::String m_LoadDirectory;
	};

	class CRecordingSettings : public TSTask::CSettingsBase
	{
	public:
		CRecordingSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		bool GetConfirmChannelChange() const;
		void SetConfirmChannelChange(bool fConfirm);
		bool GetConfirmExit() const;
		void SetConfirmExit(bool fConfirm);
		bool GetConfirmStop() const;
		void SetConfirmStop(bool fConfirm);

	private:
		bool m_fConfirmChannelChange;
		bool m_fConfirmExit;
		bool m_fConfirmStop;
	};

	class CMainBoardSettings : public TSTask::CSettingsBase
	{
	public:
		static const int TRANSLUCENT_OPACITY_MIN=30;
		static const DWORD STATUS_UPDATE_INTERVAL_MIN=500;
		static const DWORD INFORMATION_BAR_UPDATE_INTERVAL_MIN=500;

		struct StatusItemInfo
		{
			int ID;
			int Order;
			int Width;
			bool fVisible;
		};

		typedef std::vector<StatusItemInfo> StatusItemList;

		struct InformationBarItemInfo
		{
			TSTask::String Format;
			int Width;
			int TextAlign;

			InformationBarItemInfo() : Width(-1), TextAlign(-1) {}
		};

		typedef std::vector<InformationBarItemInfo> InformationBarItemList;

		CMainBoardSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		bool GetPosition(RECT *pPosition) const;
		bool SetPosition(int Left,int Top,int Width,int Height);
		bool GetMinimize() const;
		void SetMinimize(bool fMinimize);

		bool GetShowTrayIcon() const;
		void SetShowTrayIcon(bool fShow);
		bool GetMinimizeToTray() const;
		void SetMinimizeToTray(bool fMinimizeToTray);
		bool GetShowInTaskbar() const;
		void SetShowInTaskbar(bool fShow);
		bool GetAlwaysOnTop() const;
		void SetAlwaysOnTop(bool fAlwaysOnTop);
		bool GetTranslucent() const;
		void SetTranslucent(bool fTranslucent);
		int GetTranslucentOpacity() const;
		BYTE GetTranslucentOpacity8() const;
		bool SetTranslucentOpacity(int Opacity);

		bool GetConfirmClose() const;
		void SetConfirmClose(bool fConfirm);
		bool GetConfirmCloseTaskExists() const;
		void SetConfirmCloseTaskExists(bool fConfirm);
		bool GetConfirmTaskExit() const;
		void SetConfirmTaskExit(bool fConfirm);

		bool GetThemeFile(TSTask::String *pFile) const;
		bool SetThemeFile(const TSTask::String &File);

		bool GetStatusMultiRow() const;
		void SetStatusMultiRow(bool fMultiRow);
		int GetStatusMaxRows() const;
		bool SetStatusMaxRows(int MaxRows);
		bool GetStatusFont(LOGFONT *pFont) const;
		bool SetStatusFont(const LOGFONT &Font);
		bool GetStatusCaptionFont(LOGFONT *pFont) const;
		bool SetStatusCaptionFont(const LOGFONT &Font);
		DWORD GetStatusUpdateInterval() const { return m_StatusUpdateInterval; }
		bool SetStatusUpdateInterval(DWORD Interval);
		bool GetStatusItemList(StatusItemList *pList) const;
		bool GetStatusItemListSortedByOrder(StatusItemList *pList) const;
		bool SetStatusItemInfo(const StatusItemInfo &Info);

		bool GetShowInformationBar() const;
		void SetShowInformationBar(bool fShow);
		bool GetInformationBarFont(LOGFONT *pFont) const;
		bool SetInformationBarFont(const LOGFONT &Font);
		DWORD GetInformationBarUpdateInterval() const;
		bool SetInformationBarUpdateInterval(DWORD Interval);
		bool GetInformationBarShowError() const;
		void SetInformationBarShowError(bool fShow);
		DWORD GetInformationBarErrorDuration() const;
		bool SetInformationBarErrorDuration(DWORD Duration);
		bool GetInformationBarItemList(InformationBarItemList *pList) const;
		bool SetInformationBarItemList(const InformationBarItemList &List);

	private:
		int m_Left;
		int m_Top;
		int m_Width;
		int m_Height;
		bool m_fMinimize;

		bool m_fShowTrayIcon;
		bool m_fMinimizeToTray;
		bool m_fShowInTaskbar;
		bool m_fAlwaysOnTop;
		bool m_fTranslucent;
		int m_TranslucentOpacity;

		bool m_fConfirmClose;
		bool m_fConfirmCloseTaskExists;
		bool m_fConfirmTaskExit;

		TSTask::String m_ThemeFile;

		bool m_fStatusMultiRow;
		int m_StatusMaxRows;
		LOGFONT m_StatusFont;
		LOGFONT m_StatusCaptionFont;
		DWORD m_StatusUpdateInterval;
		StatusItemInfo m_StatusItemList[CTSTaskBar::ITEM_ID_TRAILER];

		bool m_fShowInformationBar;
		LOGFONT m_InformationBarFont;
		DWORD m_InformationBarUpdateInterval;
		bool m_fInformationBarShowError;
		DWORD m_InformationBarErrorDuration;
		InformationBarItemList m_InformationBarItemList;
	};

	class CToolsSettings : public TSTask::CSettingsBase
	{
	public:
		struct ToolInfo
		{
			TSTask::String Name;
			TSTask::String Command;
			TSTask::String CurrentDirectory;
			int ShowCommand;
			std::vector<TSTask::String> Triggers;
			bool fShowInMenu;

			ToolInfo() : ShowCommand(SW_SHOWNORMAL), fShowInMenu(true) {}
		};

		CToolsSettings(TSTask::CRWLock *pLock);

	// CSettingsBase
		void SetDefault() override;
		bool Load(TSTask::CSettings &Settings) override;
		bool Save(TSTask::CSettings &Settings) const override;

		size_t GetToolCount() const;
		bool GetToolInfo(size_t Index,ToolInfo *pInfo) const;
		bool SetToolInfo(size_t Index,const ToolInfo &Info);
		bool AddToolInfo(const ToolInfo &Info);
		bool DeleteToolInfo(size_t Index);

		bool IsTrigger(LPCWSTR pszEvent) const;
		int FindTrigger(LPCWSTR pszEvent,int First=0) const;

	private:
		std::vector<ToolInfo> m_ToolList;
	};

	class CTSTaskCentreSettings
	{
	public:
		CGeneralSettings General;
		CTaskSettings Task;
		CBonDriverSettings BonDriver;
		CRecordingSettings Recording;
		CMainBoardSettings MainBoard;
		CToolsSettings Tools;

		CTSTaskCentreSettings();
		bool Load(TSTask::CSettings &Settings);
		bool Save(TSTask::CSettings &Settings) const;
		void Set(const CTSTaskCentreSettings &Src);

	private:
		static TSTask::CRWLock m_Lock;
	};

	class CSettingsHandler abstract
	{
	public:
		virtual ~CSettingsHandler() {}
		virtual bool OnSettingsChanged(const CTSTaskCentreSettings &Settings) = 0;
	};

}


#endif
