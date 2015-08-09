#include "stdafx.h"
#include <wincrypt.h>
#include "TSTaskCentre.h"
#include "ThemeSettings.h"

#pragma comment(lib, "crypt32.lib")


namespace TSTaskCentre
{

	static const size_t MAX_SETTING_KEYWORD=256;


	CThemeSettings::CThemeSettings()
	{
		Reset();
	}

	CThemeSettings::~CThemeSettings()
	{
	}

	void CThemeSettings::Reset()
	{
		CMainBoard::GetDefaultTheme(&m_MainBoardTheme);
	}

	bool CThemeSettings::LoadFromFile(LPCWSTR pszFileName,LPCWSTR pszSection)
	{
		if (TSTask::IsStringEmpty(pszFileName))
			return false;

		TSTask::CSettings Settings;

		if (!Settings.Open(pszFileName,TSTask::CSettings::OPEN_READ))
			return false;

		if (!Settings.SetSection(TSTask::IsStringEmpty(pszSection)?L"Theme":pszSection))
			return false;

		Reset();
		m_FileName=pszFileName;

		ReadBackgroundStyle(Settings,L"MainBoard.Back",&m_MainBoardTheme.Background);
		ReadItemStyle(Settings,L"MainBoard.Base",&m_MainBoardTheme.TSTaskBarBase);
		ReadItemStyle(Settings,L"MainBoard.Caption.Active",&m_MainBoardTheme.Caption.Active);
		ReadItemStyle(Settings,L"MainBoard.Caption.Inactive",&m_MainBoardTheme.Caption.Inactive);

		ReadBackgroundStyle(Settings,L"StatusBar.Back",&m_MainBoardTheme.TSTaskBar.Status.BackgroundStyle);
		ReadBackgroundStyle(Settings,L"StatusBar.Row.Back",&m_MainBoardTheme.TSTaskBar.Status.RowBackgroundStyle);
		m_MainBoardTheme.TSTaskBar.Status.TopRowBackgroundStyle=m_MainBoardTheme.TSTaskBar.Status.RowBackgroundStyle;
		m_MainBoardTheme.TSTaskBar.Status.MiddleRowBackgroundStyle=m_MainBoardTheme.TSTaskBar.Status.RowBackgroundStyle;
		m_MainBoardTheme.TSTaskBar.Status.BottomRowBackgroundStyle=m_MainBoardTheme.TSTaskBar.Status.RowBackgroundStyle;
		ReadBackgroundStyle(Settings,L"StatusBar.Row.Top.Back",&m_MainBoardTheme.TSTaskBar.Status.TopRowBackgroundStyle);
		ReadBackgroundStyle(Settings,L"StatusBar.Row.Middle.Back",&m_MainBoardTheme.TSTaskBar.Status.MiddleRowBackgroundStyle);
		ReadBackgroundStyle(Settings,L"StatusBar.Row.Bottom.Back",&m_MainBoardTheme.TSTaskBar.Status.BottomRowBackgroundStyle);
		ReadItemStyle(Settings,L"StatusBar.Item",&m_MainBoardTheme.TSTaskBar.Status.ItemStyle);
		ReadItemStyle(Settings,L"StatusBar.Item.Hover",&m_MainBoardTheme.TSTaskBar.Status.HighlightItemStyle);

		ReadForegroundStyle(Settings,L"StatusBar.Recording.Circle",&m_MainBoardTheme.TSTaskBar.Recording.Circle);
		ReadItemStyle(Settings,L"StatusBar.Caption",&m_MainBoardTheme.TSTaskBar.CaptionStyle);

		for (int i=0;i<_countof(m_MainBoardTheme.TvRockCaptionStyles);i++) {
			WCHAR szKey[64];
			TSTask::FormatString(szKey,_countof(szKey),L"StatusBar.Caption%c",L'A'+i);
			ReadItemStyle(Settings,szKey,&m_MainBoardTheme.TvRockCaptionStyles[i]);
		}

		ReadBackgroundStyle(Settings,L"InformationBar.Back",&m_MainBoardTheme.InformationBar.BackgroundStyle);
		ReadItemStyle(Settings,L"InformationBar.Item",&m_MainBoardTheme.InformationBar.ItemStyle);
		ReadForegroundStyle(Settings,L"InformationBar.Info.Fore",&m_MainBoardTheme.InformationBar.InfoStyle);
		ReadForegroundStyle(Settings,L"InformationBar.Error.Fore",&m_MainBoardTheme.InformationBar.ErrorStyle);
		ReadForegroundStyle(Settings,L"InformationBar.Warning.Fore",&m_MainBoardTheme.InformationBar.WarningStyle);

		return true;
	}

