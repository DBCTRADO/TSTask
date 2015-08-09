#include "stdafx.h"
#include <dwmapi.h>
#include "TSTaskCentre.h"
#include "AboutDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CAboutDialog::CAboutDialog(CTSTaskCentreCore &Core)
		: m_Core(Core)
		, m_pLogoImage(nullptr)
		, m_fTransparentLogo(false)
	{
	}

	bool CAboutDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		m_hinst=hinst;

		if (!CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_ABOUT)))
			return false;

		m_Core.AddModelessDialog(this);

		return true;
	}

	INT_PTR CAboutDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				SetItemText(IDC_ABOUT_VERSION,
							APP_NAME_W L" ver." APP_VERSION_TEXT_W L" (" APP_PLATFORM_W L")");

				m_pLogoImage=m_Core.GetGraphicSystem().CreateImage();
				m_pLogoImage->LoadFromResource(m_hinst,MAKEINTRESOURCE(ID_PNG_LOGO),L"PNG");

				m_fTransparentLogo=false;
				BOOL fCompositionEnabled;
				if (::DwmIsCompositionEnabled(&fCompositionEnabled)==S_OK && fCompositionEnabled) {
					HWND hwndLogo=GetItemHandle(IDC_ABOUT_LOGO);
					RECT rc;
					::GetWindowRect(hwndLogo,&rc);
					MARGINS Margins={rc.right-rc.left,0,0,0};
					if (::DwmExtendFrameIntoClientArea(hDlg,&Margins)==S_OK)
						m_fTransparentLogo=true;
				}

				AdjustPos();
			}
			return TRUE;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rcLogo,rcClient;

				::BeginPaint(hDlg,&ps);

				Graphics::CCanvas *pCanvas=m_Core.GetGraphicSystem().CreateCanvas(ps.hdc);
				if (m_fTransparentLogo)
					pCanvas->Clear(Graphics::Color(0,0,0,0));
				::GetWindowRect(::GetDlgItem(hDlg,IDC_ABOUT_LOGO),&rcLogo);
				::OffsetRect(&rcLogo,-rcLogo.left,-rcLogo.top);
				::GetClientRect(hDlg,&rcClient);
				rcClient.left=rcLogo.right;
				pCanvas->FillRect(rcClient,Graphics::Color(::GetSysColor(COLOR_3DFACE)));
				if (!m_fTransparentLogo)
					pCanvas->FillRect(rcLogo,Graphics::Color(255,255,255));
				if (m_pLogoImage!=nullptr) {
					pCanvas->DrawImage((rcLogo.right-m_pLogoImage->GetWidth())/2,
									   (rcLogo.bottom-m_pLogoImage->GetHeight())/2,
									   m_pLogoImage);
				}
				delete pCanvas;

				::EndPaint(hDlg,&ps);
			}
			return TRUE;

		case WM_DWMCOMPOSITIONCHANGED:
			m_fTransparentLogo=false;
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDOK:
			case IDCANCEL:
				//::EndDialog(hDlg,LOWORD(wParam));
				Destroy();
				return TRUE;
			}
			return TRUE;

		case WM_DESTROY:
			TSTask::SafeDelete(m_pLogoImage);
			m_Core.RemoveModelessDialog(this);
			return TRUE;
		}

		return FALSE;
	}

}
