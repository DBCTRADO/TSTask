#ifndef TSTASK_UTILITY_H
#define TSTASK_UTILITY_H


namespace TSTask
{

	template<typename T> inline void SafeDelete(T *&pPointer)
	{
		if (pPointer!=nullptr) {
			delete pPointer;
			pPointer=nullptr;
		}
	}

	template<typename T> inline void SafeDeleteArray(T *&pPointer)
	{
		if (pPointer!=nullptr) {
			delete [] pPointer;
			pPointer=nullptr;
		}
	}

	template<typename T> inline void SafeRelease(T *&pPointer)
	{
		if (pPointer!=nullptr) {
			pPointer->Release();
			pPointer=nullptr;
		}
	}

	inline bool IsStringEmpty(LPCWSTR pszString)
	{
		return pszString==nullptr || pszString[0]==L'\0';
	}

	inline bool IsStringEmpty(LPCSTR pszString)
	{
		return pszString==nullptr || pszString[0]=='\0';
	}

	int FormatString(LPWSTR pszBuffer,size_t BufferLength,LPCWSTR pszFormat, ...);
	int FormatStringV(LPWSTR pszBuffer,size_t BufferLength,LPCWSTR pszFormat,va_list Args);
	int FormatString(char *pszBuffer,size_t BufferLength,const char *pszFormat, ...);
	int FormatStringV(char *pszBuffer,size_t BufferLength,const char *pszFormat,va_list Args);

	bool IsMatchWildcard(LPCWSTR pszString,LPCWSTR pszWildcard);

	WCHAR ToUpper(WCHAR Char);
	WCHAR ToLower(WCHAR Char);

	bool GetSystemErrorMessage(DWORD Error,String *pMessage);

	bool GetModuleFilePath(HMODULE hModule,String *pFilePath);
	bool GetModuleDirectory(HMODULE hModule,String *pDirectory);

