#ifndef TSTASK_DEBUG_H
#define TSTASK_DEBUG_H


namespace TSTask
{

	namespace Debug
	{

		void Initialize();
		void Trace(LPCTSTR pszText, ...);

	}

}


#ifdef _DEBUG
#define TSTASK_DEBUG_INITIALIZE TSTask::Debug::Initialize()
#define TRACE TSTask::Debug::Trace
#else
#define TSTASK_DEBUG_INITIALIZE __noop
#define TRACE __noop
#endif


#endif
