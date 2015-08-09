#ifndef TS_TASK_BASE_H
#define TS_TASK_BASE_H


#include <string>
#include <vector>
#include <algorithm>
#include "../BonTsEngine/Common.h"


#define TSTASK_ABSTRACT_CLASS(name) __declspec(novtable) name abstract

#define TSTASK_DEFINE_STRING(name,str) \
	extern const __declspec(selectany) LPCWSTR name=str


namespace TSTask
{

	enum
	{
		MAX_SERVER_TASKS = 50,
		MAX_CLIENT_TASKS = 50
	};

	typedef std::wstring String;
	typedef std::string AnsiString;

	enum
	{
		TS_PACKET_SIZE = 188
	};

	enum
	{
		NETWORK_ID_INVALID          = 0xFFFF,
		TRANSPORT_STREAM_ID_INVALID = 0xFFFF,
		SERVICE_ID_INVALID          = 0xFFFF
	};

	struct TuningChannelInfo
	{
		static const DWORD SPACE_INVALID   = 0xFFFFFFFF;
		static const DWORD CHANNEL_INVALID = 0xFFFFFFFF;

		DWORD Space = SPACE_INVALID;
		DWORD Channel = CHANNEL_INVALID;
		WORD ServiceID = SERVICE_ID_INVALID;
	};

	struct ScannedChannelInfo
	{
		int Space = -1;
		int Channel = -1;
		WORD ServiceID = SERVICE_ID_INVALID;
	};

	struct ChannelStreamIDs
	{
		WORD TransportStreamID;
		WORD NetworkID;
		WORD ServiceID;
	};

	struct ChannelInfo
	{
		int Space;
		int Channel;
		int ScannedChannel;
		int RemoteControlKeyID;
		String ChannelName;
		String SpaceName;
		WORD ServiceID;
		WORD ScannedServiceID;
		WORD NetworkID;
		WORD TransportStreamID;
	};

	struct ServiceInfo
	{
		String ServiceName;
		WORD ServiceID = SERVICE_ID_INVALID;
		BYTE ServiceType = SERVICE_TYPE_INVALID;
	};
	typedef std::vector<ServiceInfo> ServiceList;

	struct EventInfo
	{
		WORD EventID;
		SYSTEMTIME StartTime;
		DWORD Duration;
		String EventName;
		String EventText;
	};

	enum ServiceSelectType
	{
		SERVICE_SELECT_ALL,
		SERVICE_SELECT_CURRENT,
		SERVICE_SELECT_TRAILER
	};

	enum
	{
		STREAM_MPEG1_VIDEO			= 0x00000001UL,
		STREAM_MPEG2_VIDEO			= 0x00000002UL,
		STREAM_MPEG1_AUDIO			= 0x00000004UL,
		STREAM_MPEG2_AUDIO			= 0x00000008UL,
		STREAM_AAC					= 0x00000010UL,
		STREAM_MPEG4_VISUAL			= 0x00000020UL,
		STREAM_MPEG4_AUDIO			= 0x00000040UL,
		STREAM_H264					= 0x00000080UL,
		STREAM_H265					= 0x00000100UL,
		STREAM_AC3					= 0x00000200UL,
		STREAM_DTS					= 0x00000400UL,
		STREAM_TRUEHD				= 0x00000800UL,
		STREAM_DOLBY_DIGITAL_PLUS	= 0x00001000UL,
		STREAM_CAPTION				= 0x00002000UL,
		STREAM_DATA_CARROUSEL		= 0x00004000UL,
		STREAM_ALL					= 0x0000FFFFUL,
		STREAM_1SEG					= 0x00010000UL
	};

	struct RecordingSettings
	{
		String FileName;
		std::vector<String> Directories;
		ServiceSelectType ServiceSelect;
		DWORD Streams;
	};

	struct ReserveSettings
	{
		enum StartTimeType
		{
			START_TIME_NOT_SPECIFIED,
			START_TIME_DATE,
			START_TIME_DELAY,
			START_TIME_TRAILER
		};

		enum EndTimeType
		{
			END_TIME_NOT_SPECIFIED,
			END_TIME_DATE,
			END_TIME_DURATION,
			END_TIME_TRAILER
		};

		struct {
			FILETIME Time;
			ULONGLONG TickCount;
		} ReserveTime;

		struct {
			StartTimeType Type;
			FILETIME Time;
			ULONGLONG Delay;
			ULONGLONG TickCount;
		} StartTime;

		struct {
			EndTimeType Type;
			FILETIME Time;
			ULONGLONG Duration;
		} EndTime;

#if 0	// ‚»‚Ì‚¤‚¿...
		String BonDriverName;
		WORD NetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
		WORD EventID;
#endif
	};

	enum RecordingState
	{
		RECORDING_STATE_NOT_RECORDING,
		RECORDING_STATE_RECORDING
	};

	struct RecordingInfo
	{
		RecordingState State;
		RecordingSettings Settings;
		std::vector<String> FilePaths;
		FILETIME StartTime;
		ULONGLONG StartTickCount;
		int CurDirectory;
	};

	struct ReserveInfo
	{
		ReserveSettings Reserve;
		RecordingSettings Settings;
	};

	struct StreamStatistics
	{
		float SignalLevel;
		DWORD BitRate;
		ULONGLONG InputPacketCount;
		ULONGLONG ErrorPacketCount;
		ULONGLONG DiscontinuityCount;
		ULONGLONG ScramblePacketCount;
	};

	inline DWORD PackVersion(BYTE Major,WORD Minor,WORD Revision)
	{
		return ((DWORD)Major << 24) | ((DWORD)Minor << 12) | (DWORD)Revision;
	}
	inline BYTE GetVersionMajor(DWORD Version) { return (BYTE)(Version >> 24); }
	inline WORD GetVersionMinor(DWORD Version) { return (WORD)((Version >> 12) & 0xFFF); }
	inline WORD GetVersionRevision(DWORD Version) { return (WORD)(Version & 0xFFF); }

}


#include "Debug.h"
#include "Utility.h"
#include "Logger.h"


#endif
