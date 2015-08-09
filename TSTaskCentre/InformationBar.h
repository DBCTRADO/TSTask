#ifndef TSTASKCENTRE_INFORMATION_BAR_H
#define TSTASKCENTRE_INFORMATION_BAR_H


#include "TSTaskCentreCore.h"
#include "LayeredWidget.h"
#include "Theme.h"


namespace TSTaskCentre
{

	class CInformationBar : public CLayeredWidget
	{
	public:
		class TSTASK_ABSTRACT_CLASS(CItem)
		{
		public:
			struct DrawInfo
			{
				Graphics::CSystem *pSystem;
				Graphics::CCanvas *pCanvas;
				const Theme::ItemStyle *pStyle;
				const Graphics::CFont *pFont;
				Theme::CStylePainter *pStylePainter;
			};

			enum TextAlign
			{
				TEXT_ALIGN_LEFT,
				TEXT_ALIGN_RIGHT,
				TEXT_ALIGN_CENTER,
				TEXT_ALIGN_TRAILER
			};

			static const int DEFAULT_WIDTH=100;

			CItem(int ID);
			virtual ~CItem() {}
			virtual void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) = 0;
			virtual bool Update() { return true; }
			int GetID() const { return m_ID; }
			int GetWidth() const { return m_Width; }
			bool SetWidth(int Width);
			TextAlign GetTextAlign() const { return m_TextAlign; }
			bool SetTextAlign(TextAlign Align);

		protected:
			void DrawText(const DrawInfo &Info,LPCTSTR pszText,
						  const RECT &TextRect,const RECT &ClipRect) const;

			int m_ID;
			int m_Width;
			TextAlign m_TextAlign;
		};

		class TSTASK_ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual ~CEventHandler() {}
			virtual void OnInformationBarLButtonDown(CInformationBar *pBar,int x,int y) {}
			virtual void OnInformationBarLButtonUp(CInformationBar *pBar,int x,int y) {}
			virtual void OnInformationBarRButtonDown(CInformationBar *pBar,int x,int y) {}
			virtual void OnInformationBarRButtonUp(CInformationBar *pBar,int x,int y) {}
		};

		struct ThemeInfo
		{
			Theme::BackgroundStyle BackgroundStyle;
			Theme::ItemStyle ItemStyle;
			Theme::ForegroundStyle InfoStyle;
			Theme::ForegroundStyle ErrorStyle;
			Theme::ForegroundStyle WarningStyle;
		};

		enum TextType
		{
			TEXT_TYPE_INFO,
			TEXT_TYPE_ERROR,
			TEXT_TYPE_WARNING
		};

		static bool Initialize(HINSTANCE hinst);
		static bool GetDefaultTheme(ThemeInfo *pTheme);

		CInformationBar(CTSTaskCentreCore &Core);
		~CInformationBar();
		bool Create(HWND hwndParent,int ID=0) override;
		size_t GetItemCount() const { return m_ItemList.size(); }
		bool AddItem(CItem *pItem);
		CItem *GetItem(size_t Index);
		const CItem *GetItem(size_t Index) const;
		bool DeleteItem(size_t Index);
		void DeleteAllItems();
		void UpdateItems(bool fRedraw=true);
		void SetSingleText(TextType Type,LPCWSTR pszText,DWORD Time=0);
		bool SetTheme(const ThemeInfo &Theme);
		bool SetFont(const LOGFONT &Font);
		bool SetItemMargin(const RECT &Margin);
		RECT GetItemMargin() const;
		RECT GetDefaultItemMargin() const;
		int GetItemHeight() const { return m_ItemHeight; }
		void SetEventHandler(CEventHandler *pHandler) { m_pEventHandler=pHandler; }
		bool DrawItemPreview(CItem *pItem,HDC hdc,const RECT &Rect) const;

	private:
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) override;
		bool IsTransparentBackground() const override;

		void CalcItemHeight();
		void AdjustHeight();

		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		CTSTaskCentreCore &m_Core;
		ThemeInfo m_Theme;
		LOGFONT m_Font;
		RECT m_ItemMargin;
		int m_ItemHeight;
		bool m_fFitItemWidth;
		std::vector<CItem*> m_ItemList;
		TextType m_SingleTextType;
		TSTask::String m_SingleText;
		CEventHandler *m_pEventHandler;
	};

}


#endif
