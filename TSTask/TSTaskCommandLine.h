#ifndef TSTASK_SERVER_COMMAND_LINE_H
#define TSTASK_SERVER_COMMAND_LINE_H


#include <vector>


namespace TSTask
{

	class CTSTaskCommandLine
	{
	public:
		CTSTaskCommandLine();
		bool Parse(LPCWSTR pszCommandLine);

		String m_IniFileName;
		String m_BonDriverFileName;
		TaskID m_TaskID;
		int m_ProcessPriority;

		int m_ClientShowCommand;

		int m_TuningSpace;
		int m_Channel;
		int m_RemoteControlKeyID;
		int m_NetworkID;
		int m_TransportStreamID;
		int m_ServiceID;

		BoolSettingValue m_Record;
		String m_RecordingFileName;
		std::vector<String> m_RecordingDirectories;
		FILETIME m_RecordingStartTime;
		int m_RecordingDelay;
		int m_RecordingDuration;
		int m_RecordingService;
		BoolSettingValue m_Record1Seg;
		LONGLONG m_RecordingPreAllocateSize;
		BoolSettingValue m_ExitOnRecordingStop;

		int m_TvRockDeviceID;
		BoolSettingValue m_ExecuteClient;

		int m_LoggingLevel;
		BoolSettingValue m_LogOutputToFile;
		String m_LogFileName;
		BoolSettingValue m_LogOverwrite;
		BoolSettingValue m_DebugLog;

		NetworkProtocolType m_StreamingProtocol;
		BoolSettingValue m_StreamingUDP;
		BoolSettingValue m_StreamingTCP;
		String m_StreamingAddress;
		int m_StreamingPort;
		int m_StreamingService;
		BoolSettingValue m_Streaming1Seg;

		int m_FirstChannelSetDelay;
		int m_MinChannelChangeInterval;
	};

}


#endif
