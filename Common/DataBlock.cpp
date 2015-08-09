#include "stdafx.h"
#include "TSTaskBase.h"
#include "DataBlock.h"
#include "DebugDef.h"


namespace TSTask
{

	CDataBlock::CDataBlock()
		: m_Size(0)
		, m_BufferSize(0)
		, m_pBuffer(nullptr)
	{
	}

	CDataBlock::CDataBlock(const CDataBlock &Src)
		: CDataBlock()
	{
		*this=Src;
	}

	CDataBlock::CDataBlock(CDataBlock &&Src)
		: CDataBlock()
	{
		*this=std::move(Src);
	}

	CDataBlock::~CDataBlock()
	{
		Free();
	}

	CDataBlock &CDataBlock::operator=(const CDataBlock &Src)
	{
		if (&Src!=this) {
			Free();
			if (Src.m_Size>0) {
				if (SetSize(Src.m_Size))
					::CopyMemory(m_pBuffer,Src.m_pBuffer,Src.m_Size);
			}
		}
		return *this;
	}

	CDataBlock &CDataBlock::operator=(CDataBlock &&Src)
	{
		if (&Src!=this) {
			Free();
			m_Size=Src.m_Size;
			m_BufferSize=Src.m_BufferSize;
			m_pBuffer=Src.m_pBuffer;
			Src.m_Size=0;
			Src.m_BufferSize=0;
			Src.m_pBuffer=nullptr;
		}
		return *this;
	}

	void CDataBlock::Free()
	{
		if (m_pBuffer!=nullptr) {
			std::free(m_pBuffer);
			m_pBuffer=nullptr;
		}
		m_Size=0;
		m_BufferSize=0;
	}

	bool CDataBlock::Reserve(size_t Size)
	{
		if (Size>m_BufferSize) {
			void *pNewBuffer=std::realloc(m_pBuffer,Size);
			if (pNewBuffer==nullptr) {
				OutLog(LOG_ERROR,L"ƒƒ‚ƒŠ‚ðŠm•Û‚Å‚«‚Ü‚¹‚ñB(%Iu bytes)",Size);
				return false;
			}
			m_pBuffer=pNewBuffer;
			m_BufferSize=Size;
		}
		return true;
	}

	bool CDataBlock::Compact()
	{
		if (m_Size==0) {
			Free();
		} else if (m_BufferSize>m_Size) {
			void *pNewBuffer=std::realloc(m_pBuffer,m_Size);
			if (pNewBuffer==nullptr)
				return false;
			m_pBuffer=pNewBuffer;
			m_BufferSize=m_Size;
		}
		return true;
	}

	bool CDataBlock::SetSize(size_t Size)
	{
		if (Size>m_BufferSize && !Reserve(Size))
			return false;
		m_Size=Size;
		return true;
	}

	bool CDataBlock::SetData(const void *pData,size_t Size)
	{
		if (Size==0) {
			m_Size=0;
		} else {
			if (pData==nullptr)
				return false;
			if (!SetSize(Size))
				return false;
			::CopyMemory(m_pBuffer,pData,Size);
		}
		return true;
	}

	bool CDataBlock::AppendData(const void *pData,size_t Size)
	{
		if (pData==nullptr || Size==0)
			return false;

		if (m_Size+Size>m_BufferSize && !Reserve(m_Size+Size))
			return false;

		::CopyMemory(static_cast<BYTE*>(m_pBuffer)+m_Size,pData,Size);
		m_Size+=Size;

		return true;
	}

	void *CDataBlock::GetBuffer(size_t Pos)
	{
		if (Pos>=m_Size) {
#ifdef _DEBUG
			::DebugBreak();
#endif
			return nullptr;
		}
		return static_cast<BYTE*>(m_pBuffer)+Pos;
	}

	const void *CDataBlock::GetBuffer(size_t Pos) const
	{
		if (Pos>=m_Size) {
#ifdef _DEBUG
			::DebugBreak();
#endif
			return nullptr;
		}
		return static_cast<const BYTE*>(m_pBuffer)+Pos;
	}


#if 0

	CDataBlockStream::CDataBlockStream(CDataBlock *pDataBlock)
		: m_pDataBlock(pDataBlock)
		, m_Pos(0)
	{
	}

	CDataBlockStream::~CDataBlockStream()
	{
	}

	bool CDataBlockStream::Read(void *pBuffer,DWORD Size)
	{
		if (pBuffer==NULL || Size==0
				|| m_Pos+Size>m_pDataBlock->GetSize())
			return false;
		::CopyMemory(pBuffer,m_pDataBlock->GetBuffer(m_Pos),Size);
		m_Pos+=Size;
		return true;
	}

	bool CDataBlockStream::Write(const void *pBuffer,DWORD Size)
	{
		if (pBuffer==NULL || Size==0)
			return false;
		if (!m_pDataBlock->Reserve(m_Pos+Size))
			return false;
		::CopyMemory(m_pDataBlock->GetBuffer(m_Pos),pBuffer,Size);
		m_Pos+=Size;
		return true;
	}

	bool CDataBlockStream::Read(bool *pValue)
	{
		if (pValue==NULL)
			return false;
		BYTE Value;
		if (!Read(&Value))
			return false;
		*pValue=Value!=0;
		return true;
	}

	bool CDataBlockStream::Write(bool Value)
	{
		return Write((BYTE)Value);
	}

	bool CDataBlockStream::Read(String *pValue)
	{
		if (pValue==NULL)
			return false;
		DWORD Length;
		if (!Read(&Length))
			return false;
		if (Length>0) {
			LPWSTR *pString=new WCHAR[Length+1];
			if (!Read(pString,Length*sizeof(WCHAR)))
				return false;
			pValue->set(pString);
			delete [] pString;
		} else {
			pValue->clear();
		}
		return true;
	}

	bool CDataBlockStream::Write(const String &Value)
	{
		DWORD Length=Value.length();
		if (!Write(Length))
			return false;
		if (Length>0) {
			if (!Write(Value.c_str(),Length*sizeof(WCHAR)))
				return false;
		}
		return true;
	}

#endif

}
