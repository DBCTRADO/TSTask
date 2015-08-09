#ifndef TSTASK_TVROCK_H
#define TSTASK_TVROCK_H


#include "TSTaskCore.h"
#include "TSTaskAppCore.h"
#include "TvRockDTV.h"


namespace TSTask
{

	class CTvRockDTVTarget : public CEventHandler, public CTSTaskCore::ITsGrabber
	{
	public:
		static const size_t QUEUE_BLOCK_SIZE=(1024*1024)/188*188;
		static const size_t MAX_QUEUE_BLOCKS_DEFAULT=32;
		static const size_t MAX_QUEUE_BLOCKS_MIN=2;
		static const size_t MAX_QUEUE_BLOCKS_MAX=256;
		static const size_t DEFAULT_SEND_BUFFER_LENGTH=1024;

		CTvRockDTVTarget(CTSTaskAppCore &AppCore);
		~CTvRockDTVTarget();
		bool Initialize(int DeviceID,CSettings *pSettings);
		void Finalize();
		int GetDeviceID() const { return m_DeviceID; }
		int GetFrequencyByTSID(WORD TransportStreamID);

	private:
		bool InitializeTvRockDTV(int DeviceID,CSettings *pSettings);
		void PurgeStream();
		unsigned int StreamThread();
		void NotifyStreamChange();

	// CEventHandler
		void OnTunerOpened() override;
		void OnChannelChanged(DWORD Space,DWORD Channel,WORD ServiceID) override;
		void OnStreamChanged() override;
		void OnServiceChanged(WORD ServiceID) override;

	// CTSTaskCore::ITsGrabber
		bool OnPacket(const ::CTsPacket *pPacket) override;
		void OnReset() override;

		CTSTaskAppCore &m_AppCore;
		CTSTaskCore &m_Core;
		HMODULE m_hDTVModule;
		TvRock::TvRockDTV *m_pTvRock;
		class CTvRockRequest *m_pRequest;
		class CChannelSearch *m_pChannel;
		int m_DeviceID;
		CEvent m_ProcessTSEvent;
		CStreamQueue m_StreamQueue;
		size_t m_StreamQueueMaxBlocks;
		size_t m_StreamSendBufferLength;
		CThread m_StreamThread;
		CEvent m_StreamEndEvent;
	};

}

#endif