	void *GetModuleFunction(LPCWSTR pszModule,LPCSTR pszFuction);
#define TSTASK_GET_MODULE_FUNC(hModule,Func) \
	reinterpret_cast<decltype(Func)*>(::GetProcAddress(hModule,#Func))
#define TSTASK_GET_DLL_FUNC(pszDLLName,Func) \
	TSTASK_GET_MODULE_FUNC(::GetModuleHandle(pszDLLName),Func)

	extern __declspec(selectany) const FILETIME FILETIME_NULL={0,0};
	extern __declspec(selectany) const LONGLONG FILETIME_MILLISECOND	=10000LL;
	extern __declspec(selectany) const LONGLONG FILETIME_SECOND			=1000LL*FILETIME_MILLISECOND;
	extern __declspec(selectany) const LONGLONG FILETIME_MINUTE			=60LL*FILETIME_SECOND;
	extern __declspec(selectany) const LONGLONG FILETIME_HOUR			=60LL*FILETIME_MINUTE;

	bool OffsetFileTime(FILETIME *pTime,LONGLONG Offset);
	bool OffsetSystemTime(SYSTEMTIME *pTime,LONGLONG Offset);
	LONGLONG FileTimeSpan(const FILETIME &Time1,const FILETIME &Time2);
	LONGLONG SystemTimeSpan(const SYSTEMTIME &Time1,const SYSTEMTIME &Time2);
	bool GetDayOfWeekText(int DayOfWeek,String *pText,bool fShort=true,LPCWSTR pszLocale=nullptr);


	namespace StringUtility
	{

		void Reserve(String &Str,size_t Size);
		int Format(String &Str,LPCWSTR pszFormat, ...);
		int FormatV(String &Str,LPCWSTR pszFormat,va_list Args);
		int CompareNoCase(const String &String1,const String &String2);
		int CompareNoCase(const String &String1,LPCWSTR pszString2);
		int CompareNoCase(const String &String1,const String &String2,String::size_type Length);
		int CompareNoCase(const String &String1,LPCWSTR pszString2,String::size_type Length);
		bool Trim(String &Str,LPCWSTR pszSpaces=L" \t");
		bool Replace(String &Str,LPCWSTR pszFrom,LPCWSTR pszTo);
		bool Replace(String &Str,String::value_type From,String::value_type To);
		bool ToAnsi(const String &Src,AnsiString *pDst);
		bool Split(const String &Src,LPCWSTR pszDelimiter,std::vector<String> *pList);
		bool Combine(const std::vector<String> &List,LPCWSTR pszDelimiter,String *pDst);
		void ToUpper(String &Str);
		void ToLower(String &Str);

	}

	namespace PathUtility
	{

		bool RemoveExtension(String *pPath);
		bool RenameExtension(String *pPath,LPCWSTR pszExtension);
		bool GetExtension(const String &Path,String *pExtension);
		bool RemoveFileName(String *pPath);
		bool Append(String *pPath,LPCWSTR pszMore);
		inline bool Append(String *pPath,const String &More) { return Append(pPath,More.c_str()); }
		bool GetFileName(const String &Path,String *pFileName);
		bool Split(const String &Path,String *pDirectory,String *pFileName);
		bool AppendDelimiter(String *pPath);
		bool RemoveDelimiter(String *pPath);
		bool IsAbsolute(const String &Path);
		inline bool IsRelative(const String &Path) { return !IsAbsolute(Path); }
		bool RelativeToAbsolute(String *pAbsolutePath,const String &BasePath,const String &RelativePath);
		bool IsExists(const String &Path);
		bool IsFileExists(const String &Path);
		bool IsRoot(const String &Path);
		bool CorrectFileName(String *pPath);

	}

	namespace ListUtility
	{

		template<typename Iterator,typename Predicator> bool Enum(Iterator begin,Iterator end,Predicator pr)
		{
			for (Iterator i=begin;i!=end;i++) {
				if (!pr(*i))
					return false;
			}
			return true;
		}

		template<typename List> void DeleteAll(List &list)
		{
			for (List::iterator i=list.begin();i!=list.end();i++)
				delete *i;
			list.clear();
		}

	}

	class TSTASK_ABSTRACT_CLASS(CReferenceObject)
	{
	public:
		CReferenceObject()
			: m_ReferenceCount(1)
		{
		}

		void Refer()
		{
			::InterlockedIncrement(&m_ReferenceCount);
		}

		void Release()
		{
			if (::InterlockedDecrement(&m_ReferenceCount)==0)
				delete this;
		}

	protected:
		virtual ~CReferenceObject() {}

	private:
		LONG m_ReferenceCount;

		CReferenceObject(const CReferenceObject &) = delete;
		CReferenceObject &operator=(const CReferenceObject &) = delete;
	};

	class CLocalLock
	{
	public:
		CLocalLock()
		{
			::InitializeCriticalSection(&m_CriticalSection);
		}

		~CLocalLock()
		{
			::DeleteCriticalSection(&m_CriticalSection);
		}

		void Lock()
		{
			::EnterCriticalSection(&m_CriticalSection);
		}

		void Unlock()
		{
			::LeaveCriticalSection(&m_CriticalSection);
		}

		bool TryLock()
		{
			return ::TryEnterCriticalSection(&m_CriticalSection)!=FALSE;
		}

		bool TryLock(DWORD Timeout);

	private:
		CRITICAL_SECTION m_CriticalSection;

		CLocalLock(const CLocalLock &) = delete;
		CLocalLock &operator=(const CLocalLock &) = delete;
	};

	class CBlockLock
	{
	public:
		CBlockLock(CLocalLock &Lock)
			: m_Lock(Lock)
		{
			m_Lock.Lock();
		}

		~CBlockLock()
		{
			m_Lock.Unlock();
		}

	private:
		CLocalLock &m_Lock;

		CBlockLock(const CBlockLock &) = delete;
		CBlockLock &operator=(const CBlockLock &) = delete;
	};

	class CTryBlockLock
	{
	public:
		CTryBlockLock(CLocalLock &Lock)
			: m_Lock(Lock)
			, m_fLocked(false)
		{
		}

		~CTryBlockLock()
		{
			if (m_fLocked)
				m_Lock.Unlock();
		}

		bool TryLock(DWORD Timeout)
		{
			if (!m_Lock.TryLock(Timeout))
				return false;
			m_fLocked=true;
			return true;
		}

	private:
		CLocalLock &m_Lock;
		bool m_fLocked;

		CTryBlockLock(const CTryBlockLock &) = delete;
		CTryBlockLock &operator=(const CTryBlockLock &) = delete;
	};

	class CRWLock
	{
	public:
		CRWLock()
		{
			::InitializeSRWLock(&m_SRWLock);
		}

		void ReadLock()
		{
			::AcquireSRWLockShared(&m_SRWLock);
		}

		void ReadUnlock()
		{
			::ReleaseSRWLockShared(&m_SRWLock);
		}

		void WriteLock()
		{
			::AcquireSRWLockExclusive(&m_SRWLock);
		}

		void WriteUnlock()
		{
			::ReleaseSRWLockExclusive(&m_SRWLock);
		}

	private:
		SRWLOCK m_SRWLock;

		CRWLock(const CRWLock &) = delete;
		CRWLock &operator=(const CRWLock &) = delete;
	};

	class CRWLockRead
	{
	public:
		CRWLockRead(CRWLock &Lock)
			: m_Lock(Lock)
		{
			m_Lock.ReadLock();
		}

		~CRWLockRead()
		{
			m_Lock.ReadUnlock();
		}

	private:
		CRWLock &m_Lock;

		CRWLockRead(const CRWLockRead &) = delete;
		CRWLockRead &operator=(const CRWLockRead &) = delete;
	};

	class CRWLockWrite
	{
	public:
		CRWLockWrite(CRWLock &Lock)
			: m_Lock(Lock)
		{
			m_Lock.WriteLock();
		}

		~CRWLockWrite()
		{
			m_Lock.WriteUnlock();
		}

	private:
		CRWLock &m_Lock;

		CRWLockWrite(const CRWLockWrite &) = delete;
		CRWLockWrite &operator=(const CRWLockWrite &) = delete;
	};

	class CHandleBase abstract
	{
	public:
		CHandleBase();
		virtual ~CHandleBase();
		void Close();
		bool IsOpened() const { return m_hHandle!=nullptr; }
		HANDLE GetHandle() const { return m_hHandle; }
		DWORD Wait(DWORD Timeout=INFINITE);

	protected:
		HANDLE m_hHandle;
	};

	class CEvent : public CHandleBase
	{
	public:
		CEvent();
		~CEvent();
		bool Create(LPCTSTR pszName=nullptr,bool fManualReset=false,bool fInitialState=false);
		bool Set();
		bool Reset();
	};

	class CMutex : public CHandleBase
	{
	public:
		CMutex();
		~CMutex();
		bool Create(LPCTSTR pszName=nullptr,bool fInitialOwner=false);
		bool Open(LPCTSTR pszName,
				  DWORD DesiredAccess = MUTEX_ALL_ACCESS,
				  bool fInheritHandle = false);
		bool Release();
		bool IsAlreadyExists() const;

	private:
		bool m_fAlreadyExists;

		CMutex(const CMutex &) = delete;
		CMutex &operator=(const CMutex &) = delete;
	};

	class CThread : public CHandleBase
	{
	public:
		class CFunctorBase
		{
		public:
			virtual ~CFunctorBase() {}
			virtual unsigned int operator()() = 0;
		};

		template<typename TClass> class CFunctor : public CFunctorBase
		{
		public:
			typedef unsigned int (TClass::*EntryFunc)();

			CFunctor(TClass *pObject,EntryFunc pEntry)
				: m_pObject(pObject)
				, m_pEntry(pEntry)
			{
			}

			unsigned int operator()() override
			{
				return (m_pObject->*m_pEntry)();
			}

		private:
			TClass *m_pObject;
			EntryFunc m_pEntry;
		};

		CThread();
		~CThread();
		bool Begin(CFunctorBase *pFunctor);
		bool Terminate(DWORD ExitCode=(DWORD)-1);

	private:
		static unsigned int _stdcall ThreadProc(void *pParameter);

		CFunctorBase *m_pEntryFunctor;
	};

	class CBasicSecurityAttributes : public SECURITY_ATTRIBUTES
	{
	public:
		CBasicSecurityAttributes();
		bool Initialize();

	private:
		SECURITY_DESCRIPTOR m_SecurityDescriptor;
	};

	void ResetStreamStatistics(StreamStatistics *pStatistics);

}


#endif
