#include "stdafx.h"
#include "TSTaskCentre.h"
#include "Theme.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	namespace Theme
	{

		bool BackgroundStyle::IsTransparent() const
		{
			switch (Fill.Type) {
			case FILL_NONE:
				return true;

			case FILL_SOLID:
				return Fill.Solid.Color.Alpha<255;

			case FILL_GRADIENT:
				return Fill.Gradient.Color1.Alpha<255
					|| Fill.Gradient.Color2.Alpha<255;
			}

			return true;
		}

		bool BackgroundStyle::CalcContentRect(RECT *pRect) const
		{
			return DecreaseBorderRect(Border,pRect);
		}

		bool BackgroundStyle::CalcBoundingRect(RECT *pRect) const
		{
			return IncreaseBorderRect(Border,pRect);
		}


		bool BorderStyle::IsTransparent() const
		{
			return Type>BORDER_NONE && Type<BORDER_TRAILER
				&& Color.Alpha<255;
		}


		static const LPCWSTR g_FillTypeList[] = {
			L"none",
			L"solid",
			L"gradient",
		};
		static_assert(_countof(g_FillTypeList)==FILL_TRAILER,"塗りつぶしタイプの数が一致しません。");

		static const LPCWSTR g_BorderTypeList[] = {
			L"none",
			L"solid",
			L"sunken",
			L"raised",
		};
		static_assert(_countof(g_BorderTypeList)==BORDER_TRAILER,"枠のスタイルの数が一致しません。");

		static const LPCWSTR g_GlowTypeList[] = {
			L"none",
			L"fadeout",
		};
		static_assert(_countof(g_GlowTypeList)==GLOW_TRAILER,"光彩のスタイルの数が一致しません。");

		static const LPCWSTR g_AntiAliasingTypeList[] = {
			L"default",
			L"none",
			L"antialias",
			L"antialias-hinting",
		};
		static_assert(_countof(g_AntiAliasingTypeList)==ANTIALIASING_TRAILER,"アンチエイリアシングのタイプの数が一致しません。");

		LPCWSTR GetFillTypeText(FillType Type)
		{
			if (Type<0 || Type>=FILL_TRAILER)
				return nullptr;

			return g_FillTypeList[Type];
		}

		FillType ParseFillTypeText(LPCWSTR pszText)
		{
			if (pszText==nullptr)
				return FILL_INVALID;

			for (int i=0;i<_countof(g_FillTypeList);i++) {
				if (::lstrcmpiW(g_FillTypeList[i],pszText)==0)
					return FillType(i);
			}

			return FILL_INVALID;
		}

		LPCWSTR GetBorderTypeText(BorderType Type)
		{
			if (Type<0 || Type>=BORDER_TRAILER)
				return nullptr;

			return g_BorderTypeList[Type];
		}

		BorderType ParseBorderTypeText(LPCWSTR pszText)
		{
			if (pszText==nullptr)
				return BORDER_INVALID;

			for (int i=0;i<_countof(g_BorderTypeList);i++) {
				if (::lstrcmpiW(g_BorderTypeList[i],pszText)==0)
					return BorderType(i);
			}

			return BORDER_INVALID;
		}

		LPCWSTR GetGlowTypeText(GlowType Type)
		{
			if (Type<0 || Type>=GLOW_TRAILER)
				return nullptr;

			return g_GlowTypeList[Type];
		}

		GlowType ParseGlowTypeText(LPCWSTR pszText)
		{
			if (pszText==nullptr)
				return GLOW_INVALID;

			for (int i=0;i<_countof(g_GlowTypeList);i++) {
				if (::lstrcmpiW(g_GlowTypeList[i],pszText)==0)
					return GlowType(i);
			}

			return GLOW_INVALID;
		}

		LPCWSTR GetAntiAliasingTypeText(AntiAliasingType Type)
		{
			if (Type<0 || Type>=ANTIALIASING_TRAILER)
				return nullptr;

			return g_AntiAliasingTypeList[Type];
		}

		AntiAliasingType ParseAntiAliasingTypeText(LPCWSTR pszText)
		{
			if (pszText==nullptr)
				return ANTIALIASING_INVALID;

			for (int i=0;i<_countof(g_AntiAliasingTypeList);i++) {
				if (::lstrcmpiW(g_AntiAliasingTypeList[i],pszText)==0)
					return AntiAliasingType(i);
			}

			return ANTIALIASING_INVALID;
		}

		bool DecreaseBorderRect(const BorderStyle &Style,RECT *pRect)
		{
			switch (Style.Type) {
			case BORDER_NONE:
				break;

			case BORDER_SOLID:
			case BORDER_SUNKEN:
			case BORDER_RAISED:
				::InflateRect(pRect,-1,-1);
				break;

			default:
				return false;
			}

			return true;
		}

		bool IncreaseBorderRect(const BorderStyle &Style,RECT *pRect)
		{
			switch (Style.Type) {
			case BORDER_NONE:
				break;

			case BORDER_SOLID:
			case BORDER_SUNKEN:
			case BORDER_RAISED:
				::InflateRect(pRect,1,1);
				break;

			default:
				return false;
			}

			return true;
		}


		CStylePainter::CStylePainter(Graphics::CSystem &System,Graphics::CCanvas *pCanvas)
			: m_System(System)
			, m_pCanvas(pCanvas)
		{
		}

		bool CStylePainter::Fill(const FillStyle &Style,const RECT &Rect)
		{
			if (Style.Type!=FILL_NONE) {
				Graphics::CBrush *pBrush=CreateFillBrush(Style);

				if (pBrush==nullptr)
					return false;

				m_pCanvas->FillRect(Rect,pBrush);

				delete pBrush;
			}

			return true;
		}

		inline int RGBIntensity(const Graphics::Color &Color)
		{
			return (Color.Red*19672+Color.Green*38621+Color.Blue*7500)>>16;
		}

		inline Graphics::Color GetHighlightColor(const Graphics::Color &Color)
		{
			return Graphics::MixColor(Graphics::Color(255,255,255,Color.Alpha),Color,48+RGBIntensity(Color)/3);
		}

		inline Graphics::Color GetShadowColor(const Graphics::Color &Color)
		{
			return Graphics::MixColor(Color,Graphics::Color(0,0,0,Color.Alpha),96+RGBIntensity(Color)/2);
		}

		bool CStylePainter::DrawBorder(const BorderStyle &Style,const RECT &Rect)
		{
			switch (Style.Type) {
			case BORDER_NONE:
				break;

			case BORDER_SOLID:
				m_pCanvas->DrawRect(Rect,Style.Color);
				break;

			case BORDER_SUNKEN:
				{
					POINT Points[3];

					Points[0].x=Rect.left;
					Points[0].y=Rect.bottom-1;
					Points[1].x=Rect.left;
					Points[1].y=Rect.top;
					Points[2].x=Rect.right-1;
					Points[2].y=Rect.top;
					m_pCanvas->DrawPolyline(Points,3,GetShadowColor(Style.Color));
					Points[0].x=Rect.right-1;
					Points[0].y=Rect.top+1;
					Points[1].x=Rect.right-1;
					Points[1].y=Rect.bottom-1;
					Points[2].x=Rect.left+1;
					Points[2].y=Rect.bottom-1;
					m_pCanvas->DrawPolyline(Points,3,GetHighlightColor(Style.Color));
				}
				break;

			case BORDER_RAISED:
				{
					POINT Points[3];

					Points[0].x=Rect.left;
					Points[0].y=Rect.bottom-1;
					Points[1].x=Rect.right-1;
					Points[1].y=Rect.bottom-1;
					Points[2].x=Rect.right-1;
					Points[2].y=Rect.top;
					m_pCanvas->DrawPolyline(Points,3,GetShadowColor(Style.Color));
					Points[0].x=Rect.right-1;
					Points[0].y=Rect.top;
					Points[1].x=Rect.left;
					Points[1].y=Rect.top;
					Points[2].x=Rect.left;
					Points[2].y=Rect.bottom-2;
					m_pCanvas->DrawPolyline(Points,3,GetHighlightColor(Style.Color));
				}
				break;

			default:
				return false;
			}

			return true;
		}

		bool CStylePainter::DrawBackground(const BackgroundStyle &Style,const RECT &Rect)
		{
			RECT rc=Rect;
			if (Style.Border.Type!=BORDER_NONE && Style.Border.Color.Alpha==255)
				DecreaseBorderRect(Style.Border,&rc);
			Fill(Style.Fill,rc);

			if ((!Style.Image.FileName.empty() || Style.Image.Data.Binary.GetSize()>0)
					&& Style.Image.Opacity>0) {
				Graphics::CImage *pImage;

				if (Style.Image.Data.Binary.GetSize()>0) {
					pImage=m_System.LoadPoolImage(Style.Image.Data.Binary.GetBuffer(),
												  Style.Image.Data.Binary.GetSize(),
												  Style.Image.Data.Hash);
				} else {
					pImage=m_System.LoadPoolImage(Style.Image.FileName.c_str());
				}

				if (pImage!=nullptr) {
					const int ImageWidth=pImage->GetWidth();
					const int ImageHeight=pImage->GetHeight();
					const int Width=rc.right-rc.left;
					const int Height=rc.bottom-rc.top;

					int DstWidth=ImageWidth,DstHeight=ImageHeight;
					switch (Style.Image.Fit) {
					case FIT_HORIZONTAL:
						DstWidth=rc.right-rc.left;
						break;
					case FIT_VERTICAL:
						DstHeight=rc.bottom-rc.top;
						break;
					case FIT_ALL:
						DstWidth=rc.right-rc.left;
						DstHeight=rc.bottom-rc.top;
						break;
					case FIT_HORIZONTAL_KEEP_ASPECT_RATIO:
						DstWidth=Width;
						DstHeight=max(ImageHeight*Width/ImageWidth,1);
						break;
					case FIT_VERTICAL_KEEP_ASPECT_RATIO:
						DstHeight=Height;
						DstWidth=max(ImageWidth*Height/ImageHeight,1);
						break;
					case FIT_INSCRIBE_KEEP_ASPECT_RATIO:
						DstWidth=min(ImageWidth*Height/ImageHeight,Width);
						DstHeight=min(ImageHeight*Width/ImageWidth,Height);
						break;
					case FIT_CIRCUMSCRIBE_KEEP_ASPECT_RATIO:
						DstWidth=max(ImageWidth*Height/ImageHeight,Width);
						DstHeight=max(ImageHeight*Width/ImageWidth,Height);
						break;
					}

					int DstX=rc.left,DstY=rc.top;
					if ((Style.Image.Align & ALIGN_RIGHT)!=0)
						DstX=rc.right-DstWidth;
					else if ((Style.Image.Align & ALIGN_HORIZONTAL_CENTER)!=0)
						DstX=rc.left+(Width-DstWidth)/2;
					if ((Style.Image.Align & ALIGN_BOTTOM)!=0)
						DstY=rc.bottom-DstHeight;
					else if ((Style.Image.Align & ALIGN_VERTICAL_CENTER)!=0)
						DstY=rc.top+(Height-DstHeight)/2;

					m_pCanvas->DrawImage(DstX,DstY,DstWidth,DstHeight,
										 pImage,0,0,ImageWidth,ImageHeight,
										 (float)Style.Image.Opacity/255.0f,
										 &rc);
				}
			}

			if (Style.Border.Type!=BORDER_NONE)
				DrawBorder(Style.Border,Rect);

			return true;
		}

		bool CStylePainter::DrawText(const ForegroundStyle &Style,LPCWSTR pszText,
									 const RECT &TextRect,const RECT &ClipRect,
									 const Graphics::CFont *pFont,UINT Flags)
		{
			if (TSTask::IsStringEmpty(pszText))
				return false;

			if (Style.Glow.Type!=GLOW_NONE) {
				if (Style.Fill.Type!=FILL_NONE && Style.AntiAliasing==ANTIALIASING_ANTIALIAS) {
					Graphics::CBrush *pBrush=CreateFillBrush(Style.Fill);
					if (pBrush==nullptr)
						return false;

					m_pCanvas->DrawTextGlow(pszText,TextRect,ClipRect,pFont,
											Style.Glow.Color,Style.Glow.Radius,Flags,pBrush);

					delete pBrush;

					return true;
				}

				m_pCanvas->DrawTextGlow(pszText,TextRect,ClipRect,pFont,
										Style.Glow.Color,Style.Glow.Radius,Flags);
			}

			if (Style.Fill.Type!=FILL_NONE) {
				Graphics::CBrush *pBrush=CreateFillBrush(Style.Fill);
				if (pBrush==nullptr)
					return false;

				switch (Style.AntiAliasing) {
				case ANTIALIASING_NONE:
					Flags|=Graphics::TEXT_DRAW_NO_ANTIALIAS;
					break;
				case ANTIALIASING_ANTIALIAS:
					Flags|=Graphics::TEXT_DRAW_ANTIALIAS;
					break;
				case ANTIALIASING_ANTIALIAS_HINTING:
					Flags|=Graphics::TEXT_DRAW_ANTIALIAS | Graphics::TEXT_DRAW_HINTING;
					break;
				}

				m_pCanvas->DrawText(pszText,TextRect,pFont,pBrush,Flags);

				delete pBrush;
			}

			return true;
		}

		Graphics::CBrush *CStylePainter::CreateFillBrush(const FillStyle &Style)
		{
			Graphics::CBrush *pBrush;

			switch (Style.Type) {
			case FILL_SOLID:
				pBrush=m_System.CreateBrush(Style.Solid.Color);
				break;

			case FILL_GRADIENT:
				pBrush=m_System.CreateGradientBrush(
					Style.Gradient.Color1,Style.Gradient.Color2,Style.Gradient.Direction);
				break;

			default:
				return nullptr;
			}

			return pBrush;
		}

	}

}
