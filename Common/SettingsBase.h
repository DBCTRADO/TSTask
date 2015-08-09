#ifndef TSTASK_SETTINGS_BASE_H
#define TSTASK_SETTINGS_BASE_H


#include "Settings.h"


namespace TSTask
{

	class CSettingsBase
	{
	public:
		CSettingsBase(CRWLock *pLock);
		virtual ~CSettingsBase();
		virtual void SetDefault() = 0;
		virtual bool Load(CSettings &Settings) = 0;
		virtual bool Save(CSettings &Settings) const = 0;
		void ReadLock();
		void ReadUnlock();
		void WriteLock();
		void WriteUnlock();

	protected:
		CRWLock *m_pLock;
	};

}


#endif
