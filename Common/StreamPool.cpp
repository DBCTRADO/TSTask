#include "stdafx.h"
#include "TSTaskBase.h"
#include "StreamPool.h"
#include "DebugDef.h"


namespace TSTask
{

	CStreamPool::CStreamPool()
		: m_pBuffer(nullptr)
		, m_Timeout(3000)
	{
	}

	CStreamPool::~CStreamPool()
	{
		Close();
	}

	bool CStreamPool::Create(LPCWSTR pszName,DWORD BufferLength)
	{
		if (IsStringEmpty(pszName) || BufferLength==0)
			return false;

		bool fExists;
		if (!m_SharedMemory.Create(pszName,sizeof(StreamInfo)+BufferLength*TS_PACKET_SIZE,
								   PAGE_READWRITE,&fExists))
			return false;
		if (fExists) {
			m_SharedMemory.Close();
			return false;
		}

		m_pBuffer=static_cast<BYTE*>(m_SharedMemory.Map());
		if (m_pBuffer==nullptr) {
			m_SharedMemory.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"ストリームプールのメモリをマップしました。(Address %p)",m_pBuffer);

		StreamInfo *pInfo=reinterpret_cast<StreamInfo*>(m_pBuffer);
		::ZeroMemory(pInfo,sizeof(StreamInfo));
		pInfo->BufferLength=BufferLength;

		return true;
	}

	bool CStreamPool::Open(LPCWSTR pszName)
	{
		if (!m_SharedMemory.Open(pszName,FILE_MAP_READ))
			return false;

		m_pBuffer=static_cast<BYTE*>(m_SharedMemory.Map(FILE_MAP_READ));
		if (m_pBuffer==nullptr) {
			m_SharedMemory.Close();
			return false;
		}

		OutLog(LOG_VERBOSE,L"ストリームプールの読み込み用メモリをマップしました。(Address %p)",m_pBuffer);

		return true;
	}

	void CStreamPool::Close()
	{
		if (m_pBuffer!=nullptr) {
			m_SharedMemory.Unmap(m_pBuffer);
			m_pBuffer=nullptr;
		}
		m_SharedMemory.Close();
	}

	bool CStreamPool::IsOpened() const
	{
		return m_SharedMemory.IsOpened();
	}

	bool CStreamPool::End()
	{
		if (m_pBuffer==nullptr)
			return false;

		/*
		if (!m_SharedMemory.Lock(m_Timeout))
			return false;
		*/
		m_SharedMemory.Lock(m_Timeout);

		StreamInfo *pInfo=reinterpret_cast<StreamInfo*>(m_pBuffer);
		pInfo->Status|=STATUS_ENDED;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CStreamPool::IsEnded()
	{
		if (m_pBuffer==nullptr)
			return true;

		/*
		if (!m_SharedMemory.Lock(m_Timeout))
			return false;
		*/
		m_SharedMemory.Lock(m_Timeout);

		const StreamInfo *pInfo=reinterpret_cast<const StreamInfo*>(m_pBuffer);
		bool fEnded=(pInfo->Status&STATUS_ENDED)!=0;

		m_SharedMemory.Unlock();

		return fEnded;
	}

	bool CStreamPool::Clear()
	{
		if (m_pBuffer==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_Timeout))
			return false;

		StreamInfo *pInfo=reinterpret_cast<StreamInfo*>(m_pBuffer);
		pInfo->FirstPos=0;
		pInfo->StreamLength=0;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CStreamPool::Write(const void *pBuffer,DWORD BufferLength)
	{
		if (pBuffer==nullptr || BufferLength==0)
			return false;
		if (m_pBuffer==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_Timeout))
			return false;

		StreamInfo *pInfo=reinterpret_cast<StreamInfo*>(m_pBuffer);

		if (pInfo->FirstPos>=pInfo->BufferLength
				|| pInfo->StreamLength>pInfo->BufferLength) {
			m_SharedMemory.Unlock();
			OutLog(LOG_ERROR,L"ストリームプールのヘッダが不正です。");
			return false;
		}

		const BYTE *pSrcBuffer=static_cast<const BYTE*>(pBuffer);
		DWORD SrcLength=BufferLength;
		if (pInfo->BufferLength<SrcLength) {
			pSrcBuffer+=(SrcLength-pInfo->BufferLength)*TS_PACKET_SIZE;
			SrcLength=pInfo->BufferLength;
		}

		BYTE *pDstBuffer=m_pBuffer+sizeof(StreamInfo);
		DWORD DstPos=(pInfo->FirstPos+pInfo->StreamLength)%pInfo->BufferLength;
		size_t SrcSize=(size_t)SrcLength*TS_PACKET_SIZE;
		size_t CopySize=(size_t)min(SrcLength,pInfo->BufferLength-DstPos)*TS_PACKET_SIZE;
		::CopyMemory(pDstBuffer+DstPos*TS_PACKET_SIZE,pSrcBuffer,CopySize);
		if (CopySize<SrcSize)
			::CopyMemory(pDstBuffer,pSrcBuffer+CopySize,SrcSize-CopySize);

		if (pInfo->BufferLength-pInfo->StreamLength>=SrcLength) {
			pInfo->StreamLength+=SrcLength;
		} else {
			pInfo->FirstPos+=(pInfo->StreamLength+SrcLength)-pInfo->BufferLength;
			pInfo->FirstPos%=pInfo->BufferLength;
			pInfo->StreamLength=pInfo->BufferLength;
		}

		pInfo->PacketCount+=SrcLength;

		m_SharedMemory.Unlock();

		return true;
	}

