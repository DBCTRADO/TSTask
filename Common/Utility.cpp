#include "stdafx.h"
#include "TSTaskBase.h"
#include "DebugDef.h"


namespace TSTask
{

	int FormatString(LPWSTR pszBuffer,size_t BufferLength,LPCWSTR pszFormat, ...)
	{
		if (pszBuffer==nullptr || BufferLength==0 || pszFormat==nullptr)
			return 0;

		va_list Args;

		va_start(Args,pszFormat);

		int Length=::_vsnwprintf_s(pszBuffer,BufferLength,_TRUNCATE,pszFormat,Args);

		va_end(Args);

		return Length;
	}

	int FormatStringV(LPWSTR pszBuffer,size_t BufferLength,LPCWSTR pszFormat,va_list Args)
	{
		if (pszBuffer==nullptr || BufferLength==0 || pszFormat==nullptr)
			return 0;

		return ::_vsnwprintf_s(pszBuffer,BufferLength,_TRUNCATE,pszFormat,Args);
	}

	int FormatString(char *pszBuffer,size_t BufferLength,const char *pszFormat, ...)
	{
		if (pszBuffer==nullptr || BufferLength==0 || pszFormat==nullptr)
			return 0;

		va_list Args;

		va_start(Args,pszFormat);

		int Length=::_vsnprintf_s(pszBuffer,BufferLength,_TRUNCATE,pszFormat,Args);

		va_end(Args);

		return Length;
	}

	int FormatStringV(char *pszBuffer,size_t BufferLength,const char *pszFormat,va_list Args)
	{
		if (pszBuffer==nullptr || BufferLength==0 || pszFormat==nullptr)
			return 0;

		return ::_vsnprintf_s(pszBuffer,BufferLength,_TRUNCATE,pszFormat,Args);
	}

	static bool IsMatchWildcardSub(LPCWSTR pszString,LPCWSTR pszWildcard)
	{
		switch (*pszWildcard) {
		case L'\0':
			return *pszString==L'\0';
		case L'*':
			return IsMatchWildcardSub(pszString,pszWildcard+1)
				|| (*pszString!=L'\0' && IsMatchWildcardSub(pszString+1,pszWildcard));
		case L'?':
			return *pszString!=L'\0'
				&& IsMatchWildcardSub(pszString+1,pszWildcard+1);
		}

		return *pszString==*pszWildcard
			&& IsMatchWildcardSub(pszString+1,pszWildcard+1);
	}

	bool IsMatchWildcard(LPCWSTR pszString,LPCWSTR pszWildcard)
	{
		if (pszString==nullptr || pszWildcard==nullptr)
			return false;

		return IsMatchWildcardSub(pszString,pszWildcard);
	}

	WCHAR ToUpper(WCHAR Char)
	{
#if 0
		if (Char>=L'a' && Char<=L'z')
			Char-=L'a'-L'A';
		return Char;
#else
		::CharUpperBuff(&Char,1);
		return Char;
#endif
	}

	WCHAR ToLower(WCHAR Char)
	{
#if 0
		if (Char>=L'A' && Char<=L'Z')
			Char+=L'a'-L'A';
		return Char;
#else
		::CharLowerBuff(&Char,1);
		return Char;
#endif
	}

	bool GetSystemErrorMessage(DWORD Error,String *pMessage)
	{
		if (pMessage==nullptr)
			return false;

		LPTSTR pszMessage;
		DWORD Length=::FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,Error,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
			reinterpret_cast<LPTSTR>(&pszMessage),0,NULL);
		if (Length==0) {
			pMessage->clear();
			return false;
		}

		pMessage->assign(pszMessage);

		::LocalFree(pszMessage);

