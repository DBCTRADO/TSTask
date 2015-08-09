#include "stdafx.h"
#include "TSTaskBase.h"
#include "Settings.h"
#include "DebugDef.h"


namespace TSTask
{

	static int HexToNum(WCHAR cCode)
	{
		if (cCode>=L'0' && cCode<=L'9')
			return cCode-L'0';
		if (cCode>=L'A' && cCode<=L'F')
			return cCode-L'A'+10;
		if (cCode>=L'a' && cCode<=L'f')
			return cCode-L'a'+10;
		return 0;
	}


	CSettings::CSettings()
		: m_OpenFlags(0)
	{
	}

	CSettings::~CSettings()
	{
		Close();
	}

	bool CSettings::Open(LPCWSTR pszFileName,unsigned int Flags)
	{
		if (IsStringEmpty(pszFileName) || Flags==0)
			return false;

		UINT IniFlags=0;
		if ((Flags&OPEN_READ))
			IniFlags|=CIniFile::OPEN_READ;
		if ((Flags&OPEN_WRITE))
			IniFlags|=CIniFile::OPEN_WRITE;

		if (!m_IniFile.Open(pszFileName,IniFlags))
			return false;

		m_OpenFlags=Flags;

		return true;
	}

	void CSettings::Close()
	{
		m_IniFile.Close();
		m_OpenFlags=0;
	}

	bool CSettings::IsOpen() const
	{
		return m_OpenFlags!=0;
	}

	bool CSettings::Clear()
	{
		if ((m_OpenFlags&OPEN_WRITE)==0)
			return false;

		return m_IniFile.ClearSection();
	}

	bool CSettings::Flush()
	{
		if ((m_OpenFlags&OPEN_WRITE)==0)
			return false;

		//return m_IniFile.Flush();
		return true;
	}

	bool CSettings::SetSection(LPCWSTR pszSection)
	{
		if (m_OpenFlags==0 || IsStringEmpty(pszSection))
			return false;

		return m_IniFile.SelectSection(pszSection);
	}

	bool CSettings::Read(LPCWSTR pszValueName,int *pData)
	{
		if (IsStringEmpty(pszValueName) || pData==nullptr)
			return false;

		LONGLONG Value;
		if (!Read(pszValueName,&Value))
			return false;

		if (Value<INT_MIN || Value>INT_MAX)
			return false;

		*pData=(int)Value;

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,int Data)
	{
		if (IsStringEmpty(pszValueName))
			return false;

		WCHAR szValue[16];

		FormatString(szValue,_countof(szValue),L"%d",Data);

		return Write(pszValueName,szValue);
	}

	bool CSettings::Read(LPCWSTR pszValueName,unsigned int *pData)
	{
		if (IsStringEmpty(pszValueName) || pData==nullptr)
			return false;

		LONGLONG Value;
		if (!Read(pszValueName,&Value))
			return false;

		if (Value<0 || Value>UINT_MAX)
			return false;

		*pData=(unsigned int)Value;

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,unsigned int Data)
	{
		if (IsStringEmpty(pszValueName))
			return false;

		WCHAR szValue[16];

		FormatString(szValue,_countof(szValue),L"%u",Data);

		return Write(pszValueName,szValue);
	}

	bool CSettings::Read(LPCWSTR pszValueName,LONGLONG *pData)
	{
		if (IsStringEmpty(pszValueName) || pData==nullptr)
			return false;

		WCHAR szValue[64];

		if (!Read(pszValueName,szValue,_countof(szValue)))
			return false;

		LPWSTR p=szValue;
		while (*p==L' ' || *p==L'\t')
			p++;
		if ((*p<L'0' || *p>L'9') && *p!=L'+' && *p!=L'-')
			return false;

		wchar_t *pEnd;
		__int64 Value=::_wcstoi64(p,&pEnd,0);
		// TODO: オーバーフローとアンダーフローをチェックする
		switch (*pEnd) {
		case L'K':	Value*=1024;			break;
		case L'k':	Value*=1000;			break;
		case L'M':	Value*=1024*1024;		break;
		case L'G':	Value*=1024*1024*1024;	break;
		case L'T':	Value*=0x10000000000LL;	break;
		}

		*pData=Value;

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,LONGLONG Data)
	{
		if (IsStringEmpty(pszValueName))
			return false;

		WCHAR szValue[64];

		FormatString(szValue,_countof(szValue),L"%lld",Data);

		return Write(pszValueName,szValue);
	}

	bool CSettings::Read(LPCWSTR pszValueName,LPWSTR pszData,unsigned int Max)
	{
		if (IsStringEmpty(pszValueName) || pszData==nullptr || Max==0
				 || (m_OpenFlags&OPEN_READ)==0)
			return false;

		String Value;
		if (!Read(pszValueName,&Value))
			return false;

		::lstrcpynW(pszData,Value.c_str(),Max);

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,LPCWSTR pszData)
	{
		if (IsStringEmpty(pszValueName) || pszData==nullptr || (m_OpenFlags&OPEN_WRITE)==0)
			return false;

		// 文字列が ' か " で囲まれていると読み込み時に除去されてしまうので、
		// 余分に " で囲っておく。
		if (pszData[0]==L'"' || pszData[0]==L'\'') {
			String Buff;
			Buff=L"\"";
			Buff+=pszData;
			Buff+=L"\"";
			return m_IniFile.SetValue(pszValueName,Buff.c_str());
		}

		return m_IniFile.SetValue(pszValueName,pszData);
	}

