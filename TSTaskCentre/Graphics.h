#ifndef TSTASKCENTRE_GRAPHICS_H
#define TSTASKCENTRE_GRAPHICS_H


#include <map>


namespace TSTaskCentre
{

	namespace Graphics
	{

		struct Color
		{
			BYTE Red;
			BYTE Green;
			BYTE Blue;
			BYTE Alpha;

			Color() : Color(0,0,0,0) {}
			Color(BYTE R,BYTE G,BYTE B,BYTE A=255) : Red(R), Green(G), Blue(B), Alpha(A) {}
			Color(COLORREF c) : Color(GetRValue(c),GetGValue(c),GetBValue(c)) {}
			void Set(BYTE R,BYTE G,BYTE B,BYTE A=255) { Red=R; Green=G; Blue=B; Alpha=A; }
			void Set(COLORREF c) { Red=GetRValue(c); Green=GetGValue(c); Blue=GetBValue(c); Alpha=255; }
		};

		Color MixColor(const Color &color1,const Color &color2,int Ratio=128);

		enum SystemFontType
		{
			SYSTEM_FONT_MESSAGE,
			SYSTEM_FONT_MENU,
			SYSTEM_FONT_CAPTION,
			SYSTEM_FONT_SMALL_CAPTION,
			SYSTEM_FONT_STATUS
		};

		bool GetSystemFont(SystemFontType Type,LOGFONT *pFont,bool fPreferUIFont=false);
		bool GetFontMetrics(HDC hdc,const LOGFONT &Font,TEXTMETRIC *pMetrics);
		int GetFontPointSize(HDC hdc,const LOGFONT &Font);

		enum GradientDirection
		{
			DIRECTION_HORIZONTAL,
			DIRECTION_VERTICAL,
			DIRECTION_TRAILER
		};

		enum
		{
			TEXT_FORMAT_LEFT					= 0x0000U,
			TEXT_FORMAT_RIGHT					= 0x0001U,
			TEXT_FORMAT_HORIZONTAL_CENTER		= 0x0002U,
			TEXT_FORMAT_HORIZONTAL_ALIGN_MASK	= 0x0003U,
			TEXT_FORMAT_TOP						= 0x0000U,
			TEXT_FORMAT_BOTTOM					= 0x0004U,
			TEXT_FORMAT_VERTICAL_CENTER			= 0x0008U,
			TEXT_FORMAT_VERTICAL_ALIGN_MASK		= 0x000CU,
			TEXT_FORMAT_NO_WRAP					= 0x0010U,
			TEXT_FORMAT_END_ELLIPSIS			= 0x0020U,
			TEXT_FORMAT_WORD_ELLIPSIS			= 0x0040U,
			TEXT_DRAW_ANTIALIAS					= 0x0100U,
			TEXT_DRAW_NO_ANTIALIAS				= 0x0200U,
			TEXT_DRAW_HINTING					= 0x0400U
		};

		class TSTASK_ABSTRACT_CLASS(CFont)
		{
		public:
			virtual ~CFont() = 0;
		};

		class TSTASK_ABSTRACT_CLASS(CBrush)
		{
		public:
			virtual ~CBrush() = 0;
		};

		class TSTASK_ABSTRACT_CLASS(CPen)
		{
		public:
			virtual ~CPen() = 0;
		};

