#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TSTaskCentreSettingsDialog.h"
#include "GeneralSettingsDialog.h"
#include "StatusBarSettingsDialog.h"
#include "InformationBarSettingsDialog.h"
#include "RecordingSettingsDialog.h"
#include "ToolsSettingsDialog.h"
#include "LogSettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsDialog(CTSTaskCentreCore &Core,CSettingsHandler *pHandler)
		: CSettingsDialog(Core)
		, m_pHandler(pHandler)
	{
	}

	CSettingsDialog::CPage *CTSTaskCentreSettingsDialog::CreatePage(int Page)
	{
		switch (Page) {
		case PAGE_GENERAL:
			return new CGeneralSettingsDialog(m_Core,PAGE_GENERAL);
		case PAGE_STATUS_BAR:
			return new CStatusBarSettingsDialog(m_Core,PAGE_STATUS_BAR);
		case PAGE_INFORMATION_BAR:
			return new CInformationBarSettingsDialog(m_Core,PAGE_INFORMATION_BAR);
		case PAGE_RECORDING:
			return new CRecordingSettingsDialog(m_Core,PAGE_RECORDING);
		case PAGE_TOOLS:
			return new CToolsSettingsDialog(m_Core,PAGE_TOOLS);
		case PAGE_LOG:
			return new CLogSettingsDialog(m_Core,PAGE_LOG);
		}

		return nullptr;
	}

	bool CTSTaskCentreSettingsDialog::OnOK()
	{
		CTSTaskCentreSettings Settings=m_Core.GetSettings();

		for (auto e:m_PageList) {
			CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage *pPage=
				dynamic_cast<CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage*>(e);
			if (pPage!=nullptr && pPage->IsCreated()) {
				if (!pPage->OnOK(Settings)) {
					return false;
				}
			}
		}

		m_Core.GetSettings().Set(Settings);

		m_pHandler->OnSettingsChanged(Settings);

		return true;
	}

	void CTSTaskCentreSettingsDialog::OnCancel()
	{
		for (auto e:m_PageList) {
			CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage *pPage=
				dynamic_cast<CTSTaskCentreSettingsDialog::CTSTaskCentreSettingsPage*>(e);
			if (pPage!=nullptr && pPage->IsCreated()) {
				pPage->OnCancel(m_Core.GetSettings());
			}
		}
	}

}
