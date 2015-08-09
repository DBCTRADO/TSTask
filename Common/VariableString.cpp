#include "stdafx.h"
#include "TSTaskBase.h"
#include "VariableString.h"


namespace TSTask
{

	bool FormatVariableString(String *pString,CVariableStringMap *pVariableMap,LPCWSTR pszFormat)
	{
		if (pString==nullptr || pVariableMap==nullptr || pszFormat==nullptr)
			return false;

		pString->clear();

		if (!pVariableMap->BeginFormat())
			return false;

		LPCWSTR p=pszFormat;

		while (*p!=L'\0') {
			if (*p==L'%') {
				p++;
				if (*p==L'%') {
					pString->append(L"%");
					p++;
				} else {
					String Keyword,Text;

					while (*p!=L'%' && *p!=L'\0')
						Keyword.push_back(*p++);
					if (*p==L'%') {
						p++;
						if (pVariableMap->GetString(Keyword.c_str(),&Text)) {
							if (pVariableMap->IsFileName())
								PathUtility::CorrectFileName(&Text);
							pString->append(Text);
						} else {
							pString->append(L"%");
							if (pVariableMap->IsFileName())
								PathUtility::CorrectFileName(&Keyword);
							pString->append(Keyword);
							pString->append(L"%");
						}
					} else {
						pString->append(L"%");
						if (pVariableMap->IsFileName())
							PathUtility::CorrectFileName(&Keyword);
						pString->append(Keyword);
					}
				}
			} else {
				pString->push_back(*p++);
			}
		}

		pVariableMap->EndFormat();

		return true;
	}


