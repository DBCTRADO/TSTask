#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskAppCore.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CReserveManager::CReserveManager(CTSTaskAppCore &Core)
		: m_Core(Core)
		, m_LockTimeout(5000)
		, m_fReserved(false)
		, m_fStarted(false)
		, m_hTimer(nullptr)
	{
	}

	CReserveManager::~CReserveManager()
	{
		Cancel();
	}

	bool CReserveManager::Reserve(const ReserveSettings &Reserve,const RecordingSettings &RecSettings)
	{
		OutLog(LOG_INFO,L"�\��̓o�^���s���܂��B");

		if (Reserve.StartTime.Type<0 || Reserve.StartTime.Type>=ReserveSettings::START_TIME_TRAILER
				|| Reserve.EndTime.Type<0 || Reserve.EndTime.Type>=ReserveSettings::END_TIME_TRAILER) {
			OutLog(LOG_ERROR,L"�\������̎w�肪�s���ł��B");
			return false;
		}

		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (m_fReserved) {
			OutLog(LOG_ERROR,L"�\�񂪊��ɍs���Ă��邽�߁A�V�����\�񂪂ł��܂���B");
			return false;
		}

		m_ReserveInfo.Reserve=Reserve;
		m_ReserveInfo.Settings=RecSettings;
		::GetSystemTimeAsFileTime(&m_ReserveInfo.Reserve.ReserveTime.Time);
		m_ReserveInfo.Reserve.ReserveTime.TickCount=::GetTickCount64();

		m_fReserved=true;
		m_fStarted=false;

		m_EndEvent.Create();
		m_hTimer=::CreateWaitableTimer(nullptr,FALSE,nullptr);
		if (m_hTimer==nullptr) {
			OutSystemErrorLog(::GetLastError(),L"Waitable timer ���쐬�ł��܂���B");
			Cancel();
			return false;
		}

		CThread::CFunctor<CReserveManager> *pFunctor=
			new CThread::CFunctor<CReserveManager>(this,&CReserveManager::TimerWaitThread);
		if (!m_TimerThread.Begin(pFunctor)) {
			OutLog(LOG_ERROR,L"�\��^�C�}�[�̃X���b�h���쐬�ł��܂���B");
			delete pFunctor;
			Cancel();
			return false;
		}

		return true;
	}

	bool CReserveManager::Cancel()
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (m_hTimer!=nullptr) {
			OutLog(LOG_VERBOSE,L"�\����L�����Z�����܂��B");
			::CancelWaitableTimer(m_hTimer);
		}

		if (m_TimerThread.IsOpened()) {
			m_EndEvent.Set();
			if (m_TimerThread.Wait(5000)==WAIT_TIMEOUT) {
				OutLog(LOG_WARNING,L"�\��^�C�}�[�̃X���b�h���������Ȃ����ߋ����I�����܂��B");
				m_TimerThread.Terminate();
			}
			m_TimerThread.Close();
		}

		if (m_hTimer!=nullptr) {
			::CloseHandle(m_hTimer);
			m_hTimer=nullptr;
		}

		m_EndEvent.Close();

		m_fReserved=false;
		m_fStarted=false;

		return true;
	}

	bool CReserveManager::IsReserved() const
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		return m_fReserved;
	}

	bool CReserveManager::GetReserveInfo(ReserveInfo *pInfo) const
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (!m_fReserved)
			return false;

		if (pInfo!=nullptr)
			*pInfo=m_ReserveInfo;

		return true;
	}

#if 0

	bool CReserveManager::QueryStart(int Margin,ReserveInfo *pInfo)
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (m_fReserved && !m_fStarted) {
			bool fStart=false;

			switch (m_ReserveInfo.Reserve.StartTime.Type) {
			case ReserveSettings::START_TIME_DATE:
				{
					FILETIME CurTime;

					::GetSystemTimeAsFileTime(&CurTime);
					if (FileTimeSpan(CurTime,m_ReserveInfo.Reserve.StartTime.Time)<=Margin*FILETIME_MILLISECOND)
						fStart=true;
				}
				break;

			case ReserveSettings::START_TIME_DELAY:
				if (m_ReserveInfo.Reserve.ReserveTime.TickCount+m_ReserveInfo.Reserve.StartTime.Delay<=
						::GetTickCount64()+Margin)
					fStart=true;
				break;
			}

			if (fStart) {
				if (pInfo!=nullptr)
					*pInfo=m_ReserveInfo;
				m_ReserveInfo.Reserve.StartTime.TickCount=::GetTickCount64();
				m_fStarted=true;
				return true;
			}
		}

		return false;
	}

	bool CReserveManager::QueryStop()
	{
		CTryBlockLock Lock(m_Lock);
		if (!Lock.TryLock(m_LockTimeout)) {
			OutLog(LOG_ERROR,L"�^�C���A�E�g���܂����B");
			return false;
		}

		if (m_fReserved && m_fStarted) {
			bool fStop=false;

			switch (m_ReserveInfo.Reserve.EndTime.Type) {
			case ReserveSettings::END_TIME_DATE:
				{
					FILETIME CurTime;

					::GetSystemTimeAsFileTime(&CurTime);
					if (::CompareFileTime(&CurTime,&m_ReserveInfo.Reserve.EndTime.Time)>=0)
						fStop=true;
				}
				break;

			case ReserveSettings::END_TIME_DURATION:
				if (m_ReserveInfo.Reserve.StartTime.TickCount+m_ReserveInfo.Reserve.EndTime.Duration<=::GetTickCount64())
					fStop=true;
				break;
			}

			if (fStop) {
				m_fReserved=false;
				m_fStarted=false;
				return true;
			}
		}

		return false;
	}