		class TSTASK_ABSTRACT_CLASS(CImage)
		{
		public:
			virtual ~CImage() = 0;
			virtual void Free() = 0;
			virtual bool LoadFromFile(LPCWSTR pszFileName) = 0;
			virtual bool LoadFromMemory(const void *pData,size_t Size) = 0;
			virtual bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName) = 0;
			virtual bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName,LPCWSTR pszType) = 0;
			virtual bool Create(int Width,int Height,int BitsPerPixel) = 0;
			virtual bool CreateFromIcon(HICON hico) = 0;
			virtual bool IsCreated() const = 0;
			virtual int GetWidth() const = 0;
			virtual int GetHeight() const = 0;
		};

		class TSTASK_ABSTRACT_CLASS(CCanvas)
		{
		public:
			virtual ~CCanvas() = 0;
			virtual bool Clear(const Color &color) = 0;
			bool Clear() { return Clear(Color()); }
			virtual bool SetOverwriteMode(bool fOverwrite) = 0;
			virtual bool SetClip(const RECT &Rect) = 0;
			virtual bool ResetClip() = 0;
			virtual bool FillRect(const RECT &Rect,const CBrush *pBrush) = 0;
			virtual bool FillRect(const RECT &Rect,const Color &color) = 0;
			virtual bool DrawRect(const RECT &Rect,const CPen *pPen) = 0;
			virtual bool DrawRect(const RECT &Rect,const Color &color,int Width=1) = 0;
			virtual bool DrawLine(int x1,int y1,int x2,int y2,const CPen *pPen) = 0;
			virtual bool DrawLine(int x1,int y1,int x2,int y2,const Color &color,int Width=1) = 0;
			virtual bool DrawPolyline(const POINT *pPoints,int NumPoints,const CPen *pPen) = 0;
			virtual bool DrawPolyline(const POINT *pPoints,int NumPoints,const Color &color,int Width=1) = 0;
			virtual bool DrawText(LPCWSTR pszText,const RECT &Rect,
								  const CFont *pFont,const CBrush *pBrush,
								  UINT Flags=0) = 0;
			virtual bool DrawTextGlow(LPCWSTR pszText,const RECT &TextRect,const RECT &ClipRect,
									  const CFont *pFont,const Color &color,int Radius,
									  UINT Flags=0,const CBrush *pBrush=nullptr) = 0;
			virtual bool DrawImage(int x,int y,const CImage *pImage) = 0;
			virtual bool DrawImage(int DstX,int DstY,int DstWidth,int DstHeight,
								   const CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,
								   float Opacity=1.0f,const RECT *pClipRect=nullptr) = 0;
		};

		class TSTASK_ABSTRACT_CLASS(COffscreen)
		{
		public:
			virtual ~COffscreen() = 0;
			virtual bool Create(int Width,int Height) = 0;
			virtual CCanvas *GetCanvas() const = 0;
			virtual CImage *GetImage() const = 0;
			virtual int GetWidth() const = 0;
			virtual int GetHeight() const = 0;
			virtual bool Clear() = 0;
			virtual bool Transfer(CCanvas *pCanvas,const RECT &PaintRect) = 0;
		};

		class CSystem
		{
		public:
			struct ImageHash
			{
				BYTE Value[16];
			};

			static bool Initialize();
			static void Finalize();

			CSystem();
			~CSystem();
			CCanvas *CreateCanvas(HDC hdc);
			CCanvas *CreateCanvas(CImage *pImage);
			bool CreateOffscreen(COffscreen **ppOffscreen,int Width,int Height);
			CFont *CreateFont(const LOGFONT &LogFont);
			CBrush *CreateBrush(const Color &color);
			CBrush *CreateGradientBrush(const Color &color1,const Color &color2,GradientDirection Direction);
			CPen *CreatePen(const Color &color,int Width=1);
			CImage *CreateImage();

			void ClearImagePool();
			CImage *LoadPoolImage(LPCWSTR pszFileName);
			CImage *LoadPoolImage(const void *pData,size_t Size,const ImageHash &Hash);
			CImage *GetPoolImage(LPCWSTR pszFileName) const;
			CImage *GetPoolImage(const ImageHash &Hash) const;
			static bool GetImageHash(const void *pData,size_t Size,ImageHash *pHash);

		private:
			static bool m_fInitialized;
			static ULONG_PTR m_Token;

			std::map<TSTask::String,CImage*> m_ImagePool;
		};

		class CDoubleBufferedPainter
		{
		public:
			CDoubleBufferedPainter(CSystem &System);
			~CDoubleBufferedPainter();
			CCanvas *BeginPaint(HDC hdc,const RECT &PaintRect,COffscreen **ppOffscreen);
			void EndPaint();

		private:
			CSystem &m_System;
			CCanvas *m_pDCCanvas;
			COffscreen *m_pOffscreen;
			RECT m_PaintRect;
		};

	}

}


#endif
