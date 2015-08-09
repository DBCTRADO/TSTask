#ifndef TSTASK_MODULE_H
#define TSTASK_MODULE_H


namespace TSTask
{

	class CModule
	{
	public:
		CModule();
		virtual ~CModule();
		bool Load(LPCWSTR pszFileName);
		void Unload();
		bool IsLoaded() const { return m_hModule!=nullptr; }
		HMODULE GetHandle() const { return m_hModule; }
		bool GetFilePath(String *pFilePath) const;
		void *GetFunction(const char *pName) const;

	protected:
		HMODULE m_hModule;

	private:
		CModule(const CModule &) = delete;
		CModule &operator=(const CModule &) = delete;
	};

}


#endif
