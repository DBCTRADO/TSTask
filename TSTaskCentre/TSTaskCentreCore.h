#ifndef TSTASKCENTRE_CORE_H
#define TSTASKCENTRE_CORE_H


#include <vector>
#include "TSTaskManager.h"
#include "TSTaskCentreSettings.h"
#include "BonDriverManager.h"
#include "ThemeManager.h"
#include "Dialog.h"


namespace TSTaskCentre
{

	class CTSTaskCentreCore
	{
	public:
		CTSTaskCentreCore();
		~CTSTaskCentreCore();
		bool Initialize();
		void Finalize();
		TSTask::TaskID GetTaskID() const { return m_TaskIdentity.GetTaskID(); }
		bool ChangeSettings(const CTSTaskCentreSettings &Settings);

		int GetTaskCount() const;
		bool GetTaskList(TSTask::TaskUtility::TaskIDList *pList) const;
		bool AddTask(TSTask::TaskID ID);
		bool RemoveTask(TSTask::TaskID ID);
		bool NewTask();

		TSTask::CBasicLogger &GetLogger() { return m_Logger; }
		CTSTaskManager &GetTSTaskManager() { return m_TSTaskManager; }
		const CTSTaskManager &GetTSTaskManager() const { return m_TSTaskManager; }
		CTSTaskCentreSettings &GetSettings() { return m_Settings; }
		CTSTaskCentreSettings &GetCurSettings() { return m_CurSettings; }
		const CTSTaskCentreSettings &GetSettings() const { return m_Settings; }
		Graphics::CSystem &GetGraphicSystem() { return m_GraphicSystem; }
		CThemeManager &GetThemeManager() { return m_ThemeManager; }

		bool GetBonDriverFileList(LPCWSTR pszDirectory,CBonDriverManager::BonDriverFileList *pList);

		int GetDPIForUI() const { return m_DPI; }
		int ScaleDPI(int DPI,int Value) const;
		int ScaleDPI(int Value) const { return ScaleDPI(m_DPI,Value); }
		RECT ScaleDPI(int DPI,const RECT &Rect) const;
		RECT ScaleDPI(const RECT &Rect) const { return ScaleDPI(m_DPI,Rect); }

		bool AddModelessDialog(CDialog *pDialog);
		bool RemoveModelessDialog(CDialog *pDialog);
		bool ProcessDialogMessage(MSG *pMsg);

		bool CopyTextToClipboard(const TSTask::String &Text) const;

		bool ExecuteTaskTool(TSTask::TaskID ID,const CToolsSettings::ToolInfo &Info);
		bool ExecuteTaskToolOnEvent(TSTask::TaskID ID,LPCWSTR pszEvent);

	private:
		//bool SetMessageDefaultProperties(TSTask::CMessage *pMessage);

		TSTask::CTaskIdentity m_TaskIdentity;
		TSTask::CBasicLogger m_Logger;
		CTSTaskManager m_TSTaskManager;
		CTSTaskCentreSettings m_Settings;
		CTSTaskCentreSettings m_CurSettings;
		CBonDriverManager m_BonDriverManager;
		Graphics::CSystem m_GraphicSystem;
		CThemeManager m_ThemeManager;
		std::vector<CDialog*> m_DialogList;
		int m_DPI;

		CTSTaskCentreCore(const CTSTaskCentreCore &) = delete;
		CTSTaskCentreCore &operator=(const CTSTaskCentreCore &) = delete;
	};

}


#endif
