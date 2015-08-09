#include "stdafx.h"
#include "TSTaskBase.h"
#include "CommandLine.h"
#include "DebugDef.h"


namespace TSTask
{

	CCommandLineParser::CCommandLineParser()
		: m_ppszArgList(nullptr)
		, m_ArgCount(0)
	{
	}

	CCommandLineParser::~CCommandLineParser()
	{
		Clear();
	}

	bool CCommandLineParser::Parse(LPCWSTR pszCommandLine)
	{
		if (pszCommandLine==nullptr)
			return false;

		Clear();

		if (pszCommandLine[0]!=L'\0') {
			m_ppszArgList=::CommandLineToArgvW(pszCommandLine,&m_ArgCount);
			if (m_ppszArgList==nullptr)
				return false;

			for (int i=0;i<m_ArgCount;i++) {
				LPCWSTR pszOption=m_ppszArgList[i];
				if ((pszOption[0]==L'/' || pszOption[0]==L'-') && pszOption[1]!=L'\0') {
					m_OptionMap.insert(std::pair<LPCWSTR,int>(pszOption+1,i));
				}
			}
		}

		return true;
	}

	void CCommandLineParser::Clear()
	{
		if (m_ppszArgList!=nullptr) {
			::LocalFree(m_ppszArgList);
			m_ppszArgList=nullptr;
		}
		m_ArgCount=0;
		m_OptionMap.clear();
	}

	bool CCommandLineParser::HasOption(LPCWSTR pszKey) const
	{
		if (IsStringEmpty(pszKey))
			return false;

		return m_OptionMap.find(pszKey)!=m_OptionMap.end();
	}

	bool CCommandLineParser::GetOption(LPCWSTR pszKey,int *pValue) const
	{
		if (IsStringEmpty(pszKey) || pValue==nullptr)
			return false;

		LONGLONG Value;
		if (!GetOption(pszKey,&Value))
			return false;

		if (Value<INT_MIN || Value>INT_MAX)
			return false;

		*pValue=(int)Value;

		return true;
	}

	bool CCommandLineParser::GetOption(LPCWSTR pszKey,LONGLONG *pValue) const
	{
		if (IsStringEmpty(pszKey) || pValue==nullptr)
			return false;

		auto i=m_OptionMap.find(pszKey);
		if (i==m_OptionMap.end() || i->second+1>=m_ArgCount)
			return false;

		wchar_t *pEnd;
		__int64 Value=::_wcstoi64(m_ppszArgList[i->second+1],&pEnd,0);
		// TODO: オーバフロー、アンダーフローのチェック
		switch (*pEnd) {
		case L'K':	Value*=1024;			break;
		case L'k':	Value*=1000;			break;
		case L'M':	Value*=1024*1024;		break;
		case L'G':	Value*=1024*1024*1024;	break;
		case L'T':	Value*=0x10000000000LL;	break;
		}

		*pValue=Value;

		return true;
	}

	bool CCommandLineParser::GetOption(LPCWSTR pszKey,String *pValue) const
	{
		if (IsStringEmpty(pszKey) || pValue==nullptr)
			return false;

		auto i=m_OptionMap.find(pszKey);
		if (i==m_OptionMap.end() || i->second+1>=m_ArgCount)
			return false;

		*pValue=m_ppszArgList[i->second+1];

		return true;
	}

	bool CCommandLineParser::GetOption(LPCWSTR pszKey,BoolSettingValue *pValue) const
	{
		if (IsStringEmpty(pszKey) || pValue==nullptr)
			return false;

		auto i=m_OptionMap.find(pszKey);
		if (i!=m_OptionMap.end()) {
			*pValue=BOOL_TRUE;
		} else {
			String Key(pszKey);
			Key+=L"-";
			i=m_OptionMap.find(Key.c_str());
			if (i!=m_OptionMap.end())
				*pValue=BOOL_FALSE;
			else
				*pValue=BOOL_DEFAULT;
		}

		return true;
	}

	bool CCommandLineParser::GetOption(LPCWSTR pszKey,FILETIME *pTime) const
	{
		if (IsStringEmpty(pszKey) || pTime==nullptr)
			return false;

		auto i=m_OptionMap.find(pszKey);
		if (i==m_OptionMap.end() || i->second+1>=m_ArgCount)
			return false;

		if (!ParseTime(m_ppszArgList[i->second+1],pTime))
			return false;

		return true;
	}

	bool CCommandLineParser::GetOptionDuration(LPCWSTR pszKey,int *pDuration) const
	{
		if (IsStringEmpty(pszKey) || pDuration==nullptr)
			return false;

		auto i=m_OptionMap.find(pszKey);
		if (i==m_OptionMap.end() || i->second+1>=m_ArgCount)
			return false;

		if (!ParseDuration(m_ppszArgList[i->second+1],pDuration))
			return false;

		return true;
	}

