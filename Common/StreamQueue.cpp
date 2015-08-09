#include "stdafx.h"
#include "TSTaskBase.h"
#include "StreamQueue.h"
#include "DebugDef.h"


namespace TSTask
{

	CStreamQueue::CStreamQueue()
		: m_BlockSize(0)
		, m_MaxBlocks(0)
	{
	}

	CStreamQueue::CStreamQueue(size_t BlockSize, size_t MaxBlocks)
		: m_BlockSize(BlockSize)
		, m_MaxBlocks(MaxBlocks)
	{
	}

	CStreamQueue::~CStreamQueue()
	{
	}

	bool CStreamQueue::Initialize(size_t BlockSize,size_t MaxBlocks)
	{
		if (BlockSize == 0 || MaxBlocks == 0)
			return false;

		CBlockLock Lock(m_Lock);

		Clear();

		m_BlockSize = BlockSize;
		m_MaxBlocks = MaxBlocks;

		return true;
	}

	void CStreamQueue::Clear()
	{
		CBlockLock Lock(m_Lock);

		while (!m_Queue.empty()) {
			delete [] m_Queue.front().pData;
			m_Queue.pop_front();
		}
	}

	bool CStreamQueue::AddData(const void *pData, size_t DataSize)
	{
		if (pData == nullptr || DataSize == 0)
			return false;

		CBlockLock Lock(m_Lock);

		if (m_BlockSize == 0 || m_MaxBlocks == 0)
			return false;

		size_t RemainSize = DataSize;
		const BYTE *p = static_cast<const BYTE*>(pData);

		if (!m_Queue.empty()) {
			BlockInfo &Last = m_Queue.back();
			if (Last.Size < m_BlockSize) {
				const size_t AvailSize = min(m_BlockSize - Last.Size, RemainSize);
				::CopyMemory(Last.pData + Last.Size, p, AvailSize);
				Last.Size += AvailSize;
				if (AvailSize == RemainSize)
					return true;
				RemainSize -= AvailSize;
				p += AvailSize;
			}
		}

		do {
			const size_t BlockSize = min(RemainSize, m_BlockSize);
			BlockInfo Block;

			if (m_Queue.size() < m_MaxBlocks) {
				try {
					Block.pData = new BYTE[m_BlockSize];
				} catch (...) {
					return false;
				}
			} else {
				Block = m_Queue.front();
				m_Queue.pop_front();
			}
			Block.Size = BlockSize;
			Block.Offset = 0;
			::CopyMemory(Block.pData, p, BlockSize);
			m_Queue.push_back(Block);

			RemainSize -= BlockSize;
			p += BlockSize;
		} while (RemainSize > 0);

		return true;
	}

	bool CStreamQueue::GetData(void *pData, size_t *pDataSize)
	{
		if (pData == nullptr || pDataSize == nullptr || *pDataSize == 0)
			return false;

		CBlockLock Lock(m_Lock);

		if (m_Queue.empty() || m_Queue.front().Size == 0) {
			*pDataSize = 0;
			return false;
		}

		size_t RemainSize = *pDataSize;
		BYTE *p = static_cast<BYTE*>(pData);

		do {
			BlockInfo &Block = m_Queue.front();
			const size_t AvailSize = Block.Size - Block.Offset;
			const size_t CopySize = min(RemainSize, AvailSize);
			::CopyMemory(p, Block.pData + Block.Offset, CopySize);
			RemainSize -= CopySize;
			if (CopySize < AvailSize) {
				Block.Offset += CopySize;
				break;
			} else {
				if (m_Queue.size() > 1) {
					delete [] Block.pData;
					m_Queue.pop_front();
				} else {
					Block.Size = 0;
					Block.Offset = 0;
					break;
				}
			}
			p += CopySize;
		} while (RemainSize > 0);

		*pDataSize -= RemainSize;

		return true;
	}

}
