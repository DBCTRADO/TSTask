#include "stdafx.h"
#include "TSTaskBase.h"


namespace TSTask
{

	namespace Debug
	{

		void Initialize()
		{
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF/* | _CRTDBG_CHECK_ALWAYS_DF*/);
		}

		void Trace(LPCTSTR pszFormat, ...)
		{
			TCHAR szTempStr[1024];
			int Length;

			SYSTEMTIME st;
			::GetLocalTime(&st);
			Length = ::_sntprintf_s(szTempStr, _countof(szTempStr), _TRUNCATE,
									   TEXT("%02d/%02d %02d:%02d:%02d > "),
									   st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

			va_list Args;
			va_start(Args, pszFormat);
			::_vsntprintf_s(szTempStr + Length, _countof(szTempStr) - Length, _TRUNCATE, pszFormat, Args);
			va_end(Args);

			::OutputDebugString(szTempStr);
		}

	}

}
