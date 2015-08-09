#ifndef TSTASKCENTRE_SETTINGS_DIALOG_H
#define TSTASKCENTRE_SETTINGS_DIALOG_H


#include "Dialog.h"
#include "TSTaskCentreCore.h"


namespace TSTaskCentre
{

	class TSTASK_ABSTRACT_CLASS(CSettingsDialog) : public CDialog
	{
	public:
		class TSTASK_ABSTRACT_CLASS(CPage): public CDialog
		{
		public:
			CPage(CTSTaskCentreCore &Core,int PageID) : m_Core(Core), m_PageID(PageID) {}
			virtual ~CPage() {}
			virtual LPCWSTR GetTitle() const = 0;
			virtual COLORREF GetTitleColor() const = 0;
			virtual int GetIcon() const = 0;
			virtual bool QueryOK() { return true; }

		protected:
			void OnSettingError(int ID,LPCWSTR pszMessage);

			CTSTaskCentreCore &m_Core;
			const int m_PageID;

			enum
			{
				ICON_GENERAL,
				ICON_STATUS_BAR,
				ICON_INFORMATION_BAR,
				ICON_RECORDING,
				ICON_STREAMING,
				ICON_TOOLS,
				ICON_LOG,
				ICON_TRAILER
			};
		};

		CSettingsDialog(CTSTaskCentreCore &Core);
		~CSettingsDialog();

	// CDialog
		bool Show(HWND hwndOwner,HINSTANCE hinst) override;
		bool Create(HWND hwndOwner,HINSTANCE hinst) override;

	protected:
		virtual LPCWSTR GetTitle() const = 0;
		virtual CPage *CreatePage(int Page) = 0;
		virtual bool QueryOK();
		virtual bool OnOK() = 0;
		virtual void OnCancel() {}

		CTSTaskCentreCore &m_Core;
		int m_StartPage;
		int m_CurrentPage;
		HINSTANCE m_hinst;
		std::vector<CPage*> m_PageList;
		HIMAGELIST m_himlIcons;

	private:
		bool SetPage(int Page);
		void DeleteAllPages();

	// CDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
	};

}


#endif
