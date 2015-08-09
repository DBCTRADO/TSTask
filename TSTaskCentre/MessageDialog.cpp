#include "stdafx.h"
#include "TSTaskCentre.h"
#include "MessageDialog.h"
#include "../Common/DebugDef.h"
#include "resource.h"


namespace TSTaskCentre
{

	static void CalcEditControlSize(HWND hwndEdit,SIZE *pSize)
	{
		LRESULT LineCount=::SendMessage(hwndEdit,EM_GETLINECOUNT,0,0);

		int OrigLength=::GetWindowTextLength(hwndEdit);
		::SendMessage(hwndEdit,EM_SETSEL,OrigLength,-1);
		::SendMessage(hwndEdit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(TEXT("\r\n\r\n")));

		SIZE Size={0,0};

		for (LRESULT i=0;i<=LineCount;i++) {
			LRESULT Index=::SendMessage(hwndEdit,EM_LINEINDEX,i,0);
			LRESULT Result=::SendMessage(hwndEdit,EM_POSFROMCHAR,
										 Index+::SendMessage(hwndEdit,EM_LINELENGTH,Index,0),0);
			if (LOWORD(Result)>Size.cx)
				Size.cx=LOWORD(Result);
			if (i==LineCount)
				Size.cy=HIWORD(Result);
		}

		::SendMessage(hwndEdit,EM_SETSEL,OrigLength,-1);
		::SendMessage(hwndEdit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(TEXT("")));

		*pSize=Size;
	}


	CMessageDialog::CMessageDialog()
	{
	}

