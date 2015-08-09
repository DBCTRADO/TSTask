#include "stdafx.h"
#include <gdiplus.h>
#include <wincrypt.h>
#include "TSTaskCentre.h"
#include "Graphics.h"
#include "../BonTsEngine/TsUtilClass.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "crypt32.lib")


namespace TSTaskCentre
{

	namespace Graphics
	{

		Color MixColor(const Color &color1,const Color &color2,int Ratio)
		{
			return Color((color1.Red*Ratio+color2.Red*(255-Ratio))/255,
						 (color1.Green*Ratio+color2.Green*(255-Ratio))/255,
						 (color1.Blue*Ratio+color2.Blue*(255-Ratio))/255,
						 (color1.Alpha*Ratio+color2.Alpha*(255-Ratio))/255);
		}

		bool GetSystemFont(SystemFontType Type,LOGFONT *pFont,bool fPreferUIFont)
		{
			if (pFont==nullptr)
				return false;

			NONCLIENTMETRICS ncm;

			ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
			if (!::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0)) {
				::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pFont);
				return false;
			}

			switch (Type) {
			case SYSTEM_FONT_MESSAGE:		*pFont=ncm.lfMessageFont;	break;
			case SYSTEM_FONT_MENU:			*pFont=ncm.lfMenuFont;		break;
			case SYSTEM_FONT_CAPTION:		*pFont=ncm.lfCaptionFont;	break;
			case SYSTEM_FONT_SMALL_CAPTION:	*pFont=ncm.lfSmCaptionFont;	break;
			case SYSTEM_FONT_STATUS:		*pFont=ncm.lfStatusFont;	break;
			default:
				return false;
			}

			if (fPreferUIFont) {
				if (::lstrcmp(pFont->lfFaceName,TEXT("メイリオ"))==0
						|| ::lstrcmpi(pFont->lfFaceName,TEXT("Meiryo"))==0)
					::lstrcpy(pFont->lfFaceName,TEXT("Meiryo UI"));
			}

