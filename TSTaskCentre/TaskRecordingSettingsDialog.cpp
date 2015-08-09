#include "stdafx.h"
#include "TSTaskCentre.h"
#include "TaskRecordingSettingsDialog.h"
#include "MiscDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	static const ULONGLONG PRE_ALLOCATE_SIZE_UNIT=1024*1024;
	static const LPCWSTR PRE_ALLOCATE_SIZE_UNIT_TEXT=L"MiB";
	static const UINT WRITE_SIZE_UNIT=1024;
	static const LPCWSTR WRITE_SIZE_UNIT_TEXT=L"KiB";

	static const UINT_PTR EDIT_SUBCLASS_ID=1;


	CTaskRecordingSettingsDialog::CTaskRecordingSettingsDialog(CTSTaskCentreCore &Core,int PageID,const TSTask::CTSTaskSettings &Settings)
		: CTaskSettingsPage(Core,PageID,Settings)
	{
	}

	CTaskRecordingSettingsDialog::~CTaskRecordingSettingsDialog()
	{
		Destroy();
	}

	bool CTaskRecordingSettingsDialog::Create(HWND hwndOwner,HINSTANCE hinst)
	{
		return CreateDialogWindow(hwndOwner,hinst,MAKEINTRESOURCE(IDD_TASK_SETTINGS_RECORDING));
	}

	INT_PTR CTaskRecordingSettingsDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				TSTask::String Text;
				m_Settings.Recording.GetDefaultDirectory(&Text);
				if (!Text.empty())
					SetItemString(IDC_TASK_SETTINGS_RECORDING_FOLDER,Text);
				m_Settings.Recording.GetFileName(&Text);
				if (!Text.empty())
					SetItemString(IDC_TASK_SETTINGS_RECORDING_FILE_NAME,Text);

				HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
				ListView_SetExtendedListViewStyle(hwndList,
					LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
				RECT rc;
				::GetClientRect(hwndList,&rc);
				LVCOLUMN lvc;
				lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvc.fmt=LVCFMT_LEFT;
				lvc.cx=rc.right-rc.left-::GetSystemMetrics(SM_CXVSCROLL);
				lvc.pszText=TEXT("");
				lvc.iSubItem=0;
				ListView_InsertColumn(hwndList,0,&lvc);

				std::vector<TSTask::String> Directories;
				m_Settings.Recording.GetDirectories(&Directories);
				LVITEM lvi;
				lvi.mask=LVIF_TEXT;
				lvi.iSubItem=0;
				lvi.iItem=0;
				for (size_t i=0;i<Directories.size();i++) {
					lvi.pszText=const_cast<LPWSTR>(Directories[i].c_str());
					ListView_InsertItem(hwndList,&lvi);
					lvi.iItem++;
				}
				lvi.pszText=TEXT("");
				ListView_InsertItem(hwndList,&lvi);

				CheckItem(IDC_TASK_SETTINGS_RECORDING_CURRENT_SERVICE,
						  m_Settings.Recording.GetServiceSelectType()==TSTask::SERVICE_SELECT_CURRENT);
				const DWORD Streams=m_Settings.Recording.GetStreams();
				CheckItem(IDC_TASK_SETTINGS_RECORDING_SAVE_CAPTION,
						  (Streams&TSTask::STREAM_CAPTION)!=0);
				CheckItem(IDC_TASK_SETTINGS_RECORDING_SAVE_DATA_CARROUSEL,
						  (Streams&TSTask::STREAM_DATA_CARROUSEL)!=0);

				CheckItem(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE,
						  m_Settings.Recording.GetPreAllocate());
				SetItemUInt(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE,
							(UINT)(m_Settings.Recording.GetPreAllocateSize()/PRE_ALLOCATE_SIZE_UNIT));
				SetItemText(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE_UNIT,
							PRE_ALLOCATE_SIZE_UNIT_TEXT);
				EnableItems(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE_LABEL,
							IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE_UNIT,
							m_Settings.Recording.GetPreAllocate());

				SetItemUInt(IDC_TASK_SETTINGS_RECORDING_WRITE_SIZE,
							m_Settings.Recording.GetWriteBufferSize()/WRITE_SIZE_UNIT);
				SetItemText(IDC_TASK_SETTINGS_RECORDING_WRITE_SIZE_UNIT,WRITE_SIZE_UNIT_TEXT);
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_TASK_SETTINGS_RECORDING_FOLDER_BROWSE:
				{
					TSTask::String Folder;

					GetItemString(IDC_TASK_SETTINGS_RECORDING_FOLDER,&Folder);
					if (BrowseFolderDialog(hDlg,&Folder,L"録画ファイルの保存先フォルダ"))
						SetItemString(IDC_TASK_SETTINGS_RECORDING_FOLDER,Folder);
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_FILE_NAME:
				if (HIWORD(wParam)==EN_CHANGE) {
					TSTask::String FileName;
					if (GetItemString(IDC_TASK_SETTINGS_RECORDING_FILE_NAME,&FileName)) {
						TSTask::CEventVariableStringMap::EventInfo EventInfo;
						TSTask::CEventVariableStringMap::GetSampleEventInfo(&EventInfo);
						TSTask::CEventVariableStringMap StringMap(EventInfo);
						TSTask::String Text;
						if (FormatVariableString(&Text,&StringMap,FileName.c_str())) {
							SetItemString(IDC_TASK_SETTINGS_RECORDING_FILE_NAME_PREVIEW,Text);
						}
					}
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_FILE_NAME_FORMAT:
				{
					HMENU hmenu=::CreatePopupMenu();
					TSTask::CEventVariableStringMap::ParameterInfo Parameter;

					for (int i=0;TSTask::CEventVariableStringMap::GetParameterInfo(i,&Parameter);i++) {
						WCHAR szText[128];

						TSTask::FormatString(szText,_countof(szText),L"%s\t%s",
											 Parameter.pszParameter,Parameter.pszText);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
					}

					RECT rc;
					::GetWindowRect(GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FILE_NAME_FORMAT),&rc);
					int Result=::TrackPopupMenu(hmenu,TPM_RETURNCMD,rc.left,rc.bottom,0,hDlg,nullptr);
					::DestroyMenu(hmenu);

					if (Result>0) {
						HWND hwndEdit=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FILE_NAME);
						DWORD Start,End;

						TSTask::CEventVariableStringMap::GetParameterInfo(Result-1,&Parameter);
						::SendMessage(hwndEdit,EM_GETSEL,
									  reinterpret_cast<WPARAM>(&Start),reinterpret_cast<LPARAM>(&End));
						::SendMessage(hwndEdit,EM_REPLACESEL,
									  TRUE,reinterpret_cast<LPARAM>(Parameter.pszParameter));
						if (End<Start)
							Start=End;
						::SendMessage(hwndEdit,EM_SETSEL,
									  Start,Start+::lstrlen(Parameter.pszParameter));
						::SetFocus(hwndEdit);
					}
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_BROWSE:
				{
					HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
					TSTask::String Folder;

					if (Sel>=0)
						GetFolderListItem(Sel,&Folder);
					if (BrowseFolderDialog(hDlg,&Folder,L"予備の録画先フォルダ")) {
						LVITEM lvi;
						lvi.mask=LVIF_TEXT;
						lvi.iSubItem=0;
						lvi.pszText=const_cast<LPTSTR>(Folder.c_str());
						if (Sel>=0) {
							lvi.iItem=Sel;
							ListView_SetItem(hwndList,&lvi);
						} else {
							lvi.iItem=ListView_GetItemCount(hwndList)-1;
							ListView_InsertItem(hwndList,&lvi);
							ListView_SetItemState(hwndList,lvi.iItem,
								LVIS_FOCUSED | LVIS_SELECTED,LVIS_FOCUSED | LVIS_SELECTED);
						}

						if (lvi.iItem==ListView_GetItemCount(hwndList)-1) {
							AddDummyFolderListItem();
						}

						SetFolderListItemStatus();

						/*
						::SetFocus(hwndList);
						HWND hwndEdit=ListView_EditLabel(hwndList,lvi.iItem);
						if (hwndEdit!=nullptr)
							::SendMessage(hwndEdit,EM_SETSEL,-1,-1);
						*/
					}
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_UP:
			case IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_DOWN:
				{
					HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0) {
						int To;
						if (LOWORD(wParam)==IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_UP) {
							To=Sel-1;
							if (To<0)
								return TRUE;
						} else {
							To=Sel+1;
							if (To>=ListView_GetItemCount(hwndList)-1)
								return TRUE;
						}

						LVITEM lvi;
						TCHAR szDir[MAX_PATH];
						lvi.mask=LVIF_TEXT;
						lvi.iItem=Sel;
						lvi.iSubItem=0;
						lvi.pszText=szDir;
						lvi.cchTextMax=_countof(szDir);
						if (ListView_GetItem(hwndList,&lvi)) {
							ListView_DeleteItem(hwndList,Sel);
							lvi.iItem=To;
							ListView_InsertItem(hwndList,&lvi);
							ListView_SetItemState(hwndList,To,
								LVIS_FOCUSED | LVIS_SELECTED,LVIS_FOCUSED | LVIS_SELECTED);
							SetFolderListItemStatus();
						}
					}
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_REMOVE:
				{
					HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0 && Sel<ListView_GetItemCount(hwndList)-1) {
						ListView_DeleteItem(hwndList,Sel);
						SetFolderListItemStatus();
					}
				}
				return TRUE;

			case IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE:
				EnableItems(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE_LABEL,
							IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE_UNIT,
							IsItemChecked(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE));
				return TRUE;
			}
			return TRUE;

		case WM_NOTIFY:
			switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
			case LVN_ITEMCHANGED:
				SetFolderListItemStatus();
				break;

			case LVN_BEGINLABELEDIT:
				{
					NMLVDISPINFO *pnmlvdi=reinterpret_cast<NMLVDISPINFO*>(lParam);
					HWND hwndList=pnmlvdi->hdr.hwndFrom;
					HWND hwndEdit=ListView_GetEditControl(hwndList);

					if (hwndEdit!=nullptr) {
						ListView_GetSubItemRect(hwndList,pnmlvdi->item.iItem,0,
												LVIR_BOUNDS,&m_rcEdit);
						::SetWindowSubclass(hwndEdit,EditSubclassProc,EDIT_SUBCLASS_ID,
											reinterpret_cast<DWORD_PTR>(this));
					}
				}
				SetMessageResult(FALSE);
				return TRUE;

			case LVN_ENDLABELEDIT:
				{
					NMLVDISPINFO *pnmlvdi=reinterpret_cast<NMLVDISPINFO*>(lParam);

					if (pnmlvdi->item.pszText!=nullptr) {
						LPCTSTR p=pnmlvdi->item.pszText;
						while (*p==_T(' '))
							p++;
						if (*p==_T('\0')) {
							SetMessageResult(FALSE);
							return TRUE;
						}

						if (pnmlvdi->item.iItem==ListView_GetItemCount(pnmlvdi->hdr.hwndFrom)-1) {
							AddDummyFolderListItem();
						}
					}
				}
				SetMessageResult(TRUE);
				return TRUE;

			case NM_DBLCLK:
				{
					NMITEMACTIVATE *pnmia=reinterpret_cast<NMITEMACTIVATE*>(lParam);

					if (pnmia->iItem>=0)
						ListView_EditLabel(pnmia->hdr.hwndFrom,pnmia->iItem);
				}
				return TRUE;

			case NM_CUSTOMDRAW:
				{
					NMLVCUSTOMDRAW *pnmlvcd=reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);

					if (pnmlvcd->nmcd.hdr.idFrom==IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST) {
						switch (pnmlvcd->nmcd.dwDrawStage) {
						case CDDS_PREPAINT:
							SetMessageResult(CDRF_NOTIFYITEMDRAW);
							return TRUE;

						case CDDS_ITEMPREPAINT:
							if (pnmlvcd->nmcd.dwItemSpec==(DWORD_PTR)(ListView_GetItemCount(pnmlvcd->nmcd.hdr.hwndFrom)-1)) {
								SetMessageResult(CDRF_NOTIFYPOSTPAINT);
								return TRUE;
							}
							break;

						case CDDS_ITEMPOSTPAINT:
							if (pnmlvcd->nmcd.dwItemSpec==(DWORD_PTR)(ListView_GetItemCount(pnmlvcd->nmcd.hdr.hwndFrom)-1)) {
								HDC hdc=pnmlvcd->nmcd.hdc;
								int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
								/*
								COLORREF OldTextColor=::SetTextColor(hdc,
									RGB((GetRValue(pnmlvcd->clrText)+GetRValue(pnmlvcd->clrTextBk))/2,
										(GetGValue(pnmlvcd->clrText)+GetGValue(pnmlvcd->clrTextBk))/2,
										(GetBValue(pnmlvcd->clrText)+GetBValue(pnmlvcd->clrTextBk))/2));
								*/
								COLORREF OldTextColor=::SetTextColor(hdc,::GetSysColor(COLOR_GRAYTEXT));
								::DrawText(hdc,
										   TEXT("ここをダブルクリックして新しいフォルダを入力"),-1,
										   &pnmlvcd->nmcd.rc,
										   DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
								::SetTextColor(hdc,OldTextColor);
								::SetBkMode(hdc,OldBkMode);
								SetMessageResult(CDRF_SKIPDEFAULT);
								return TRUE;
							}
							break;
						}
					}
				}
				break;
			}
			break;
		}

		return FALSE;
	}

	bool CTaskRecordingSettingsDialog::QueryOK()
	{
		if (IsItemChecked(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE)) {
			UINT PreAllocateSize=GetItemUInt(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE);
			if (PreAllocateSize>TSTask::CRecordingSettings::PRE_ALLOCATE_SIZE_MAX/PRE_ALLOCATE_SIZE_UNIT) {
				TSTask::String Message;
				TSTask::StringUtility::Format(Message,
					L"ファイル領域の事前確保単位は %u %s 以下で指定してください。",
					(UINT)(TSTask::CRecordingSettings::PRE_ALLOCATE_SIZE_MAX/PRE_ALLOCATE_SIZE_UNIT),
					PRE_ALLOCATE_SIZE_UNIT_TEXT);
				OnSettingError(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE,Message.c_str());
				return false;
			}
		}

		UINT WriteBufferSize=GetItemUInt(IDC_TASK_SETTINGS_RECORDING_WRITE_SIZE);
		if (WriteBufferSize<(TSTask::CRecordingSettings::WRITE_BUFFER_SIZE_MIN+WRITE_SIZE_UNIT-1)/WRITE_SIZE_UNIT
				|| WriteBufferSize>TSTask::CRecordingSettings::WRITE_BUFFER_SIZE_MAX/WRITE_SIZE_UNIT) {
			TSTask::String Message;
			TSTask::StringUtility::Format(Message,
				L"ファイルへの書き出し単位は %u %s から %u %s の範囲で指定してください。",
				(TSTask::CRecordingSettings::WRITE_BUFFER_SIZE_MIN+WRITE_SIZE_UNIT-1)/WRITE_SIZE_UNIT,
				WRITE_SIZE_UNIT_TEXT,
				TSTask::CRecordingSettings::WRITE_BUFFER_SIZE_MAX/WRITE_SIZE_UNIT,
				WRITE_SIZE_UNIT_TEXT);
			OnSettingError(IDC_TASK_SETTINGS_RECORDING_WRITE_SIZE,Message.c_str());
			return false;
		}

		return true;
	}

	bool CTaskRecordingSettingsDialog::OnOK(TSTask::CTSTaskSettings &Settings)
	{
		TSTask::String Text;
		if (GetItemString(IDC_TASK_SETTINGS_RECORDING_FOLDER,&Text))
			Settings.Recording.SetDefaultDirectory(Text.c_str());
		if (GetItemString(IDC_TASK_SETTINGS_RECORDING_FILE_NAME,&Text))
			Settings.Recording.SetFileName(Text.c_str());

		HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
		const int ItemCount=ListView_GetItemCount(hwndList);
		std::vector<TSTask::String> Directories;
		for (int i=0;i<ItemCount;i++) {
			TSTask::String Dir;
			if (GetFolderListItem(i,&Dir))
				Directories.push_back(Dir);
		}
		Settings.Recording.SetDirectories(Directories);

		Settings.Recording.SetServiceSelectType(
			IsItemChecked(IDC_TASK_SETTINGS_RECORDING_CURRENT_SERVICE)?
				TSTask::SERVICE_SELECT_CURRENT:TSTask::SERVICE_SELECT_ALL);
		Settings.Recording.SetStreamFlag(TSTask::STREAM_CAPTION,
			IsItemChecked(IDC_TASK_SETTINGS_RECORDING_SAVE_CAPTION));
		Settings.Recording.SetStreamFlag(TSTask::STREAM_DATA_CARROUSEL,
			IsItemChecked(IDC_TASK_SETTINGS_RECORDING_SAVE_DATA_CARROUSEL));

		Settings.Recording.SetPreAllocate(
			IsItemChecked(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE));
		Settings.Recording.SetPreAllocateSize(
			(ULONGLONG)GetItemUInt(IDC_TASK_SETTINGS_RECORDING_PRE_ALLOCATE_SIZE)*PRE_ALLOCATE_SIZE_UNIT);
		Settings.Recording.SetWriteBufferSize(
			GetItemUInt(IDC_TASK_SETTINGS_RECORDING_WRITE_SIZE)*WRITE_SIZE_UNIT);

		return true;
	}

	bool CTaskRecordingSettingsDialog::GetFolderListItem(int Item,TSTask::String *pText)
	{
		LVITEM lvi;
		TCHAR szText[MAX_PATH];

		lvi.mask=LVIF_TEXT;
		lvi.iItem=Item;
		lvi.iSubItem=0;
		lvi.pszText=szText;
		lvi.cchTextMax=_countof(szText);
		if (!ListView_GetItem(GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST),&lvi))
			return false;

		pText->assign(szText);
		TSTask::StringUtility::Trim(*pText,L" ");

		return !pText->empty();
	}

	bool CTaskRecordingSettingsDialog::AddDummyFolderListItem()
	{
		HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
		LVITEM lvi;

		lvi.mask=LVIF_TEXT;
		lvi.iItem=ListView_GetItemCount(hwndList);
		lvi.iSubItem=0;
		lvi.pszText=TEXT("");
		return ListView_InsertItem(hwndList,&lvi)!=FALSE;
	}

	void CTaskRecordingSettingsDialog::SetFolderListItemStatus()
	{
		HWND hwndList=GetItemHandle(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST);
		int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
		int ItemCount=ListView_GetItemCount(hwndList);

		SetItemText(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_BROWSE,
					Sel>=0?TEXT("参照..."):TEXT("追加..."));
		EnableItem(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_UP,Sel>0 && Sel<ItemCount-1);
		EnableItem(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_DOWN,Sel>=0 && Sel+1<ItemCount-1);
		EnableItem(IDC_TASK_SETTINGS_RECORDING_FOLDER_LIST_REMOVE,Sel>=0 && Sel<ItemCount-1);
	}

	LRESULT CALLBACK CTaskRecordingSettingsDialog::EditSubclassProc(
		HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
	{
		switch (uMsg) {
		case WM_WINDOWPOSCHANGING:
			{
				CTaskRecordingSettingsDialog *pThis=
					reinterpret_cast<CTaskRecordingSettingsDialog*>(dwRefData);
				WINDOWPOS *pPos=reinterpret_cast<WINDOWPOS*>(lParam);

				pPos->x=pThis->m_rcEdit.left;
				pPos->y=pThis->m_rcEdit.top;
				pPos->cx=pThis->m_rcEdit.right-pThis->m_rcEdit.left;
				pPos->cy=pThis->m_rcEdit.bottom-pThis->m_rcEdit.top;
			}
			return 0;
		}

		return ::DefSubclassProc(hwnd,uMsg,wParam,lParam);
	}

}
