#ifndef TSTASK_SHARED_MEMORY_H
#define TSTASK_SHARED_MEMORY_H


namespace TSTask
{

	class CSharedMemory
	{
	public:
		CSharedMemory();
		~CSharedMemory();
		bool Create(LPCWSTR pszName,size_t Size,DWORD Protect=PAGE_READWRITE,bool *pfExists=nullptr);
		bool Open(LPCWSTR pszName,DWORD DesiredAccess=FILE_MAP_ALL_ACCESS,bool fInheritHandle=false);
		void Close();
		bool IsOpened() const;
		void *Map(DWORD DesiredAccess=FILE_MAP_ALL_ACCESS,size_t Offset=0,size_t Size=0);
		bool Unmap(void *pBuffer);
		bool Lock(DWORD Timeout=INFINITE);
		bool Unlock();

	private:
		void GetMutexName(LPCWSTR pszName,String *pMutexName) const;

		CSharedMemory(const CSharedMemory&) = delete;
		CSharedMemory &operator=(const CSharedMemory&) = delete;

		HANDLE m_hFileMapping;
		CMutex m_Mutex;
	};

}


#endif
