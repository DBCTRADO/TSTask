#include "stdafx.h"
#include "TSTaskBase.h"
#include "Module.h"
#include "DebugDef.h"


namespace TSTask
{

	CModule::CModule()
		: m_hModule(nullptr)
	{
	}

	CModule::~CModule()
	{
		Unload();
	}

	bool CModule::Load(LPCWSTR pszFileName)
	{
		if (IsStringEmpty(pszFileName))
			return false;

		if (m_hModule!=nullptr)
			return false;

		OutLog(LOG_VERBOSE,L"モジュール(%s)をロードします。",pszFileName);

		m_hModule=::LoadLibrary(pszFileName);
		if (m_hModule==nullptr) {
			OutSystemErrorLog(::GetLastError(),L"モジュール(%s)をロードできません。",
							  pszFileName);
			return false;
		}

		OutLog(LOG_VERBOSE,L"モジュール(%s)をロードしました。(%p)",pszFileName,m_hModule);

		return true;
	}

	void CModule::Unload()
	{
		if (m_hModule!=nullptr) {
			OutLog(LOG_VERBOSE,L"モジュール(%p)を解放します。",m_hModule);
			::FreeLibrary(m_hModule);
			OutLog(LOG_VERBOSE,L"モジュール(%p)を解放しました。",m_hModule);
			m_hModule=nullptr;
		}
	}

	bool CModule::GetFilePath(String *pFilePath) const
	{
		if (pFilePath==nullptr)
			return false;

		pFilePath->clear();

		if (m_hModule==nullptr)
			return false;

		return GetModuleFilePath(m_hModule,pFilePath);
	}

	void *CModule::GetFunction(const char *pName) const
	{
		if (m_hModule==nullptr)
			return nullptr;

		return ::GetProcAddress(m_hModule,pName);
	}

}
