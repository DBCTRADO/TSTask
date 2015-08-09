#include "stdafx.h"
#include "TSTaskCentre.h"
#include "InformationBarItems.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	namespace InformationBarItems
	{

		class CCustomInformationStringMap : public TSTask::CVariableStringMap
		{
		public:
			struct CPUUsageInfo
			{
				ULONGLONG TickTime;
				ULONGLONG IdleTime;
				ULONGLONG KernelTime;
				ULONGLONG UserTime;
				int Percentage;
			};

			struct MemoryStatus
			{
				ULONGLONG TickTime;
				MEMORYSTATUSEX Status;
			};

			struct DriveSpaceInfo
			{
				ULONGLONG TickTime;
				ULARGE_INTEGER Total;
				ULARGE_INTEGER Free;
				ULARGE_INTEGER Available;
				DriveSpaceInfo() : TickTime(0) {}
			};

			bool BeginFormat() override
			{
				FILETIME ftUTC,ftLocal;
				::GetSystemTimeAsFileTime(&ftUTC);
				TSTask::OffsetFileTime(&ftUTC,TSTask::FILETIME_SECOND/2);
				::FileTimeToLocalFileTime(&ftUTC,&ftLocal);
				::FileTimeToSystemTime(&ftLocal,&m_CurTime);

				return true;
			}

			bool GetString(LPCWSTR pszKeyword,TSTask::String *pString) override
			{
				static const LPCWSTR NOT_AVAILABLE=L"n/a";

				if (::lstrcmpiW(pszKeyword,L"date")==0) {
					TSTask::StringUtility::Format(*pString,L"%d/%02d/%02d",
												  m_CurTime.wYear,m_CurTime.wMonth,m_CurTime.wDay);
				} else if (::lstrcmpiW(pszKeyword,L"time")==0) {
					TSTask::StringUtility::Format(*pString,L"%02d:%02d",
												  m_CurTime.wHour,m_CurTime.wMinute);
				} else if (::lstrcmpiW(pszKeyword,L"cpu")==0) {
					TSTask::StringUtility::Format(*pString,L"%d",GetCPUUsage());
				} else if (::lstrcmpiW(pszKeyword,L"cpu2")==0) {
					TSTask::StringUtility::Format(*pString,L"%02d",GetCPUUsage());
				} else if (::lstrcmpiW(pszKeyword,L"mem-total")==0) {
					if (GetMemoryStatus())
						FormatBytes(pString,m_MemoryStatus.Status.ullTotalPhys,false);
					else
						*pString=NOT_AVAILABLE;
				} else if (::lstrcmpiW(pszKeyword,L"mem-free")==0) {
					if (GetMemoryStatus())
						FormatBytes(pString,m_MemoryStatus.Status.ullAvailPhys,false);
					else
						*pString=NOT_AVAILABLE;
				} else if (::lstrcmpiW(pszKeyword,L"mem-used")==0) {
					if (GetMemoryStatus())
						FormatBytes(pString,m_MemoryStatus.Status.ullTotalPhys-m_MemoryStatus.Status.ullAvailPhys,false);
					else
						*pString=NOT_AVAILABLE;
				} else if (::lstrcmpiW(pszKeyword,L"mem-percentage")==0) {
					if (GetMemoryStatus() && m_MemoryStatus.Status.ullTotalPhys!=0) {
						ULONGLONG Used=m_MemoryStatus.Status.ullTotalPhys-m_MemoryStatus.Status.ullAvailPhys;
						TSTask::StringUtility::Format(*pString,L"%d",
							(int)((Used*100+m_MemoryStatus.Status.ullTotalPhys/2)/
								  m_MemoryStatus.Status.ullTotalPhys));
					} else {
						*pString=NOT_AVAILABLE;
					}
				} else if (::lstrcmpiW(pszKeyword,L"mem-free-percentage")==0) {
					if (GetMemoryStatus() && m_MemoryStatus.Status.ullTotalPhys!=0) {
						TSTask::StringUtility::Format(*pString,L"%d",
							(int)((m_MemoryStatus.Status.ullAvailPhys*100+m_MemoryStatus.Status.ullTotalPhys/2)/
								  m_MemoryStatus.Status.ullTotalPhys));
					} else {
						*pString=NOT_AVAILABLE;
					}
				} else if (::StrCmpNIW(pszKeyword,L"drive-avail-",12)==0) {
					DriveSpaceInfo Info;
					if (GetDriveSpace(pszKeyword[12],&Info))
						FormatBytes(pString,Info.Available.QuadPart,true);
					else
						*pString=NOT_AVAILABLE;
				} else if (::StrCmpNIW(pszKeyword,L"drive-free-",11)==0) {
					DriveSpaceInfo Info;
					if (GetDriveSpace(pszKeyword[11],&Info))
						FormatBytes(pString,Info.Free.QuadPart,true);
					else
						*pString=NOT_AVAILABLE;
				} else if (::StrCmpNIW(pszKeyword,L"drive-total-",12)==0) {
					DriveSpaceInfo Info;
					if (GetDriveSpace(pszKeyword[12],&Info))
						FormatBytes(pString,Info.Total.QuadPart,true);
					else
						*pString=NOT_AVAILABLE;
				} else if (::StrCmpNIW(pszKeyword,L"drive-used-",11)==0) {
					DriveSpaceInfo Info;
					if (GetDriveSpace(pszKeyword[11],&Info))
						FormatBytes(pString,
							Info.Total.QuadPart>Info.Available.QuadPart?
								Info.Total.QuadPart-Info.Available.QuadPart:0,true);
					else
						*pString=NOT_AVAILABLE;
				} else {
					return GetTimeString(pszKeyword,m_CurTime,pString);
				}

				return true;
			}

		private:
			int GetCPUUsage()
			{
				CPUUsageInfo Usage;
				Usage.TickTime=::GetTickCount64();
				if (Usage.TickTime-m_CPUUsage.TickTime>=1000) {
					::GetSystemTimes(reinterpret_cast<FILETIME*>(&Usage.IdleTime),
									 reinterpret_cast<FILETIME*>(&Usage.KernelTime),
									 reinterpret_cast<FILETIME*>(&Usage.UserTime));
					ULONGLONG Total=(Usage.KernelTime-m_CPUUsage.KernelTime)+
									(Usage.UserTime-m_CPUUsage.UserTime);
					if (Total!=0)
						Usage.Percentage=int(((Total-(Usage.IdleTime-m_CPUUsage.IdleTime))*100+Total/2)/Total);
					else
						Usage.Percentage=0;
					m_CPUUsage=Usage;
				}
				return m_CPUUsage.Percentage;
			}

			bool GetMemoryStatus()
			{
				MemoryStatus MemStat;
				MemStat.TickTime=::GetTickCount64();
				if (MemStat.TickTime-m_MemoryStatus.TickTime>=1000) {
					MemStat.Status.dwLength=sizeof(MEMORYSTATUSEX);
					if (!::GlobalMemoryStatusEx(&MemStat.Status))
						::ZeroMemory(&MemStat.Status,sizeof(MemStat.Status));
					m_MemoryStatus=MemStat;
				}

				return m_MemoryStatus.Status.ullTotalPhys!=0;
			}

			bool GetDriveSpace(WCHAR Drive,DriveSpaceInfo *pInfo)
			{
				if (Drive==L'\0')
					return false;

				const ULONGLONG CurTime=::GetTickCount64();
				const WCHAR DriveLetter=TSTask::ToUpper(Drive);

				auto i=m_DriveInfoMap.find(DriveLetter);
				if (i!=m_DriveInfoMap.end()
						&& CurTime-i->second.TickTime<5000) {
					if (i->second.Total.QuadPart==0)
						return false;
					*pInfo=i->second;
					return true;
				}

				WCHAR szDrive[4];
				szDrive[0]=DriveLetter;
				szDrive[1]=L':';
				szDrive[2]=L'\\';
				szDrive[3]=L'\0';
				DriveSpaceInfo Space;
				bool fOK;
				if (::GetDiskFreeSpaceEx(szDrive,&Space.Available,&Space.Total,&Space.Free)) {
					fOK=true;
				} else {
					fOK=false;
					Space.Available.QuadPart=0;
					Space.Total.QuadPart=0;
					Space.Free.QuadPart=0;
				}
				Space.TickTime=CurTime;

				if (i!=m_DriveInfoMap.end()) {
					i->second=Space;
				} else {
					m_DriveInfoMap.insert(std::pair<WCHAR,DriveSpaceInfo>(DriveLetter,Space));
				}

				*pInfo=Space;

				return fOK;
			}

			void FormatBytes(TSTask::String *pText,ULONGLONG Bytes,bool fDecimal) const
			{
				static const ULONGLONG MEGA=1024*1024;
				static const ULONGLONG GIGA=MEGA*1024ULL;
				static const ULONGLONG TERA=GIGA*1024ULL;

				if (Bytes<1024) {
					TSTask::StringUtility::Format(*pText,L"%uB",(unsigned int)Bytes);
				} else {
					if (fDecimal) {
						if (Bytes<MEGA)
							TSTask::StringUtility::Format(*pText,L"%u.%02uKB",
														  (unsigned int)Bytes/1024,
														  (unsigned int)Bytes%1024*100/1024);
						else if (Bytes<GIGA)
							TSTask::StringUtility::Format(*pText,L"%u.%02uMB",
														  (unsigned int)Bytes/(1024*1024),
														  (unsigned int)Bytes/1024%1024*100/1024);
						else if (Bytes<TERA)
							TSTask::StringUtility::Format(*pText,L"%u.%02uGB",
														  (unsigned int)(Bytes/GIGA),
														  (unsigned int)(Bytes/MEGA)%1024*100/1024);
						else
							TSTask::StringUtility::Format(*pText,L"%u.%02uTB",
														  (unsigned int)(Bytes/TERA),
														  (unsigned int)(Bytes/GIGA)%1024*100/1024);
					} else {
						if (Bytes<MEGA)
							TSTask::StringUtility::Format(*pText,L"%uKB",(unsigned int)Bytes/1024);
						else if (Bytes<GIGA)
							TSTask::StringUtility::Format(*pText,L"%uMB",(unsigned int)Bytes/(1024*1024));
						else if (Bytes<TERA)
							TSTask::StringUtility::Format(*pText,L"%uGB",(unsigned int)(Bytes/GIGA));
						else
							TSTask::StringUtility::Format(*pText,L"%uTB",(unsigned int)(Bytes/TERA));
					}
				}
			}

			SYSTEMTIME m_CurTime;
			static CPUUsageInfo m_CPUUsage;
			static MemoryStatus m_MemoryStatus;
			static std::map<WCHAR,DriveSpaceInfo> m_DriveInfoMap;
		};

		CCustomInformationStringMap::CPUUsageInfo CCustomInformationStringMap::m_CPUUsage;
		CCustomInformationStringMap::MemoryStatus CCustomInformationStringMap::m_MemoryStatus;
		std::map<WCHAR,CCustomInformationStringMap::DriveSpaceInfo> CCustomInformationStringMap::m_DriveInfoMap;


		CCustomInformationItem::CCustomInformationItem(int ID)
			: CItem(ID)
		{
			m_TextAlign=TEXT_ALIGN_RIGHT;
		}

		CCustomInformationItem::~CCustomInformationItem()
		{
		}

		void CCustomInformationItem::Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect)
		{
			if (!m_Text.empty())
				DrawText(Info,m_Text.c_str(),TextRect,ClipRect);
		}

		bool CCustomInformationItem::Update()
		{
			if (!m_Format.empty()) {
				GetText(m_Format.c_str(),&m_Text);
			} else {
				m_Text.clear();
			}

			return true;
		}

		bool CCustomInformationItem::SetFormat(LPCWSTR pszFormat)
		{
			if (!TSTask::IsStringEmpty(pszFormat)) {
				m_Format=pszFormat;
			} else {
				m_Format.clear();
			}

			Update();

			return true;
		}

		bool CCustomInformationItem::GetText(LPCWSTR pszFormat,TSTask::String *pText)
		{
			if (pText==nullptr)
				return false;

			pText->clear();

			if (pszFormat==nullptr)
				return false;

			CCustomInformationStringMap Map;
			if (!TSTask::FormatVariableString(pText,&Map,pszFormat)) {
				pText->clear();
				return false;
			}

			return true;
		}

		bool CCustomInformationItem::GetParameterInfo(int Index,ParameterInfo *pInfo)
		{
			static const ParameterInfo ParameterList[] = {
				{L"%date%",					L"日付(年/月/日)"},
				{L"%time%",					L"時刻(時/分)"},
				{L"%year%",					L"年"},
				{L"%year2%",				L"年(2桁)"},
				{L"%month%",				L"月"},
				{L"%month2%",				L"月(2桁)"},
				{L"%day%",					L"日"},
				{L"%day2%",					L"日(2桁)"},
				{L"%day-of-week%",			L"曜日"},
				{L"%day-of-week-en%",		L"曜日(英語)"},
				{L"%hour%",					L"時(24時間制)"},
				{L"%hour2%",				L"時(24時間制)(2桁)"},
				{L"%hour-12%",				L"時(12時間制)"},
				{L"%hour2-12%",				L"時(12時間制)(2桁)"},
				{L"%minute%",				L"分"},
				{L"%minute2%",				L"分(2桁)"},
				{L"%second%",				L"秒"},
				{L"%second2%",				L"秒(2桁)"},
				{L"%cpu%",					L"CPU使用率"},
				{L"%cpu2%",					L"CPU使用率(2桁)"},
				{L"%mem-total%",			L"物理メモリサイズ"},
				{L"%mem-free%",				L"物理メモリ空きサイズ"},
				{L"%mem-used%",				L"物理メモリ使用サイズ"},
				{L"%mem-percentage%",		L"物理メモリ使用率"},
				{L"%mem-free-percentage%",	L"物理メモリ空き率"},
				{L"%drive-avail-c%",		L"ドライブ利用可能容量"},
				{L"%drive-free-c%",			L"ドライブ空き容量"},
				{L"%drive-total-c%",		L"ドライブ容量"},
				{L"%drive-used-c%",			L"ドライブ使用量"},
			};

			if (Index<0 || Index>=_countof(ParameterList) || pInfo==nullptr)
				return false;

			*pInfo=ParameterList[Index];

			return true;
		}

	}

}
