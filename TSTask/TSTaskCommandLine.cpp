#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskCommandLine.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CTSTaskCommandLine::CTSTaskCommandLine()
		: m_TaskID(INVALID_TASK_ID)
		, m_ProcessPriority(-100)

		, m_ClientShowCommand(-1)

		, m_TuningSpace(-1)
		, m_Channel(-1)
		, m_RemoteControlKeyID(0)
		, m_NetworkID(0)
		, m_TransportStreamID(0)
		, m_ServiceID(0)

		, m_Record(BOOL_DEFAULT)
		, m_RecordingStartTime(FILETIME_NULL)
		, m_RecordingDelay(0)
		, m_RecordingDuration(0)
		, m_RecordingService(-1)
		, m_Record1Seg(BOOL_DEFAULT)
		, m_RecordingPreAllocateSize(-1)
		, m_ExitOnRecordingStop(BOOL_DEFAULT)

		, m_TvRockDeviceID(-1)
		, m_ExecuteClient(BOOL_DEFAULT)

		, m_LoggingLevel(-1)
		, m_LogOutputToFile(BOOL_DEFAULT)
		, m_LogOverwrite(BOOL_DEFAULT)
		, m_DebugLog(BOOL_DEFAULT)

		, m_StreamingProtocol(PROTOCOL_UNDEFINED)
		, m_StreamingUDP(BOOL_DEFAULT)
		, m_StreamingTCP(BOOL_DEFAULT)
		, m_StreamingPort(-1)
		, m_StreamingService(-1)
		, m_Streaming1Seg(BOOL_DEFAULT)

		, m_FirstChannelSetDelay(-1)
		, m_MinChannelChangeInterval(-1)
	{
	}

	bool CTSTaskCommandLine::Parse(LPCWSTR pszCommandLine)
	{
		CCommandLineParser Parser;

		if (!Parser.Parse(pszCommandLine))
			return false;

		Parser.GetOption(L"ini",&m_IniFileName);
		Parser.GetOption(L"d",&m_BonDriverFileName);
		int ID;
		if (Parser.GetOption(L"taskid",&ID))
			m_TaskID=TaskID(ID);
		Parser.GetOption(L"priority",&m_ProcessPriority);

		if (Parser.HasOption(L"min"))
			m_ClientShowCommand=SW_SHOWMINNOACTIVE;
		else
			Parser.GetOption(L"show",&m_ClientShowCommand);

		Parser.GetOption(L"chspace",&m_TuningSpace);
		Parser.GetOption(L"ch",&m_Channel);
		Parser.GetOption(L"rch",&m_RemoteControlKeyID);
		Parser.GetOption(L"nid",&m_NetworkID);
		Parser.GetOption(L"tsid",&m_TransportStreamID);
		Parser.GetOption(L"sid",&m_ServiceID);

		Parser.GetOption(L"rec",&m_Record);
		Parser.GetOption(L"recfile",&m_RecordingFileName);
		Parser.GetOptions(L"recdir",&m_RecordingDirectories);
		Parser.GetOption(L"recstarttime",&m_RecordingStartTime);
		Parser.GetOption(L"recdelay",&m_RecordingDelay);
		Parser.GetOptionDuration(L"recduration",&m_RecordingDuration);
		if (Parser.HasOption(L"reccurservice"))
			m_RecordingService=SERVICE_SELECT_CURRENT;
		else
			Parser.GetOption(L"recservice",&m_RecordingService);
		Parser.GetOption(L"rec1seg",&m_Record1Seg);
		Parser.GetOption(L"recallocsize",&m_RecordingPreAllocateSize);
		Parser.GetOption(L"recexit",&m_ExitOnRecordingStop);

		String DID;
		if (Parser.GetOption(L"did",&DID) && DID.length()>=1) {
			if (DID[0]>=L'A' && DID[0]<=L'Z')
				m_TvRockDeviceID=DID[0]-L'A';
			else if (DID[0]>=L'a' && DID[0]<=L'z')
				m_TvRockDeviceID=DID[0]-L'a';
		}

		Parser.GetOption(L"xclient",&m_ExecuteClient);

		Parser.GetOption(L"loglevel",&m_LoggingLevel);
		Parser.GetOption(L"log",&m_LogOutputToFile);
		Parser.GetOption(L"logfile",&m_LogFileName);
		Parser.GetOption(L"logoverwrite",&m_LogOverwrite);
		Parser.GetOption(L"debuglog",&m_DebugLog);

		String Protocol;
		if (Parser.GetOption(L"protocol",&Protocol))
			m_StreamingProtocol=Streaming::ParseProtocolText(Protocol.c_str());
		Parser.GetOption(L"udp",&m_StreamingUDP);
		Parser.GetOption(L"tcp",&m_StreamingTCP);
		Parser.GetOption(L"address",&m_StreamingAddress);
		Parser.GetOption(L"port",&m_StreamingPort);
		Parser.GetOption(L"sendservice",&m_StreamingService);
		Parser.GetOption(L"send1seg",&m_Streaming1Seg);

		Parser.GetOption(L"chsetdelay",&m_FirstChannelSetDelay);
		Parser.GetOption(L"chsetinterval",&m_MinChannelChangeInterval);

		return true;
	}

}
