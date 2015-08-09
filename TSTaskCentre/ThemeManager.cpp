#include "stdafx.h"
#include "TSTaskCentre.h"
#include "ThemeManager.h"
#include "ThemeSettings.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CThemeManager::CThemeManager()
		: m_pApplier(nullptr)
	{
	}

	bool CThemeManager::GetThemesDirectory(TSTask::String *pDirectory)
	{
		if (pDirectory==nullptr)
			return false;

		pDirectory->clear();

		if (!TSTask::GetModuleDirectory(nullptr,pDirectory))
			return false;

		TSTask::PathUtility::Append(pDirectory,L"TSTaskThemes");

		return true;
	}

	bool CThemeManager::GetThemeFileList(std::vector<TSTask::String> *pList)
	{
		if (pList==nullptr)
			return false;

		pList->clear();

		TSTask::String Filter;
		if (!GetThemesDirectory(&Filter))
			return false;
		TSTask::PathUtility::Append(&Filter,L"*");
		Filter+=GetThemeExtension();

		WIN32_FIND_DATA fd;
		HANDLE hFind=::FindFirstFile(Filter.c_str(),&fd);
		if (hFind==INVALID_HANDLE_VALUE)
			return false;

		do {
			if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0)
				pList->push_back(TSTask::String(fd.cFileName));
		} while (::FindNextFile(hFind,&fd));

		::FindClose(hFind);

		return true;
	}

	bool CThemeManager::LoadTheme(LPCWSTR pszFileName,CThemeSettings *pTheme)
	{
		if (TSTask::IsStringEmpty(pszFileName) || pTheme==nullptr)
			return false;

		TSTask::String Path;

		if (!GetThemesDirectory(&Path))
			return false;

		TSTask::PathUtility::Append(&Path,pszFileName);
		if (!TSTask::PathUtility::IsFileExists(Path))
			return false;

		return pTheme->LoadFromFile(Path.c_str());
	}

	bool CThemeManager::ApplyTheme(const CThemeSettings &Theme)
	{
		if (m_pApplier==nullptr)
			return false;

		return m_pApplier->ApplyTheme(Theme);
	}

	bool CThemeManager::ApplyTheme(LPCWSTR pszFileName)
	{
		if (m_pApplier==nullptr)
			return false;

		CThemeSettings Theme;

		if (!TSTask::IsStringEmpty(pszFileName)) {
			if (!LoadTheme(pszFileName,&Theme))
				return false;
		}

		return ApplyTheme(Theme);
	}

	void CThemeManager::SetApplier(CApplier *pApplier)
	{
		m_pApplier=pApplier;
	}

}
