#ifndef TSTASKCENTRE_MESSAGE_DIALOG_H
#define TSTASKCENTRE_MESSAGE_DIALOG_H


#include "Dialog.h"


namespace TSTaskCentre
{

	class CMessageDialog : public CDialog
	{
	public:
		enum MessageType
		{
			TYPE_INFO,
			TYPE_QUESTION,
			TYPE_WARNING,
			TYPE_ERROR
		};

		enum
		{
			STYLE_OK				=0x0001U,
			STYLE_CANCEL			=0x0002U,
			STYLE_OK_CANCEL			=0x0003U,
			STYLE_DEFAULT_CANCEL	=0x0004U
		};

		CMessageDialog();
		int Show(HWND hwndOwner,HINSTANCE hinst,
				 MessageType Type,LPCWSTR pszText,LPCWSTR pszCaption,
				 UINT Style=STYLE_OK,bool *pDontAskAgain=nullptr);

	private:
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		MessageType m_Type;
		LPCWSTR m_pszText;
		LPCWSTR m_pszCaption;
		UINT m_Style;
		bool *m_pDontAskAgain;
	};

}


#endif
