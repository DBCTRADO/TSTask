#ifndef TSTASK_STREAM_QUEUE
#define TSTASK_STREAM_QUEUE


#include <deque>


namespace TSTask
{

	class CStreamQueue
	{
	public:
		CStreamQueue();
		CStreamQueue(size_t BlockSize, size_t MaxBlocks);
		~CStreamQueue();
		bool Initialize(size_t BlockSize, size_t MaxBlocks);
		void Clear();
		bool AddData(const void *pData, size_t DataSize);
		bool GetData(void *pData, size_t *pDataSize);

	private:
		struct BlockInfo
		{
			BYTE *pData;
			size_t Size;
			size_t Offset;
		};

		size_t m_BlockSize;
		size_t m_MaxBlocks;
		std::deque<BlockInfo> m_Queue;
		CLocalLock m_Lock;
	};

}


#endif
