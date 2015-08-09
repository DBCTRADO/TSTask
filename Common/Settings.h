#ifndef TSTASK_SETTINGS_H
#define TSTASK_SETTINGS_H


#include "IniFile.h"


namespace TSTask
{

	class CSettings
	{
	public:
		enum {
			OPEN_READ  = 0x00000001,
			OPEN_WRITE = 0x00000002
		};

		typedef std::vector<CIniFile::CEntry> EntryList;

		CSettings();
		~CSettings();
		bool Open(LPCWSTR pszFileName,unsigned int Flags=OPEN_READ | OPEN_WRITE);
		void Close();
		bool IsOpen() const;
		bool Clear();
		bool Flush();
		bool SetSection(LPCWSTR pszSection);
		bool Read(LPCWSTR pszValueName,int *pData);
		bool Write(LPCWSTR pszValueName,int Data);
		bool Read(LPCWSTR pszValueName,unsigned int *pData);
		bool Write(LPCWSTR pszValueName,unsigned int Data);
		bool Read(LPCWSTR pszValueName,LONGLONG *pData);
		bool Write(LPCWSTR pszValueName,LONGLONG Data);
		bool Read(LPCWSTR pszValueName,LPWSTR pszData,unsigned int Max);
		bool Write(LPCWSTR pszValueName,LPCWSTR pszData);
		bool Read(LPCWSTR pszValueName,String *pData);
		bool Write(LPCWSTR pszValueName,const String &Data);
		bool Read(LPCWSTR pszValueName,bool *pfData);
		bool Write(LPCWSTR pszValueName,bool fData);
		bool ReadColor(LPCWSTR pszValueName,COLORREF *pcrData);
		bool WriteColor(LPCWSTR pszValueName,COLORREF crData);
		bool Read(LPCWSTR pszValueName,LOGFONT *pFont);
		bool Write(LPCWSTR pszValueName,const LOGFONT &Font);
		bool IsKeyExists(LPCWSTR pszValueName);
		bool DeleteKey(LPCWSTR pszValueName);
		bool DeleteKeys(LPCWSTR pszValueMask);
		bool GetEntries(EntryList *pEntries);

	private:
		CIniFile m_IniFile;
		unsigned int m_OpenFlags;
	};

}


#endif