	bool CSettings::Read(LPCWSTR pszValueName,String *pData)
	{
		if (IsStringEmpty(pszValueName) || pData==nullptr || (m_OpenFlags&OPEN_READ)==0)
			return false;

		return m_IniFile.GetValue(pszValueName,pData);
	}

	bool CSettings::Write(LPCWSTR pszValueName,const String &Data)
	{
		return Write(pszValueName,Data.c_str());
	}

	bool CSettings::Read(LPCWSTR pszValueName,bool *pfData)
	{
		String Text;
		if (!Read(pszValueName,&Text))
			return false;

		if (StringUtility::CompareNoCase(Text,L"true")==0)
			*pfData=true;
		else if (StringUtility::CompareNoCase(Text,L"false")==0)
			*pfData=false;
		else
			return false;

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,bool fData)
	{
		return Write(pszValueName,fData?L"true":L"false");
	}

	bool CSettings::ReadColor(LPCWSTR pszValueName,COLORREF *pcrData)
	{
		String Text;
		if (!Read(pszValueName,&Text)
				|| Text.length()<7
				|| Text[0]!=L'#')
			return false;

		*pcrData=RGB((HexToNum(Text[1])<<4) | HexToNum(Text[2]),
					 (HexToNum(Text[3])<<4) | HexToNum(Text[4]),
					 (HexToNum(Text[5])<<4) | HexToNum(Text[6]));

		return true;
	}

	bool CSettings::WriteColor(LPCWSTR pszValueName,COLORREF crData)
	{
		WCHAR szText[8];

		FormatString(szText,_countof(szText),L"#%02x%02x%02x",
					 GetRValue(crData),GetGValue(crData),GetBValue(crData));

		return Write(pszValueName,szText);
	}

	enum {
		FONT_FLAG_ITALIC	=0x0001U,
		FONT_FLAG_UNDERLINE	=0x0002U,
		FONT_FLAG_STRIKEOUT	=0x0004U
	};

	bool CSettings::Read(LPCWSTR pszValueName,LOGFONT *pFont)
	{
		if (pFont==nullptr)
			return false;

		WCHAR szData[LF_FACESIZE+32];

		if (!Read(pszValueName,szData,_countof(szData)) || szData[0]==L'\0')
			return false;

		LPWSTR p=szData,q;
		for (int i=0;*p!=L'\0';i++) {
			while (*p==L' ')
				p++;
			q=p;
			while (*p!=L'\0' && *p!=L',')
				p++;
			if (*p!=L'\0')
				*p++=L'\0';
			if (*q!=L'\0') {
				switch (i) {
				case 0:
					::lstrcpynW(pFont->lfFaceName,q,LF_FACESIZE);
					pFont->lfWidth=0;
					pFont->lfEscapement=0;
					pFont->lfOrientation=0;
					pFont->lfWeight=FW_NORMAL;
					pFont->lfItalic=0;
					pFont->lfUnderline=0;
					pFont->lfStrikeOut=0;
					pFont->lfCharSet=DEFAULT_CHARSET;
					pFont->lfOutPrecision=OUT_DEFAULT_PRECIS;
					pFont->lfClipPrecision=CLIP_DEFAULT_PRECIS;
					pFont->lfQuality=DRAFT_QUALITY;
					pFont->lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
					break;

				case 1:
					pFont->lfHeight=std::wcstol(q,nullptr,0);
					break;

				case 2:
					pFont->lfWeight=std::wcstol(q,nullptr,0);
					break;

				case 3:
					{
						unsigned int Flags=std::wcstoul(q,nullptr,0);
						pFont->lfItalic=(Flags&FONT_FLAG_ITALIC)!=0;
						pFont->lfUnderline=(Flags&FONT_FLAG_UNDERLINE)!=0;
						pFont->lfStrikeOut=(Flags&FONT_FLAG_STRIKEOUT)!=0;
					}
					break;
				}
			} else if (i==0) {
				return false;
			}
		}

		return true;
	}

	bool CSettings::Write(LPCWSTR pszValueName,const LOGFONT &Font)
	{
		WCHAR szData[LF_FACESIZE+32];
		unsigned int Flags=0;

		if (Font.lfItalic)
			Flags|=FONT_FLAG_ITALIC;
		if (Font.lfUnderline)
			Flags|=FONT_FLAG_UNDERLINE;
		if (Font.lfStrikeOut)
			Flags|=FONT_FLAG_STRIKEOUT;

		FormatString(szData,_countof(szData),L"%s,%d,%d,%u",
					 Font.lfFaceName,Font.lfHeight,Font.lfWeight,Flags);

		return Write(pszValueName,szData);
	}

	bool CSettings::IsKeyExists(LPCWSTR pszValueName)
	{
		if (IsStringEmpty(pszValueName) || m_OpenFlags==0)
			return false;

		return m_IniFile.IsValueExists(pszValueName);
	}

	bool CSettings::DeleteKey(LPCWSTR pszValueName)
	{
		if (IsStringEmpty(pszValueName) || (m_OpenFlags&OPEN_WRITE)==0)
			return false;

		return m_IniFile.DeleteValue(pszValueName);
	}

	bool CSettings::DeleteKeys(LPCWSTR pszValueMask)
	{
		if (IsStringEmpty(pszValueMask) || (m_OpenFlags&OPEN_WRITE)==0)
			return false;

		return m_IniFile.DeleteValues(pszValueMask);
	}

	bool CSettings::GetEntries(EntryList *pEntries)
	{
		if (pEntries==nullptr || (m_OpenFlags&OPEN_READ)==0)
			return false;

		return m_IniFile.GetEntries(pEntries);
	}

}