	bool CThemeSettings::SaveToFile(LPCWSTR pszFileName,LPCWSTR pszSection) const
	{
		if (TSTask::IsStringEmpty(pszFileName))
			return false;

		TSTask::CSettings Settings;

		if (!Settings.Open(pszFileName,TSTask::CSettings::OPEN_WRITE))
			return false;

		if (!Settings.SetSection(TSTask::IsStringEmpty(pszSection)?L"Theme":pszSection))
			return false;

		WriteBackgroundStyle(Settings,L"MainBoard.Back",m_MainBoardTheme.Background);
		WriteItemStyle(Settings,L"MainBoard.Base",m_MainBoardTheme.TSTaskBarBase);
		WriteItemStyle(Settings,L"MainBoard.Caption.Active",m_MainBoardTheme.Caption.Active);
		WriteItemStyle(Settings,L"MainBoard.Caption.Inactive",m_MainBoardTheme.Caption.Inactive);

		WriteBackgroundStyle(Settings,L"StatusBar.Back",m_MainBoardTheme.TSTaskBar.Status.BackgroundStyle);
		WriteBackgroundStyle(Settings,L"StatusBar.Row.Back",m_MainBoardTheme.TSTaskBar.Status.RowBackgroundStyle);
		WriteBackgroundStyle(Settings,L"StatusBar.Row.Top.Back",m_MainBoardTheme.TSTaskBar.Status.TopRowBackgroundStyle);
		WriteBackgroundStyle(Settings,L"StatusBar.Row.Middle.Back",m_MainBoardTheme.TSTaskBar.Status.MiddleRowBackgroundStyle);
		WriteBackgroundStyle(Settings,L"StatusBar.Row.Bottom.Back",m_MainBoardTheme.TSTaskBar.Status.BottomRowBackgroundStyle);
		WriteItemStyle(Settings,L"StatusBar.Item",m_MainBoardTheme.TSTaskBar.Status.ItemStyle);
		WriteItemStyle(Settings,L"StatusBar.Item.Hover",m_MainBoardTheme.TSTaskBar.Status.HighlightItemStyle);

		WriteForegroundStyle(Settings,L"StatusBar.Recording.Circle",m_MainBoardTheme.TSTaskBar.Recording.Circle);
		WriteItemStyle(Settings,L"StatusBar.Caption",m_MainBoardTheme.TSTaskBar.CaptionStyle);

		for (int i=0;i<_countof(m_MainBoardTheme.TvRockCaptionStyles);i++) {
			WCHAR szKey[64];
			TSTask::FormatString(szKey,_countof(szKey),L"StatusBar.Caption%c",L'A'+i);
			WriteItemStyle(Settings,szKey,m_MainBoardTheme.TvRockCaptionStyles[i]);
		}

		WriteBackgroundStyle(Settings,L"InformationBar.Back",m_MainBoardTheme.InformationBar.BackgroundStyle);
		WriteItemStyle(Settings,L"InformationBar.Item",m_MainBoardTheme.InformationBar.ItemStyle);
		WriteForegroundStyle(Settings,L"InformationBar.Info.Fore",m_MainBoardTheme.InformationBar.InfoStyle);
		WriteForegroundStyle(Settings,L"InformationBar.Error.Fore",m_MainBoardTheme.InformationBar.ErrorStyle);
		WriteForegroundStyle(Settings,L"InformationBar.Warning.Fore",m_MainBoardTheme.InformationBar.WarningStyle);

		return true;
	}

	const CMainBoard::ThemeInfo &CThemeSettings::GetMainBoardTheme() const
	{
		return m_MainBoardTheme;
	}

	static inline BYTE GetCharHex(WCHAR Char)
	{
		if (Char>=L'0' && Char<=L'9')
			return (BYTE)(Char-'0');
		if (Char>=L'a' && Char<=L'f')
			return (BYTE)(Char-'a'+10);
		if (Char>=L'A' && Char<=L'F')
			return (BYTE)(Char-'A'+10);
		return 0;
	}

	static BYTE GetHex(LPCWSTR pValue)
	{
		return (GetCharHex(pValue[0])<<4) | GetCharHex(pValue[1]);
	}

	bool CThemeSettings::ReadColor(TSTask::CSettings &Settings,LPCWSTR pszKeyword,
								   Graphics::Color *pColor) const
	{
		WCHAR szValue[16];

		if (!Settings.Read(pszKeyword,szValue,_countof(szValue))
				|| ::lstrlenW(szValue)<7
				|| szValue[0]!=L'#')
			return false;

		pColor->Red=GetHex(&szValue[1]);
		pColor->Green=GetHex(&szValue[3]);
		pColor->Blue=GetHex(&szValue[5]);
		if (::lstrlenW(szValue)>=9)
			pColor->Alpha=GetHex(&szValue[7]);
		else
			pColor->Alpha=255;

		return true;
	}

