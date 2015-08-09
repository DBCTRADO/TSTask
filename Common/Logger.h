#ifndef TSTASK_LOGGER_H
#define TSTASK_LOGGER_H


#include <vector>
#include <deque>


namespace TSTask
{

	enum LogType
	{
		LOG_VERBOSE,
		LOG_INFO,
		LOG_IMPORTANT,
		LOG_WARNING,
		LOG_ERROR,
		LOG_NONE,
		LOG_TRAILER
	};

	struct LogInfo
	{
		LogType Type;
		unsigned int Number;
		String Text;
		FILETIME Time;
	};

	typedef std::vector<LogInfo> LogList;

	class TSTASK_ABSTRACT_CLASS(CLogger)
	{
	public:
		CLogger();
		virtual ~CLogger() = 0;
		virtual void Clear() = 0;
		virtual void OutLogV(LogType Type,LPCWSTR pszText,va_list Args) = 0;
		void OutLog(LogType Type,LPCWSTR pszText, ...);
	};

	class CBasicLogger : public CLogger
	{
	public:
		enum
		{
			OPEN_APPEND			= 0x0001U,
			OPEN_WRITE_OLD_LOG	= 0x0002U,
			OPEN_AUTO_RENAME	= 0x0004U
		};

		class TSTASK_ABSTRACT_CLASS(CHandler)
		{
		public:
			virtual ~CHandler() {}
			virtual void OnLog(const LogInfo &Info) {}
		};

		static unsigned int MakeTypeFlag(LogType Type) { return 1U<<Type; }
		static const unsigned int LOG_TYPE_ALL=(1U<<LOG_TRAILER)-1;

		CBasicLogger();
		~CBasicLogger();
		void Clear() override;
		void OutLogV(LogType Type,LPCWSTR pszText,va_list Args) override;
		bool GetLog(LogList *pList,size_t Max=0) const;
		bool GetLogByNumber(unsigned int Number,LogInfo *pInfo) const;
		bool GetLastLog(LogInfo *pInfo,unsigned int Types=LOG_TYPE_ALL) const;
		bool SetMaxLog(size_t Max);
		size_t GetMaxLog() const { return m_MaxLog; }
		bool SetLoggingLevel(LogType Level);
		LogType GetLoggingLevel() const { return m_LoggingLevel; }
		bool SaveToFile(LPCWSTR pszFileName,bool fAppend=false) const;
		bool OpenFile(LPCWSTR pszFileName,unsigned int Flags);
		bool CloseFile();
		bool GetFileName(String *pFileName) const;
		void EnableDebugTrace(bool fEnable);
		void SetHandler(CHandler *pHandler);

		enum {
			MAX_TIME_TEXT=64
		};
		static int FormatTime(const FILETIME &Time,LPWSTR pszText,int MaxLength);
		static bool FormatInfo(const LogInfo &Info,String *pText);

	protected:
		bool WriteToFile(HANDLE hFile,const LogInfo *pInfo) const;

		mutable CLocalLock m_Lock;
		std::deque<LogInfo*> m_LogList;
		size_t m_MaxLog;
		LogType m_LoggingLevel;
		unsigned int m_Number;
		HANDLE m_hFile;
		String m_FileName;
		bool m_fDebugTrace;
		CHandler *m_pHandler;
	};

	class CDebugLogger : public CLogger
	{
	public:
		void Clear() override;
		void OutLogV(LogType Type,LPCWSTR pszText,va_list Args) override;
	};

	void SetGlobalLogger(CLogger *pLogger);
	void OutLog(LogType Type,LPCWSTR pszText, ...);
	void OutLogV(LogType Type,LPCWSTR pszText,va_list Args);
	void OutSystemErrorLog(DWORD Error,LPCWSTR pszText, ...);

}


#endif
