#include "stdafx.h"
#include "TSTaskBase.h"
#include "Message.h"
#include "DebugDef.h"


namespace TSTask
{

	CMessageProperty::CMessageProperty(ValueType Type)
		: m_Type(Type)
	{
	}

	CMessageProperty::CMessageProperty(ValueType Type,const String &Name)
		: m_Type(Type)
		, m_Name(Name)
	{
	}

	CMessageProperty::~CMessageProperty()
	{
	}


	CMessagePropertyInt::CMessagePropertyInt()
		: CMessageProperty(TYPE_INT)
	{
	}

	CMessagePropertyInt::CMessagePropertyInt(const String &Name,IntType Value)
		: CMessageProperty(TYPE_INT,Name)
		, m_Value(Value)
	{
	}

	CMessageProperty *CMessagePropertyInt::Duplicate() const
	{
		return new CMessagePropertyInt(*this);
	}

	void CMessagePropertyInt::ToString(String *pString) const
	{
		wchar_t szBuffer[32];

		::_i64tow_s(m_Value,szBuffer,_countof(szBuffer),10);
		pString->assign(szBuffer);
	}


	CMessagePropertyString::CMessagePropertyString()
		: CMessageProperty(TYPE_STRING)
	{
	}

	CMessagePropertyString::CMessagePropertyString(const String &Name,const String &Value)
		: CMessageProperty(TYPE_STRING,Name)
		, m_Value(Value)
	{
	}

	CMessageProperty *CMessagePropertyString::Duplicate() const
	{
		return new CMessagePropertyString(*this);
	}

	void CMessagePropertyString::ToString(String *pString) const
	{
		StringUtility::Reserve(*pString,m_Value.length()+2);
		pString->assign(L"\"");
		for (String::const_iterator i=m_Value.begin();i!=m_Value.end();i++) {
			switch (*i) {
			case L'\\':	pString->append(L"\\\\");	break;
			case L'\r':	pString->append(L"\\r");	break;
			case L'\n':	pString->append(L"\\n");	break;
			case L'\t':	pString->append(L"\\t");	break;
			case L'"':	pString->append(L"\\\"");	break;
			default:	pString->push_back(*i);		break;
			}
		}
		pString->append(L"\"");
	}


	CMessagePropertyBool::CMessagePropertyBool()
		: CMessageProperty(TYPE_BOOL)
	{
	}

	CMessagePropertyBool::CMessagePropertyBool(const String &Name,bool Value)
		: CMessageProperty(TYPE_BOOL,Name)
		, m_Value(Value)
	{
	}

	CMessageProperty *CMessagePropertyBool::Duplicate() const
	{
		return new CMessagePropertyBool(*this);
	}

	void CMessagePropertyBool::ToString(String *pString) const
	{
		pString->assign(m_Value?L"true":L"false");
	}


	CMessagePropertyList::CMessagePropertyList()
	{
	}

	CMessagePropertyList::CMessagePropertyList(const CMessagePropertyList &Src)
	{
		*this=Src;
	}

	CMessagePropertyList::~CMessagePropertyList()
	{
		Clear();
	}

	CMessagePropertyList &CMessagePropertyList::operator=(const CMessagePropertyList &Src)
	{
		if (&Src!=this) {
			Clear();

			for (auto i=Src.m_PropertyList.begin();i!=Src.m_PropertyList.end();i++) {
				SetProperty((*i)->Duplicate());
			}
		}

		return *this;
	}

	void CMessagePropertyList::Clear()
	{
		ListUtility::DeleteAll(m_PropertyList);
	}

	size_t CMessagePropertyList::NumProperties() const
	{
		return m_PropertyList.size();
	}

	bool CMessagePropertyList::SetProperty(CMessageProperty *pProperty)
	{
		if (pProperty==nullptr)
			return false;
		DeleteProperty(pProperty->GetName());
		m_PropertyList.push_back(pProperty);
		return true;
	}

	bool CMessagePropertyList::DeleteProperty(LPCWSTR pszName)
	{
		auto i=FindProperty(pszName);
		if (i==m_PropertyList.end())
			return false;
		delete *i;
		m_PropertyList.erase(i);
		return true;
	}

