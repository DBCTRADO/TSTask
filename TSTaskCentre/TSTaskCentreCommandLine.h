#ifndef TSTASKCENTRE_COMMAND_LINE_H
#define TSTASKCENTRE_COMMAND_LINE_H


namespace TSTaskCentre
{

	class CTSTaskCentreCommandLine
	{
	public:
		CTSTaskCentreCommandLine();
		bool Parse(LPCWSTR pszCommandLine);

		TSTask::BoolSettingValue m_Minimize;

		int m_LoggingLevel;
		TSTask::BoolSettingValue m_LogOutputToFile;
		TSTask::String m_LogFileName;
		TSTask::BoolSettingValue m_LogOverwrite;
		TSTask::BoolSettingValue m_DebugLog;
	};

}


#endif
