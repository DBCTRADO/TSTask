#ifndef TSTASK_MESSAGE_H
#define TSTASK_MESSAGE_H


#include <vector>
#include "DataBlock.h"


namespace TSTask
{

	class CMessageProperty
	{
	public:
		enum ValueType
		{
			TYPE_NULL,
			TYPE_INT,
			TYPE_STRING,
			TYPE_BOOL
		};

		typedef LONGLONG IntType;

		virtual ~CMessageProperty();
		LPCWSTR GetName() const { return m_Name.c_str(); }
		void SetName(LPCWSTR pszName) { m_Name.assign(pszName); }
		void SetName(LPCWSTR pName,size_t Length) { m_Name.assign(pName,Length); }
		ValueType GetType() const { return m_Type; }
		virtual CMessageProperty *Duplicate() const = 0;
		virtual void ToString(String *pString) const = 0;

	protected:
		CMessageProperty(ValueType Type);
		CMessageProperty(ValueType Type,const String &Name);

		String m_Name;
		ValueType m_Type;
	};

	class CMessagePropertyInt : public CMessageProperty
	{
	public:
		CMessagePropertyInt();
		CMessagePropertyInt(const String &Name,IntType Value);
		IntType GetValue() const { return m_Value; }
		void SetValue(IntType Value) { m_Value=Value; }
		CMessageProperty *Duplicate() const override;
		void ToString(String *pString) const override;

	private:
		IntType m_Value;
	};

	class CMessagePropertyString : public CMessageProperty
	{
	public:
		CMessagePropertyString();
		CMessagePropertyString(const String &Name,const String &Value);
		LPCWSTR GetValue() const { return m_Value.c_str(); }
		void SetValue(LPCWSTR pszValue) { m_Value.assign(pszValue); }
		void SetValue(LPCWSTR pValue,size_t Length) { m_Value.assign(pValue,Length); }
		CMessageProperty *Duplicate() const override;
		void ToString(String *pString) const override;

	private:
		String m_Value;
	};

	class CMessagePropertyBool : public CMessageProperty
	{
	public:
		CMessagePropertyBool();
		CMessagePropertyBool(const String &Name,bool Value);
		bool GetValue() const { return m_Value; }
		void SetValue(bool Value) { m_Value=Value; }
		CMessageProperty *Duplicate() const override;
		void ToString(String *pString) const override;

	private:
		bool m_Value;
	};

	class CMessagePropertyList
	{
	public:
		CMessagePropertyList();
		CMessagePropertyList(const CMessagePropertyList &Src);
		~CMessagePropertyList();
		CMessagePropertyList &operator=(const CMessagePropertyList &Src);
		void Clear();
		size_t NumProperties() const;
		bool SetProperty(CMessageProperty *pProperty);
		bool DeleteProperty(LPCWSTR pszName);
		bool HasProperty(LPCWSTR pszName) const;
		const CMessageProperty *GetPropertyByIndex(size_t Index) const;
		const CMessageProperty *GetPropertyByName(LPCWSTR pszName) const;

	private:
		typedef std::vector<CMessageProperty*> PropertyList;

		PropertyList::iterator FindProperty(LPCWSTR pszName);
		PropertyList::const_iterator FindProperty(LPCWSTR pszName) const;

		PropertyList m_PropertyList;
	};

	class CMessage
	{
	public:
		CMessage();
		CMessage(LPCWSTR pszName);
		virtual ~CMessage();
		LPCWSTR GetName() const { return m_Name.c_str(); }
		void SetName(LPCWSTR pszName) { m_Name.assign(pszName); }
		void SetName(LPCWSTR pName,size_t Length) { m_Name.assign(pName,Length); }
		void Clear();
		bool IsEmpty() const;
		size_t NumProperties() const;
		bool SetProperty(CMessageProperty *pProperty);
		bool SetProperty(LPCWSTR pszName,CMessageProperty::IntType Value);
		bool SetPropertyInt(LPCWSTR pszName,CMessageProperty::IntType Value) { return SetProperty(pszName,Value); }
		bool SetProperty(LPCWSTR pszName,LPCWSTR pszValue);
		bool SetProperty(LPCWSTR pszName,LPCWSTR pValue,size_t Length);
		bool SetProperty(LPCWSTR pszName,const String &Value) { return SetProperty(pszName,Value.c_str()); }
		bool SetProperty(LPCWSTR pszName,bool Value);
		bool GetProperty(LPCWSTR pszName,CMessageProperty::IntType *pValue) const;
		bool GetProperty(LPCWSTR pszName,String *pValue) const;
		bool GetProperty(LPCWSTR pszName,bool *pValue) const;
		bool DeleteProperty(LPCWSTR pszName);
		bool HasProperty(LPCWSTR pszName) const;
		const CMessageProperty *GetPropertyByIndex(size_t Index) const;
		const CMessageProperty *GetPropertyByName(LPCWSTR pszName) const;

	protected:
		String m_Name;
		CMessagePropertyList m_PropertyList;
	};

	class CMessageTranslator
	{
	public:
		CMessageTranslator();
		~CMessageTranslator();
		bool Parse(CMessage *pMessage,const void *pData,size_t DataSize);
		bool Parse(CMessage *pMessage,LPCWSTR pszText);
		bool ParseSingleLine(std::vector<CMessage> *pMessages,LPCWSTR pszText);
		bool Format(const CMessage *pMessage,CDataBlock *pData);
		bool Format(const CMessage *pMessage,String *pText,LPCWSTR pszDelimiter=nullptr);
	};

	enum
	{
		MESSAGE_PIPE_MAX_SEND_SIZE			=10*1024*1024,
		MESSAGE_PIPE_SEND_BUFFER_SIZE		=1024*1024,
		MESSAGE_PIPE_RECEIVE_BUFFER_SIZE	=1024*1024
	};

}


#endif