	bool CStreamPool::Read(void *pBuffer,DWORD *pBufferLength,CStreamPosition *pPosition)
	{
		if (pBufferLength==nullptr || *pBufferLength==0)
			return false;

		const DWORD BufferLength=*pBufferLength;
		*pBufferLength=0;

		if (pBuffer==nullptr)
			return false;

		if (m_pBuffer==nullptr)
			return false;

		if (!m_SharedMemory.Lock(m_Timeout))
			return false;

		const StreamInfo *pInfo=reinterpret_cast<const StreamInfo*>(m_pBuffer);

		if ((pInfo->Status&STATUS_ENDED)!=0) {
			m_SharedMemory.Unlock();
			OutLog(LOG_ERROR,L"ストリームが既に終了しています。");
			return false;
		}

		if (pInfo->FirstPos>=pInfo->BufferLength
				|| pInfo->StreamLength>pInfo->BufferLength) {
			m_SharedMemory.Unlock();
			OutLog(LOG_ERROR,L"ストリームプールのヘッダが不正です。");
			return false;
		}

		DWORD CopyLength=min(BufferLength,pInfo->StreamLength);

		if (CopyLength>0) {
			const BYTE *pSrcBuffer=m_pBuffer+sizeof(StreamInfo);
			size_t CopySize=(size_t)CopyLength*TS_PACKET_SIZE;

			if (pPosition==nullptr || pPosition->IsTail()) {
				size_t SrcPos=(pInfo->FirstPos+(pInfo->StreamLength-CopyLength))%pInfo->BufferLength;
				size_t Size=min(CopySize,((size_t)pInfo->BufferLength-SrcPos)*TS_PACKET_SIZE);

				::CopyMemory(pBuffer,pSrcBuffer+SrcPos*TS_PACKET_SIZE,Size);
				if (Size<CopySize)
					::CopyMemory(static_cast<BYTE*>(pBuffer)+Size,pSrcBuffer,CopySize-Size);

				if (pPosition!=nullptr)
					pPosition->Pos=pInfo->PacketCount;
			} else {
				if (pInfo->PacketCount>pPosition->Pos) {
					size_t RemainPackets=(size_t)(pInfo->PacketCount-pPosition->Pos);
					size_t SrcPos;

					if (RemainPackets<pInfo->StreamLength) {
						if (RemainPackets<CopyLength) {
							CopyLength=(DWORD)RemainPackets;
							CopySize=(size_t)CopyLength*TS_PACKET_SIZE;
						}
						SrcPos=(pInfo->FirstPos+(pInfo->StreamLength-RemainPackets))%pInfo->BufferLength;
					} else {
						SrcPos=pInfo->FirstPos;
					}

					size_t Size=min(CopySize,((size_t)pInfo->BufferLength-SrcPos)*TS_PACKET_SIZE);
					::CopyMemory(pBuffer,pSrcBuffer+SrcPos*TS_PACKET_SIZE,Size);
					if (Size<CopySize)
						::CopyMemory(static_cast<BYTE*>(pBuffer)+Size,pSrcBuffer,CopySize-Size);

					if (pInfo->PacketCount-pInfo->StreamLength<=pPosition->Pos) {
						pPosition->Pos+=CopyLength;
					} else {
						pPosition->Pos=pInfo->PacketCount-(pInfo->StreamLength-CopyLength);
					}
				} else {
					CopyLength=0;
				}
			}

			*pBufferLength=CopyLength;
		}

		m_SharedMemory.Unlock();

		return true;
	}

	bool CStreamPool::Read(CDataBlock *pData,CStreamPosition *pPosition)
	{
		if (pData==nullptr)
			return false;

		DWORD Length=(DWORD)(pData->GetReservedSize()/TS_PACKET_SIZE);
		if (!Read(pData->GetBuffer(),&Length,pPosition))
			return false;

		pData->SetSize((size_t)Length*TS_PACKET_SIZE);

		return true;
	}

	DWORD CStreamPool::GetStreamLength()
	{
		if (m_pBuffer==nullptr)
			return 0;

		if (!m_SharedMemory.Lock(m_Timeout))
			return 0;

		const StreamInfo *pInfo=reinterpret_cast<const StreamInfo*>(m_pBuffer);
		DWORD Length;

		if ((pInfo->Status&STATUS_ENDED)!=0)
			Length=0;
		else
			Length=pInfo->StreamLength;

		m_SharedMemory.Unlock();

		return Length;
	}

}