	bool CMessagePropertyList::HasProperty(LPCWSTR pszName) const
	{
		return FindProperty(pszName)!=m_PropertyList.end();
	}

	const CMessageProperty *CMessagePropertyList::GetPropertyByIndex(size_t Index) const
	{
		if (Index>=m_PropertyList.size())
			return nullptr;
		return m_PropertyList[Index];
	}

	const CMessageProperty *CMessagePropertyList::GetPropertyByName(LPCWSTR pszName) const
	{
		auto i=FindProperty(pszName);
		if (i==m_PropertyList.end())
			return nullptr;

		return *i;
	}

	CMessagePropertyList::PropertyList::iterator CMessagePropertyList::FindProperty(LPCWSTR pszName)
	{
		if (IsStringEmpty(pszName))
			return m_PropertyList.end();

		return std::find_if(m_PropertyList.begin(),m_PropertyList.end(),
							[pszName](const CMessageProperty *pProperty) { return ::lstrcmpiW(pProperty->GetName(),pszName)==0; });
	}

	CMessagePropertyList::PropertyList::const_iterator CMessagePropertyList::FindProperty(LPCWSTR pszName) const
	{
		if (IsStringEmpty(pszName))
			return m_PropertyList.end();

		return std::find_if(m_PropertyList.begin(),m_PropertyList.end(),
							[pszName](const CMessageProperty *pProperty) { return ::lstrcmpiW(pProperty->GetName(),pszName)==0; });
	}


	CMessage::CMessage()
	{
	}

	CMessage::CMessage(LPCWSTR pszName)
		: m_Name(pszName)
	{
	}

	CMessage::~CMessage()
	{
	}

	void CMessage::Clear()
	{
		m_Name.clear();
		m_PropertyList.Clear();
	}

	bool CMessage::IsEmpty() const
	{
		return m_Name.empty() && m_PropertyList.NumProperties()==0;
	}

	size_t CMessage::NumProperties() const
	{
		return m_PropertyList.NumProperties();
	}

	bool CMessage::SetProperty(CMessageProperty *pProperty)
	{
		return m_PropertyList.SetProperty(pProperty);
	}

	bool CMessage::SetProperty(LPCWSTR pszName,CMessageProperty::IntType Value)
	{
		if (IsStringEmpty(pszName))
			return false;

		CMessagePropertyInt *pProperty=new CMessagePropertyInt();
		pProperty->SetName(pszName);
		pProperty->SetValue(Value);
		if (!m_PropertyList.SetProperty(pProperty)) {
			delete pProperty;
			return false;
		}

		return true;
	}

	bool CMessage::SetProperty(LPCWSTR pszName,LPCWSTR pszValue)
	{
		if (IsStringEmpty(pszName) || pszValue==nullptr)
			return false;

		CMessagePropertyString *pProperty=new CMessagePropertyString();
		pProperty->SetName(pszName);
		pProperty->SetValue(pszValue);
		if (!m_PropertyList.SetProperty(pProperty)) {
			delete pProperty;
			return false;
		}

		return true;
	}

	bool CMessage::SetProperty(LPCWSTR pszName,LPCWSTR pValue,size_t Length)
	{
		if (IsStringEmpty(pszName) || pValue==nullptr)
			return false;

		CMessagePropertyString *pProperty=new CMessagePropertyString();
		pProperty->SetName(pszName);
		pProperty->SetValue(pValue,Length);
		if (!m_PropertyList.SetProperty(pProperty)) {
			delete pProperty;
			return false;
		}

		return true;
	}

	bool CMessage::SetProperty(LPCWSTR pszName,bool Value)
	{
		if (IsStringEmpty(pszName))
			return false;

		CMessagePropertyBool *pProperty=new CMessagePropertyBool();
		pProperty->SetName(pszName);
		pProperty->SetValue(Value);
		if (!m_PropertyList.SetProperty(pProperty)) {
			delete pProperty;
			return false;
		}

		return true;
	}

