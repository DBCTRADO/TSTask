#ifndef TSTASK_RESERVE_MANAGER_H
#define TSTASK_RESERVE_MANAGER_H


namespace TSTask
{

	class CTSTaskAppCore;

	class CReserveManager
	{
	public:
		CReserveManager(CTSTaskAppCore &Core);
		~CReserveManager();
		bool Reserve(const ReserveSettings &Reserve,const RecordingSettings &RecSettings);
		bool Cancel();
		bool IsReserved() const;
		bool GetReserveInfo(ReserveInfo *pInfo) const;
#if 0
		bool QueryStart(int Margin=0,ReserveInfo *pInfo=nullptr);
		bool QueryStop();
#endif

	private:
		unsigned int TimerWaitThread();

		CTSTaskAppCore &m_Core;
		mutable CLocalLock m_Lock;
		DWORD m_LockTimeout;
		bool m_fReserved;
		bool m_fStarted;
		ReserveInfo m_ReserveInfo;
		CThread m_TimerThread;
		HANDLE m_hTimer;
		CEvent m_EndEvent;
	};

}


#endif
