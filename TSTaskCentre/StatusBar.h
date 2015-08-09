#ifndef TSTASKCENTRE_STATUS_BAR_H
#define TSTASKCENTRE_STATUS_BAR_H


#include <vector>
#include "LayeredWidget.h"
#include "Theme.h"


namespace TSTaskCentre
{

	class CStatusBar : public CLayeredWidget
	{
	public:
		struct ThemeInfo
		{
			Theme::BackgroundStyle BackgroundStyle;
			Theme::BackgroundStyle RowBackgroundStyle;
			Theme::BackgroundStyle TopRowBackgroundStyle;
			Theme::BackgroundStyle MiddleRowBackgroundStyle;
			Theme::BackgroundStyle BottomRowBackgroundStyle;
			Theme::ItemStyle ItemStyle;
			Theme::ItemStyle HighlightItemStyle;
		};

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

			static const int EM_UNIT=-100;

			CItem(int ID,int DefaultWidth,int MinWidth=0);
			virtual ~CItem() {}
			int GetIndex() const;
			bool GetRect(RECT *pRect) const;
			bool GetClientRect(RECT *pRect) const;
			int GetID() const { return m_ID; }
			int GetDefaultWidth() const { return m_DefaultWidth; }
			int GetDefaultWidthPixels() const;
			int GetWidth() const { return m_Width; }
			bool SetWidth(int Width);
			int GetMinWidth() const { return m_MinWidth; }
			void SetVisible(bool fVisible);
			bool GetVisible() const { return m_fVisible; }
			bool Update() const;
			virtual LPCTSTR GetName() const = 0;
			virtual void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const = 0;
			virtual void DrawPreview(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) const;
			virtual void OnLButtonDown(int x,int y) {}
			virtual void OnRButtonDown(int x,int y) { OnLButtonDown(x,y); }
			virtual void OnLButtonDoubleClick(int x,int y) { OnLButtonDown(x,y); }
			virtual void OnMouseMove(int x,int y) {}
			virtual void OnVisibleChange(bool fVisible) {}
			virtual void OnFocus(bool fFocus) {}
			virtual bool OnMouseHover(int x,int y) { return false; }
			virtual LRESULT OnNotifyMessage(LPNMHDR pnmh) { return 0; }

			friend CStatusBar;

		protected:
			CStatusBar *m_pStatusBar;
			int m_ID;
			int m_DefaultWidth;
			int m_MinWidth;
			int m_Width;
			bool m_fVisible;
			bool m_fBreak;

			bool GetMenuPos(POINT *pPos,UINT *pFlags);
			void DrawText(const DrawInfo &Info,LPCTSTR pszText,
						  const RECT &TextRect,const RECT &ClipRect) const;
		};

		class TSTASK_ABSTRACT_CLASS(CEventHandler)
		{
		public:
			CEventHandler();
			virtual ~CEventHandler();
			virtual void OnMouseLeave() {}
			virtual void OnHeightChanged(int Height) {}
			friend CStatusBar;

		protected:
			CStatusBar *m_pStatusBar;
		};

		static bool Initialize(HINSTANCE hinst);
		static bool GetDefaultTheme(ThemeInfo *pTheme);

		CStatusBar(Graphics::CSystem &GraphicSystem);
		~CStatusBar();
		bool Create(HWND hwndParent,int ID=0) override;
		int NumItems() const { return (int)m_ItemList.size(); }
		const CItem *GetItemByIndex(int Index) const;
		CItem *GetItemByIndex(int Index);
		const CItem *GetItemByID(int ID) const;
		CItem *GetItemByID(int ID);
		bool AddItem(CItem *pItem);
		int IDToIndex(int ID) const;
		int IndexToID(int Index) const;
		bool UpdateItem(int ID);
		bool GetItemRectByID(int ID,RECT *pRect) const;
		bool GetItemRectByIndex(int Index,RECT *pRect) const;
		bool GetItemClientRectByID(int ID,RECT *pRect) const;
		int GetItemHeight() const;
		bool SetItemMargin(const RECT &Margin);
		RECT GetItemMargin() const;
		RECT GetDefaultItemMargin() const;
		int GetFontHeight() const { return m_FontHeight; }
		int GetIntegralWidth() const;
		void SetSingleText(LPCTSTR pszText);
		bool SetTheme(const ThemeInfo &Theme);
		bool GetTheme(ThemeInfo *pTheme) const;
		bool SetFont(const LOGFONT &Font);
		bool GetFont(LOGFONT *pFont) const;
		bool SetMultiRow(bool fMultiRow);
		bool GetMultiRow() const { return m_fMultiRow; }
		bool SetMaxRows(int MaxRows);
		int GetMaxRows() const { return m_MaxRows; }
		int CalcHeight(int Width) const;
		int GetCurItem() const;
		bool SetEventHandler(CEventHandler *pEventHandler);
		bool SetItemOrder(const int *pOrderList);
		bool DrawItemPreview(const CItem *pItem,HDC hdc,const RECT &Rect,
							 bool fHighlight=false,const LOGFONT *pFont=nullptr) const;
		void EnableSizeAdjustment(bool fEnable);

	private:
		static const LPCTSTR m_pszWindowClass;
		static HINSTANCE m_hinst;

		LOGFONT m_Font;
		int m_FontHeight;
		RECT m_ItemMargin;
		int m_ItemHeight;
		bool m_fMultiRow;
		int m_MaxRows;
		int m_Rows;
		ThemeInfo m_Theme;
		std::vector<CItem*> m_ItemList;
		bool m_fSingleMode;
		TSTask::String m_SingleText;
		int m_HotItem;
		bool m_fTrackMouseEvent;
		bool m_fOnButtonDown;
		CEventHandler *m_pEventHandler;
		bool m_fAdjustSize;

		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) override;
		bool IsTransparentBackground() const override;

		void CalcItemHeight();
		void SetHotItem(int Item);
		void AdjustSize();
		void CalcRows();
	};

}


#endif
