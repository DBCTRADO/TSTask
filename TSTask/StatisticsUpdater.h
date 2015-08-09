#ifndef TSTASK_STATISTICS_UPDATER_H
#define TSTASK_STATISTICS_UPDATER_H


namespace TSTask
{

	class CTSTaskAppCore;

	class CStatisticsUpdater
	{
	public:
		CStatisticsUpdater(CTSTaskAppCore &Core);
		~CStatisticsUpdater();
		bool StartThread();
		void EndThread();
		bool SetUpdateInterval(DWORD Interval);

	private:
		unsigned int UpdateThread();

		CTSTaskAppCore &m_Core;
		CThread m_Thread;
		CEvent m_EndEvent;
		DWORD m_UpdateInterval;
	};

}


#endif
