#include "stdafx.h"
#include "TSTaskBase.h"
#include "DebugDef.h"


namespace TSTask
{

	CLogger::CLogger()
	{
	}

	CLogger::~CLogger()
	{
	}

	void CLogger::OutLog(LogType Type,LPCWSTR pszText, ...)
	{
		if (pszText==nullptr)
			return;

		va_list Args;
		va_start(Args,pszText);
		OutLogV(Type,pszText,Args);
		va_end(Args);
	}


	CBasicLogger::CBasicLogger()
		: m_MaxLog(1000)
		, m_LoggingLevel(LOG_INFO)
		, m_Number(0)
		, m_hFile(INVALID_HANDLE_VALUE)
		, m_fDebugTrace(
#ifdef _DEBUG
			true
#else
			false
#endif
			)
		, m_pHandler(nullptr)
	{
	}

	CBasicLogger::~CBasicLogger()
	{
		CloseFile();
		Clear();
	}

	void CBasicLogger::Clear()
	{
		CBlockLock Lock(m_Lock);

		for (auto e:m_LogList)
			delete e;
		m_LogList.clear();
	}

	void CBasicLogger::OutLogV(LogType Type,LPCWSTR pszText,va_list Args)
	{
		FILETIME CurTime;
		::GetSystemTimeAsFileTime(&CurTime);

		CBlockLock Lock(m_Lock);

		if (Type<m_LoggingLevel || m_MaxLog==0)
			return;

		if (m_LogList.size()>=m_MaxLog) {
			delete m_LogList.front();
			m_LogList.pop_front();
		}

		LogInfo *pItem=new LogInfo;

		pItem->Type=Type;
		pItem->Number=m_Number++;
		StringUtility::FormatV(pItem->Text,pszText,Args);
		pItem->Time=CurTime;

		m_LogList.push_back(pItem);

		if (m_fDebugTrace) {
			String Text(pItem->Text);
			Text.push_back(L'\n');
			::OutputDebugString(Text.c_str());
		}

		if (m_hFile!=INVALID_HANDLE_VALUE) {
			WriteToFile(m_hFile,pItem);
			::FlushFileBuffers(m_hFile);
		}

		if (m_pHandler!=nullptr)
			m_pHandler->OnLog(*pItem);
	}

	bool CBasicLogger::GetLog(LogList *pList,size_t Max) const
	{
		if (pList==nullptr)
			return false;

		CBlockLock Lock(m_Lock);

		if (Max==0 || Max>m_LogList.size())
			Max=m_LogList.size();

		pList->resize(Max);
		auto i=m_LogList.begin();
		std::advance(i,m_LogList.size()-Max);
		for (size_t j=0;j<Max;j++)
			(*pList)[j]=**i++;

		return true;
	}

	bool CBasicLogger::GetLogByNumber(unsigned int Number,LogInfo *pInfo) const
	{
		if (pInfo==nullptr)
			return false;

		CBlockLock Lock(m_Lock);

		LogInfo Key;
		Key.Number=Number;

		auto i=std::lower_bound(m_LogList.begin(),m_LogList.end(),&Key,
								[](const LogInfo *pLog1,const LogInfo *pLog2) { return pLog1->Number<pLog2->Number; });
		if (i==m_LogList.end())
			return false;

		*pInfo=**i;

		return true;
	}

	bool CBasicLogger::GetLastLog(LogInfo *pInfo,unsigned int Types) const
	{
		if (pInfo==nullptr)
			return false;

		CBlockLock Lock(m_Lock);

		for (auto i=m_LogList.rbegin();i!=m_LogList.rend();i++) {
			if ((Types&MakeTypeFlag((*i)->Type))!=0) {
				*pInfo=**i;
				return true;
			}
		}

		return false;
	}

	bool CBasicLogger::SetMaxLog(size_t Max)
	{
		CBlockLock Lock(m_Lock);

		m_MaxLog=Max;

		if (Max>0 && m_LogList.size()>Max) {
			do {
				delete m_LogList.front();
				m_LogList.pop_front();
			} while (m_LogList.size()>Max);
		}

		return true;
	}

	bool CBasicLogger::SetLoggingLevel(LogType Level)
	{
		if (Level<0 || Level>LOG_NONE)
			return false;

		CBlockLock Lock(m_Lock);

		m_LoggingLevel=Level;

		return true;
	}

	bool CBasicLogger::SaveToFile(LPCWSTR pszFileName,bool fAppend) const
	{
		if (IsStringEmpty(pszFileName))
			return false;

		TRACE(L"ログを \"%s\" に保存します。\n",pszFileName);

		HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,nullptr,
								  fAppend?OPEN_ALWAYS:CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL,nullptr);
		if (hFile==INVALID_HANDLE_VALUE)
			return false;

