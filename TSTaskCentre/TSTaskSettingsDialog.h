#ifndef TSTASKCENTRE_TSTASK_SETTINGS_DIALOG_H
#define TSTASKCENTRE_TSTASK_SETTINGS_DIALOG_H


#include "SettingsDialog.h"
#include "../TSTask/TSTaskSettings.h"


namespace TSTaskCentre
{

	class CTSTaskSettingsDialog : public CSettingsDialog
	{
	public:
		enum
		{
			PAGE_GENERAL,
			PAGE_RECORDING,
			PAGE_STREAMING,
			PAGE_TRAILER
		};

		class TSTASK_ABSTRACT_CLASS(CTaskSettingsPage) : public CPage
		{
		public:
			CTaskSettingsPage(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings)
				: CPage(Core,PageID), m_Settings(Settings) {}
			virtual bool OnOK(TSTask::CTSTaskSettings &Settings) = 0;
			virtual void OnCancel(const TSTask::CTSTaskSettings &Settings) {}

		protected:
			const TSTask::CTSTaskSettings &m_Settings;
		};

		CTSTaskSettingsDialog(CTSTaskCentreCore &Core,TSTask::CTSTaskSettings &Settings);

	private:
		LPCWSTR GetTitle() const override
		{
			return L"TSTask ÇÃê›íË";
		}
		CPage *CreatePage(int Page) override;
		bool OnOK() override;
		void OnCancel() override;

		TSTask::CTSTaskSettings &m_Settings;
	};

}


#endif
