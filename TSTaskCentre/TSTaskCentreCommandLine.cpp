#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskCentreCommandLine.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTSTaskCentreCommandLine::CTSTaskCentreCommandLine()
		: m_Minimize(TSTask::BOOL_DEFAULT)

		, m_LoggingLevel(-1)
		, m_LogOutputToFile(TSTask::BOOL_DEFAULT)
		, m_LogOverwrite(TSTask::BOOL_DEFAULT)
		, m_DebugLog(TSTask::BOOL_DEFAULT)
	{
	}

	bool CTSTaskCentreCommandLine::Parse(LPCWSTR pszCommandLine)
	{
		TSTask::CCommandLineParser Parser;

		if (!Parser.Parse(pszCommandLine))
			return false;

		Parser.GetOption(L"min",&m_Minimize);

		Parser.GetOption(L"loglevel",&m_LoggingLevel);
		Parser.GetOption(L"log",&m_LogOutputToFile);
		Parser.GetOption(L"logfile",&m_LogFileName);
		Parser.GetOption(L"logoverwrite",&m_LogOverwrite);
		Parser.GetOption(L"debuglog",&m_DebugLog);

		return true;
	}

}
