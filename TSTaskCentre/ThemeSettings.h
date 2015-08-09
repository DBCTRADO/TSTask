#ifndef TSTASKCENTRE_THEME_SETTINGS_H
#define TSTASKCENTRE_THEME_SETTINGS_H


#include "Theme.h"
#include "MainBoard.h"


namespace TSTaskCentre
{

	class CThemeSettings
	{
	public:
		CThemeSettings();
		~CThemeSettings();
		void Reset();
		bool LoadFromFile(LPCWSTR pszFileName,LPCWSTR pszSection=nullptr);
		bool SaveToFile(LPCWSTR pszFileName,LPCWSTR pszSection=nullptr) const;

		const CMainBoard::ThemeInfo &GetMainBoardTheme() const;

	private:
		bool ReadColor(TSTask::CSettings &Settings,LPCWSTR pszKeyword,
					   Graphics::Color *pColor) const;
		bool WriteColor(TSTask::CSettings &Settings,LPCWSTR pszKeyword,
						const Graphics::Color &Color) const;
		void ReadItemStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
						   Theme::ItemStyle *pStyle) const;
		void WriteItemStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							const Theme::ItemStyle &Style) const;
		void ReadBackgroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
								 Theme::BackgroundStyle *pStyle) const;
		void WriteBackgroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
								  const Theme::BackgroundStyle &Style) const;
		void ReadForegroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
								 Theme::ForegroundStyle *pStyle) const;
		void WriteForegroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
								  const Theme::ForegroundStyle &Style) const;
		void ReadFillStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
						   Theme::FillStyle *pStyle) const;
		void WriteFillStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							const Theme::FillStyle &Style) const;
		void ReadBorderStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							 Theme::BorderStyle *pStyle) const;
		void WriteBorderStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							  const Theme::BorderStyle &Style) const;
		void ReadGlowStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
						   Theme::GlowStyle *pStyle) const;
		void WriteGlowStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							const Theme::GlowStyle &Style) const;
		void ReadImageStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							Theme::ImageStyle *pStyle) const;
		void WriteImageStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
							 const Theme::ImageStyle &Style) const;

		TSTask::String m_FileName;
		CMainBoard::ThemeInfo m_MainBoardTheme;
	};

}


#endif