		DWORD Write;

		if (fAppend) {
			LARGE_INTEGER FileSize;

			if (!::GetFileSizeEx(hFile,&FileSize)) {
				::CloseHandle(hFile);
				return false;
			}
			if (FileSize.QuadPart>0) {
				::SetFilePointer(hFile,0,nullptr,FILE_END);
				::WriteFile(hFile,L"\r\n",2*sizeof(WCHAR),&Write,nullptr);
			} else {
				fAppend=false;
			}
		}

		if (!fAppend) {
			static const WCHAR BOM=0xFEFF;
			::WriteFile(hFile,&BOM,2,&Write,nullptr);
		}

		m_Lock.Lock();

		for (auto e:m_LogList) {
			if (!WriteToFile(hFile,e))
				break;
		}

		m_Lock.Unlock();

		return true;
	}

	bool CBasicLogger::OpenFile(LPCWSTR pszFileName,unsigned int Flags)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		CBlockLock Lock(m_Lock);

		if (m_hFile!=INVALID_HANDLE_VALUE)
			return false;

		OutLog(LOG_INFO,L"ログを \"%s\" に書き出します。",pszFileName);

		bool fAppend=(Flags & OPEN_APPEND)!=0;

		HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,nullptr,
								  fAppend?OPEN_ALWAYS:CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL,nullptr);
		if (hFile!=INVALID_HANDLE_VALUE) {
			m_FileName=pszFileName;
		} else {
			DWORD Error=::GetLastError();
			if ((Flags & OPEN_AUTO_RENAME)!=0
					&& Error==ERROR_SHARING_VIOLATION) {
				String FileName(pszFileName),Extension,Path;
				PathUtility::GetExtension(FileName,&Extension);
				PathUtility::RemoveExtension(&FileName);
				for (int i=1;i<=MAX_SERVER_TASKS;i++) {
					StringUtility::Format(Path,L"%s.%d%s",
										  FileName.c_str(),i,Extension.c_str());
					OutLog(LOG_VERBOSE,L"ファイル \"%s\" を開きます。",Path.c_str());
					hFile=::CreateFile(Path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,
									   fAppend?OPEN_ALWAYS:CREATE_ALWAYS,
									   FILE_ATTRIBUTE_NORMAL,nullptr);
					if (hFile!=INVALID_HANDLE_VALUE) {
						OutLog(LOG_INFO,L"ファイル \"%s\" は既に使用されているため、\"%s\" に書き出します。",
							   pszFileName,Path.c_str());
						m_FileName=Path;
						break;
					}
				}
				if (hFile==INVALID_HANDLE_VALUE) {
					OutLog(LOG_ERROR,L"ログファイルを開けません。");
					return false;
				}
			} else {
				OutSystemErrorLog(Error,L"ファイル \"%s\" が開けません。",pszFileName);
				return false;
			}
		}

		DWORD Write;

		if (fAppend) {
			LARGE_INTEGER FileSize;

			if (!::GetFileSizeEx(hFile,&FileSize)) {
				::CloseHandle(hFile);
				return false;
			}
			if (FileSize.QuadPart>0) {
				::SetFilePointer(hFile,0,nullptr,FILE_END);
				::WriteFile(hFile,L"\r\n",2*sizeof(WCHAR),&Write,nullptr);
			} else {
				fAppend=false;
			}
		}

		if (!fAppend) {
			static const WCHAR BOM=0xFEFF;
			::WriteFile(hFile,&BOM,2,&Write,nullptr);
		}

		m_hFile=hFile;

		if ((Flags & OPEN_WRITE_OLD_LOG)!=0) {
			for (auto e:m_LogList) {
				if (!WriteToFile(hFile,e))
					break;
			}
		}

		return true;
	}

	bool CBasicLogger::CloseFile()
	{
		CBlockLock Lock(m_Lock);

		if (m_hFile!=INVALID_HANDLE_VALUE) {
			::CloseHandle(m_hFile);
			m_hFile=INVALID_HANDLE_VALUE;
		}

		m_FileName.clear();

		return true;
	}

	bool CBasicLogger::GetFileName(String *pFileName) const
	{
		if (pFileName==nullptr)
			return false;

		CBlockLock Lock(m_Lock);

		*pFileName=m_FileName;

		return !m_FileName.empty();
	}

	void CBasicLogger::EnableDebugTrace(bool fEnable)
	{
		CBlockLock Lock(m_Lock);

		m_fDebugTrace=fEnable;
	}

	void CBasicLogger::SetHandler(CHandler *pHandler)
	{
		CBlockLock Lock(m_Lock);

		m_pHandler=pHandler;
	}

	int CBasicLogger::FormatTime(const FILETIME &Time,LPWSTR pszText,int MaxLength)
	{
		if (pszText==nullptr || MaxLength<1)
			return false;

		pszText[0]=L'\0';

		SYSTEMTIME stUTC,stLocal;
		int Length;

		::FileTimeToSystemTime(&Time,&stUTC);
		::SystemTimeToTzSpecificLocalTime(nullptr,&stUTC,&stLocal);
		Length=::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,
							   &stLocal,nullptr,pszText,MaxLength);
		if (Length<=0)
			return 0;
		pszText[Length-1]=L' ';

