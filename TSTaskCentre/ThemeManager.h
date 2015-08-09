#ifndef TSTASKCENTRE_THEME_MANAGER_H
#define TSTASKCENTRE_THEME_MANAGER_H


namespace TSTaskCentre
{

	class CThemeSettings;

	class CThemeManager
	{
	public:
		class TSTASK_ABSTRACT_CLASS(CApplier)
		{
		public:
			virtual bool ApplyTheme(const CThemeSettings &Theme) = 0;
		};

		CThemeManager();
		bool GetThemesDirectory(TSTask::String *pDirectory);
		bool GetThemeFileList(std::vector<TSTask::String> *pList);
		bool LoadTheme(LPCWSTR pszFileName,CThemeSettings *pTheme);
		bool ApplyTheme(const CThemeSettings &Theme);
		bool ApplyTheme(LPCWSTR pszFileName);
		void SetApplier(CApplier *pApplier);

		static LPCWSTR GetThemeExtension() { return L".tttheme"; }

	private:
		CApplier *m_pApplier;
	};

}


#endif
