#ifndef TSTASKCENTRE_TSTASKCENTRE_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TSTASKCENTRE_SETTINGS_DIALOG_H


#include "SettingsDialog.h"


namespace TSTaskCentre
{

	class CTSTaskCentreSettingsDialog : public CSettingsDialog
	{
	public:
		enum
		{
			PAGE_GENERAL,
			PAGE_STATUS_BAR,
			PAGE_INFORMATION_BAR,
			PAGE_RECORDING,
			PAGE_TOOLS,
			PAGE_LOG,
			PAGE_TRAILER
		};

		class TSTASK_ABSTRACT_CLASS(CTSTaskCentreSettingsPage) : public CPage
		{
		public:
			CTSTaskCentreSettingsPage(CTSTaskCentreCore &Core,int PageID) : CPage(Core,PageID) {}
			virtual bool OnOK(CTSTaskCentreSettings &Settings) = 0;
			virtual void OnCancel(const CTSTaskCentreSettings &Settings) {}
		};

		CTSTaskCentreSettingsDialog(CTSTaskCentreCore &Core,CSettingsHandler *pHandler);

	private:
	// CSettingsDialog
		LPCWSTR GetTitle() const override { return L"TSTaskCentre ÇÃê›íË"; }
		CPage *CreatePage(int Page) override;
		bool OnOK() override;
		void OnCancel() override;

		CSettingsHandler *m_pHandler;
	};

}


#endif