	bool CMessage::GetProperty(LPCWSTR pszName,CMessageProperty::IntType *pValue) const
	{
		if (IsStringEmpty(pszName) || pValue==nullptr)
			return false;

		const CMessagePropertyInt *pProperty=
			dynamic_cast<const CMessagePropertyInt*>(GetPropertyByName(pszName));

		if (pProperty==nullptr)
			return false;

		*pValue=pProperty->GetValue();

		return true;
	}

	bool CMessage::GetProperty(LPCWSTR pszName,String *pValue) const
	{
		if (IsStringEmpty(pszName) || pValue==nullptr)
			return false;

		const CMessagePropertyString *pProperty=
			dynamic_cast<const CMessagePropertyString*>(GetPropertyByName(pszName));

		if (pProperty==nullptr)
			return false;

		pValue->assign(pProperty->GetValue());

		return true;
	}

	bool CMessage::GetProperty(LPCWSTR pszName,bool *pValue) const
	{
		if (IsStringEmpty(pszName) || pValue==nullptr)
			return false;

		const CMessagePropertyBool *pProperty=
			dynamic_cast<const CMessagePropertyBool*>(GetPropertyByName(pszName));

		if (pProperty==nullptr)
			return false;

		*pValue=pProperty->GetValue();

		return true;
	}

	bool CMessage::DeleteProperty(LPCWSTR pszName)
	{
		return m_PropertyList.DeleteProperty(pszName);
	}

	bool CMessage::HasProperty(LPCWSTR pszName) const
	{
		return m_PropertyList.HasProperty(pszName);
	}

	const CMessageProperty *CMessage::GetPropertyByIndex(size_t Index) const
	{
		return m_PropertyList.GetPropertyByIndex(Index);
	}

	const CMessageProperty *CMessage::GetPropertyByName(LPCWSTR pszName) const
	{
		return m_PropertyList.GetPropertyByName(pszName);
	}


	CMessageTranslator::CMessageTranslator()
	{
	}

	CMessageTranslator::~CMessageTranslator()
	{
	}

	static inline bool IsLineSeparator(WCHAR Char)
	{
		return Char==L'\r' || Char==L'\n';
	}

	bool CMessageTranslator::Parse(CMessage *pMessage,const void *pData,size_t DataSize)
	{
		if (pMessage==nullptr)
			return false;

		pMessage->Clear();

		if (pData==nullptr || DataSize<sizeof(WCHAR))
			return false;

		LPCWSTR pStart=static_cast<LPCWSTR>(pData);
		LPCWSTR pEnd=pStart+DataSize/sizeof(WCHAR);

		LPCWSTR p=pStart;
		while (p<pEnd && !IsLineSeparator(*p))
			p++;
		if (p==pStart) {
			OutLog(LOG_ERROR,L"メッセージ名がありません。");
			return false;
		}

		pMessage->SetName(pStart,p-pStart);

		while (p<pEnd) {
			while (p<pEnd && IsLineSeparator(*p))
				p++;
			if (p==pEnd)
				break;

			LPCWSTR pKeyword=p;
			while (p<pEnd && *p!=L':' && !IsLineSeparator(*p))
				p++;
			if (p+1<pEnd && *p==L':' && p>pKeyword) {
				String Keyword(pKeyword,p-pKeyword);

				p++;
				while (p<pEnd && *p==L' ')
					p++;
				if (p==pEnd)
					break;
				if (*p==L'"') {
					String Str;

					for (p++;p<pEnd && *p!=L'"' && !IsLineSeparator(*p);p++) {
						if (*p==L'\\') {
							p++;
							if (p==pEnd || IsLineSeparator(*p))
								break;
							switch (*p) {
							case L'\\':	Str.push_back(L'\\');	break;
							case L'r':	Str.push_back(L'\r');	break;
							case L'n':	Str.push_back(L'\n');	break;
							case L't':	Str.push_back(L'\t');	break;
							default:	Str.push_back(*p);		break;
							}
						} else {
							Str.push_back(*p);
						}
					}
					pMessage->SetProperty(Keyword.c_str(),Str.c_str());
				} else {
					LPCWSTR q=p;
					while (q<pEnd && !IsLineSeparator(*q))
						q++;

					String Value(p,q-p);
					if ((*p>=L'0' && *p<=L'9') || *p==L'-' || *p==L'+')
						pMessage->SetProperty(Keyword.c_str(),::_wcstoi64(Value.c_str(),nullptr,0));
					else if (StringUtility::CompareNoCase(Value,L"true")==0)
						pMessage->SetProperty(Keyword.c_str(),true);
					else if (StringUtility::CompareNoCase(Value,L"false")==0)
						pMessage->SetProperty(Keyword.c_str(),false);
					else
						OutLog(LOG_ERROR,L"メッセージのフォーマットが不正です。(%s:%s)",
							   Keyword.c_str(),Value.c_str());

					p=q;
				}
			}

			while (p<pEnd && !IsLineSeparator(*p))
				p++;
		}

		return true;
	}