			return true;
		}

		bool GetFontMetrics(HDC hdc,const LOGFONT &Font,TEXTMETRIC *pMetrics)
		{
			if (hdc==nullptr || pMetrics==nullptr)
				return false;

			HFONT hfont=::CreateFontIndirect(&Font);
			HFONT hfontOld=SelectFont(hdc,hfont);
			::GetTextMetrics(hdc,pMetrics);
			SelectFont(hdc,hfontOld);
			::DeleteObject(hfont);

			return true;
		}

		int GetFontPointSize(HDC hdc,const LOGFONT &Font)
		{
			if (hdc==NULL)
				return 0;

			HFONT hfont=::CreateFontIndirect(&Font);
			if (hfont==NULL)
				return 0;

			HFONT hfontOld=SelectFont(hdc,hfont);
			TEXTMETRIC tm;
			::GetTextMetrics(hdc,&tm);
			int PixelsPerInch=::GetDeviceCaps(hdc,LOGPIXELSY);
			SelectFont(hdc,hfontOld);
			::DeleteObject(hfont);
			if (PixelsPerInch==0)
				return 0;

			return ::MulDiv(tm.tmHeight-tm.tmInternalLeading,72,PixelsPerInch);
		}


		CFont::~CFont()
		{
		}


		CBrush::~CBrush()
		{
		}


		CPen::~CPen()
		{
		}


		CImage::~CImage()
		{
		}


		CCanvas::~CCanvas()
		{
		}


		COffscreen::~COffscreen()
		{
		}


		static inline Gdiplus::Color GdiplusColor(const Color &color)
		{
			return Gdiplus::Color(color.Alpha,color.Red,color.Green,color.Blue);
		}

		static inline Gdiplus::Rect GdiplusRect(const RECT &Rect)
		{
			return Gdiplus::Rect(Rect.left,Rect.top,Rect.right-Rect.left,Rect.bottom-Rect.top);
		}

		static inline Gdiplus::RectF GdiplusRectF(const RECT &Rect)
		{
			return Gdiplus::RectF((Gdiplus::REAL)Rect.left,
								  (Gdiplus::REAL)Rect.top,
								  (Gdiplus::REAL)(Rect.right-Rect.left),
								  (Gdiplus::REAL)(Rect.bottom-Rect.top));
		}

		static inline Gdiplus::Point GdiplusPoint(const POINT &Point)
		{
			return Gdiplus::Point(Point.x,Point.y);
		}


		class CGdiplusFont : public CFont
		{
		public:
			CGdiplusFont(const LOGFONT &LogFont);
			~CGdiplusFont();
			const Gdiplus::Font *GetFont() const { return &m_Font; }
			bool GetFontFamily(Gdiplus::FontFamily *pFamily) const {
				return m_Font.GetFamily(pFamily)==Gdiplus::Ok;
			}
			Gdiplus::REAL GetSize() const { return m_Font.GetSize(); }
			int GetStyle() const { return m_Font.GetStyle(); }
			Gdiplus::REAL GetHeight(const Gdiplus::Graphics &Graphics) const {
				return m_Font.GetHeight(&Graphics);
			}

		private:
			Gdiplus::Font m_Font;
		};

		CGdiplusFont::CGdiplusFont(const LOGFONT &LogFont)
			: m_Font(LogFont.lfFaceName,
					 (Gdiplus::REAL)std::abs(LogFont.lfHeight),
					 (LogFont.lfWeight>=FW_BOLD?Gdiplus::FontStyleBold:0) |
					 (LogFont.lfItalic?Gdiplus::FontStyleItalic:0) |
					 (LogFont.lfUnderline?Gdiplus::FontStyleUnderline:0) |
					 (LogFont.lfStrikeOut?Gdiplus::FontStyleStrikeout:0),
					 Gdiplus::UnitPixel)
		{
		}

		CGdiplusFont::~CGdiplusFont()
		{
		}


		class CGdiplusBrush : public CBrush
		{
		public:
			CGdiplusBrush(const Color &color);
			CGdiplusBrush(const Color &color1,const Color &color2,GradientDirection Direction);
			~CGdiplusBrush();
			const Gdiplus::Brush *GetBrush() const { return m_pBrush; }
			bool FillRect(Gdiplus::Graphics &Graphics,const RECT &Rect) const;
			void SetRect(const RECT &Rect) const;

		private:
			Gdiplus::Brush *m_pBrush;
			GradientDirection m_Direction;
		};

		CGdiplusBrush::CGdiplusBrush(const Color &color)
			: m_pBrush(new Gdiplus::SolidBrush(GdiplusColor(color)))
		{
		}

		CGdiplusBrush::CGdiplusBrush(const Color &color1,const Color &color2,GradientDirection Direction)
			: m_Direction(Direction)
		{
			Gdiplus::LinearGradientBrush *pBrush;

			pBrush=new Gdiplus::LinearGradientBrush(Gdiplus::Rect(0,0,1,1),
													GdiplusColor(color1),GdiplusColor(color2),
													Gdiplus::LinearGradientModeHorizontal);
			pBrush->SetWrapMode(Gdiplus::WrapModeClamp);

			m_pBrush=pBrush;
		}

		CGdiplusBrush::~CGdiplusBrush()
		{
			delete m_pBrush;
		}

		bool CGdiplusBrush::FillRect(Gdiplus::Graphics &Graphics,const RECT &Rect) const
		{
			SetRect(Rect);

			return Graphics.FillRectangle(m_pBrush,GdiplusRect(Rect))==Gdiplus::Ok;
		}

		void CGdiplusBrush::SetRect(const RECT &Rect) const
		{
			if (m_pBrush->GetType()==Gdiplus::BrushTypeLinearGradient) {
				Gdiplus::LinearGradientBrush *pGradientBrush=
					dynamic_cast<Gdiplus::LinearGradientBrush*>(m_pBrush);

				if (pGradientBrush!=nullptr) {
					pGradientBrush->ResetTransform();
					if (m_Direction==DIRECTION_VERTICAL)
						pGradientBrush->RotateTransform(90.0f);
					pGradientBrush->ScaleTransform(Gdiplus::REAL(Rect.right-Rect.left)+1.0f,
												   Gdiplus::REAL(Rect.bottom-Rect.top)+1.0f,
												   Gdiplus::MatrixOrderAppend);
					pGradientBrush->TranslateTransform(Gdiplus::REAL(Rect.left)-0.5f,
													   Gdiplus::REAL(Rect.top)-0.5f,
													   Gdiplus::MatrixOrderAppend);
				}
			}
		}


		class CGdiplusPen : public CPen
		{
		public:
			CGdiplusPen(const Color &color,float Width);
			const Gdiplus::Pen *GetPen() const { return &m_Pen; }

		private:
			Gdiplus::Pen m_Pen;
		};

		CGdiplusPen::CGdiplusPen(const Color &color,float Width)
			: m_Pen(GdiplusColor(color),Width)
		{
		}


		class CGdiplusImage : public CImage
		{
		public:
			CGdiplusImage();
			~CGdiplusImage();
			void Free() override;
			bool LoadFromFile(LPCWSTR pszFileName) override;
			bool LoadFromMemory(const void *pData,size_t Size) override;
			bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName) override;
			bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName,LPCWSTR pszType) override;
			bool Create(int Width,int Height,int BitsPerPixel) override;
			bool CreateFromIcon(HICON hico) override;
			bool IsCreated() const override;
			int GetWidth() const override;
			int GetHeight() const override;
			Gdiplus::Bitmap *GetBitmap() const { return m_pBitmap; }

		private:
			Gdiplus::Bitmap *m_pBitmap;
		};

		CGdiplusImage::CGdiplusImage()
			: m_pBitmap(nullptr)
		{
		}

		CGdiplusImage::~CGdiplusImage()
		{
			Free();
		}

		void CGdiplusImage::Free()
		{
			TSTask::SafeDelete(m_pBitmap);
		}

		bool CGdiplusImage::LoadFromFile(LPCWSTR pszFileName)
		{
			if (TSTask::IsStringEmpty(pszFileName))
				return false;

			Free();

			m_pBitmap=Gdiplus::Bitmap::FromFile(pszFileName);

			return m_pBitmap!=nullptr;
		}

		bool CGdiplusImage::LoadFromMemory(const void *pData,size_t Size)
		{
			if (pData==nullptr || Size==0)
				return false;

			Free();

			HGLOBAL hBuffer=::GlobalAlloc(GMEM_MOVEABLE,Size);
			if (hBuffer==nullptr)
				return false;
			void *pBuffer=::GlobalLock(hBuffer);
			if (pBuffer==nullptr) {
				::GlobalFree(hBuffer);
				return false;
			}
			::CopyMemory(pBuffer,pData,Size);
			::GlobalUnlock(hBuffer);

			IStream *pStream;
			if (::CreateStreamOnHGlobal(hBuffer,TRUE,&pStream)!=S_OK) {
				::GlobalFree(hBuffer);
				return false;
			}
			m_pBitmap=Gdiplus::Bitmap::FromStream(pStream);
			pStream->Release();

			return m_pBitmap!=nullptr;
		}

		bool CGdiplusImage::LoadFromResource(HINSTANCE hinst,LPCWSTR pszName)
		{
			if (hinst==nullptr || pszName==nullptr)
				return false;

			Free();

			m_pBitmap=Gdiplus::Bitmap::FromResource(hinst,pszName);

			return m_pBitmap!=nullptr;
		}

		bool CGdiplusImage::LoadFromResource(HINSTANCE hinst,LPCWSTR pszName,LPCWSTR pszType)
		{
			if (hinst==nullptr || pszName==nullptr || TSTask::IsStringEmpty(pszType))
				return false;

			Free();

			HRSRC hRes=::FindResource(hinst,pszName,pszType);
			if (hRes==nullptr)
				return false;
			DWORD Size=::SizeofResource(hinst,hRes);
			if (Size==0)
				return false;
			HGLOBAL hData=::LoadResource(hinst,hRes);
			const void *pData=::LockResource(hData);
			if (pData==nullptr)
				return false;

			return LoadFromMemory(pData,Size);
		}

		bool CGdiplusImage::Create(int Width,int Height,int BitsPerPixel)
		{
			Free();

			if (Width<=0 || Height<=0)
				return false;

			Gdiplus::PixelFormat Format;
			switch (BitsPerPixel) {
			case 1:		Format=PixelFormat1bppIndexed;	break;
			case 4:		Format=PixelFormat4bppIndexed;	break;
			case 8:		Format=PixelFormat8bppIndexed;	break;
			case 24:	Format=PixelFormat24bppRGB;		break;
			case 32:	Format=PixelFormat32bppARGB;	break;
			default:	return false;
			}

			m_pBitmap=new Gdiplus::Bitmap(Width,Height,Format);
			if (m_pBitmap==nullptr)
				return false;

			return true;
		}

		bool CGdiplusImage::CreateFromIcon(HICON hico)
		{
			Free();

			if (hico==nullptr)
				return false;

			m_pBitmap=Gdiplus::Bitmap::FromHICON(hico);

			return m_pBitmap!=nullptr;
		}

		bool CGdiplusImage::IsCreated() const
		{
			return m_pBitmap!=nullptr;
		}

		int CGdiplusImage::GetWidth() const
		{
			if (m_pBitmap==nullptr)
				return 0;

			return m_pBitmap->GetWidth();
		}

		int CGdiplusImage::GetHeight() const
		{
			if (m_pBitmap==nullptr)
				return 0;

			return m_pBitmap->GetHeight();
		}


		class CGdiplusCanvas : public CCanvas
		{
		public:
			CGdiplusCanvas(HDC hdc);
			CGdiplusCanvas(Gdiplus::Image *pImage);
			~CGdiplusCanvas();
			bool Clear(const Color &color) override;
			bool SetOverwriteMode(bool fOverwrite) override;
			bool SetClip(const RECT &Rect) override;
			bool ResetClip() override;
			bool FillRect(const RECT &Rect,const CBrush *pBrush) override;
			bool FillRect(const RECT &Rect,const Color &color) override;
			bool DrawRect(const RECT &Rect,const CPen *pPen) override;
			bool DrawRect(const RECT &Rect,const Color &color,int Width) override;
			bool DrawLine(int x1,int y1,int x2,int y2,const CPen *pPen) override;
			bool DrawLine(int x1,int y1,int x2,int y2,const Color &color,int Width) override;
			bool DrawPolyline(const POINT *pPoints,int NumPoints,const CPen *pPen) override;
			bool DrawPolyline(const POINT *pPoints,int NumPoints,const Color &color,int Width) override;
			bool DrawText(LPCWSTR pszText,const RECT &Rect,
						  const CFont *pFont,const CBrush *pBrush,
						  UINT Flags) override;
			bool DrawTextGlow(LPCWSTR pszText,const RECT &TextRect,const RECT &ClipRect,
							  const CFont *pFont,const Color &color,int Radius,
							  UINT Flags,const CBrush *pBrush=nullptr) override;
			bool DrawImage(int x,int y,const CImage *pImage) override;
			bool DrawImage(int DstX,int DstY,int DstWidth,int DstHeight,
						   const CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,
						   float Opacity,const RECT *pClipRect) override;

		private:
			void SetStringFormat(Gdiplus::StringFormat &Format,UINT Flags) const;

			Gdiplus::Graphics m_Graphics;
		};

		CGdiplusCanvas::CGdiplusCanvas(HDC hdc)
			: m_Graphics(hdc)
		{
			m_Graphics.SetPageUnit(Gdiplus::UnitPixel);
		}

		CGdiplusCanvas::CGdiplusCanvas(Gdiplus::Image *pImage)
			: m_Graphics(pImage)
		{
			m_Graphics.SetPageUnit(Gdiplus::UnitPixel);
			m_Graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
		}

		CGdiplusCanvas::~CGdiplusCanvas()
		{
		}

		bool CGdiplusCanvas::Clear(const Color &color)
		{
			return m_Graphics.Clear(GdiplusColor(color))==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::SetOverwriteMode(bool fOverwrite)
		{
			return m_Graphics.SetCompositingMode(fOverwrite?
												 Gdiplus::CompositingModeSourceCopy:
												 Gdiplus::CompositingModeSourceOver)==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::SetClip(const RECT &Rect)
		{
			return m_Graphics.SetClip(GdiplusRect(Rect))==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::ResetClip()
		{
			return m_Graphics.ResetClip()==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::FillRect(const RECT &Rect,const CBrush *pBrush)
		{
			if (pBrush==nullptr)
				return false;
			return static_cast<const CGdiplusBrush*>(pBrush)->FillRect(m_Graphics,Rect);
		}

		bool CGdiplusCanvas::FillRect(const RECT &Rect,const Color &color)
		{
			Gdiplus::SolidBrush Brush(GdiplusColor(color));

			return m_Graphics.FillRectangle(&Brush,GdiplusRect(Rect))==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawRect(const RECT &Rect,const CPen *pPen)
		{
			if (pPen==nullptr)
				return false;
			return m_Graphics.DrawRectangle(static_cast<const CGdiplusPen*>(pPen)->GetPen(),
											GdiplusRect(Rect))==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawRect(const RECT &Rect,const Color &color,int Width)
		{
			if (Width<=0)
				return false;

			Gdiplus::Pen Pen(GdiplusColor(color),(Gdiplus::REAL)Width);

			//return m_Graphics.DrawRectangle(&Pen,GdiplusRect(Rect))==Gdiplus::Ok;
			return m_Graphics.DrawRectangle(&Pen,
											Rect.left,Rect.top,
											Rect.right-Rect.left-1,
											Rect.bottom-Rect.top-1)==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawLine(int x1,int y1,int x2,int y2,const CPen *pPen)
		{
			if (pPen==nullptr)
				return false;
			return m_Graphics.DrawLine(static_cast<const CGdiplusPen*>(pPen)->GetPen(),x1,y1,x2,y2)==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawLine(int x1,int y1,int x2,int y2,const Color &color,int Width)
		{
			Gdiplus::Pen Pen(GdiplusColor(color),(Gdiplus::REAL)Width);

			return m_Graphics.DrawLine(&Pen,x1,y1,x2,y2)==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawPolyline(const POINT *pPoints,int NumPoints,const CPen *pPen)
		{
			if (pPoints==nullptr || NumPoints<2 || pPen==nullptr)
				return false;

			std::vector<Gdiplus::Point> Points;
			Points.reserve(NumPoints);
			for (int i=0;i<NumPoints;i++)
				Points.push_back(GdiplusPoint(pPoints[i]));

			return m_Graphics.DrawLines(static_cast<const CGdiplusPen*>(pPen)->GetPen(),
										&Points[0],NumPoints)==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawPolyline(const POINT *pPoints,int NumPoints,const Color &color,int Width)
		{
			CGdiplusPen Pen(color,(Gdiplus::REAL)Width);

			return DrawPolyline(pPoints,NumPoints,&Pen);
		}

		bool CGdiplusCanvas::DrawText(LPCWSTR pszText,const RECT &Rect,
									  const CFont *pFont,const CBrush *pBrush,
									  UINT Flags)
		{
			if (TSTask::IsStringEmpty(pszText) || pFont==nullptr || pBrush==nullptr)
				return false;

			Gdiplus::StringFormat Format((Flags&TEXT_FORMAT_NO_WRAP)!=0?Gdiplus::StringFormatFlagsNoWrap:0);
			SetStringFormat(Format,Flags);

			const CGdiplusBrush *pGdipBrush=static_cast<const CGdiplusBrush*>(pBrush);
			pGdipBrush->SetRect(Rect);

			m_Graphics.SetTextRenderingHint((Flags&TEXT_DRAW_ANTIALIAS)!=0?
											 ((Flags&TEXT_DRAW_HINTING)!=0?
											  Gdiplus::TextRenderingHintAntiAliasGridFit:
											  Gdiplus::TextRenderingHintAntiAlias):
											(Flags&TEXT_DRAW_NO_ANTIALIAS)!=0?
											 ((Flags&TEXT_DRAW_HINTING)!=0?
											  Gdiplus::TextRenderingHintSingleBitPerPixelGridFit:
											  Gdiplus::TextRenderingHintSingleBitPerPixel):
											Gdiplus::TextRenderingHintClearTypeGridFit);

			return m_Graphics.DrawString(pszText,-1,
										 static_cast<const CGdiplusFont*>(pFont)->GetFont(),
										 GdiplusRectF(Rect),
										 &Format,
										 pGdipBrush->GetBrush())==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawTextGlow(LPCWSTR pszText,const RECT &TextRect,const RECT &ClipRect,
										  const CFont *pFont,const Color &color,int Radius,
										  UINT Flags,const CBrush *pBrush)
		{
			if (TSTask::IsStringEmpty(pszText) || pFont==nullptr || Radius<1)
				return false;

			Gdiplus::StringFormat Format((Flags&TEXT_FORMAT_NO_WRAP)!=0?Gdiplus::StringFormatFlagsNoWrap:0);
			SetStringFormat(Format,Flags);

			const CGdiplusFont *pGdipFont=static_cast<const CGdiplusFont*>(pFont);

			Gdiplus::GraphicsPath Path;
			Gdiplus::FontFamily FontFamily;
			pGdipFont->GetFontFamily(&FontFamily);
			if (Path.AddString(pszText,-1,
							   &FontFamily,pGdipFont->GetStyle(),pGdipFont->GetSize(),
							   GdiplusRect(TextRect),&Format)!=Gdiplus::Ok)
				return false;

			m_Graphics.SetClip(GdiplusRect(ClipRect));

			Gdiplus::SmoothingMode OldSmoothingMode=m_Graphics.GetSmoothingMode();
			m_Graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

			Gdiplus::Pen Pen(Gdiplus::Color(),1.0f);
			Pen.SetLineJoin(Gdiplus::LineJoinRound);
			for (int i=1;i<=Radius;i++) {
				BYTE Alpha=color.Alpha/Radius;
				if (Alpha>0) {
					Pen.SetColor(Gdiplus::Color(Alpha,color.Red,color.Green,color.Blue));
					Pen.SetWidth(Gdiplus::REAL(i));
					m_Graphics.DrawPath(&Pen,&Path);
				}
			}

			if (pBrush!=nullptr) {
				const CGdiplusBrush *pGdipBrush=static_cast<const CGdiplusBrush*>(pBrush);
				pGdipBrush->SetRect(TextRect);
				m_Graphics.FillPath(pGdipBrush->GetBrush(),&Path);
			}

			m_Graphics.SetSmoothingMode(OldSmoothingMode);

			m_Graphics.ResetClip();

			return true;
		}

		void CGdiplusCanvas::SetStringFormat(Gdiplus::StringFormat &Format,UINT Flags) const
		{
			switch (Flags&TEXT_FORMAT_HORIZONTAL_ALIGN_MASK) {
			case TEXT_FORMAT_LEFT:				Format.SetAlignment(Gdiplus::StringAlignmentNear);		break;
			case TEXT_FORMAT_RIGHT:				Format.SetAlignment(Gdiplus::StringAlignmentFar);		break;
			case TEXT_FORMAT_HORIZONTAL_CENTER:	Format.SetAlignment(Gdiplus::StringAlignmentCenter);	break;
			}
			switch (Flags&TEXT_FORMAT_VERTICAL_ALIGN_MASK) {
			case TEXT_FORMAT_TOP:				Format.SetLineAlignment(Gdiplus::StringAlignmentNear);		break;
			case TEXT_FORMAT_BOTTOM:			Format.SetLineAlignment(Gdiplus::StringAlignmentFar);		break;
			case TEXT_FORMAT_VERTICAL_CENTER:	Format.SetLineAlignment(Gdiplus::StringAlignmentCenter);	break;
			}
			Format.SetTrimming((Flags&TEXT_FORMAT_END_ELLIPSIS)!=0?Gdiplus::StringTrimmingEllipsisCharacter:
							   (Flags&TEXT_FORMAT_WORD_ELLIPSIS)!=0?Gdiplus::StringTrimmingEllipsisWord:
							   Gdiplus::StringTrimmingCharacter);
		}

		bool CGdiplusCanvas::DrawImage(int x,int y,const CImage *pImage)
		{
			const CGdiplusImage *pGdipImage=dynamic_cast<const CGdiplusImage*>(pImage);
			if (pGdipImage==nullptr)
				return false;

			return m_Graphics.DrawImage(pGdipImage->GetBitmap(),x,y,
										pGdipImage->GetWidth(),
										pGdipImage->GetHeight())==Gdiplus::Ok;
		}

		bool CGdiplusCanvas::DrawImage(int DstX,int DstY,int DstWidth,int DstHeight,
									   const CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,
									   float Opacity,const RECT *pClipRect)
		{
			if (DstWidth<=0 || DstHeight<=0 || SrcWidth<=0 || SrcHeight<=0 || Opacity<=0.0f)
				return false;

			const CGdiplusImage *pGdipImage=dynamic_cast<const CGdiplusImage*>(pImage);
			if (pGdipImage==nullptr)
				return false;

			Gdiplus::Region ClipRegion;
			bool fClipValid;
			if (pClipRect!=nullptr) {
				fClipValid=!m_Graphics.IsClipEmpty() && m_Graphics.GetClip(&ClipRegion)==Gdiplus::Ok;
				m_Graphics.SetClip(GdiplusRect(*pClipRect),Gdiplus::CombineModeUnion);
			}

			Gdiplus::ImageAttributes Attributes;
			if (Opacity!=1.0f) {
				Gdiplus::ColorMatrix Matrix = {
					1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		 		};
				Matrix.m[3][3]=Opacity;
				Attributes.SetColorMatrix(&Matrix);
			}

			bool fResult=m_Graphics.DrawImage(pGdipImage->GetBitmap(),
											  Gdiplus::Rect(DstX,DstY,DstWidth,DstHeight),
											  SrcX,SrcY,SrcWidth,SrcHeight,
											  Gdiplus::UnitPixel,
											  Opacity!=1.0f?&Attributes:nullptr)==Gdiplus::Ok;

			if (pClipRect!=nullptr) {
				if (fClipValid)
					m_Graphics.SetClip(&ClipRegion);
				else
					m_Graphics.ResetClip();
			}

			return fResult;
		}


		class CGdiplusOffscreen : public COffscreen
		{
		public:
			CGdiplusOffscreen();
			~CGdiplusOffscreen();
			bool Create(int Width,int Height) override;
			void Free();
			CCanvas *GetCanvas() const override { return m_pCanvas; }
			CImage *GetImage() const override { return m_pImage; }
			int GetWidth() const override;
			int GetHeight() const override;
			bool Clear() override;
			bool Transfer(CCanvas *pCanvas,const RECT &PaintRect) override;

		private:
			CGdiplusCanvas *m_pCanvas;
			CGdiplusImage *m_pImage;
		};

		CGdiplusOffscreen::CGdiplusOffscreen()
			: m_pCanvas(nullptr)
			, m_pImage(nullptr)
		{
		}

		CGdiplusOffscreen::~CGdiplusOffscreen()
		{
			Free();
		}

		bool CGdiplusOffscreen::Create(int Width,int Height)
		{
			if (m_pImage!=nullptr) {
				if (Width<=m_pImage->GetWidth() && Height<=m_pImage->GetHeight())
					return true;
				Free();
			}

			if (Width<=0 || Height<=0)
				return false;

			m_pImage=new CGdiplusImage;
			if (!m_pImage->Create(Width,Height,32)) {
				Free();
				return false;
			}

			m_pCanvas=new CGdiplusCanvas(m_pImage->GetBitmap());

			return true;
		}

		void CGdiplusOffscreen::Free()
		{
			TSTask::SafeDelete(m_pCanvas);
			TSTask::SafeDelete(m_pImage);
		}

		int CGdiplusOffscreen::GetWidth() const
		{
			if (m_pImage==nullptr)
				return 0;
			return m_pImage->GetWidth();
		}

		int CGdiplusOffscreen::GetHeight() const
		{
			if (m_pImage==nullptr)
				return 0;
			return m_pImage->GetHeight();
		}

		bool CGdiplusOffscreen::Clear()
		{
			if (m_pCanvas==nullptr)
				return false;

			return m_pCanvas->Clear(Color());
		}

		bool CGdiplusOffscreen::Transfer(CCanvas *pCanvas,const RECT &PaintRect)
		{
			if (pCanvas==nullptr || m_pImage==nullptr
					|| PaintRect.left>=PaintRect.right
					|| PaintRect.top>=PaintRect.bottom)
				return false;

			pCanvas->SetOverwriteMode(true);

			bool fResult=pCanvas->DrawImage(PaintRect.left,PaintRect.top,
											PaintRect.right-PaintRect.left,
											PaintRect.bottom-PaintRect.top,
											m_pImage,
											PaintRect.left,PaintRect.top,
											PaintRect.right-PaintRect.left,
											PaintRect.bottom-PaintRect.top);

			pCanvas->SetOverwriteMode(false);

			return fResult;
		}


		bool CSystem::m_fInitialized=false;
		ULONG_PTR CSystem::m_Token=0;

		bool CSystem::Initialize()
		{
			if (!m_fInitialized) {
				Gdiplus::GdiplusStartupInput StartupInput;

				if (Gdiplus::GdiplusStartup(&m_Token,&StartupInput,nullptr)!=Gdiplus::Ok)
					return false;

				m_fInitialized=true;
			}
			return true;
		}

		void CSystem::Finalize()
		{
			if (m_fInitialized) {
				Gdiplus::GdiplusShutdown(m_Token);
				m_fInitialized=false;
			}
		}

		CSystem::CSystem()
		{
		}

		CSystem::~CSystem()
		{
			ClearImagePool();
		}

		CCanvas *CSystem::CreateCanvas(HDC hdc)
		{
			return new CGdiplusCanvas(hdc);
		}

		CCanvas *CSystem::CreateCanvas(CImage *pImage)
		{
			CGdiplusImage *pGdipImage=dynamic_cast<CGdiplusImage*>(pImage);
			if (pGdipImage==nullptr || pGdipImage->GetBitmap()==nullptr)
				return false;
			return new CGdiplusCanvas(pGdipImage->GetBitmap());
		}

		bool CSystem::CreateOffscreen(COffscreen **ppOffscreen,int Width,int Height)
		{
			if (*ppOffscreen!=nullptr) {
				if (!(*ppOffscreen)->Create(Width,Height))
					return false;
			} else {
				CGdiplusOffscreen *pOffscreen=new CGdiplusOffscreen;
				if (!pOffscreen->Create(Width,Height)) {
					delete pOffscreen;
					return false;
				}
				*ppOffscreen=pOffscreen;
			}

			return true;
		}

		CFont *CSystem::CreateFont(const LOGFONT &LogFont)
		{
			return new CGdiplusFont(LogFont);
		}

		CBrush *CSystem::CreateBrush(const Color &color)
		{
			return new CGdiplusBrush(color);
		}

		CBrush *CSystem::CreateGradientBrush(const Color &color1,const Color &color2,GradientDirection Direction)
		{
			return new CGdiplusBrush(color1,color2,Direction);
		}

		CPen *CSystem::CreatePen(const Color &color,int Width)
		{
			return new CGdiplusPen(color,(float)Width);
		}

		CImage *CSystem::CreateImage()
		{
			return new CGdiplusImage();
		}

		void CSystem::ClearImagePool()
		{
			for (auto &e:m_ImagePool)
				delete e.second;
			m_ImagePool.clear();
		}

		CImage *CSystem::LoadPoolImage(LPCWSTR pszFileName)
		{
			if (TSTask::IsStringEmpty(pszFileName))
				return nullptr;

			TSTask::String FileName(pszFileName);
			TSTask::StringUtility::ToLower(FileName);

			auto i=m_ImagePool.find(FileName);
			if (i!=m_ImagePool.end())
				return i->second;

			TRACE(L"Graphics::CSystem::LoadPoolIamge : %s\n",pszFileName);

			CGdiplusImage *pImage=new CGdiplusImage;
			if (!pImage->LoadFromFile(pszFileName)) {
				delete pImage;
				pImage=nullptr;
				// 描画の度に読み込みを試行しないように、nullptr を map に入れておく
			}

			m_ImagePool.insert(std::pair<TSTask::String,CImage*>(FileName,pImage));

			return pImage;
		}

		static void HashString(const CSystem::ImageHash &Hash,TSTask::String &Str)
		{
			TSTask::StringUtility::Format(
				Str,L":%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				Hash.Value[ 0],Hash.Value[ 1],Hash.Value[ 2],Hash.Value[ 3],
				Hash.Value[ 4],Hash.Value[ 5],Hash.Value[ 6],Hash.Value[ 7],
				Hash.Value[ 8],Hash.Value[ 9],Hash.Value[10],Hash.Value[11],
				Hash.Value[12],Hash.Value[13],Hash.Value[14],Hash.Value[15]);
		}

		CImage *CSystem::LoadPoolImage(const void *pData,size_t Size,const ImageHash &Hash)
		{
			if (pData==nullptr || Size==0)
				return nullptr;

			TSTask::String Key;
			HashString(Hash,Key);

			auto i=m_ImagePool.find(Key);
			if (i!=m_ImagePool.end())
				return i->second;

			TRACE(L"Graphics::CSystem::LoadPoolIamge : %s\n",Key.c_str());

			CGdiplusImage *pImage=new CGdiplusImage;
			if (!pImage->LoadFromMemory(pData,Size)) {
				delete pImage;
				pImage=nullptr;
				// 描画の度に読み込みを試行しないように、nullptr を map に入れておく
			}

			m_ImagePool.insert(std::pair<TSTask::String,CImage*>(Key,pImage));

			return pImage;
		}

		CImage *CSystem::GetPoolImage(LPCWSTR pszFileName) const
		{
			if (TSTask::IsStringEmpty(pszFileName))
				return nullptr;

			TSTask::String FileName(pszFileName);
			TSTask::StringUtility::ToLower(FileName);

			auto i=m_ImagePool.find(FileName);
			if (i==m_ImagePool.end())
				return nullptr;

			return i->second;
		}

		CImage *CSystem::GetPoolImage(const ImageHash &Hash) const
		{
			TSTask::String Key;
			HashString(Hash,Key);

			auto i=m_ImagePool.find(Key);
			if (i==m_ImagePool.end())
				return nullptr;

			return i->second;
		}

		bool CSystem::GetImageHash(const void *pData,size_t Size,ImageHash *pHash)
		{
			if (pData==nullptr || pHash==nullptr)
				return false;

			bool fOK=false;
			HCRYPTPROV hProv;
			if (::CryptAcquireContext(&hProv,nullptr,nullptr,PROV_RSA_FULL,
									  CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) {
				HCRYPTHASH hHash;
				if (::CryptCreateHash(hProv,CALG_MD5,0,0,&hHash)) {
					if (::CryptHashData(hHash,
										const_cast<BYTE*>(static_cast<const BYTE*>(pData)),
										static_cast<DWORD>(Size),
										0)) {
						DWORD Size=sizeof(pHash->Value);
						if (::CryptGetHashParam(hHash,HP_HASHVAL,pHash->Value,&Size,0))
							fOK=true;
					}
					::CryptDestroyHash(hHash);
				}
				::CryptReleaseContext(hProv,0);
			}

			return fOK;
		}


		CDoubleBufferedPainter::CDoubleBufferedPainter(CSystem &System)
			: m_System(System)
			, m_pDCCanvas(nullptr)
			, m_pOffscreen(nullptr)
		{
		}

		CDoubleBufferedPainter::~CDoubleBufferedPainter()
		{
		}

		CCanvas *CDoubleBufferedPainter::BeginPaint(HDC hdc,const RECT &PaintRect,COffscreen **ppOffscreen)
		{
			TSTask::SafeDelete(m_pDCCanvas);

			m_pDCCanvas=m_System.CreateCanvas(hdc);
			m_PaintRect=PaintRect;

			CCanvas *pCanvas;
			RECT rc;
			::GetClientRect(::WindowFromDC(hdc),&rc);
			if (m_System.CreateOffscreen(ppOffscreen,rc.right,rc.bottom)) {
				m_pOffscreen=*ppOffscreen;
				pCanvas=m_pOffscreen->GetCanvas();
			} else {
				m_pOffscreen=nullptr;
				pCanvas=m_pDCCanvas;
			}

			return pCanvas;
		}

		void CDoubleBufferedPainter::EndPaint()
		{
			if (m_pOffscreen!=nullptr) {
				m_pOffscreen->Transfer(m_pDCCanvas,m_PaintRect);
				m_pOffscreen=nullptr;
			}

			TSTask::SafeDelete(m_pDCCanvas);
		}

	}

}
