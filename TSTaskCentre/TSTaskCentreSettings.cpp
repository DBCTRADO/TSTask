#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskCentreSettings.h"
#include "Graphics.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CGeneralSettings::CGeneralSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CGeneralSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		TSTask::GetModuleFilePath(nullptr,&m_IniFilePath);
		TSTask::PathUtility::RenameExtension(&m_IniFilePath,L".ini");

		m_ProcessPriority=PROCESS_PRIORITY_INVALID;

		m_fExitWhenAllTaskEnded=true;

		m_LoggingLevel=TSTask::LOG_INFO;
		m_MaxLog=500;
		m_fLogOutputToFile=false;
		TSTask::PathUtility::GetFileName(m_IniFilePath,&m_LogFileName);
		TSTask::PathUtility::RenameExtension(&m_LogFileName,L".log");
		m_fLogOverwrite=true;
		m_fDebugLog=
#ifdef _DEBUG
			true;
#else
			false;
#endif
	}

	bool CGeneralSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		int Value;

		if (Settings.Read(L"General.ProcessPriority",&Value))
			SetProcessPriority(ProcessPriority(Value));

		Settings.Read(L"General.ExitWhenAllTaskEnded",&m_fExitWhenAllTaskEnded);

		if (Settings.Read(L"Logging.Level",&Value))
			SetLoggingLevel(TSTask::LogType(TSTask::LOG_NONE-Value));
		Settings.Read(L"Logging.MaxCount",&m_MaxLog);
		Settings.Read(L"Logging.OutputToFile",&m_fLogOutputToFile);
		TSTask::String FileName;
		if (Settings.Read(L"Logging.FileName",&FileName) && !FileName.empty())
			m_LogFileName=FileName;
		Settings.Read(L"Logging.Overwrite",&m_fLogOverwrite);
		Settings.Read(L"Logging.Debug",&m_fDebugLog);

		return true;
	}

	bool CGeneralSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		if (m_ProcessPriority!=PROCESS_PRIORITY_INVALID)
			Settings.Write(L"General.ProcessPriority",(int)m_ProcessPriority);
		else
			Settings.Write(L"General.ProcessPriority",L"");

		Settings.Write(L"General.ExitWhenAllTaskEnded",m_fExitWhenAllTaskEnded);

		Settings.Write(L"Logging.Level",TSTask::LOG_NONE-(int)m_LoggingLevel);
		Settings.Write(L"Logging.MaxCount",m_MaxLog);
		Settings.Write(L"Logging.OutputToFile",m_fLogOutputToFile);
		Settings.Write(L"Logging.FileName",m_LogFileName);
		Settings.Write(L"Logging.Overwrite",m_fLogOverwrite);
		Settings.Write(L"Logging.Debug",m_fDebugLog);

		return true;
	}

	bool CGeneralSettings::GetIniFilePath(TSTask::String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFilePath=m_IniFilePath;

		return true;
	}

	bool CGeneralSettings::SetIniFilePath(const TSTask::String &FilePath)
	{
		if (FilePath.empty())
			return false;

		TSTask::CRWLockWrite Lock(*m_pLock);

		m_IniFilePath=FilePath;

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

	bool CGeneralSettings::GetExitWhenAllTaskEnded() const
	{
		return m_fExitWhenAllTaskEnded;
	}

	void CGeneralSettings::SetExitWhenAllTaskEnded(bool fExit)
	{
		m_fExitWhenAllTaskEnded=fExit;
	}

	bool CGeneralSettings::SetLoggingLevel(TSTask::LogType Level)
	{
		if (Level<0 || Level>TSTask::LOG_NONE)
			return false;

		m_LoggingLevel=Level;

		return true;
	}

	TSTask::LogType CGeneralSettings::GetLoggingLevel() const
	{
		return m_LoggingLevel;
	}

	void CGeneralSettings::SetMaxLog(unsigned int Max)
	{
		m_MaxLog=Max;
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

	bool CGeneralSettings::GetLogFilePath(TSTask::String *pPath) const
	{
		if (pPath==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		if (m_LogFileName.find(L'\\')!=TSTask::String::npos) {
			*pPath=m_LogFileName;
		} else {
			TSTask::GetModuleDirectory(nullptr,pPath);
			TSTask::PathUtility::Append(pPath,m_LogFileName.c_str());
		}

		return true;
	}

	bool CGeneralSettings::SetLogFileName(const TSTask::String &FileName)
	{
		if (FileName.empty())
			return false;

		TSTask::CRWLockWrite Lock(*m_pLock);

		m_LogFileName=FileName;

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


	CTaskSettings::CTaskSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CTaskSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_Directory.clear();
		m_ExeFileName=L"TSTask.exe";
		m_CommandLineOptions.clear();
	}

	bool CTaskSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"Task.Directory",&m_Directory);
		Settings.Read(L"Task.ExeFileName",&m_ExeFileName);
		Settings.Read(L"Task.CommandLineOptions",&m_CommandLineOptions);

		return true;
	}

	bool CTaskSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		Settings.Write(L"Task.Directory",m_Directory);
		Settings.Write(L"Task.ExeFileName",m_ExeFileName);
		Settings.Write(L"Task.CommandLineOptions",m_CommandLineOptions);

		return true;
	}

	bool CTaskSettings::GetDirectory(TSTask::String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pDirectory=m_Directory;

		return true;
	}

	bool CTaskSettings::SetDirectory(const TSTask::String &Directory)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_Directory=Directory;

		return true;
	}

	bool CTaskSettings::GetExeFileName(TSTask::String *pFileName) const
	{
		if (pFileName==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFileName=m_ExeFileName;

		return true;
	}

	bool CTaskSettings::SetExeFileName(const TSTask::String &FileName)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_ExeFileName=FileName;

		return true;
	}

	bool CTaskSettings::GetExeFilePath(TSTask::String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		if (m_ExeFileName.empty())
			return false;

		if (m_Directory.empty())
			TSTask::GetModuleDirectory(nullptr,pFilePath);
		else
			pFilePath->assign(m_Directory);
		TSTask::PathUtility::Append(pFilePath,m_ExeFileName.c_str());

		return true;
	}

	bool CTaskSettings::GetCommandLineOptions(TSTask::String *pOptions) const
	{
		if (pOptions==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pOptions=m_CommandLineOptions;

		return true;
	}

	bool CTaskSettings::SetCommandLineOptions(const TSTask::String &Options)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_CommandLineOptions=Options;

		return true;
	}


	CBonDriverSettings::CBonDriverSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CBonDriverSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_LoadDirectory.clear();
	}

	bool CBonDriverSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"BonDriver.LoadDirectory",&m_LoadDirectory);

		return true;
	}

	bool CBonDriverSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		Settings.Write(L"BonDriver.LoadDirectory",m_LoadDirectory);

		return true;
	}

	bool CBonDriverSettings::GetLoadDirectory(TSTask::String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pDirectory=m_LoadDirectory;

		return true;
	}

	bool CBonDriverSettings::GetLoadDirectoryAbsolute(TSTask::String *pDirectory) const
	{
		if (pDirectory==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		if (m_LoadDirectory.empty())
			return TSTask::GetModuleDirectory(nullptr,pDirectory);

		*pDirectory=m_LoadDirectory;

		return true;
	}

	bool CBonDriverSettings::SetLoadDirectory(const TSTask::String &Directory)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_LoadDirectory=Directory;

		return true;
	}


	CRecordingSettings::CRecordingSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CRecordingSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_fConfirmChannelChange=true;
		m_fConfirmExit=true;
		m_fConfirmStop=true;
	}

	bool CRecordingSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"Recording.ConfirmChannelChange",&m_fConfirmChannelChange);
		Settings.Read(L"Recording.ConfirmExit",&m_fConfirmExit);
		Settings.Read(L"Recording.ConfirmStop",&m_fConfirmStop);

		return true;
	}

	bool CRecordingSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		Settings.Write(L"Recording.ConfirmChannelChange",m_fConfirmChannelChange);
		Settings.Write(L"Recording.ConfirmExit",m_fConfirmExit);
		Settings.Write(L"Recording.ConfirmStop",m_fConfirmStop);

		return true;
	}

	bool CRecordingSettings::GetConfirmChannelChange() const
	{
		return m_fConfirmChannelChange;
	}

	void CRecordingSettings::SetConfirmChannelChange(bool fConfirm)
	{
		m_fConfirmChannelChange=fConfirm;
	}

	bool CRecordingSettings::GetConfirmExit() const
	{
		return m_fConfirmExit;
	}

	void CRecordingSettings::SetConfirmExit(bool fConfirm)
	{
		m_fConfirmExit=fConfirm;
	}

	bool CRecordingSettings::GetConfirmStop() const
	{
		return m_fConfirmStop;
	}

	void CRecordingSettings::SetConfirmStop(bool fConfirm)
	{
		m_fConfirmStop=fConfirm;
	}


	CMainBoardSettings::CMainBoardSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CMainBoardSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_Left=0;
		m_Top=0;
		m_Width=0;
		m_Height=0;
		m_fMinimize=false;

		m_fShowTrayIcon=false;
		m_fMinimizeToTray=false;
		m_fShowInTaskbar=true;
		m_fAlwaysOnTop=false;
		m_fTranslucent=false;
		m_TranslucentOpacity=80;

		m_fConfirmClose=true;
		m_fConfirmCloseTaskExists=true;
		m_fConfirmTaskExit=false;

		m_ThemeFile=L"";

		m_fStatusMultiRow=false;
		m_StatusMaxRows=2;
		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_STATUS,&m_StatusFont,true);
		Graphics::GetSystemFont(Graphics::SYSTEM_FONT_CAPTION,&m_StatusCaptionFont);
		m_StatusCaptionFont.lfWeight=FW_BOLD;
		m_StatusUpdateInterval=1000;

		for (int i=0;i<_countof(m_StatusItemList);i++) {
			m_StatusItemList[i].ID=i;
			m_StatusItemList[i].Order=i;
			m_StatusItemList[i].Width=-1;
			m_StatusItemList[i].fVisible=i!=CTSTaskBar::ITEM_ID_CLOCK;
		}

		m_fShowInformationBar=true;
		m_InformationBarFont=m_StatusFont;
		m_InformationBarUpdateInterval=1000;
		m_fInformationBarShowError=true;
		m_InformationBarErrorDuration=5000;

		m_InformationBarItemList.clear();
		InformationBarItemInfo Item;
		Item.Format=L"%date%(%day-of-week%) %time%:%second2%";
		Item.Width=std::abs(m_InformationBarFont.lfHeight)*16;
		Item.TextAlign=1;
		m_InformationBarItemList.push_back(Item);
	}

	bool CMainBoardSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		Settings.Read(L"MainBoard.Left",&m_Left);
		Settings.Read(L"MainBoard.Top",&m_Top);
		Settings.Read(L"MainBoard.Width",&m_Width);
		Settings.Read(L"MainBoard.Height",&m_Height);
		Settings.Read(L"MainBoard.Minimize",&m_fMinimize);
		Settings.Read(L"MainBoard.ShowTrayIcon",&m_fShowTrayIcon);
		Settings.Read(L"MainBoard.MinimizeToTray",&m_fMinimizeToTray);
		Settings.Read(L"MainBoard.ShowInTaskbar",&m_fShowInTaskbar);
		Settings.Read(L"MainBoard.AlwaysOnTop",&m_fAlwaysOnTop);
		Settings.Read(L"MainBoard.Translucent",&m_fTranslucent);
		int Opacity;
		if (Settings.Read(L"MainBoard.TranslucentOpacity",&Opacity))
			SetTranslucentOpacity(Opacity);

		Settings.Read(L"MainBoard.ConfirmClose",&m_fConfirmClose);
		Settings.Read(L"MainBoard.ConfirmCloseTaskExists",&m_fConfirmCloseTaskExists);
		Settings.Read(L"MainBoard.ConfirmTaskExit",&m_fConfirmTaskExit);

		Settings.Read(L"MainBoard.ThemeFile",&m_ThemeFile);

		Settings.Read(L"Status.MultiRow",&m_fStatusMultiRow);
		Settings.Read(L"Status.MaxRows",&m_StatusMaxRows);
		Settings.Read(L"Status.Font",&m_StatusFont);
		Settings.Read(L"Status.CaptionFont",&m_StatusCaptionFont);
		unsigned int UpdateInterval;
		if (Settings.Read(L"Status.UpdateInterval",&UpdateInterval))
			SetStatusUpdateInterval(UpdateInterval);

		TSTask::String Key;
		for (int i=0;i<_countof(m_StatusItemList);i++) {
			CTSTaskBar::ItemInfo Item;

			CTSTaskBar::GetItemInfo(i,&Item);
			TSTask::StringUtility::Format(Key,L"Status.%s.Order",Item.pszName);
			Settings.Read(Key.c_str(),&m_StatusItemList[i].Order);
			TSTask::StringUtility::Format(Key,L"Status.%s.Width",Item.pszName);
			Settings.Read(Key.c_str(),&m_StatusItemList[i].Width);
			TSTask::StringUtility::Format(Key,L"Status.%s.Visible",Item.pszName);
			Settings.Read(Key.c_str(),&m_StatusItemList[i].fVisible);
		}

		Settings.Read(L"InformationBar.Show",&m_fShowInformationBar);
		Settings.Read(L"InformationBar.Font",&m_InformationBarFont);
		if (Settings.Read(L"InformationBar.UpdateInterval",&UpdateInterval))
			SetInformationBarUpdateInterval(UpdateInterval);
		Settings.Read(L"InformationBar.ShowError",&m_fInformationBarShowError);
		unsigned int Duration;
		if (Settings.Read(L"InformationBar.ErrorDuration",&Duration))
			SetInformationBarErrorDuration(Duration);

		int ItemCount;
		if (Settings.Read(L"InformationBar.ItemCount",&ItemCount) && ItemCount>=0) {
			m_InformationBarItemList.clear();

			for (int i=0;i<ItemCount;i++) {
				InformationBarItemInfo Item;

				TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.Format",i);
				if (!Settings.Read(Key.c_str(),&Item.Format))
					break;
				TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.Width",i);
				Settings.Read(Key.c_str(),&Item.Width);
				TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.TextAlign",i);
				Settings.Read(Key.c_str(),&Item.TextAlign);

				m_InformationBarItemList.push_back(Item);
			}
		}

		return true;
	}

	bool CMainBoardSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		Settings.Write(L"MainBoard.Left",m_Left);
		Settings.Write(L"MainBoard.Top",m_Top);
		Settings.Write(L"MainBoard.Width",m_Width);
		Settings.Write(L"MainBoard.Height",m_Height);
		Settings.Write(L"MainBoard.Minimize",m_fMinimize);
		Settings.Write(L"MainBoard.ShowTrayIcon",m_fShowTrayIcon);
		Settings.Write(L"MainBoard.MinimizeToTray",m_fMinimizeToTray);
		Settings.Write(L"MainBoard.ShowInTaskbar",m_fShowInTaskbar);
		Settings.Write(L"MainBoard.AlwaysOnTop",m_fAlwaysOnTop);
		Settings.Write(L"MainBoard.Translucent",m_fTranslucent);
		Settings.Write(L"MainBoard.TranslucentOpacity",m_TranslucentOpacity);

		Settings.Write(L"MainBoard.ConfirmClose",m_fConfirmClose);
		Settings.Write(L"MainBoard.ConfirmCloseTaskExists",m_fConfirmCloseTaskExists);
		Settings.Write(L"MainBoard.ConfirmTaskExit",m_fConfirmTaskExit);

		Settings.Write(L"MainBoard.ThemeFile",m_ThemeFile);

		Settings.Write(L"Status.MultiRow",m_fStatusMultiRow);
		Settings.Write(L"Status.MaxRows",m_StatusMaxRows);
		Settings.Write(L"Status.Font",m_StatusFont);
		Settings.Write(L"Status.CaptionFont",m_StatusCaptionFont);
		Settings.Write(L"Status.UpdateInterval",(unsigned int)m_StatusUpdateInterval);

		TSTask::String Key;
		for (int i=0;i<_countof(m_StatusItemList);i++) {
			CTSTaskBar::ItemInfo Item;

			CTSTaskBar::GetItemInfo(i,&Item);
			TSTask::StringUtility::Format(Key,L"Status.%s.Order",Item.pszName);
			Settings.Write(Key.c_str(),m_StatusItemList[i].Order);
			TSTask::StringUtility::Format(Key,L"Status.%s.Width",Item.pszName);
			Settings.Write(Key.c_str(),m_StatusItemList[i].Width);
			TSTask::StringUtility::Format(Key,L"Status.%s.Visible",Item.pszName);
			Settings.Write(Key.c_str(),m_StatusItemList[i].fVisible);
		}

		Settings.Write(L"InformationBar.Show",m_fShowInformationBar);
		Settings.Write(L"InformationBar.Font",m_InformationBarFont);
		Settings.Write(L"InformationBar.UpdateInterval",(unsigned int)m_InformationBarUpdateInterval);
		Settings.Write(L"InformationBar.ShowError",m_fInformationBarShowError);
		Settings.Write(L"InformationBar.ErrorDuration",(unsigned int)m_InformationBarErrorDuration);

		Settings.Write(L"InformationBar.ItemCount",(unsigned int)m_InformationBarItemList.size());
		for (int i=0;i<(int)m_InformationBarItemList.size();i++) {
			const InformationBarItemInfo &Item=m_InformationBarItemList[i];

			TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.Format",i);
			Settings.Write(Key.c_str(),Item.Format);
			TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.Width",i);
			Settings.Write(Key.c_str(),Item.Width);
			TSTask::StringUtility::Format(Key,L"InformationBar.Item%d.TextAlign",i);
			Settings.Write(Key.c_str(),Item.TextAlign);
		}

		return true;
	}

	bool CMainBoardSettings::GetPosition(RECT *pPosition) const
	{
		if (pPosition==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		pPosition->left=m_Left;
		pPosition->top=m_Top;
		pPosition->right=m_Left+m_Width;
		pPosition->bottom=m_Top+m_Height;

		return true;
	}

	bool CMainBoardSettings::SetPosition(int Left,int Top,int Width,int Height)
	{
		if (Width<=0 || Height<=0)
			return false;

		TSTask::CRWLockWrite Lock(*m_pLock);

		m_Left=Left;
		m_Top=Top;
		m_Width=Width;
		m_Height=Height;

		return true;
	}

	bool CMainBoardSettings::GetMinimize() const
	{
		return m_fMinimize;
	}

	void CMainBoardSettings::SetMinimize(bool fMinimize)
	{
		m_fMinimize=fMinimize;
	}

	bool CMainBoardSettings::GetShowTrayIcon() const
	{
		return m_fShowTrayIcon;
	}

	void CMainBoardSettings::SetShowTrayIcon(bool fShow)
	{
		m_fShowTrayIcon=fShow;
	}

	bool CMainBoardSettings::GetMinimizeToTray() const
	{
		return m_fMinimizeToTray;
	}

	void CMainBoardSettings::SetMinimizeToTray(bool fMinimizeToTray)
	{
		m_fMinimizeToTray=fMinimizeToTray;
	}

	bool CMainBoardSettings::GetShowInTaskbar() const
	{
		return m_fShowInTaskbar;
	}

	void CMainBoardSettings::SetShowInTaskbar(bool fShow)
	{
		m_fShowInTaskbar=fShow;
	}

	bool CMainBoardSettings::GetAlwaysOnTop() const
	{
		return m_fAlwaysOnTop;
	}

	void CMainBoardSettings::SetAlwaysOnTop(bool fAlwaysOnTop)
	{
		m_fAlwaysOnTop=fAlwaysOnTop;
	}

	bool CMainBoardSettings::GetTranslucent() const
	{
		return m_fTranslucent;
	}

	void CMainBoardSettings::SetTranslucent(bool fTranslucent)
	{
		m_fTranslucent=fTranslucent;
	}

	int CMainBoardSettings::GetTranslucentOpacity() const
	{
		return m_TranslucentOpacity;
	}

	BYTE CMainBoardSettings::GetTranslucentOpacity8() const
	{
		return BYTE(m_TranslucentOpacity*255/100);
	}

	bool CMainBoardSettings::SetTranslucentOpacity(int Opacity)
	{
		if (Opacity<TRANSLUCENT_OPACITY_MIN || Opacity>=100)
			return false;

		m_TranslucentOpacity=Opacity;

		return true;
	}

	void CMainBoardSettings::SetConfirmClose(bool fConfirm)
	{
		m_fConfirmClose=fConfirm;
	}

	bool CMainBoardSettings::GetConfirmClose() const
	{
		return m_fConfirmClose;
	}

	void CMainBoardSettings::SetConfirmCloseTaskExists(bool fConfirm)
	{
		m_fConfirmCloseTaskExists=fConfirm;
	}

	bool CMainBoardSettings::GetConfirmCloseTaskExists() const
	{
		return m_fConfirmCloseTaskExists;
	}

	bool CMainBoardSettings::GetConfirmTaskExit() const
	{
		return m_fConfirmTaskExit;
	}

	void CMainBoardSettings::SetConfirmTaskExit(bool fConfirm)
	{
		m_fConfirmTaskExit=fConfirm;
	}

	bool CMainBoardSettings::GetThemeFile(TSTask::String *pFile) const
	{
		if (pFile==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFile=m_ThemeFile;

		return true;
	}

	bool CMainBoardSettings::SetThemeFile(const TSTask::String &File)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_ThemeFile=File;

		return true;
	}

	bool CMainBoardSettings::GetStatusMultiRow() const
	{
		return m_fStatusMultiRow;
	}

	void CMainBoardSettings::SetStatusMultiRow(bool fMultiRow)
	{
		m_fStatusMultiRow=fMultiRow;
	}

	int CMainBoardSettings::GetStatusMaxRows() const
	{
		return m_StatusMaxRows;
	}

	bool CMainBoardSettings::SetStatusMaxRows(int MaxRows)
	{
		if (MaxRows<1)
			return false;

		m_StatusMaxRows=MaxRows;

		return true;
	}

	bool CMainBoardSettings::GetStatusFont(LOGFONT *pFont) const
	{
		if (pFont==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFont=m_StatusFont;

		return true;
	}

	bool CMainBoardSettings::SetStatusFont(const LOGFONT &Font)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_StatusFont=Font;

		return true;
	}

	bool CMainBoardSettings::GetStatusCaptionFont(LOGFONT *pFont) const
	{
		if (pFont==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFont=m_StatusCaptionFont;

		return true;
	}

	bool CMainBoardSettings::SetStatusCaptionFont(const LOGFONT &Font)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_StatusCaptionFont=Font;

		return true;
	}

	bool CMainBoardSettings::SetStatusUpdateInterval(DWORD Interval)
	{
		if (Interval<STATUS_UPDATE_INTERVAL_MIN)
			return false;

		m_StatusUpdateInterval=Interval;

		return true;
	}

	bool CMainBoardSettings::GetStatusItemList(StatusItemList *pList) const
	{
		if (pList==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		pList->resize(_countof(m_StatusItemList));
		for (size_t i=0;i<_countof(m_StatusItemList);i++)
			(*pList)[i]=m_StatusItemList[i];

		return true;
	}

	bool CMainBoardSettings::GetStatusItemListSortedByOrder(StatusItemList *pList) const
	{
		if (!GetStatusItemList(pList))
			return false;

		std::sort(pList->begin(),pList->end(),
				  [](const StatusItemInfo &Item1,const StatusItemInfo &Item2) { return Item1.Order<Item2.Order; });

		return true;
	}

	bool CMainBoardSettings::SetStatusItemInfo(const StatusItemInfo &Info)
	{
		if (Info.ID<0 || Info.ID>=_countof(m_StatusItemList))
			return false;

		TSTask::CRWLockWrite Lock(*m_pLock);

		m_StatusItemList[Info.ID]=Info;

		return true;
	}

	bool CMainBoardSettings::GetShowInformationBar() const
	{
		return m_fShowInformationBar;
	}

	void CMainBoardSettings::SetShowInformationBar(bool fShow)
	{
		m_fShowInformationBar=fShow;
	}

	bool CMainBoardSettings::GetInformationBarFont(LOGFONT *pFont) const
	{
		if (pFont==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pFont=m_InformationBarFont;

		return true;
	}

	bool CMainBoardSettings::SetInformationBarFont(const LOGFONT &Font)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_InformationBarFont=Font;

		return true;
	}

	DWORD CMainBoardSettings::GetInformationBarUpdateInterval() const
	{
		return m_InformationBarUpdateInterval;
	}

	bool CMainBoardSettings::SetInformationBarUpdateInterval(DWORD Interval)
	{
		if (Interval<INFORMATION_BAR_UPDATE_INTERVAL_MIN)
			return false;

		m_InformationBarUpdateInterval=Interval;

		return true;
	}

	bool CMainBoardSettings::GetInformationBarShowError() const
	{
		return m_fInformationBarShowError;
	}

	void CMainBoardSettings::SetInformationBarShowError(bool fShow)
	{
		m_fInformationBarShowError=fShow;
	}

	DWORD CMainBoardSettings::GetInformationBarErrorDuration() const
	{
		return m_InformationBarErrorDuration;
	}

	bool CMainBoardSettings::SetInformationBarErrorDuration(DWORD Duration)
	{
		if (Duration<1000)
			return false;

		m_InformationBarErrorDuration=Duration;

		return true;
	}

	bool CMainBoardSettings::GetInformationBarItemList(InformationBarItemList *pList) const
	{
		if (pList==nullptr)
			return false;

		TSTask::CRWLockRead Lock(*m_pLock);

		*pList=m_InformationBarItemList;

		return true;
	}

	bool CMainBoardSettings::SetInformationBarItemList(const InformationBarItemList &List)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_InformationBarItemList=List;

		return true;
	}


	CToolsSettings::CToolsSettings(TSTask::CRWLock *pLock)
		: CSettingsBase(pLock)
	{
		SetDefault();
	}

	void CToolsSettings::SetDefault()
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_ToolList.clear();

		ToolInfo Info;
		Info.Name=L"TVTestで視聴";
		Info.Command=L"TVTest.exe /d BonDriver_TSTask.dll /chspace 0 /ch %task-id% /sid %service-id%";
		m_ToolList.push_back(Info);
		Info.Name=L"TVTestでネットワーク視聴";
		Info.Command=L"TVTest.exe /d BonDriver_%protocol%.dll /port %port% /sid %service-id%";
		m_ToolList.push_back(Info);
	}

	bool CToolsSettings::Load(TSTask::CSettings &Settings)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_ToolList.clear();

		for (unsigned int i=0;;i++) {
			ToolInfo Info;
			WCHAR szKey[64];

			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Name",i);
			if (!Settings.Read(szKey,&Info.Name))
				break;

			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Command",i);
			if (!Settings.Read(szKey,&Info.Command))
				break;

			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.CurrentDirectory",i);
			if (!Settings.Read(szKey,&Info.CurrentDirectory))
				Info.CurrentDirectory.clear();

			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.ShowCommand",i);
			if (!Settings.Read(szKey,&Info.ShowCommand))
				Info.ShowCommand=SW_SHOWNORMAL;

			TSTask::String Triggers;
			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Triggers",i);
			if (Settings.Read(szKey,&Triggers))
				TSTask::StringUtility::Split(Triggers,L",",&Info.Triggers);
			else
				Info.Triggers.clear();

			TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.ShowInMenu",i);
			Settings.Read(szKey,&Info.fShowInMenu);

			m_ToolList.push_back(Info);
		}

		return true;
	}

	bool CToolsSettings::Save(TSTask::CSettings &Settings) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		for (unsigned int i=0;;i++) {
			WCHAR szKey[64];

			if (i<m_ToolList.size()) {
				const ToolInfo &Info=m_ToolList[i];

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Name",i);
				Settings.Write(szKey,Info.Name);

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Command",i);
				Settings.Write(szKey,Info.Command);

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.CurrentDirectory",i);
				Settings.Write(szKey,Info.CurrentDirectory);

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.ShowCommand",i);
				Settings.Write(szKey,Info.ShowCommand);

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.Triggers",i);
				TSTask::String Triggers;
				TSTask::StringUtility::Combine(Info.Triggers,L",",&Triggers);
				Settings.Write(szKey,Triggers);

				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.ShowInMenu",i);
				Settings.Write(szKey,Info.fShowInMenu);
			} else {
				TSTask::FormatString(szKey,_countof(szKey),L"Tool%u.*",i);
				if (!Settings.DeleteKeys(szKey))
					break;
			}
		}

		return true;
	}

	size_t CToolsSettings::GetToolCount() const
	{
		return m_ToolList.size();
	}

	bool CToolsSettings::GetToolInfo(size_t Index,ToolInfo *pInfo) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		if (Index>=m_ToolList.size() || pInfo==nullptr)
			return false;

		*pInfo=m_ToolList[Index];

		return true;
	}

	bool CToolsSettings::SetToolInfo(size_t Index,const ToolInfo &Info)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		if (Index>=m_ToolList.size())
			return false;

		m_ToolList[Index]=Info;

		return true;
	}

	bool CToolsSettings::AddToolInfo(const ToolInfo &Info)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		m_ToolList.push_back(Info);

		return true;
	}

	bool CToolsSettings::DeleteToolInfo(size_t Index)
	{
		TSTask::CRWLockWrite Lock(*m_pLock);

		if (Index>=m_ToolList.size())
			return false;

		auto i=m_ToolList.begin();
		std::advance(i,Index);
		m_ToolList.erase(i);

		return true;
	}

	bool CToolsSettings::IsTrigger(LPCWSTR pszEvent) const
	{
		return FindTrigger(pszEvent)>=0;
	}

	int CToolsSettings::FindTrigger(LPCWSTR pszEvent,int First) const
	{
		TSTask::CRWLockRead Lock(*m_pLock);

		if (TSTask::IsStringEmpty(pszEvent)
				|| First<0 || (size_t)First>=m_ToolList.size())
			return -1;

		for (size_t i=First;i<m_ToolList.size();i++) {
			const ToolInfo &Tool=m_ToolList[i];

			for (size_t j=0;j<Tool.Triggers.size();j++) {
				if (TSTask::StringUtility::CompareNoCase(Tool.Triggers[j],pszEvent)==0)
					return (int)i;
			}
		}

		return -1;
	}


	TSTask::CRWLock CTSTaskCentreSettings::m_Lock;

	CTSTaskCentreSettings::CTSTaskCentreSettings()
		: General(&m_Lock)
		, Task(&m_Lock)
		, BonDriver(&m_Lock)
		, Recording(&m_Lock)
		, MainBoard(&m_Lock)
		, Tools(&m_Lock)
	{
	}

	bool CTSTaskCentreSettings::Load(TSTask::CSettings &Settings)
	{
		if (!Settings.SetSection(L"Settings"))
			return false;

		General.Load(Settings);
		Task.Load(Settings);
		BonDriver.Load(Settings);
		Recording.Load(Settings);
		MainBoard.Load(Settings);
		Tools.Load(Settings);

		return true;
	}

	bool CTSTaskCentreSettings::Save(TSTask::CSettings &Settings) const
	{
		if (!Settings.SetSection(L"Settings"))
			return false;

		General.Save(Settings);
		Task.Save(Settings);
		BonDriver.Save(Settings);
		Recording.Save(Settings);
		MainBoard.Save(Settings);
		Tools.Save(Settings);

		return true;
	}

	void CTSTaskCentreSettings::Set(const CTSTaskCentreSettings &Src)
	{
		TSTask::CRWLockWrite Lock(m_Lock);

		*this=Src;
	}

}