#if 0
		Length+=::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,
								&stLocal,nullptr,pszText+Length,MaxLength-Length);

		return Length-1;
#else
		Length+=FormatString(pszText+Length,MaxLength-Length,
							 L"%02d:%02d:%02d.%03d",
							 stLocal.wHour,stLocal.wMinute,stLocal.wSecond,stLocal.wMilliseconds);

		return Length;
#endif
	}

	bool CBasicLogger::FormatInfo(const LogInfo &Info,String *pText)
	{
		if (pText==nullptr)
			return false;

		WCHAR szTime[MAX_TIME_TEXT];

		FormatTime(Info.Time,szTime,_countof(szTime));
		*pText=szTime;
		*pText+=L' ';
		*pText+=Info.Text;

		return true;
	}

	bool CBasicLogger::WriteToFile(HANDLE hFile,const LogInfo *pInfo) const
	{
		WCHAR szTime[MAX_TIME_TEXT];
		DWORD Write;

		int Length=FormatTime(pInfo->Time,szTime,_countof(szTime));
		szTime[Length++]=L' ';

		if (!::WriteFile(hFile,szTime,Length*sizeof(WCHAR),&Write,nullptr)
				|| !::WriteFile(hFile,pInfo->Text.data(),(DWORD)(pInfo->Text.length()*sizeof(WCHAR)),&Write,nullptr)
				|| !::WriteFile(hFile,L"\r\n",2*sizeof(WCHAR),&Write,nullptr))
			return false;

		return true;
	}


	void CDebugLogger::Clear()
	{
	}

	void CDebugLogger::OutLogV(LogType Type,LPCWSTR pszText,va_list Args)
	{
		String Text;
		StringUtility::FormatV(Text,pszText,Args);
		Text+=L"\n";
		::OutputDebugString(Text.c_str());
	}


	static CLogger *g_pGlobalLogger;

#ifdef _DEBUG
	static class CDefaultDebugLogger : public CDebugLogger
	{
	public:
		CDefaultDebugLogger()
		{
			g_pGlobalLogger=this;
		}

		~CDefaultDebugLogger()
		{
			if (g_pGlobalLogger==this)
				g_pGlobalLogger=nullptr;
		}
	} g_DefaultLogger;
#endif

	void SetGlobalLogger(CLogger *pLogger)
	{
#ifdef _DEBUG
		if (pLogger==nullptr)
			g_pGlobalLogger=&g_DefaultLogger;
		else
#endif
		g_pGlobalLogger=pLogger;
	}

	void OutLog(LogType Type,LPCWSTR pszText, ...)
	{
		if (pszText==nullptr)
			return;

		if (g_pGlobalLogger!=nullptr) {
			va_list Args;
			va_start(Args,pszText);
			g_pGlobalLogger->OutLogV(Type,pszText,Args);
			va_end(Args);
		}
	}

	void OutLogV(LogType Type,LPCWSTR pszText,va_list Args)
	{
		if (pszText==nullptr)
			return;

		if (g_pGlobalLogger!=nullptr)
			g_pGlobalLogger->OutLogV(Type,pszText,Args);
	}

	void OutSystemErrorLog(DWORD Error,LPCWSTR pszText, ...)
	{
		if (pszText==nullptr)
			return;

		va_list Args;
		String Text,Message;

		va_start(Args,pszText);
		StringUtility::FormatV(Text,pszText,Args);
		va_end(Args);
		if (GetSystemErrorMessage(Error,&Message)) {
			OutLog(LOG_ERROR,L"%s(Code 0x%x : %s)",Text.c_str(),Error,Message.c_str());
		} else {
			OutLog(LOG_ERROR,L"%s(Code 0x%x)",Text.c_str(),Error);
		}
	}

}
