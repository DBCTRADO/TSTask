#ifndef TSTASK_DATABLOCK_H
#define TSTASK_DATABLOCK_H


namespace TSTask
{

	class CDataBlock
	{
	public:
		CDataBlock();
		CDataBlock(const CDataBlock &Src);
		CDataBlock(CDataBlock &&Src);
		virtual ~CDataBlock();
		CDataBlock &operator=(const CDataBlock &Src);
		CDataBlock &operator=(CDataBlock &&Src);
		void Free();
		bool Reserve(size_t Size);
		size_t GetReservedSize() const { return m_BufferSize; }
		bool Compact();
		bool SetSize(size_t Size);
		size_t GetSize() const { return m_Size; }
		bool SetData(const void *pData,size_t Size);
		bool AppendData(const void *pData,size_t Size);
		void *GetBuffer() { return m_pBuffer; }
		const void *GetBuffer() const { return m_pBuffer; }
		void *GetBuffer(size_t Pos);
		const void *GetBuffer(size_t Pos) const;

	private:
		size_t m_Size;
		size_t m_BufferSize;
		void *m_pBuffer;
	};

#if 0
	class CDataBlockStream
	{
	public:
		CDataBlockStream(CDataBlock *pDataBlock);
		~CDataBlockStream();
		bool Read(void *pBuffer,size_t Size);
		bool Write(const void *pBuffer,size_t Size);
		bool Read(BYTE *pValue) { return Read(pValue,sizeof(*pValue)); }
		bool Write(BYTE Value) { return Write(&Value,sizeof(Value)); }
		bool Read(WORD *pValue) { return Read(pValue,sizeof(*pValue)); }
		bool Write(WORD Value) { return Write(&Value,sizeof(Value)); }
		bool Read(DWORD *pValue) { return Read(pValue,sizeof(*pValue)); }
		bool Write(DWORD Value) { return Write(&Value,sizeof(Value)); }
		bool Read(ULONGLONG *pValue) { return Read(pValue,sizeof(*pValue)); }
		bool Write(ULONGLONG Value) { return Write(&Value,sizeof(Value)); }
		bool Read(bool *pValue);
		bool Write(bool Value);
		bool Read(String *pValue);
		bool Write(const String &Value);

	private:
		CDataBlock *m_pDataBlock;
		size_t m_Pos;
	};
#endif

}


#endif
