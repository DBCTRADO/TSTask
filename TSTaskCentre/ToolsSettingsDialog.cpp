#include "stdafx.h"
#include "TSTaskCentre.h"
#include "ToolsSettingsDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CToolsSettingsDialog::CToolsSettingsDialog(CTSTaskCentreCore &Core,int PageID)
		: CTSTaskCentreSettingsPage(Core,PageID)
		, m_fStateChanging(false)
	{
	}

	CToolsSettingsDialog::~CToolsSettingsDialog()
	{
		Destroy();
	}

	bool CToolsSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_SETTINGS_TOOLS));
	}

	INT_PTR CToolsSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				const CTSTaskCentreSettings &Settings=m_Core.GetSettings();

				m_ListView.Attach(GetItemHandle(IDC_SETTINGS_TOOLS_LIST));
				m_ListView.SetTheme(L"explorer");
				m_ListView.SetExtendedStyle(
					LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

				m_ListView.InsertColumn(0,TEXT("名前"));
				m_ListView.InsertColumn(1,TEXT("コマンド"));

				const size_t ToolCount=Settings.Tools.GetToolCount();
				m_ListView.ReserveItemCount((int)ToolCount);
				for (size_t i=0;i<ToolCount;i++) {
					CToolsSettings::ToolInfo *pInfo=new CToolsSettings::ToolInfo;

					Settings.Tools.GetToolInfo(i,pInfo);
					InsertToolItem(-1,pInfo);
				}

				m_ListView.AdjustColumnWidth();
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_SETTINGS_TOOLS_MOVE_UP:
			case IDC_SETTINGS_TOOLS_MOVE_DOWN:
				{
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0) {
						int To;
						if (LOWORD(wParam)==IDC_SETTINGS_TOOLS_MOVE_UP) {
							if (Sel<1)
								return TRUE;
							To=Sel-1;
						} else {
							if (Sel+1>=m_ListView.GetItemCount())
								return TRUE;
							To=Sel+1;
						}

						CToolsSettings::ToolInfo *pInfo=GetToolInfo(Sel);
						if (pInfo!=nullptr) {
							m_ListView.DeleteItem(Sel);
							InsertToolItem(To,pInfo,true);
							SetItemStatus();
						}
					}
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_DELETE:
				{
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0) {
						CToolsSettings::ToolInfo *pInfo=GetToolInfo(Sel);
						if (pInfo!=nullptr) {
							m_ListView.DeleteItem(Sel);
							delete pInfo;
							SetItemStatus();
						}
					}
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_ADD:
				{
					CToolsSettings::ToolInfo Info;

					if (GetToolSettings(&Info)) {
						InsertToolItem(-1,new CToolsSettings::ToolInfo(Info),true);
						SetItemStatus();
					}
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_REPLACE:
				{
					int Sel=m_ListView.GetSelectedItem();

					if (Sel>=0) {
						CToolsSettings::ToolInfo Info;

						if (GetToolSettings(&Info))
							UpdateToolItem(Sel,Info);
					}
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_NAME:
			case IDC_SETTINGS_TOOLS_COMMAND:
				if (HIWORD(wParam)==EN_CHANGE && !m_fStateChanging) {
					int Sel=m_ListView.GetSelectedItem();
					bool fOK=GetItemTextLength(IDC_SETTINGS_TOOLS_NAME)>0
					      && GetItemTextLength(IDC_SETTINGS_TOOLS_COMMAND)>0;

					EnableItem(IDC_SETTINGS_TOOLS_ADD,fOK);
					EnableItem(IDC_SETTINGS_TOOLS_REPLACE,fOK && Sel>=0);
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_COMMAND_PARAMETERS:
				{
					static const struct {
						LPCTSTR pszParameter;
						LPCTSTR pszText;
					} ParameterList[] = {
						{TEXT("%file%"),			TEXT("録画ファイル(最初)")},
						{TEXT("%file-cur%"),		TEXT("録画中のファイル")},
						{TEXT("%rec-dir%"),			TEXT("録画フォルダ(最初)")},
						{TEXT("%rec-dir-cur%"),		TEXT("録画中のフォルダ")},
						{TEXT("%port%"),			TEXT("送信先ポート番号")},
						{TEXT("%protocol%"),		TEXT("送信プロトコル")},
						{TEXT("%task-id%"),			TEXT("タスク番号")},
						{TEXT("%task-id-0%"),		TEXT("タスク番号(0から)")},
						{TEXT("%task-id-13%"),		TEXT("タスク番号(13から)")},
						{TEXT("%service-name%"),	TEXT("サービス名")},
						{TEXT("%service-id%"),		TEXT("サービスID")},
					};

					HMENU hmenu=::CreatePopupMenu();

					for (size_t i=0;i<_countof(ParameterList);i++) {
						WCHAR szText[128];

						TSTask::FormatString(szText,_countof(szText),L"%s\t%s",
											 ParameterList[i].pszParameter,ParameterList[i].pszText);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
					}

					RECT rc;
					::GetWindowRect(GetItemHandle(IDC_SETTINGS_TOOLS_COMMAND_PARAMETERS),&rc);
					int Result=::TrackPopupMenu(hmenu,TPM_RETURNCMD,rc.left,rc.bottom,0,hDlg,nullptr);
					::DestroyMenu(hmenu);

					if (Result>0 && Result<=_countof(ParameterList)) {
						LPCTSTR pszParameter=ParameterList[Result-1].pszParameter;
						HWND hwndEdit=GetItemHandle(IDC_SETTINGS_TOOLS_COMMAND);
						DWORD Start,End;

						::SendMessage(hwndEdit,EM_GETSEL,
									  reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
						::SendMessage(hwndEdit,EM_REPLACESEL,
									  TRUE,reinterpret_cast<LPARAM>(pszParameter));
						if (End<Start)
							Start=End;
						::SendMessage(hwndEdit,EM_SETSEL,
									  Start,Start+::lstrlen(pszParameter));
						::SetFocus(hwndEdit);
					}
				}
				return TRUE;

			case IDC_SETTINGS_TOOLS_TRIGGERS_MENU:
				{
					static const struct {
						LPCTSTR pszTrigger;
						LPCTSTR pszText;
					} TriggerList[] = {
						{TEXT("BonDriverLoaded"),	TEXT("BonDriverが読み込まれた")},
						{TEXT("BonDriverUnloaded"),	TEXT("BonDriverが解放された")},
						{TEXT("TunerOpened"),		TEXT("チューナーが開かれた")},
						{TEXT("TunerClosed"),		TEXT("チューナーが閉じられた")},
						{TEXT("ChannelChanged"),	TEXT("チャンネルが変わった")},
						{TEXT("ServiceChanged"),	TEXT("サービスが変わった")},
						{TEXT("RecordingStarted"),	TEXT("録画が開始された")},
						{TEXT("RecordingStopped"),	TEXT("録画が停止された")},
						{TEXT("StreamingStarted"),	TEXT("ストリーミングが開始された")},
						{TEXT("StreamingStopped"),	TEXT("ストリーミングが停止された")},
						{TEXT("EventChanged"),		TEXT("番組が変わった")},
					};

					HMENU hmenu=::CreatePopupMenu();

					for (size_t i=0;i<_countof(TriggerList);i++) {
						WCHAR szText[128];

						TSTask::FormatString(szText,_countof(szText),L"%s\t%s",
											 TriggerList[i].pszTrigger,TriggerList[i].pszText);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
					}

					RECT rc;
					::GetWindowRect(GetItemHandle(IDC_SETTINGS_TOOLS_TRIGGERS_MENU),&rc);
					int Result=::TrackPopupMenu(hmenu,TPM_RETURNCMD,rc.left,rc.bottom,0,hDlg,nullptr);
					::DestroyMenu(hmenu);

					if (Result>0 && Result<=_countof(TriggerList)) {
						LPCTSTR pszTrigger=TriggerList[Result-1].pszTrigger;
						HWND hwndEdit=GetItemHandle(IDC_SETTINGS_TOOLS_TRIGGERS);
						DWORD Start,End;

						::SendMessage(hwndEdit,EM_GETSEL,
									  reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
						if (Start<End) {
							::SendMessage(hwndEdit,EM_REPLACESEL,
										  TRUE,reinterpret_cast<LPARAM>(pszTrigger));
							::SendMessage(hwndEdit,EM_SETSEL,
										  Start,Start+::lstrlen(pszTrigger));
						} else {
							int Length=::GetWindowTextLength(hwndEdit);
							if (Length>0) {
								TSTask::String Trigger;
								Trigger=L",";
								Trigger+=pszTrigger;
								::SendMessage(hwndEdit,EM_SETSEL,Length,-1);
								::SendMessage(hwndEdit,EM_REPLACESEL,
											  TRUE,reinterpret_cast<LPARAM>(Trigger.c_str()));
							} else {
								::SetWindowText(hwndEdit,pszTrigger);
							}
							::SendMessage(hwndEdit,EM_SETSEL,Length,-1);
						}
						::SetFocus(hwndEdit);
					}
				}
				return TRUE;
			}
			return TRUE;

		case WM_NOTIFY:
			switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
			case LVN_ITEMCHANGED:
				{
					LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

					if ((pnmlv->uOldState&LVIS_SELECTED)!=(pnmlv->uNewState&LVIS_SELECTED)) {
						if ((pnmlv->uNewState&LVIS_SELECTED)!=0) {
							CToolsSettings::ToolInfo *pInfo=GetToolInfo(pnmlv->iItem);

							if (pInfo!=nullptr) {
								m_fStateChanging=true;
								SetItemString(IDC_SETTINGS_TOOLS_NAME,pInfo->Name);
								SetItemString(IDC_SETTINGS_TOOLS_COMMAND,pInfo->Command);
								TSTask::String Triggers;
								TSTask::StringUtility::Combine(pInfo->Triggers,L",",&Triggers);
								SetItemString(IDC_SETTINGS_TOOLS_TRIGGERS,Triggers);
								CheckItem(IDC_SETTINGS_TOOLS_SHOW_IN_MENU,pInfo->fShowInMenu);
								m_fStateChanging=false;
							}
						}

						SetItemStatus();
					}
				}
				return TRUE;
			}
			break;

		case WM_DESTROY:
			{
				const int ItemCount=m_ListView.GetItemCount();
				for (int i=0;i<ItemCount;i++)
					delete GetToolInfo(i);
				m_ListView.DeleteAllItems();
				m_ListView.Detach();
			}
			return TRUE;
		}

		return FALSE;
	}

	bool CToolsSettingsDialog::OnOK(CTSTaskCentreSettings &Settings)
	{
		const int ItemCount=m_ListView.GetItemCount();
		const int ToolCount=(int)Settings.Tools.GetToolCount();

		for (int i=0;i<ItemCount;i++) {
			CToolsSettings::ToolInfo *pInfo=GetToolInfo(i);

			if (i<ToolCount)
				Settings.Tools.SetToolInfo(i,*pInfo);
			else
				Settings.Tools.AddToolInfo(*pInfo);
		}

		return true;
	}

	void CToolsSettingsDialog::SetItemStatus()
	{
		int Sel=m_ListView.GetSelectedItem();
		bool fValid=GetItemTextLength(IDC_SETTINGS_TOOLS_NAME)>0
		         && GetItemTextLength(IDC_SETTINGS_TOOLS_COMMAND)>0;

		EnableItem(IDC_SETTINGS_TOOLS_MOVE_UP,Sel>0);
		EnableItem(IDC_SETTINGS_TOOLS_MOVE_DOWN,Sel>=0 && Sel+1<m_ListView.GetItemCount());
		EnableItem(IDC_SETTINGS_TOOLS_DELETE,Sel>=0);
		EnableItem(IDC_SETTINGS_TOOLS_ADD,fValid);
		EnableItem(IDC_SETTINGS_TOOLS_REPLACE,Sel>=0 && fValid);
	}

	bool CToolsSettingsDialog::GetToolSettings(CToolsSettings::ToolInfo *pInfo) const
	{
		if (!GetItemString(IDC_SETTINGS_TOOLS_NAME,&pInfo->Name)
				|| pInfo->Name.empty()
				|| !GetItemString(IDC_SETTINGS_TOOLS_COMMAND,&pInfo->Command)
				|| pInfo->Command.empty())
			return false;

		TSTask::String Triggers;
		if (GetItemString(IDC_SETTINGS_TOOLS_TRIGGERS,&Triggers) && !Triggers.empty()) {
			TSTask::StringUtility::Split(Triggers,L",",&pInfo->Triggers);
			for (auto i=pInfo->Triggers.begin();i!=pInfo->Triggers.end();) {
				TSTask::StringUtility::Trim(*i,L" \t");
				if (i->empty())
					pInfo->Triggers.erase(i++);
				else
					i++;
			}
		} else {
			pInfo->Triggers.clear();
		}

		pInfo->fShowInMenu=IsItemChecked(IDC_SETTINGS_TOOLS_SHOW_IN_MENU);

		return true;
	}

	void CToolsSettingsDialog::InsertToolItem(int Index,CToolsSettings::ToolInfo *pInfo,bool fSelect)
	{
		Index=m_ListView.InsertItem(Index,pInfo->Name.c_str(),reinterpret_cast<LPARAM>(pInfo));
		m_ListView.SetItemText(Index,1,pInfo->Command.c_str());

		if (fSelect) {
			m_ListView.SetItemState(Index,
									LVIS_FOCUSED | LVIS_SELECTED,
									LVIS_FOCUSED | LVIS_SELECTED);
		}
	}

	bool CToolsSettingsDialog::UpdateToolItem(int Index,const CToolsSettings::ToolInfo &Info)
	{
		CToolsSettings::ToolInfo *pInfo=GetToolInfo(Index);

		if (pInfo==nullptr)
			return false;

		*pInfo=Info;

		m_ListView.SetItemText(Index,0,pInfo->Name.c_str());
		m_ListView.SetItemText(Index,1,pInfo->Command.c_str());

		return true;
	}

	CToolsSettings::ToolInfo *CToolsSettingsDialog::GetToolInfo(int Index)
	{
		return reinterpret_cast<CToolsSettings::ToolInfo*>(m_ListView.GetItemParam(Index));
	}

}
