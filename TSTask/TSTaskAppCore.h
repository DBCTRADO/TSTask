#ifndef TSTASK_APP_CORE_H
#define TSTASK_APP_CORE_H


#include "TSTaskCore.h"
#include "TSTaskSettings.h"
#include "ReserveManager.h"
#include "StatisticsUpdater.h"


namespace TSTask
{

	class CTSTaskAppCore
	{
	public:
		CTSTaskAppCore(HINSTANCE hInstance);
		virtual bool Quit() = 0;
		HINSTANCE GetInstance() const { return m_hInstance; }
		TaskID GetTaskID() const { return m_TaskIdentity.GetTaskID(); }
		CTSTaskCore &GetCore() { return m_Core; }
		CTSTaskSettings &GetSettings() { return m_Settings; }
		CTSTaskSettings &GetCurSettings() { return m_CurSettings; }
		virtual bool GetLog(LogList *pList,size_t Max=0) = 0;
		virtual HWND GetWindowHandle() const = 0;
		virtual int GetTvRockDeviceID() const = 0;
		virtual bool ReloadSettings() = 0;

		bool ExecuteClient();

		bool FormatRecordFileName(LPCWSTR pszFormat,String *pFileName);

		bool ReserveRecording(const ReserveSettings &Reserve,const RecordingSettings &RecSettings);
		bool CancelReserve();
		bool GetReserveInfo(ReserveInfo *pInfo) const;

		bool UpdateStreamStatistics();
		bool UpdateTotTime();

	protected:
		HINSTANCE m_hInstance;
		CTaskIdentity m_TaskIdentity;
		CTSTaskCore m_Core;
		CTSTaskSettings m_Settings;
		CTSTaskSettings m_CurSettings;
		CReserveManager m_ReserveManager;
		CServerTaskSharedInfo m_SharedInfo;
		CStatisticsUpdater m_StatisticsUpdater;

	private:
		CTSTaskAppCore(const CTSTaskAppCore &Src) = delete;
		CTSTaskAppCore &operator=(const CTSTaskAppCore &Src) = delete;
	};

}


#endif