	bool CCommandLineParser::GetOptions(LPCWSTR pszKey,std::vector<String> *pValues) const
	{
		if (IsStringEmpty(pszKey) || pValues==nullptr)
			return false;

		pValues->clear();

		auto p=m_OptionMap.equal_range(pszKey);
		for (auto i=p.first;i!=p.second;i++) {
			if (i->second+1<m_ArgCount)
				pValues->push_back(String(m_ppszArgList[i->second+1]));
		}

		return true;
	}

	bool CCommandLineParser::ParseTime(LPCWSTR pszText,FILETIME *pTime) const
	{
		/*
		日付と時刻のパースを行う
		Y-M-DTh:m:s や Y/M/D-h:m:s などをアバウトに受け付ける
		Y、M、sは省略可能
		日付のみを指定した場合、その日の0時0分0秒からとする
		時刻のみを指定した場合、次にその時刻が来る時とする
		 */
		SYSTEMTIME CurTime,Time;
		::GetLocalTime(&CurTime);
		::ZeroMemory(&Time,sizeof(Time));

		WORD Date[3],TimeValue[3];
		int DateCount=0,TimeCount=0;
		UINT Value=0;
		for (LPCWSTR p=pszText;;p++) {
			if (*p>=L'0' && *p<=L'9') {
				Value=Value*10+(*p-L'0');
				if (Value>0xFFFF)
					return false;
			} else {
				if (*p==L'/' || *p==L'-' || *p==L'T') {
					if (DateCount==3)
						return false;
					Date[DateCount++]=(WORD)Value;
				} else if (*p==L':' || *p==L'\0') {
					if (TimeCount==3)
						return false;
					TimeValue[TimeCount++]=(WORD)Value;
					if (*p==L'\0')
						break;
				}
				Value=0;
			}
		}

		if ((DateCount==0 && TimeCount<2) || TimeCount==1)
			return false;

		int i=0;
		if (DateCount>2) {
			Time.wYear=Date[i++];
			if (Time.wYear<100)
				Time.wYear+=CurTime.wYear/100*100;
		}
		if (DateCount>1) {
			Time.wMonth=Date[i++];
			if (Time.wMonth<1 || Time.wMonth>12)
				return false;
		}
		if (DateCount>0) {
			Time.wDay=Date[i];
			if (Time.wDay<1 || Time.wDay>31)
				return false;
		}
		if (Time.wYear==0) {
			Time.wYear=CurTime.wYear;
			if (Time.wMonth==0) {
				Time.wMonth=CurTime.wMonth;
				if (Time.wDay==0) {
					Time.wDay=CurTime.wDay;
				} else if (Time.wDay<CurTime.wDay) {
					Time.wMonth++;
					if (Time.wMonth>12) {
						Time.wMonth=1;
						Time.wYear++;
					}
				}
			} else if (Time.wMonth<CurTime.wMonth) {
				Time.wYear++;
			}
		}

		if (TimeCount>0) {
			Time.wHour=TimeValue[0];
			if (TimeCount>1) {
				Time.wMinute=TimeValue[1];
				if (Time.wMinute>59)
					return false;
				if (TimeCount>2) {
					Time.wSecond=TimeValue[2];
					if (Time.wSecond>59)	// Windowsに閏秒は無いらしい
						return false;
				}
			}
		}
		if (DateCount==0) {
			if (Time.wHour<CurTime.wHour)
				Time.wHour+=24;
		}

		SYSTEMTIME st;
		FILETIME ft;
		::ZeroMemory(&st,sizeof(st));
		st.wYear=Time.wYear;
		st.wMonth=Time.wMonth;
		st.wDay=Time.wDay;
		if (!::SystemTimeToFileTime(&st,&ft))
			return false;

		OffsetFileTime(&ft,
					   (LONGLONG)Time.wHour*FILETIME_HOUR+
					   (LONGLONG)Time.wMinute*FILETIME_MINUTE+
					   (LONGLONG)Time.wSecond*FILETIME_SECOND);

		*pTime=ft;

		return true;
	}

	bool CCommandLineParser::ParseDuration(LPCWSTR pszText,int *pDuration) const
	{
		// ?h?m?s 形式の時間指定をパースする
		// 単位の指定が無い場合は秒単位と解釈する
		LPCWSTR p=pszText;
		int DurationSec=0,Duration=0;

		while (*p!=L'\0') {
			if (*p==L'-' || (*p>=L'0' && *p<=L'9')) {
				Duration=std::wcstol(p,(wchar_t**)&p,10);
				if (Duration==LONG_MAX || Duration==LONG_MIN)
					return false;
			} else {
				switch (*p) {
				case L'h': case L'H':
					DurationSec+=Duration*(60*60);
					break;
				case L'm': case L'M':
					DurationSec+=Duration*60;
					break;
				case L's': case L'S':
					DurationSec+=Duration;
					break;
				}
				Duration=0;
				p++;
			}
		}
		DurationSec+=Duration;
		*pDuration=DurationSec;

		return true;
	}

}
