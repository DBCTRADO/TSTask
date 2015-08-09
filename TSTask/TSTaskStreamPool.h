#ifndef TSTASK_SERVER_STREAM_POOL_H
#define TSTASK_SERVER_STREAM_POOL_H


#include "DtvEngine.h"


namespace TSTask
{

	class CTSTaskStreamPool : public ::CTsGrabber::ITsHandler
	{
	public:
		CTSTaskStreamPool();
		~CTSTaskStreamPool();
		bool Create(LPCWSTR pszName,DWORD BufferLength);
		void Close();
		bool IsCreated() const;
		bool End();
		bool Clear();

	private:
	// ::CTsGrabber::ITsHandler
		bool OnPacket(const ::CTsPacket *pPacket) override;
		void OnReset() override;

		CStreamPool m_StreamPool;
		CLocalLock m_Lock;
		CDataBlock m_Buffer;
		size_t m_BufferLength;
	};

}


#endif
