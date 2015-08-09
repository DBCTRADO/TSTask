#ifndef TSTASKCENTRE_WIDGET_H
#define TSTASKCENTRE_WIDGET_H


namespace TSTaskCentre
{

	class CWidget
	{
	public:
		struct Position
		{
			int Left,Top;
			int Width,Height;
			bool fMaximized;
		};

		CWidget();
		virtual ~CWidget();
		virtual bool Create(HWND hwndParent=nullptr,int ID=0)=0;
		bool IsCreated() const { return m_hwnd!=NULL; }
		void Destroy();
		bool SetPosition(const Position &Pos);
		bool SetPosition(int Left,int Top,int Width,int Height);
		bool SetPosition(const RECT &Position);
		void GetPosition(Position *pPosition) const;
		void GetPosition(RECT *pPosition) const;
		int GetWidth() const;
		int GetHeight() const;
		bool GetScreenPosition(RECT *pPosition) const;
		void SetVisible(bool fVisible);
		bool GetVisible() const;
		bool SetMaximize(bool fMaximize);
		bool GetMaximize() const;
		HWND GetHandle() const { return m_hwnd; }
		bool Invalidate(bool fErase=true) const;
		bool Invalidate(const RECT &Rect,bool fErase=true) const;
		bool Update() const;
		bool Redraw(const RECT *pRect=nullptr,UINT Flags=RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW) const;
		bool GetClientRect(RECT *pRect) const;
		bool GetClientSize(SIZE *pSize) const;
		bool CalcPositionFromClientRect(RECT *pRect) const;
		bool SetParent(HWND hwnd);
		bool SetParent(CWidget *pWidget);
		HWND GetParent() const;
		DWORD GetStyle() const;
		bool SetStyle(DWORD Style,bool fFrameChange=false);
		DWORD GetExStyle() const;
		bool SetExStyle(DWORD ExStyle,bool fFrameChange=false);
		LRESULT SendMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
		bool PostMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
		bool SendSizeMessage();
		bool MoveToMonitorInside();

	protected:
		HWND m_hwnd;
		Position m_WindowPosition;
	};

	class CCustomWidget : public CWidget
	{
	public:
		CCustomWidget();
		virtual ~CCustomWidget();

	protected:
		static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
		static CCustomWidget *GetThisFromHandle(HWND hwnd);

		virtual LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
		bool CreateBasicWindow(HWND hwndParent,DWORD Style,DWORD ExStyle,INT_PTR ID,
							   LPCTSTR pszClassName,LPCTSTR pszText,HINSTANCE hinst);
	};

}


#endif
