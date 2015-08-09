#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskSettingsDialog.h"
#include "TaskGeneralSettingsDialog.h"
#include "TaskRecordingSettingsDialog.h"
#include "TaskStreamingSettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTSTaskSettingsDialog::CTSTaskSettingsDialog(CTSTaskCentreCore &Core,TSTask::CTSTaskSettings &Settings)
		: CSettingsDialog(Core)
		, m_Settings(Settings)
	{
	}

	CSettingsDialog::CPage *CTSTaskSettingsDialog::CreatePage(int Page)
	{
		switch (Page) {
		case PAGE_GENERAL:
			return new CTaskGeneralSettingsDialog(m_Core,PAGE_GENERAL,m_Settings);
		case PAGE_RECORDING:
			return new CTaskRecordingSettingsDialog(m_Core,PAGE_RECORDING,m_Settings);
		case PAGE_STREAMING:
			return new CTaskStreamingSettingsDialog(m_Core,PAGE_STREAMING,m_Settings);
		}

		return nullptr;
	}

	bool CTSTaskSettingsDialog::OnOK()
	{
		TSTask::CTSTaskSettings Settings=m_Settings;

		for (auto e:m_PageList) {
			CTSTaskSettingsDialog::CTaskSettingsPage *pPage=
				dynamic_cast<CTSTaskSettingsDialog::CTaskSettingsPage*>(e);
			if (pPage!=nullptr && pPage->IsCreated()) {
				if (!pPage->OnOK(Settings)) {
					return false;
				}
			}
		}

		m_Settings=Settings;

		return true;
	}

	void CTSTaskSettingsDialog::OnCancel()
	{
		for (auto e:m_PageList) {
			CTSTaskSettingsDialog::CTaskSettingsPage *pPage=
				dynamic_cast<CTSTaskSettingsDialog::CTaskSettingsPage*>(e);
			if (pPage!=nullptr && pPage->IsCreated()) {
				pPage->OnCancel(m_Settings);
			}
		}
	}

}
