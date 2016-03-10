#ifndef TSTASK_STREAM_POOL_H
#define TSTASK_STREAM_POOL_H


#include "SharedMemory.h"
#include "DataBlock.h"


namespace TSTask
{

	class CStreamPool
	{
	public:
		struct StreamInfo
		{
			DWORD Status;
			DWORD BufferLength;
			DWORD FirstPos;
			DWORD StreamLength;
			ULONGLONG PacketCount;
		};

		static const DWORD STATUS_ENDED=0x00000001;

		class CStreamPosition
		{
		public:
			ULONGLONG Pos = 0;

			static const ULONGLONG POS_TAIL=0xFFFFFFFFFFFFFFFFULL;

			void SetHead() { Pos=0; }
			void SetTail() { Pos=POS_TAIL; }
			bool IsTail() const { return Pos==POS_TAIL; }
		};

		CStreamPool();
		~CStreamPool();
		bool Create(LPCWSTR pszName,DWORD BufferLength);
		bool Open(LPCWSTR pszName);
		void Close();
		bool IsOpened() const;
		bool End();
		bool IsEnded();
		bool Clear();
		bool Write(const void *pBuffer,DWORD BufferLength);
		bool Read(void *pBuffer,DWORD *pBufferLength,CStreamPosition *pPosition);
		bool Read(CDataBlock *pData,CStreamPosition *pPosition);
		DWORD GetStreamLength();

	private:
		CSharedMemory m_SharedMemory;
		BYTE *m_pBuffer;
		DWORD m_Timeout;
	};

}


#endif
