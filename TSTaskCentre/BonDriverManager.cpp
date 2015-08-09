#include "stdafx.h"
#include "TSTaskCentre.h"
#include "BonDriverManager.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CBonDriverManager::CBonDriverManager()
	{
	}

	CBonDriverManager::~CBonDriverManager()
	{
	}

	bool CBonDriverManager::GetBonDriverFileList(LPCWSTR pszDirectory,BonDriverFileList *pList)
	{
		if (TSTask::IsStringEmpty(pszDirectory) || pList==nullptr)
			return false;

		pList->clear();

		TSTask::String FileMask;

		FileMask=pszDirectory;
		TSTask::PathUtility::Append(&FileMask,L"BonDriver_*.dll");

		WIN32_FIND_DATA fd;
		HANDLE hFind=::FindFirstFile(FileMask.c_str(),&fd);
		if (hFind==INVALID_HANDLE_VALUE)
			return true;

		do {
			if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0
					&& ::CompareStringOrdinal(fd.cFileName,-1,L"BonDriver_TSTask.dll",-1,TRUE)!=CSTR_EQUAL) {
				TSTask::String File;

				File=pszDirectory;
				TSTask::PathUtility::Append(&File,fd.cFileName);
				pList->push_back(File);
			}
		} while (::FindNextFile(hFind,&fd));

		::FindClose(hFind);

		return true;
	}

}
