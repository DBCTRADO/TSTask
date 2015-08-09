#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskAppCore.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CStatisticsUpdater::CStatisticsUpdater(CTSTaskAppCore &Core)
		: m_Core(Core)
		, m_UpdateInterval(1000)
	{
	}

	CStatisticsUpdater::~CStatisticsUpdater()
	{
		EndThread();
	}

	bool CStatisticsUpdater::StartThread()
	{
		if (m_Thread.IsOpened())
			return true;

		m_EndEvent.Create();

		CThread::CFunctorBase *pFunctor=
			new CThread::CFunctor<CStatisticsUpdater>(this,&CStatisticsUpdater::UpdateThread);
		if (!m_Thread.Begin(pFunctor)) {
			OutLog(LOG_ERROR,L"統計情報更新スレッドを開始できません。");
			delete pFunctor;
			EndThread();
			return false;
		}

		return true;
	}

	void CStatisticsUpdater::EndThread()
	{
		if (m_Thread.IsOpened()) {
			OutLog(LOG_VERBOSE,L"統計情報更新スレッドを終了します。");
			m_EndEvent.Set();
			if (m_Thread.Wait(10000)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"統計情報更新スレッドの応答が無いため強制終了します。");
				m_Thread.Terminate();
			}
			m_Thread.Close();
		}

		m_EndEvent.Close();
	}

	bool CStatisticsUpdater::SetUpdateInterval(DWORD Interval)
	{
		m_UpdateInterval=Interval;

		return true;
	}

	unsigned int CStatisticsUpdater::UpdateThread()
	{
		::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_LOWEST);

		while (m_EndEvent.Wait(m_UpdateInterval)==WAIT_TIMEOUT) {
			m_Core.UpdateStreamStatistics();
		}

		return 0;
	}

}
