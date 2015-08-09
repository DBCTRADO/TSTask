#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskAppCore.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CTSTaskAppCore::CTSTaskAppCore(HINSTANCE hInstance)
		: m_hInstance(hInstance)
		, m_ReserveManager(*this)
		, m_StatisticsUpdater(*this)
	{
	}

	bool CTSTaskAppCore::ExecuteClient()
	{
		String Command;
		if (!m_CurSettings.General.GetClientCommand(&Command))
			return false;

		OutLog(LOG_INFO,L"クライアント(%s)を起動します。",Command.c_str());

		STARTUPINFO si;
		::ZeroMemory(&si,sizeof(si));
		si.cb=sizeof(si);
		const int ShowCommand=m_CurSettings.General.GetClientShowCommand();
		if (ShowCommand>=0) {
			si.dwFlags|=STARTF_USESHOWWINDOW;
			si.wShowWindow=ShowCommand;
		}
		PROCESS_INFORMATION pi;
		::ZeroMemory(&pi,sizeof(pi));
		if (!::CreateProcess(nullptr,const_cast<LPWSTR>(Command.c_str()),
							 nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi)) {
			OutSystemErrorLog(::GetLastError(),L"クライアント(%s)を起動できません。",
							  Command.c_str());
			return false;
		}

		OutLog(LOG_VERBOSE,L"プロセス(%s)を作成しました。(PID %u)",
			   Command.c_str(),pi.dwProcessId);

		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);

		return true;
	}

	bool CTSTaskAppCore::FormatRecordFileName(LPCWSTR pszFormat,String *pFileName)
	{
		return m_Core.FormatRecordFileName(pszFormat,pFileName,
										   m_CurSettings.Recording.GetUseNextEventInfoMargin());
	}

	bool CTSTaskAppCore::ReserveRecording(const ReserveSettings &Reserve,const RecordingSettings &RecSettings)
	{
		return m_ReserveManager.Reserve(Reserve,RecSettings);
	}

	bool CTSTaskAppCore::CancelReserve()
	{
		return m_ReserveManager.Cancel();
	}

	bool CTSTaskAppCore::GetReserveInfo(ReserveInfo *pInfo) const
	{
		return m_ReserveManager.GetReserveInfo(pInfo);
	}

	bool CTSTaskAppCore::UpdateStreamStatistics()
	{
		StreamStatistics Statistics;

		if (!m_Core.GetStreamStatistics(&Statistics))
			ResetStreamStatistics(&Statistics);

		return m_SharedInfo.SetStreamStatistics(Statistics);
	}

	bool CTSTaskAppCore::UpdateTotTime()
	{
		SYSTEMTIME st;
		FILETIME ft;
		ULONGLONG Time;

		if (m_Core.GetTotTime(&st)
				&& ::SystemTimeToFileTime(&st,&ft)) {
			Time=(ULONGLONG(ft.dwHighDateTime)<<32) | ULONGLONG(ft.dwLowDateTime);
		} else {
			Time=0;
		}

		return m_SharedInfo.SetTotTime(Time);
	}

}
