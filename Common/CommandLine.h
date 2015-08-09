#ifndef TSTASK_COMMAND_LINE_H
#define TSTASK_COMMAND_LINE_H


#include <map>
#include <vector>


namespace TSTask
{

	enum BoolSettingValue
	{
		BOOL_DEFAULT,
		BOOL_TRUE,
		BOOL_FALSE
	};

	class CCommandLineParser
	{
	public:
		CCommandLineParser();
		~CCommandLineParser();
		bool Parse(LPCWSTR pszCommandLine);
		void Clear();
		bool HasOption(LPCWSTR pszKey) const;
		bool GetOption(LPCWSTR pszKey,int *pValue) const;
		bool GetOption(LPCWSTR pszKey,LONGLONG *pValue) const;
		bool GetOption(LPCWSTR pszKey,String *pValue) const;
		bool GetOption(LPCWSTR pszKey,BoolSettingValue *pValue) const;
		bool GetOption(LPCWSTR pszKey,FILETIME *pTime) const;
		bool GetOptionDuration(LPCWSTR pszKey,int *pDuration) const;
		bool GetOptions(LPCWSTR pszKey,std::vector<String> *pValues) const;

	private:
		bool ParseTime(LPCWSTR pszText,FILETIME *pTime) const;
		bool ParseDuration(LPCWSTR pszText,int *pDuration) const;

		class CKeyCompare
		{
		public:
			bool operator()(LPCWSTR pszKey1,LPCWSTR pszKey2) const
			{
				return ::lstrcmpiW(pszKey1,pszKey2)<0;
			}
		};

		LPWSTR *m_ppszArgList;
		int m_ArgCount;
		std::multimap<LPCWSTR,int,CKeyCompare> m_OptionMap;
	};

}


#endif
