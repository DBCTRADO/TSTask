#ifndef TSTASKCENTRE_THEME_H
#define TSTASKCENTRE_THEME_H


#include "Graphics.h"


namespace TSTaskCentre
{

	namespace Theme
	{

		enum FillType
		{
			FILL_INVALID=-1,
			FILL_NONE,
			FILL_SOLID,
			FILL_GRADIENT,
			FILL_TRAILER
		};

		struct FillStyle
		{
			FillType Type = FILL_SOLID;
			struct {
				Graphics::Color Color;
			} Solid;
			struct {
				Graphics::GradientDirection Direction;
				Graphics::Color Color1;
				Graphics::Color Color2;
			} Gradient;
		};

		enum
		{
			ALIGN_LEFT				=0x0000U,
			ALIGN_RIGHT				=0x0001U,
			ALIGN_HORIZONTAL_CENTER	=0x0002U,
			ALIGN_TOP				=0x0000U,
			ALIGN_BOTTOM			=0x0004U,
			ALIGN_VERTICAL_CENTER	=0x0008U
		};

		enum FitType
		{
			FIT_NONE,
			FIT_HORIZONTAL,
			FIT_VERTICAL,
			FIT_ALL,
			FIT_HORIZONTAL_KEEP_ASPECT_RATIO,
			FIT_VERTICAL_KEEP_ASPECT_RATIO,
			FIT_INSCRIBE_KEEP_ASPECT_RATIO,
			FIT_CIRCUMSCRIBE_KEEP_ASPECT_RATIO,
			FIT_TRAILER
		};

		struct ImageStyle
		{
			TSTask::String FileName;
			struct {
				TSTask::CDataBlock Binary;
				Graphics::CSystem::ImageHash Hash;
			} Data;
			int Opacity = 255;
			unsigned int Align = 0;
			FitType Fit = FIT_NONE;
		};

		enum BorderType
		{
			BORDER_INVALID=-1,
			BORDER_NONE,
			BORDER_SOLID,
			BORDER_SUNKEN,
			BORDER_RAISED,
			BORDER_TRAILER
		};

		struct BorderStyle
		{
			BorderType Type = BORDER_NONE;
			Graphics::Color Color;

			bool IsTransparent() const;
		};

		enum GlowType
		{
			GLOW_INVALID=-1,
			GLOW_NONE,
			GLOW_FADEOUT,
			GLOW_TRAILER
		};

		struct GlowStyle
		{
			GlowType Type = GLOW_NONE;
			Graphics::Color Color;
			int Radius = 0;
		};

		struct BackgroundStyle
		{
			FillStyle Fill;
			BorderStyle Border;
			ImageStyle Image;

			bool IsTransparent() const;
			bool IsOpaque() const { return !IsTransparent(); }
			bool CalcContentRect(RECT *pRect) const;
			bool CalcBoundingRect(RECT *pRect) const;
		};

		enum AntiAliasingType
		{
			ANTIALIASING_INVALID=-1,
			ANTIALIASING_DEFAULT,
			ANTIALIASING_NONE,
			ANTIALIASING_ANTIALIAS,
			ANTIALIASING_ANTIALIAS_HINTING,
			ANTIALIASING_TRAILER
		};

		struct ForegroundStyle
		{
			FillStyle Fill;
			GlowStyle Glow;
			AntiAliasingType AntiAliasing = ANTIALIASING_DEFAULT;
		};

		struct ItemStyle
		{
			BackgroundStyle Background;
			ForegroundStyle Foreground;
		};

		LPCWSTR GetFillTypeText(FillType Type);
		FillType ParseFillTypeText(LPCWSTR pszText);
		LPCWSTR GetBorderTypeText(BorderType Type);
		BorderType ParseBorderTypeText(LPCWSTR pszText);
		LPCWSTR GetGlowTypeText(GlowType Type);
		GlowType ParseGlowTypeText(LPCWSTR pszText);
		LPCWSTR GetAntiAliasingTypeText(AntiAliasingType Type);
		AntiAliasingType ParseAntiAliasingTypeText(LPCWSTR pszText);

		bool DecreaseBorderRect(const BorderStyle &Style,RECT *pRect);
		bool IncreaseBorderRect(const BorderStyle &Style,RECT *pRect);

		class CStylePainter
		{
		public:
			CStylePainter(Graphics::CSystem &System,Graphics::CCanvas *pCanvas);
			bool Fill(const FillStyle &Style,const RECT &Rect);
			bool DrawBorder(const BorderStyle &Style,const RECT &Rect);
			bool DrawBackground(const BackgroundStyle &Style,const RECT &Rect);
			bool DrawText(const ForegroundStyle &Style,LPCWSTR pszText,
						  const RECT &TextRect,const RECT &ClipRect,
						  const Graphics::CFont *pFont,UINT Flags=0);

		private:
			Graphics::CBrush *CreateFillBrush(const FillStyle &Style);

			Graphics::CSystem &m_System;
			Graphics::CCanvas *m_pCanvas;
		};

	}

}


#endif