	int CMessageDialog::Show(HWND hwndOwner,HINSTANCE hinst,
							 MessageType Type,LPCWSTR pszText,LPCWSTR pszCaption,UINT Style,
							 bool *pDontAskAgain)
	{
		m_Type=Type;
		m_pszText=pszText;
		m_pszCaption=pszCaption;
		m_Style=Style;
		m_pDontAskAgain=pDontAskAgain;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_MESSAGE));
	}

	INT_PTR CMessageDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				if (!TSTask::IsStringEmpty(m_pszCaption))
					::SetWindowText(hDlg,m_pszCaption);

				LPCTSTR pszIcon=nullptr;
				switch (m_Type) {
				case TYPE_INFO:		pszIcon=IDI_INFORMATION;	break;
				case TYPE_QUESTION:	pszIcon=IDI_QUESTION;		break;
				case TYPE_WARNING:	pszIcon=IDI_WARNING;		break;
				case TYPE_ERROR:	pszIcon=IDI_ERROR;			break;
				}
				if (pszIcon!=nullptr) {
					::SendDlgItemMessage(hDlg,IDC_MESSAGE_ICON,STM_SETICON,
										 reinterpret_cast<WPARAM>(::LoadIcon(nullptr,pszIcon)),0);
				}

				HWND hwndEdit=::GetDlgItem(hDlg,IDC_MESSAGE_TEXT);
				::SetWindowText(hwndEdit,m_pszText);
				SIZE EditSize;
				CalcEditControlSize(hwndEdit,&EditSize);
				EditSize.cx+=8;

				RECT rcEdit,rcIcon,rcDlg,rcClient;
				::GetWindowRect(hwndEdit,&rcEdit);
				::OffsetRect(&rcEdit,-rcEdit.left,-rcEdit.top);
				::GetWindowRect(::GetDlgItem(hDlg,IDC_MESSAGE_ICON),&rcIcon);
				rcIcon.bottom-=rcIcon.top;
				if (EditSize.cy<max(rcIcon.bottom,rcEdit.bottom))
					EditSize.cy=max(rcIcon.bottom,rcEdit.bottom);
				if (EditSize.cx<rcEdit.right)
					EditSize.cx=rcEdit.right;
				::SetWindowPos(hwndEdit,nullptr,0,0,EditSize.cx,EditSize.cy,
							   SWP_NOMOVE | SWP_NOZORDER);
				::GetWindowRect(hDlg,&rcDlg);
				::GetClientRect(hDlg,&rcClient);
				const int XOffset=EditSize.cx-rcEdit.right;
				const int YOffset=EditSize.cy-rcEdit.bottom;
				::SetWindowPos(hDlg,nullptr,0,0,
							   (rcDlg.right-rcDlg.left)+XOffset,
							   (rcDlg.bottom-rcDlg.top)+YOffset,
							   SWP_NOMOVE | SWP_NOZORDER);

				if ((m_Style&STYLE_OK)!=0) {
					RECT rcOK;
					::GetWindowRect(::GetDlgItem(hDlg,(m_Style&STYLE_CANCEL)!=0?IDOK:IDCANCEL),&rcOK);
					MapWindowRect(nullptr,hDlg,&rcOK);
					::SetWindowPos(::GetDlgItem(hDlg,IDOK),nullptr,
								   rcOK.left+XOffset,rcOK.top+YOffset,0,0,
								   SWP_NOSIZE | SWP_NOZORDER);
				} else {
					::ShowWindow(::GetDlgItem(hDlg,IDOK),SW_HIDE);
				}

				if ((m_Style&STYLE_CANCEL)!=0) {
					RECT rcCancel;
					::GetWindowRect(::GetDlgItem(hDlg,IDCANCEL),&rcCancel);
					MapWindowRect(nullptr,hDlg,&rcCancel);
					::SetWindowPos(::GetDlgItem(hDlg,IDCANCEL),nullptr,
								   rcCancel.left+XOffset,rcCancel.top+YOffset,0,0,
								   SWP_NOSIZE | SWP_NOZORDER);
				} else {
					::ShowWindow(::GetDlgItem(hDlg,IDCANCEL),SW_HIDE);
				}

				if (m_pDontAskAgain!=nullptr) {
					HWND hwnd=::GetDlgItem(hDlg,IDC_MESSAGE_DONT_ASK_AGAIN);
					RECT rc;
					::GetWindowRect(hwnd,&rc);
					MapWindowRect(nullptr,hDlg,&rc);
					::SetWindowPos(hwnd,nullptr,rc.left,rc.top+YOffset,0,0,
								   SWP_NOSIZE | SWP_NOZORDER);
					::CheckDlgButton(hDlg,IDC_MESSAGE_DONT_ASK_AGAIN,
									 *m_pDontAskAgain?BST_CHECKED:BST_UNCHECKED);
					::ShowWindow(hwnd,SW_SHOW);
				}

				int DefaultButton=IDOK;
				if ((m_Style&(STYLE_CANCEL | STYLE_DEFAULT_CANCEL))==
						(STYLE_CANCEL | STYLE_DEFAULT_CANCEL))
					DefaultButton=IDCANCEL;
				HWND hwndButton=::GetDlgItem(hDlg,DefaultButton);
				::SetWindowLong(hwndButton,GWL_STYLE,
								(::GetWindowLong(hwndButton,GWL_STYLE)&~BS_PUSHBUTTON) | BS_DEFPUSHBUTTON);
				::SetFocus(hwndButton);

				AdjustPos();
			}
			return FALSE;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rcEdit,rc;

				::BeginPaint(hDlg,&ps);
				::GetWindowRect(::GetDlgItem(hDlg,IDC_MESSAGE_TEXT),&rcEdit);
				MapWindowRect(nullptr,hDlg,&rcEdit);
				::GetClientRect(hDlg,&rc);
				rc.bottom=rcEdit.bottom;
				::FillRect(ps.hdc,&rc,::GetSysColorBrush(COLOR_WINDOW));
				::EndPaint(hDlg,&ps);
			}
			return TRUE;

		case WM_CTLCOLORSTATIC:
			if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_MESSAGE_ICON)
					|| reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_MESSAGE_TEXT))
				return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDOK:
			case IDCANCEL:
				if (m_pDontAskAgain!=nullptr) {
					*m_pDontAskAgain=
						::IsDlgButtonChecked(hDlg,IDC_MESSAGE_DONT_ASK_AGAIN)==BST_CHECKED;
				}

				::EndDialog(hDlg,LOWORD(wParam));
				return TRUE;
			}
			return TRUE;
		}

		return FALSE;
	}

}