	bool CVariableStringMap::GetTimeString(LPCWSTR pszKeyword,const SYSTEMTIME &Time,String *pString) const
	{
		if (::lstrcmpiW(pszKeyword,L"date")==0) {
			StringUtility::Format(*pString,L"%d%02d%02d",
								  Time.wYear,Time.wMonth,Time.wDay);
		} else if (::lstrcmpiW(pszKeyword,L"time")==0) {
			StringUtility::Format(*pString,L"%02d%02d%02d",
								  Time.wHour,Time.wMinute,Time.wSecond);
		} else if (::lstrcmpiW(pszKeyword,L"year")==0) {
			StringUtility::Format(*pString,L"%d",Time.wYear);
		} else if (::lstrcmpiW(pszKeyword,L"year2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wYear%100);
		} else if (::lstrcmpiW(pszKeyword,L"month")==0) {
			StringUtility::Format(*pString,L"%d",Time.wMonth);
		} else if (::lstrcmpiW(pszKeyword,L"month2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wMonth);
		} else if (::lstrcmpiW(pszKeyword,L"day")==0) {
			StringUtility::Format(*pString,L"%d",Time.wDay);
		} else if (::lstrcmpiW(pszKeyword,L"day2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wDay);
		} else if (::lstrcmpiW(pszKeyword,L"hour")==0) {
			StringUtility::Format(*pString,L"%d",Time.wHour);
		} else if (::lstrcmpiW(pszKeyword,L"hour2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wHour);
		} else if (::lstrcmpiW(pszKeyword,L"hour-12")==0) {
			StringUtility::Format(*pString,L"%d",
								  Time.wHour<=12?Time.wHour:Time.wHour-12);
		} else if (::lstrcmpiW(pszKeyword,L"hour2-12")==0) {
			StringUtility::Format(*pString,L"%02d",
								  Time.wHour<=12?Time.wHour:Time.wHour-12);
		} else if (::lstrcmpiW(pszKeyword,L"minute")==0) {
			StringUtility::Format(*pString,L"%d",Time.wMinute);
		} else if (::lstrcmpiW(pszKeyword,L"minute2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wMinute);
		} else if (::lstrcmpiW(pszKeyword,L"second")==0) {
			StringUtility::Format(*pString,L"%d",Time.wSecond);
		} else if (::lstrcmpiW(pszKeyword,L"second2")==0) {
			StringUtility::Format(*pString,L"%02d",Time.wSecond);
		} else if (::lstrcmpiW(pszKeyword,L"day-of-week")==0) {
			GetDayOfWeekText(Time.wDayOfWeek,pString);
		} else if (::lstrcmpiW(pszKeyword,L"day-of-week-en")==0) {
			static const LPCWSTR DayOfWeekList[] = {
				L"Sun",L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat"
			};
			if (Time.wDayOfWeek<_countof(DayOfWeekList))
				*pString=DayOfWeekList[Time.wDayOfWeek];
		} else {
			return false;
		}

		return true;
	}


	CEventVariableStringMap::CEventVariableStringMap(const EventInfo &Info)
		: m_EventInfo(Info)
	{
	}

	bool CEventVariableStringMap::BeginFormat()
	{
		::GetLocalTime(&m_CurrentTime);

		return true;
	}

	bool CEventVariableStringMap::GetString(LPCWSTR pszKeyword,String *pString)
	{
		if (::lstrcmpiW(pszKeyword,L"channel-name")==0) {
			*pString=m_EventInfo.Channel.ChannelName;
		} else if (::lstrcmpiW(pszKeyword,L"channel-no")==0) {
			StringUtility::Format(*pString,L"%d",m_EventInfo.Channel.RemoteControlKeyID);
		} else if (::lstrcmpiW(pszKeyword,L"channel-no2")==0) {
			StringUtility::Format(*pString,L"%02d",m_EventInfo.Channel.RemoteControlKeyID);
		} else if (::lstrcmpiW(pszKeyword,L"channel-no3")==0) {
			StringUtility::Format(*pString,L"%03d",m_EventInfo.Channel.RemoteControlKeyID);
		} else if (::lstrcmpiW(pszKeyword,L"event-name")==0) {
			*pString=m_EventInfo.Event.EventName;
		} else if (::lstrcmpiW(pszKeyword,L"event-title")==0) {
			GetEventTitle(m_EventInfo.Event.EventName,pString);
		} else if (::lstrcmpiW(pszKeyword,L"event-id")==0) {
			StringUtility::Format(*pString,L"%04X",m_EventInfo.Event.EventID);
		} else if (::lstrcmpiW(pszKeyword,L"service-id")==0) {
			StringUtility::Format(*pString,L"%04X",m_EventInfo.Channel.ServiceID);
		} else {
			return GetTimeString(pszKeyword,m_CurrentTime,pString);
		}

		return true;
	}

	bool CEventVariableStringMap::GetParameterInfo(int Index,ParameterInfo *pInfo)
	{
		static const ParameterInfo ParameterList[] = {
			{TEXT("%date%"),			TEXT("開始年月日")},
			{TEXT("%year%"),			TEXT("開始年")},
			{TEXT("%year2%"),			TEXT("開始年(下2桁)")},
			{TEXT("%month%"),			TEXT("開始月")},
			{TEXT("%month2%"),			TEXT("開始月(2桁)")},
			{TEXT("%day%"),				TEXT("開始日")},
			{TEXT("%day2%"),			TEXT("開始日(2桁)")},
			{TEXT("%time%"),			TEXT("開始時刻(時+分+秒)")},
			{TEXT("%hour%"),			TEXT("開始時")},
			{TEXT("%hour2%"),			TEXT("開始時(2桁)")},
			{TEXT("%minute%"),			TEXT("開始分")},
			{TEXT("%minute2%"),			TEXT("開始分(2桁)")},
			{TEXT("%second%"),			TEXT("開始秒")},
			{TEXT("%second2%"),			TEXT("開始秒(2桁)")},
			{TEXT("%day-of-week%"),		TEXT("開始曜日")},
			{TEXT("%day-of-week-en%"),	TEXT("開始曜日(英語)")},
			{TEXT("%channel-name%"),	TEXT("チャンネル名")},
			{TEXT("%channel-no%"),		TEXT("チャンネル番号")},
			{TEXT("%channel-no2%"),		TEXT("チャンネル番号(2桁)")},
			{TEXT("%channel-no3%"),		TEXT("チャンネル番号(3桁)")},
			{TEXT("%event-name%"),		TEXT("番組名")},
			{TEXT("%event-title%"),		TEXT("番組タイトル")},
			{TEXT("%event-id%"),		TEXT("イベントID")},
			{TEXT("%service-id%"),		TEXT("サービスID")},
		};

		if (Index<0 || Index>=_countof(ParameterList))
			return false;

		*pInfo=ParameterList[Index];

		return true;
	}

	bool CEventVariableStringMap::GetSampleEventInfo(EventInfo *pInfo)
	{
		if (pInfo==nullptr)
			return false;

		pInfo->Channel.Space=0;
		pInfo->Channel.Channel=8;
		pInfo->Channel.ScannedChannel=3;
		pInfo->Channel.RemoteControlKeyID=11;
		pInfo->Channel.ChannelName=L"アフリカ中央テレビ";
		pInfo->Channel.SpaceName=L"地デジ";
		pInfo->Channel.ServiceID=112;
		pInfo->Channel.NetworkID=2;
		pInfo->Channel.TransportStreamID=556;
		pInfo->Event.EventID=839;
		::GetLocalTime(&pInfo->Event.StartTime);
		pInfo->Event.StartTime.wMinute=30;
		pInfo->Event.StartTime.wSecond=0;
		pInfo->Event.StartTime.wMilliseconds=0;
		pInfo->Event.Duration=30;
		pInfo->Event.EventName=L"[二][字]今日のニュース";
		pInfo->Event.EventText=L"本日のニュースをお伝えします。";

		return true;
	}

	void CEventVariableStringMap::GetEventTitle(const String &EventName,String *pTitle)
	{
		// 番組名から [字] のようなものを除去する

		pTitle->clear();

		String::size_type Next=0;

		while (Next<EventName.length()) {
			String::size_type Left=EventName.find(L'[',Next);
			if (Left==String::npos) {
				pTitle->append(EventName.substr(Next));
				break;
			}

			String::size_type Right=EventName.find(L']',Left+1);
			if (Right==String::npos) {
				pTitle->append(EventName.substr(Next));
				break;
			}

			if (Left>Next)
				pTitle->append(EventName.substr(Next,Left-Next));

			if (Right-Left>3)
				pTitle->append(EventName.substr(Left,Right-Left));

			Next=Right+1;
		}

		StringUtility::Trim(*pTitle,L" ");
	}

}