		return true;
	}

	bool GetModuleFilePath(HMODULE hModule,String *pFilePath)
	{
		if (pFilePath==nullptr)
			return false;

		pFilePath->clear();

		WCHAR szPath[MAX_PATH];
		DWORD Result=::GetModuleFileName(hModule,szPath,_countof(szPath));
		if (Result==0)
			return false;
		if (Result<_countof(szPath)) {
			pFilePath->assign(szPath);
		} else {
			DWORD Length=1024;
			LPWSTR pszPath;

			while (true) {
				pszPath=new WCHAR[Length];
				Result=::GetModuleFileName(hModule,pszPath,Length);
				if (Result>0 && Result<Length)
					break;
				delete [] pszPath;
				if (Result==0 || Length>=32768)
					return false;
				Length*=2;
			}
			pFilePath->assign(pszPath);
			delete [] pszPath;
		}

		return true;
	}

	bool GetModuleDirectory(HMODULE hModule,String *pDirectory)
	{
		if (pDirectory==nullptr)
			return false;

		if (!GetModuleFilePath(hModule,pDirectory))
			return false;

		PathUtility::RemoveFileName(pDirectory);

		return true;
	}

	void *GetModuleFunction(LPCWSTR pszModule,LPCSTR pszFunction)
	{
		if (IsStringEmpty(pszModule) || IsStringEmpty(pszFunction))
			return nullptr;

		HMODULE hModule=::GetModuleHandle(pszModule);
		if (hModule==nullptr)
			return false;

		return ::GetProcAddress(hModule,pszFunction);
	}

	bool OffsetFileTime(FILETIME *pTime,LONGLONG Offset)
	{
		ULARGE_INTEGER uli;

		uli.LowPart=pTime->dwLowDateTime;
		uli.HighPart=pTime->dwHighDateTime;
		if (Offset<0) {
			if (uli.QuadPart<(ULONGLONG)-Offset)
				return false;
		} else {
			if (0xFFFFFFFFFFFFFFFFULL-uli.QuadPart<(ULONGLONG)Offset)
				return false;
		}
		uli.QuadPart+=Offset;

		pTime->dwLowDateTime=uli.LowPart;
		pTime->dwHighDateTime=uli.HighPart;

		return true;
	}

	bool OffsetSystemTime(SYSTEMTIME *pTime,LONGLONG Offset)
	{
		FILETIME ft;

		return ::SystemTimeToFileTime(pTime,&ft)
			&& OffsetFileTime(&ft,Offset*FILETIME_MILLISECOND)
			&& ::FileTimeToSystemTime(&ft,pTime);
	}

	LONGLONG FileTimeSpan(const FILETIME &Time1,const FILETIME &Time2)
	{
		return (((LONGLONG)Time2.dwHighDateTime<<32) | (LONGLONG)Time2.dwLowDateTime)-
			   (((LONGLONG)Time1.dwHighDateTime<<32) | (LONGLONG)Time1.dwLowDateTime);
	}

	LONGLONG SystemTimeSpan(const SYSTEMTIME &Time1,const SYSTEMTIME &Time2)
	{
		FILETIME ft1,ft2;

		::SystemTimeToFileTime(&Time1,&ft1);
		::SystemTimeToFileTime(&Time2,&ft2);
		return FileTimeSpan(ft1,ft2)/FILETIME_MILLISECOND;
	}

	bool GetDayOfWeekText(int DayOfWeek,String *pText,bool fShort,LPCWSTR pszLocale)
	{
		if (pText==nullptr)
			return false;

		pText->clear();

		if (DayOfWeek<0 || DayOfWeek>=7)
			return false;

		WCHAR szText[80];
		if (::GetCalendarInfoEx(pszLocale!=nullptr?pszLocale:LOCALE_NAME_USER_DEFAULT,
								CAL_GREGORIAN,nullptr,
								(fShort?CAL_SABBREVDAYNAME1:CAL_SDAYNAME1)+(DayOfWeek+6)%7,
								szText,_countof(szText),nullptr)<1)
			return false;

		pText->assign(szText);

		return true;
	}


	namespace StringUtility
	{

		void Reserve(String &Str,size_t Size)
		{
			if (Size>size_t(Str.max_size()))
				return;

			if (Str.capacity()<Size)
				Str.reserve(Size);
		}

		int Format(String &Str,LPCWSTR pszFormat, ...)
		{
			va_list Args;
			va_start(Args,pszFormat);
			int Length=FormatV(Str,pszFormat,Args);
			va_end(Args);

			return Length;
		}

		int FormatV(String &Str,LPCWSTR pszFormat,va_list Args)
		{
			if (pszFormat==nullptr) {
				Str.clear();
				return 0;
			}

			int Length=::_vscwprintf(pszFormat,Args);
			if (Length<=0) {
				Str.clear();
				return 0;
			}
			LPWSTR pszBuffer=new WCHAR[Length+1];
			int Result=::_vsnwprintf_s(pszBuffer,Length+1,_TRUNCATE,pszFormat,Args);
			Str=pszBuffer;
			delete [] pszBuffer;

			return (int)Str.length();
		}

		int CompareNoCase(const String &String1,const String &String2)
		{
			return ::lstrcmpiW(String1.c_str(),String2.c_str());
		}

		int CompareNoCase(const String &String1,LPCWSTR pszString2)
		{
			if (IsStringEmpty(pszString2)) {
				if (String1.empty())
					return 0;
				return 1;
			}

			return ::lstrcmpiW(String1.c_str(),pszString2);
		}

		int CompareNoCase(const String &String1,const String &String2,String::size_type Length)
		{
			return ::StrCmpNIW(String1.c_str(),String2.c_str(),int(Length));
		}

		int CompareNoCase(const String &String1,LPCWSTR pszString2,String::size_type Length)
		{
			if (IsStringEmpty(pszString2)) {
				if (String1.empty())
					return 0;
				return 1;
			}

			return ::StrCmpNIW(String1.c_str(),pszString2,int(Length));
		}

		bool Trim(String &Str,LPCWSTR pszSpaces)
		{
			if (IsStringEmpty(pszSpaces))
				return false;

			const String::size_type First=Str.find_first_not_of(pszSpaces);
			if (First==String::npos)
				return false;

			Str=Str.substr(First,Str.find_last_not_of(pszSpaces)-First+1);

			return true;
		}

		bool Replace(String &Str,LPCWSTR pszFrom,LPCWSTR pszTo)
		{
			if (IsStringEmpty(pszFrom))
				return false;

			const String::size_type FromLength=::lstrlenW(pszFrom);
			const String::size_type ToLength=IsStringEmpty(pszTo) ? 0 : ::lstrlenW(pszTo);

			String::size_type Next=0,Pos;
			while ((Pos=Str.find(pszFrom,Next))!=String::npos) {
				if (ToLength==0) {
					Str.erase(Pos,FromLength);
					Next=Pos;
				} else {
					Str.replace(Pos,FromLength,pszTo);
					Next=Pos+ToLength;
				}
			}

			return true;
		}

		bool Replace(String &Str,String::value_type From,String::value_type To)
		{
			String::size_type Next=0,Pos;
			while ((Pos=Str.find(From,Next))!=String::npos) {
				Str[Pos]=To;
				Next=Pos+1;
			}

			return true;
		}

		bool ToAnsi(const String &Src,AnsiString *pDst)
		{
			if (pDst==nullptr)
				return false;

			pDst->clear();

			if (!Src.empty()) {
				int Length=::WideCharToMultiByte(CP_ACP,0,Src.data(),(int)Src.length(),NULL,0,NULL,NULL);
				if (Length<1)
					return false;
				char *pszBuffer=new char[Length+1];
				::WideCharToMultiByte(CP_ACP,0,Src.data(),(int)Src.length(),pszBuffer,Length,NULL,NULL);
				pszBuffer[Length]='\0';
				pDst->assign(pszBuffer);
				delete [] pszBuffer;
			}

			return true;
		}

		bool Split(const String &Src,LPCWSTR pszDelimiter,std::vector<String> *pList)
		{
			if (pList==nullptr)
				return false;

			pList->clear();

			if (IsStringEmpty(pszDelimiter))
				return false;

			const int DelimiterLength=::lstrlenW(pszDelimiter);
			String::const_iterator SrcBegin=Src.begin();
			String::size_type Pos,Next=0;
			while ((Pos=Src.find(pszDelimiter,Next))!=String::npos) {
				pList->push_back(String(SrcBegin+Next,SrcBegin+Pos));
				Next=Pos+DelimiterLength;
			}
			pList->push_back(String(SrcBegin+Next,Src.end()));

			return true;
		}

		bool Combine(const std::vector<String> &List,LPCWSTR pszDelimiter,String *pDst)
		{
			if (pDst==nullptr)
				return false;

			pDst->clear();

			if (!List.empty()) {
				for (auto i=List.begin();;) {
					pDst->append(*i);
					i++;
					if (i==List.end())
						break;
					if (pszDelimiter!=nullptr)
						pDst->append(pszDelimiter);
				}
			}

			return true;
		}

		void ToUpper(String &Str)
		{
			for (auto &e:Str)
				e=TSTask::ToUpper(e);
		}

		void ToLower(String &Str)
		{
			for (auto &e:Str)
				e=TSTask::ToLower(e);
		}

	}


	namespace PathUtility
	{

		bool RemoveExtension(String *pPath)
		{
			if (pPath==nullptr)
				return false;

			String::size_type Pos=pPath->find_last_of(L'.');
			if (Pos!=String::npos && pPath->find(L'\\',Pos+1)==String::npos)
				pPath->resize(Pos);

			return true;
		}

		bool RenameExtension(String *pPath,LPCWSTR pszExtension)
		{
			if (pPath==nullptr)
				return false;

			RemoveExtension(pPath);
			if (!IsStringEmpty(pszExtension))
				pPath->append(pszExtension);

			return true;
		}

		bool GetExtension(const String &Path,String *pExtension)
		{
			if (pExtension==nullptr)
				return false;

			String::size_type Pos=Path.find_last_of(L'.');
			if (Pos!=String::npos && Path.find(L'\\',Pos+1)==String::npos)
				*pExtension=Path.substr(Pos);
			else
				pExtension->clear();

			return true;
		}

		bool RemoveFileName(String *pPath)
		{
			if (pPath==nullptr)
				return false;

			String::size_type Pos=pPath->find_last_of(L'\\');
			if (Pos==String::npos)
				return false;

			if (Pos>0 && pPath->at(Pos-1)==L':')
				Pos++;

			if (Pos==pPath->length())
				return false;

			pPath->resize(Pos);

			return true;
		}

		bool Append(String *pPath,LPCWSTR pszMore)
		{
			if (pPath==nullptr)
				return false;

			if (!pPath->empty() && pPath->at(pPath->length()-1)!=L'\\')
				pPath->append(L"\\");
			if (!IsStringEmpty(pszMore))
				pPath->append(pszMore[0]!=L'\\'?pszMore:pszMore+1);

			return true;
		}

		bool GetFileName(const String &Path,String *pFileName)
		{
			if (pFileName==nullptr)
				return false;

			String::size_type Pos=Path.find_last_of(L'\\');
			if (Pos==String::npos)
				*pFileName=Path;
			else
				*pFileName=Path.substr(Pos+1);

			return true;
		}

		bool Split(const String &Path,String *pDirectory,String *pFileName)
		{
			if (pDirectory==nullptr || pFileName==nullptr)
				return false;

			String::size_type Pos=Path.find_last_of(L'\\');
			if (Pos==String::npos) {
				pDirectory->clear();
				*pFileName=Path;
			} else {
				*pFileName=Path.substr(Pos+1);
				if (Pos>0 && Path.at(Pos-1)==L':')
					Pos++;
				*pDirectory=Path.substr(0,Pos);
			}

			return true;
		}

		bool AppendDelimiter(String *pPath)
		{
			if (pPath==nullptr)
				return false;

			if (pPath->length()==1 && ::IsCharAlpha((*pPath)[0])) {
				pPath->append(L":\\");
			} else if (pPath->empty() || pPath->at(pPath->length()-1)!=L'\\') {
				pPath->append(L"\\");
			}

			return true;
		}

		bool RemoveDelimiter(String *pPath)
		{
			if (pPath==nullptr || pPath->empty())
				return false;

			String::size_type Pos=pPath->length();
			while (Pos>0 && pPath->at(Pos-1)==L'\\')
				Pos--;

			pPath->resize(Pos);

			return true;
		}

		bool IsAbsolute(const String &Path)
		{
			return Path.length()>=2
				&& ((Path[0]==L'\\' && Path[1]==L'\\')
					|| (Path[1]==L':'));
		}

		bool RelativeToAbsolute(String *pAbsolutePath,const String &BasePath,const String &RelativePath)
		{
			if (pAbsolutePath==nullptr)
				return false;

			pAbsolutePath->clear();

			if (BasePath.empty()) {
				if (IsAbsolute(RelativePath)) {
					*pAbsolutePath=RelativePath;
					return true;
				}
				return false;
			}

			if (RelativePath.empty()) {
				*pAbsolutePath=BasePath;
				return true;
			}

			String Path(BasePath);
			Append(&Path,RelativePath);

			String::size_type Pos,Next=0;
			String Item;
			do {
				Pos=Path.find(L'\\',Next);
				if (Pos==String::npos)
					Pos=Path.length();
				if (Pos>Next) {
					Item=Path.substr(Next,Pos-Next);
					if (Item.compare(L".")==0) {
						Path.erase(Next,Pos-Next+1);
						Pos=Next;
					} else if (Item.compare(L"..")==0) {
						if (Next<2)
							return false;
						String::size_type Prev=Path.rfind(L'\\',Next-2);
						if (Prev==String::npos)
							return false;
						Path.erase(Prev,Pos-Prev);
						Next=Prev+1;
					} else {
						Next=Pos+1;
					}
				} else {
					Next=Pos+1;
				}
			} while (Next<Path.length());

			if (Path.length()==2 && Path[1]==L':')
				Path.push_back(L'\\');

			*pAbsolutePath=Path;

			return true;
		}

		bool IsExists(const String &Path)
		{
			String Name(Path);
			RemoveDelimiter(&Name);

			WIN32_FIND_DATA fd;
			HANDLE hFind=::FindFirstFile(Name.c_str(),&fd);
			if (hFind==INVALID_HANDLE_VALUE)
				return false;
			::FindClose(hFind);

			return true;
		}

		bool IsFileExists(const String &Path)
		{
			WIN32_FIND_DATA fd;
			HANDLE hFind=::FindFirstFile(Path.c_str(),&fd);
			if (hFind==INVALID_HANDLE_VALUE)
				return false;
			::FindClose(hFind);

			if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)
				return false;

			return true;
		}

		bool IsRoot(const String &Path)
		{
			if (Path.length()>=MAX_PATH)
				return false;

			if (Path.length()>2 && Path[0]==L'\\' && Path[1]==L'\\') {
				String Directory(Path);
				RemoveDelimiter(&Directory);
				return ::PathIsRoot(Directory.c_str())!=FALSE;
			}

			return ::PathIsRoot(Path.c_str())!=FALSE;
		}

		bool CorrectFileName(String *pPath)
		{
			if (pPath==nullptr)
				return false;

			auto i=pPath->begin();

			String::size_type Pos=pPath->find_last_of(L'\\');
			if (Pos!=String::npos)
				i+=Pos+1;

			for (;i!=pPath->end();i++) {
				static const struct {
					WCHAR From;
					WCHAR To;
				} CharMap[] = {
					{L'\\',	L'Åè'},
					{L'/',	L'Å^'},
					{L':',	L'ÅF'},
					{L'*',	L'Åñ'},
					{L'?',	L'ÅH'},
					{L'"',	L'Åh'},
					{L'<',	L'ÅÉ'},
					{L'>',	L'ÅÑ'},
					{L'|',	L'Åb'},
				};

				for (int j=0;j<_countof(CharMap);j++) {
					if (CharMap[j].From==*i) {
						*i=CharMap[j].To;
						break;
					}
				}
			}

			return true;
		}

	}


	bool CLocalLock::TryLock(DWORD Timeout)
	{
		if (::TryEnterCriticalSection(&m_CriticalSection))
			return true;
		if (Timeout>0) {
			const DWORD StartTime=::GetTickCount();
			do {
				::Sleep(1);
				if (::TryEnterCriticalSection(&m_CriticalSection))
					return true;
			} while (::GetTickCount()-StartTime<Timeout);
		}
		return false;
	}


	CHandleBase::CHandleBase()
		: m_hHandle(nullptr)
	{
	}

	CHandleBase::~CHandleBase()
	{
		Close();
	}

	void CHandleBase::Close()
	{
		if (m_hHandle!=nullptr) {
			::CloseHandle(m_hHandle);
			m_hHandle=nullptr;
		}
	}

	DWORD CHandleBase::Wait(DWORD Timeout)
	{
		if (m_hHandle==nullptr)
			return WAIT_FAILED;

		return ::WaitForSingleObject(m_hHandle,Timeout);
	}


	CEvent::CEvent()
	{
	}

	CEvent::~CEvent()
	{
	}

	bool CEvent::Create(LPCTSTR pszName,bool fManualReset,bool fInitialState)
	{
		Close();

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		m_hHandle=::CreateEvent(&SecurityAttributes,fManualReset,fInitialState,pszName);

		return m_hHandle!=nullptr;
	}

	bool CEvent::Set()
	{
		if (m_hHandle==nullptr)
			return false;
		return ::SetEvent(m_hHandle)!=FALSE;
	}

	bool CEvent::Reset()
	{
		if (m_hHandle==nullptr)
			return false;
		return ::ResetEvent(m_hHandle)!=FALSE;
	}


	CMutex::CMutex()
		: m_fAlreadyExists(false)
	{
	}

	CMutex::~CMutex()
	{
	}

	bool CMutex::Create(LPCTSTR pszName,bool fInitialOwner)
	{
		Close();

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		m_hHandle=::CreateMutex(&SecurityAttributes,fInitialOwner,pszName);

		if (m_hHandle==nullptr)
			return false;

		m_fAlreadyExists=::GetLastError()==ERROR_ALREADY_EXISTS;

		return true;
	}

	bool CMutex::Open(LPCTSTR pszName,DWORD DesiredAccess,bool fInheritHandle)
	{
		Close();

		m_hHandle=::OpenMutex(DesiredAccess,fInheritHandle,pszName);

		return m_hHandle!=nullptr;
	}

	bool CMutex::Release()
	{
		if (m_hHandle==nullptr)
			return false;
		return ::ReleaseMutex(m_hHandle)!=FALSE;
	}

	bool CMutex::IsAlreadyExists() const
	{
		return m_fAlreadyExists;
	}


	CThread::CThread()
		: m_pEntryFunctor(nullptr)
	{
	}

	CThread::~CThread()
	{
		Terminate();
		delete m_pEntryFunctor;
	}

	bool CThread::Begin(CFunctorBase *pFunctor)
	{
		if (pFunctor==nullptr)
			return false;
		if (m_hHandle!=nullptr)
			return false;

		delete m_pEntryFunctor;
		m_pEntryFunctor=pFunctor;
		m_hHandle=reinterpret_cast<HANDLE>(::_beginthreadex(nullptr,0,ThreadProc,this,0,nullptr));
		if (m_hHandle==nullptr) {
			m_pEntryFunctor=nullptr;
			return false;
		}

		return true;
	}

	bool CThread::Terminate(DWORD ExitCode)
	{
		if (m_hHandle!=nullptr) {
			if (::WaitForSingleObject(m_hHandle,0)==WAIT_TIMEOUT)
				::TerminateThread(m_hHandle,ExitCode);
		}
		return true;
	}

	unsigned int _stdcall CThread::ThreadProc(void *pParameter)
	{
		CThread *pThis=static_cast<CThread*>(pParameter);

		unsigned int Result=(*pThis->m_pEntryFunctor)();

		return Result;
	}


	CBasicSecurityAttributes::CBasicSecurityAttributes()
	{
		nLength=sizeof(SECURITY_ATTRIBUTES);
		lpSecurityDescriptor=nullptr;
		bInheritHandle=FALSE;
	}

	bool CBasicSecurityAttributes::Initialize()
	{
		::InitializeSecurityDescriptor(&m_SecurityDescriptor,SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&m_SecurityDescriptor,TRUE,nullptr,FALSE);
		lpSecurityDescriptor=&m_SecurityDescriptor;
		return true;
	}


	void ResetStreamStatistics(StreamStatistics *pStatistics)
	{
		::ZeroMemory(pStatistics,sizeof(StreamStatistics));
		pStatistics->SignalLevel=0.0f;
	}

}
