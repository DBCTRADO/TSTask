#ifndef TSTASKCENTRE_ABOUT_DIALOG_H
#define TSTASKCENTRE_ABOUT_DIALOG_H


#include "Dialog.h"
#include "TSTaskCentreCore.h"
#include "Graphics.h"


namespace TSTaskCentre
{

	class CAboutDialog : public CDialog
	{
	public:
		CAboutDialog(CTSTaskCentreCore &Core);
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CTSTaskCentreCore &m_Core;
		Graphics::CImage *m_pLogoImage;
		bool m_fTransparentLogo;
		HINSTANCE m_hinst;
	};

}


#endif