#endif

	unsigned int CReserveManager::TimerWaitThread()
	{
		const ReserveInfo Info=m_ReserveInfo;

		LARGE_INTEGER liTime;

		switch (Info.Reserve.StartTime.Type) {
		case ReserveSettings::START_TIME_NOT_SPECIFIED:
			liTime.QuadPart=0;
			break;

		case ReserveSettings::START_TIME_DATE:
			FILETIME ft;
			::GetSystemTimeAsFileTime(&ft);
			if (::CompareFileTime(&ft,&Info.Reserve.StartTime.Time)<0) {
				liTime.LowPart=Info.Reserve.StartTime.Time.dwLowDateTime;
				liTime.HighPart=Info.Reserve.StartTime.Time.dwHighDateTime;
			} else {
				liTime.QuadPart=0;
			}
			break;

		case ReserveSettings::START_TIME_DELAY:
			liTime.QuadPart=-(LONGLONG)Info.Reserve.StartTime.Delay*10000LL;
			break;

		default:
			return 1;
		}

		if (liTime.QuadPart!=0) {
			if (liTime.QuadPart<0) {
				OutLog(LOG_INFO,L"%lld �b��ɊJ�n����悤�Ƀ^�C�}�[��ݒ肵�܂��B",
					   -liTime.QuadPart/10000000LL);
			} else {
				SYSTEMTIME st;
				::FileTimeToSystemTime(reinterpret_cast<FILETIME*>(&liTime),&st);
				OutLog(LOG_INFO,L"%d/%d/%d %d:%02d:%02d (UTC) �ɊJ�n����悤�Ƀ^�C�}�[��ݒ肵�܂��B",
					   st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
			}
		} else {
			OutLog(LOG_INFO,L"�����ɊJ�n����悤�Ƀ^�C�}�[��ݒ肵�܂��B");
		}
		if (!::SetWaitableTimer(m_hTimer,&liTime,0,nullptr,nullptr,TRUE)) {
			OutSystemErrorLog(::GetLastError(),L"�\��J�n�p Waitable timer ��ݒ�ł��܂���B");
			return 1;
		}

		HANDLE Handles[2];
		Handles[0]=m_EndEvent.GetHandle();
		Handles[1]=m_hTimer;
		DWORD Result=::WaitForMultipleObjects(2,Handles,FALSE,INFINITE);

		if (Result==WAIT_OBJECT_0+1) {
			OutLog(LOG_INFO,L"�\�񂳂ꂽ�^����J�n���܂��B");
			RecordingSettings Settings=Info.Settings;
			String FileName;
			if (m_Core.FormatRecordFileName(Settings.FileName.c_str(),&FileName))
				Settings.FileName=FileName;
			if (!m_Core.GetCore().StartRecording(Settings))
				return 1;

			if (Info.Reserve.EndTime.Type!=ReserveSettings::END_TIME_NOT_SPECIFIED) {
				if (Info.Reserve.EndTime.Type==ReserveSettings::END_TIME_DATE) {
					FILETIME ft;
					::GetSystemTimeAsFileTime(&ft);
					if (::CompareFileTime(&ft,&Info.Reserve.EndTime.Time)<0) {
						liTime.LowPart=Info.Reserve.EndTime.Time.dwLowDateTime;
						liTime.HighPart=Info.Reserve.EndTime.Time.dwHighDateTime;
					} else {
						OutLog(LOG_WARNING,L"�^���~���������ɉ߂��Ă��܂��B");
						liTime.QuadPart=0;
					}
				} else {
					liTime.QuadPart=-(LONGLONG)Info.Reserve.EndTime.Duration*10000LL;
				}
				if (liTime.QuadPart!=0) {
					if (liTime.QuadPart<0) {
						OutLog(LOG_INFO,L"%lld �b��ɒ�~����悤�Ƀ^�C�}�[��ݒ肵�܂��B",
							   -liTime.QuadPart/10000000LL);
					} else {
						SYSTEMTIME st;
						::FileTimeToSystemTime(reinterpret_cast<FILETIME*>(&liTime),&st);
						OutLog(LOG_INFO,L"%d/%d/%d %d:%02d:%02d (UTC) �ɒ�~����悤�Ƀ^�C�}�[��ݒ肵�܂��B",
							   st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					}
				} else {
					OutLog(LOG_INFO,L"�����ɒ�~����悤�Ƀ^�C�}�[��ݒ肵�܂��B");
				}
				if (!::SetWaitableTimer(m_hTimer,&liTime,0,nullptr,nullptr,FALSE)) {
					OutSystemErrorLog(::GetLastError(),L"�\���~�p Waitable timer ��ݒ�ł��܂���B");
					return 1;
				}

				Result=::WaitForMultipleObjects(2,Handles,FALSE,INFINITE);
				if (Result==WAIT_OBJECT_0+1) {
					OutLog(LOG_INFO,L"�\�񂳂ꂽ�^����~���܂��B");
					m_Core.GetCore().StopRecording();
				} else if (Result==WAIT_OBJECT_0) {
					OutLog(LOG_INFO,L"�\���~�̑ҋ@���L�����Z������܂����B");
				} else {
					OutLog(LOG_ERROR,L"�\���~�̑ҋ@�ŃG���[���������܂����B(Code %lu)",Result);
				}
			}
		} else if (Result==WAIT_OBJECT_0) {
			OutLog(LOG_INFO,L"�\��J�n�̑ҋ@���L�����Z������܂����B");
		} else {
			OutLog(LOG_ERROR,L"�\��J�n�̑ҋ@�ŃG���[���������܂����B(Code %lu)",Result);
		}

		return 0;
	}

}
