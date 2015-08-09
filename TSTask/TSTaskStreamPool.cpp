#include "stdafx.h"
#include "TSTask.h"
#include "TSTaskStreamPool.h"
#include "../Common/DebugDef.h"


namespace TSTask
{

	CTSTaskStreamPool::CTSTaskStreamPool()
		: m_BufferLength(1024)
	{
	}

	CTSTaskStreamPool::~CTSTaskStreamPool()
	{
		Close();
	}

	bool CTSTaskStreamPool::Create(LPCWSTR pszName,DWORD BufferLength)
	{
		Close();

		CBlockLock Lock(m_Lock);

		if (!m_Buffer.Reserve(m_BufferLength*TS_PACKET_SIZE))
			return false;

		if (!m_StreamPool.Create(pszName,BufferLength)) {
			Close();
			return false;
		}

		return true;
	}

	void CTSTaskStreamPool::Close()
	{
		CBlockLock Lock(m_Lock);

		m_StreamPool.Close();
		m_Buffer.Free();
	}

	bool CTSTaskStreamPool::IsCreated() const
	{
		return m_StreamPool.IsOpened();
	}

	bool CTSTaskStreamPool::End()
	{
		return m_StreamPool.End();
	}

	bool CTSTaskStreamPool::Clear()
	{
		CBlockLock Lock(m_Lock);

		m_Buffer.SetSize(0);
		return m_StreamPool.Clear();
	}

	bool CTSTaskStreamPool::OnPacket(const ::CTsPacket *pPacket)
	{
		CBlockLock Lock(m_Lock);

		if (m_StreamPool.IsOpened()) {
			size_t Size=m_Buffer.GetSize();

			if (Size+188>m_Buffer.GetReservedSize()) {
				m_StreamPool.Write(m_Buffer.GetBuffer(),(DWORD)(Size/TS_PACKET_SIZE));
				m_Buffer.SetSize(0);
			}

			m_Buffer.AppendData(pPacket->GetData(),TS_PACKET_SIZE);
		}

		return true;
	}

	void CTSTaskStreamPool::OnReset()
	{
		Clear();
	}

}
