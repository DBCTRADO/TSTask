#include "stdafx.h"
#include "TSTaskBase.h"
#include "SettingsBase.h"
#include "DebugDef.h"


namespace TSTask
{

	CSettingsBase::CSettingsBase(CRWLock *pLock)
		: m_pLock(pLock)
	{
	}

	CSettingsBase::~CSettingsBase()
	{
	}

	void CSettingsBase::ReadLock()
	{
		m_pLock->ReadLock();
	}

	void CSettingsBase::ReadUnlock()
	{
		m_pLock->ReadUnlock();
	}

	void CSettingsBase::WriteLock()
	{
		m_pLock->WriteLock();
	}

	void CSettingsBase::WriteUnlock()
	{
		m_pLock->WriteUnlock();
	}

}