	bool CMessageTranslator::Parse(CMessage *pMessage,LPCWSTR pszText)
	{
		if (pMessage==nullptr || IsStringEmpty(pszText))
			return false;

		return Parse(pMessage,pszText,::lstrlenW(pszText)*sizeof(WCHAR));
	}

	static bool IsSpace(WCHAR Char)
	{
		return Char==L' ' || Char==L'\t';
	}

	bool CMessageTranslator::ParseSingleLine(std::vector<CMessage> *pMessages,LPCWSTR pszText)
	{
		if (pMessages==nullptr)
			return false;

		pMessages->clear();

		if (IsStringEmpty(pszText))
			return false;

		String MessageStr;

		LPCWSTR p=pszText;
		do {
			while (*p!=L'\0' && IsSpace(*p))
				p++;
			if (*p==L'\0')
				break;

			LPCWSTR pName=p;
			p++;
			while (*p!=L'\0' && !IsSpace(*p))
				p++;
			MessageStr.assign(pName,p-pName);

			while (*p!=L'\0') {
				while (*p!=L'\0' && IsSpace(*p))
					p++;
				if (*p==L'\0')
					break;

				LPCWSTR pProperty=p;
				while (*p!=L'\0' && *p!=L':' && !IsSpace(*p))
					p++;
				if (*p!=L':') {
					p=pProperty;
					break;
				}
				p++;
				if (*p==L'"') {
					p++;
					while (*p!=L'\0' && *p!=L'"') {
						if (*p==L'\\' && *(p+1)==L'"')
							p++;
						p++;
					}
					if (*p==L'"')
						p++;
				} else {
					while (*p!=L'\0' && !IsSpace(*p))
						p++;
				}

				MessageStr+=L'\n';
				MessageStr.append(pProperty,p-pProperty);
			}

			CMessage Message;
			if (!Parse(&Message,MessageStr.c_str()))
				return false;

			pMessages->push_back(Message);
		} while (*p!=L'\0');

		return true;
	}

	bool CMessageTranslator::Format(const CMessage *pMessage,CDataBlock *pData)
	{
		if (pMessage==nullptr || pData==nullptr)
			return false;

		String MessageString;

		if (!Format(pMessage,&MessageString,L"\n"))
			return false;

		if (!pData->SetData(MessageString.data(),MessageString.length()*sizeof(String::value_type))) {
			OutLog(LOG_ERROR,L"メッセージのデータを設定できません。");
			return false;
		}

		return true;
	}

	bool CMessageTranslator::Format(const CMessage *pMessage,String *pText,LPCWSTR pszDelimiter)
	{
		if (pMessage==nullptr || pText==nullptr)
			return false;

		if (pszDelimiter==nullptr)
			pszDelimiter=L"\r\n";

		pText->assign(pMessage->GetName());

		const size_t NumProperties=pMessage->NumProperties();
		if (NumProperties>0) {
			for (size_t i=0;i<NumProperties;i++) {
				const CMessageProperty *pProperty=pMessage->GetPropertyByIndex(i);
				String Value;

				pText->append(pszDelimiter);
				pText->append(pProperty->GetName());
				pText->append(L":");
				pProperty->ToString(&Value);
				pText->append(Value);
			}
		}

		return true;
	}

}
