#ifndef TSTASKCENTRE_LAYERED_WIDGET_H
#define TSTASKCENTRE_LAYERED_WIDGET_H


#include "Widget.h"
#include "Graphics.h"


namespace TSTaskCentre
{

	class CLayeredWidget : public CCustomWidget
	{
	public:
		CLayeredWidget(Graphics::CSystem &GraphicSystem);
		virtual ~CLayeredWidget();

	protected:
		virtual LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		virtual void Draw(Graphics::CCanvas *pCanvas,const RECT &PaintRect) = 0;
		virtual bool IsTransparentBackground() const = 0;

		void Draw(HDC hdc,const RECT &PaintRect);
		bool DrawOffscreen(const RECT &PaintRect,bool fDrawParent);
		bool DrawParent(Graphics::CCanvas *pCanvas,const RECT &PaintRect);
		void DeleteOffscreen();
		void DeleteChildrenOffscreen();

		static BOOL CALLBACK EnumChildProc(HWND hwnd,LPARAM lParam);

		enum
		{
			WM_APP_DRAW=WM_APP+0x1000,
			WM_APP_DELETE_OFFSCREEN
		};

		struct DrawInfo
		{
			Graphics::CCanvas *pCanvas;
			RECT PaintRect;
			POINT DstPos;
			bool fDrawParent;
		};

		Graphics::CSystem &m_GraphicSystem;
		Graphics::COffscreen *m_pOffscreen;
	};

}


#endif
