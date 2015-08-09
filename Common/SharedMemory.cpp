#include "stdafx.h"
#include "TSTaskBase.h"
#include "SharedMemory.h"
#include "DebugDef.h"


namespace TSTask
{

	CSharedMemory::CSharedMemory()
		: m_hFileMapping(nullptr)
	{
	}

	CSharedMemory::~CSharedMemory()
	{
		Close();
	}

	bool CSharedMemory::Create(LPCWSTR pszName,size_t Size,DWORD Protect,bool *pfExists)
	{
		if (IsStringEmpty(pszName) || Size==0)
			return false;
		if (m_hFileMapping!=nullptr)
			return false;

		String MutexName;
		GetMutexName(pszName,&MutexName);
		if (!m_Mutex.Create(MutexName.c_str(),false)) {
			OutLog(LOG_ERROR,L"���L��������Mutex(%s)���쐬�ł��܂���B",MutexName.c_str());
			return false;
		}

		OutLog(LOG_VERBOSE,L"���L��������Mutex(%s)���쐬���܂����B",MutexName.c_str());

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		m_hFileMapping=::CreateFileMapping(
			INVALID_HANDLE_VALUE,
			&SecurityAttributes,
			Protect,
#ifdef _WIN64
			(DWORD)(Size>>32),(DWORD)(Size&0xFFFFFFFFULL),
#else
			0,Size,
#endif
			pszName);
		if (m_hFileMapping==nullptr) {
			OutSystemErrorLog(::GetLastError(),
							  L"�t�@�C���}�b�s���O�I�u�W�F�N�g(%s)���쐬�ł��܂���B",
							  pszName);
			m_Mutex.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"�t�@�C���}�b�s���O�I�u�W�F�N�g(%s)���쐬���܂����B(%Iu bytes)",
			   pszName,Size);

		if (pfExists!=nullptr)
			*pfExists=::GetLastError()==ERROR_ALREADY_EXISTS;

		return true;
	}

	bool CSharedMemory::Open(LPCWSTR pszName,DWORD DesiredAccess,bool fInheritHandle)
	{
		if (IsStringEmpty(pszName))
			return false;
		if (m_hFileMapping!=nullptr)
			return false;

		String MutexName;
		GetMutexName(pszName,&MutexName);
		if (!m_Mutex.Open(MutexName.c_str())) {
			OutLog(LOG_ERROR,L"���L��������Mutex(%s)���J���܂���B",MutexName.c_str());
			return false;
		}

		OutLog(LOG_VERBOSE,L"���L��������Mutex(%s)���J���܂����B",MutexName.c_str());

		CBasicSecurityAttributes SecurityAttributes;
		SecurityAttributes.Initialize();

		m_hFileMapping=::OpenFileMapping(DesiredAccess,fInheritHandle,pszName);
		if (m_hFileMapping==nullptr) {
			OutSystemErrorLog(::GetLastError(),
							  L"�t�@�C���}�b�s���O�I�u�W�F�N�g(%s)���J���܂���B",
							  pszName);
			m_Mutex.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"�t�@�C���}�b�s���O�I�u�W�F�N�g(%s)���J���܂����B(%p)",
			   pszName,m_hFileMapping);

		return true;
	}

	void CSharedMemory::Close()
	{
		if (m_hFileMapping!=nullptr) {
			::CloseHandle(m_hFileMapping);
			OutLog(LOG_VERBOSE,L"�t�@�C���}�b�s���O�I�u�W�F�N�g(%p)����܂����B",m_hFileMapping);
			m_hFileMapping=nullptr;
		}

		m_Mutex.Close();
	}

	bool CSharedMemory::IsOpened() const
	{
		return m_hFileMapping!=nullptr;
	}

	void *CSharedMemory::Map(DWORD DesiredAccess,size_t Offset,size_t Size)
	{
		if (m_hFileMapping==nullptr)
			return false;

		return ::MapViewOfFile(m_hFileMapping,DesiredAccess,
#ifdef _WIN64
							   (DWORD)(Offset>>32),(DWORD)(Offset&0xFFFFFFFFULL),
#else
							   0,Offset,
#endif
							   Size);
	}

	bool CSharedMemory::Unmap(void *pBuffer)
	{
		if (m_hFileMapping==nullptr)
			return false;

		return ::UnmapViewOfFile(pBuffer)!=FALSE;
	}

	bool CSharedMemory::Lock(DWORD Timeout)
	{
		return m_Mutex.Wait(Timeout)==WAIT_OBJECT_0;
	}

	bool CSharedMemory::Unlock()
	{
		return m_Mutex.Release();
	}

	void CSharedMemory::GetMutexName(LPCWSTR pszName,String *pMutexName) const
	{
		pMutexName->assign(pszName);
		pMutexName->append(L"_Mutex");
	}

}