	bool CThemeSettings::WriteColor(TSTask::CSettings &Settings,LPCWSTR pszKeyword,
									const Graphics::Color &Color) const
	{
		WCHAR szValue[16];

		TSTask::FormatString(szValue,_countof(szValue),L"#%02x%02x%02x%02x",
							 Color.Red,Color.Green,Color.Blue,Color.Alpha);
		return Settings.Write(pszKeyword,szValue);
	}

	void CThemeSettings::ReadItemStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
									   Theme::ItemStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Back",pszName);
		ReadBackgroundStyle(Settings,szKeyword,&pStyle->Background);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Fore",pszName);
		ReadForegroundStyle(Settings,szKeyword,&pStyle->Foreground);
	}

	void CThemeSettings::WriteItemStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										const Theme::ItemStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Back",pszName);
		WriteBackgroundStyle(Settings,szKeyword,Style.Background);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Fore",pszName);
		WriteForegroundStyle(Settings,szKeyword,Style.Foreground);
	}

	void CThemeSettings::ReadBackgroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
											 Theme::BackgroundStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		ReadFillStyle(Settings,pszName,&pStyle->Fill);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Border",pszName);
		ReadBorderStyle(Settings,szKeyword,&pStyle->Border);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Image",pszName);
		ReadImageStyle(Settings,szKeyword,&pStyle->Image);
	}

	void CThemeSettings::WriteBackgroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
											  const Theme::BackgroundStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		WriteFillStyle(Settings,pszName,Style.Fill);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Border",pszName);
		WriteBorderStyle(Settings,szKeyword,Style.Border);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Image",pszName);
		WriteImageStyle(Settings,szKeyword,Style.Image);
	}

	void CThemeSettings::ReadForegroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
											 Theme::ForegroundStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD],szType[64];

		ReadFillStyle(Settings,pszName,&pStyle->Fill);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Glow",pszName);
		ReadGlowStyle(Settings,szKeyword,&pStyle->Glow);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.AntiAliasing",pszName);
		if (Settings.Read(szKeyword,szType,_countof(szType))) {
			Theme::AntiAliasingType Type=Theme::ParseAntiAliasingTypeText(szType);
			if (Type!=Theme::ANTIALIASING_INVALID)
				pStyle->AntiAliasing=Type;
		}
	}

	void CThemeSettings::WriteForegroundStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
											  const Theme::ForegroundStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		WriteFillStyle(Settings,pszName,Style.Fill);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Glow",pszName);
		WriteGlowStyle(Settings,szKeyword,Style.Glow);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.AntiAliasing",pszName);
		Settings.Write(szKeyword,Theme::GetAntiAliasingTypeText(Style.AntiAliasing));
	}

	void CThemeSettings::ReadFillStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
									   Theme::FillStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD],szType[64];
		int Value;

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		if (Settings.Read(szKeyword,szType,_countof(szType))) {
			Theme::FillType Type=Theme::ParseFillTypeText(szType);
			if (Type!=Theme::FILL_INVALID)
				pStyle->Type=Type;
		}

		if (pStyle->Type==Theme::FILL_SOLID) {
			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
			ReadColor(Settings,szKeyword,&pStyle->Solid.Color);
		} else if (pStyle->Type==Theme::FILL_GRADIENT) {
			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Direction",pszName);
			if (Settings.Read(szKeyword,&Value)
					&& Value>=0 && Value<Graphics::DIRECTION_TRAILER)
				pStyle->Gradient.Direction=Graphics::GradientDirection(Value);

			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
			ReadColor(Settings,szKeyword,&pStyle->Gradient.Color1);

			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color2",pszName);
			ReadColor(Settings,szKeyword,&pStyle->Gradient.Color2);
		}
	}

	void CThemeSettings::WriteFillStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										const Theme::FillStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		Settings.Write(szKeyword,Theme::GetFillTypeText(Style.Type));

		if (Style.Type==Theme::FILL_SOLID) {
			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
			WriteColor(Settings,szKeyword,Style.Solid.Color);
		} else if (Style.Type==Theme::FILL_GRADIENT) {
			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Direction",pszName);
			Settings.Write(szKeyword,int(Style.Gradient.Direction));

			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
			WriteColor(Settings,szKeyword,Style.Gradient.Color1);

			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color2",pszName);
			WriteColor(Settings,szKeyword,Style.Gradient.Color2);
		}
	}

	void CThemeSettings::ReadBorderStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										 Theme::BorderStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD],szType[64];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		if (Settings.Read(szKeyword,szType,_countof(szType))) {
			Theme::BorderType Type=Theme::ParseBorderTypeText(szType);
			if (Type!=Theme::BORDER_INVALID)
				pStyle->Type=Type;
		}

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
		ReadColor(Settings,szKeyword,&pStyle->Color);
	}

	void CThemeSettings::WriteBorderStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										  const Theme::BorderStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		Settings.Write(szKeyword,Theme::GetBorderTypeText(Style.Type));

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
		WriteColor(Settings,szKeyword,Style.Color);
	}

	void CThemeSettings::ReadGlowStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
									   Theme::GlowStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD],szType[64];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		if (Settings.Read(szKeyword,szType,_countof(szType))) {
			Theme::GlowType Type=Theme::ParseGlowTypeText(szType);
			if (Type!=Theme::GLOW_INVALID)
				pStyle->Type=Type;
		}

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
		ReadColor(Settings,szKeyword,&pStyle->Color);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Radius",pszName);
		Settings.Read(szKeyword,&pStyle->Radius);
	}

	void CThemeSettings::WriteGlowStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										const Theme::GlowStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Type",pszName);
		Settings.Write(szKeyword,Theme::GetGlowTypeText(Style.Type));

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Color",pszName);
		WriteColor(Settings,szKeyword,Style.Color);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Radius",pszName);
		Settings.Write(szKeyword,Style.Radius);
	}

	void CThemeSettings::ReadImageStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										Theme::ImageStyle *pStyle) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Name",pszName);
		TSTask::String FileName;
		if (Settings.Read(szKeyword,&FileName)) {
			if (TSTask::PathUtility::IsAbsolute(FileName)) {
				pStyle->FileName=FileName;
			} else {
				TSTask::String Directory(m_FileName);
				TSTask::PathUtility::RemoveFileName(&Directory);
				TSTask::PathUtility::RelativeToAbsolute(&pStyle->FileName,Directory,FileName);
			}
		} else {
			TSTask::String Data;
			TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Data",pszName);
			if (Settings.Read(szKeyword,&Data) && !Data.empty()) {
				DWORD Size=0;
				if (::CryptStringToBinary(
							Data.data(),static_cast<DWORD>(Data.length()),
							CRYPT_STRING_BASE64,
							nullptr,&Size,nullptr,nullptr)
						&& Size>0) {
					if (pStyle->Data.Binary.SetSize(Size)) {
						if (::CryptStringToBinary(
								Data.data(),static_cast<DWORD>(Data.length()),
								CRYPT_STRING_BASE64,
								static_cast<BYTE*>(pStyle->Data.Binary.GetBuffer()),&Size,
								nullptr,nullptr)) {
							Graphics::CSystem::GetImageHash(
								pStyle->Data.Binary.GetBuffer(),
								pStyle->Data.Binary.GetSize(),
								&pStyle->Data.Hash);
						} else {
							pStyle->Data.Binary.Free();
						}
					}
				}
			}
		}

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Opacity",pszName);
		Settings.Read(szKeyword,&pStyle->Opacity);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Align",pszName);
		Settings.Read(szKeyword,&pStyle->Align);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Fit",pszName);
		int Fit;
		if (Settings.Read(szKeyword,&Fit)
				&& Fit>=0 && Fit<Theme::FIT_TRAILER)
			pStyle->Fit=Theme::FitType(Fit);
	}

	void CThemeSettings::WriteImageStyle(TSTask::CSettings &Settings,LPCWSTR pszName,
										 const Theme::ImageStyle &Style) const
	{
		WCHAR szKeyword[MAX_SETTING_KEYWORD];

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Name",pszName);
		Settings.Write(szKeyword,Style.FileName);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Data",pszName);
		TSTask::String Data;
		if (Style.Data.Binary.GetSize()>0) {
			DWORD Size=0;
			if (::CryptBinaryToStringW(
					static_cast<const BYTE*>(Style.Data.Binary.GetBuffer()),
					static_cast<DWORD>(Style.Data.Binary.GetSize()),
					CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
					nullptr,&Size)) {
				WCHAR *pBuffer=new WCHAR[Size];
				if (::CryptBinaryToStringW(
						static_cast<const BYTE*>(Style.Data.Binary.GetBuffer()),
						static_cast<DWORD>(Style.Data.Binary.GetSize()),
						CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
						pBuffer,&Size))
					Data.assign(pBuffer,Size);
				delete [] pBuffer;
			}
		}
		Settings.Write(szKeyword,Data);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Opacity",pszName);
		Settings.Write(szKeyword,Style.Opacity);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Align",pszName);
		Settings.Write(szKeyword,Style.Align);

		TSTask::FormatString(szKeyword,_countof(szKeyword),L"%s.Fit",pszName);
		Settings.Write(szKeyword,int(Style.Fit));
	}

}
