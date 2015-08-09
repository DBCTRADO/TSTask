#include "stdafx.h"
#include "TSTaskCentre.h"
#include "ChannelListDialog.h"
#include "resource.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	enum
	{
		COLUMN_NAME,
		COLUMN_INDEX,
		COLUMN_TVROCK_CHANNEL,
		COLUMN_SERVICE_ID,
		COLUMN_TS_ID,
		COLUMN_FREQUENCY,
		COLUMN_NETWORK_ID
	};


	CChannelListDialog::CChannelListDialog(CTSTaskManager &Manager)
		: m_TSTaskManager(Manager)
		, m_TaskID(TSTask::INVALID_TASK_ID)
	{
	}

	CChannelListDialog::~CChannelListDialog()
	{
		Destroy();
	}

	bool CChannelListDialog::Show(HWND hwndOwner,HINSTANCE hinst,TSTask::TaskID TaskID)
	{
		m_hinst=hinst;
		m_TaskID=TaskID;

		return ShowDialog(hwndOwner,hinst,MAKEINTRESOURCE(IDD_CHANNEL_LIST))>=0;
	}

	INT_PTR CChannelListDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				m_ListView.Attach(GetItemHandle(IDC_CHANNELS_LIST));
				m_ListView.SetTheme(L"explorer");
				m_ListView.SetExtendedStyle(
					LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

				HIMAGELIST himlIcons=
					::ImageList_LoadImage(m_hinst,MAKEINTRESOURCE(IDB_SPACE_ICONS),16,1,
										  CLR_NONE,IMAGE_BITMAP,LR_CREATEDIBSECTION);
				m_ListView.SetImageList(himlIcons,LVSIL_SMALL);

				m_ListView.InsertColumn(COLUMN_NAME,TEXT("名称"));
				m_ListView.InsertColumn(COLUMN_INDEX,TEXT("インデックス"));
				m_ListView.InsertColumn(COLUMN_TVROCK_CHANNEL,TEXT("TvRockチャンネル"),LVCFMT_RIGHT);
				m_ListView.InsertColumn(COLUMN_SERVICE_ID,TEXT("サービス"),LVCFMT_RIGHT);
				m_ListView.InsertColumn(COLUMN_TS_ID,TEXT("TSID"),LVCFMT_RIGHT);
				m_ListView.InsertColumn(COLUMN_FREQUENCY,TEXT("周波数"),LVCFMT_RIGHT);
				m_ListView.InsertColumn(COLUMN_NETWORK_ID,TEXT("ネットワーク"),LVCFMT_RIGHT);

				TSTask::CTuningSpaceList SpaceList;
				if (m_TSTaskManager.GetScannedChannelList(m_TaskID,&SpaceList)
						&& SpaceList.GetSpaceCount()>0) {
					const TSTask::CChannelList ChannelList=SpaceList.GetAllChannelList();
					bool fTvRockChannel=false;

					m_ListView.ReserveItemCount((int)ChannelList.GetChannelCount());

					for (int i=0;i<(int)ChannelList.GetChannelCount();i++) {
						const TSTask::CChannelInfo &Info=*ChannelList.GetChannelInfo(i);
						int Space;
						WCHAR szText[256];

						if ((Info.GetTransportStreamID()&0xFF00)==0x7F00
								|| (Info.GetTransportStreamID()&0xFF00)==0x7E00)
							Space=1;
						else if (Info.GetNetworkID()==4)
							Space=2;
						else if (Info.GetNetworkID()>=6 && Info.GetNetworkID()<=10)
							Space=3;
						else
							Space=0;

						m_ListView.InsertItem(i,Info.GetName());
						m_ListView.SetItemImage(i,COLUMN_NAME,Space);

						TSTask::FormatString(szText,_countof(szText),L"%d : %d",
											 Info.GetSpace(),Info.GetChannel());
						m_ListView.SetItemText(i,COLUMN_INDEX,szText);

						if (Space<=1) {
							TSTask::FormatString(szText,_countof(szText),L"%d",Info.GetChannel()+13);
							fTvRockChannel=true;
						} else {
							if (Info.GetFrequency()!=0) {
								TSTask::FormatString(szText,_countof(szText),L"%u",
									((DWORD)Info.GetFrequency()<<16) | (DWORD)Info.GetTransportStreamID());
								fTvRockChannel=true;
							} else {
								szText[0]=L'\0';
							}
						}
						m_ListView.SetItemText(i,COLUMN_TVROCK_CHANNEL,szText);

						TSTask::FormatString(szText,_countof(szText),L"%d",Info.GetServiceID());
						m_ListView.SetItemText(i,COLUMN_SERVICE_ID,szText);

						TSTask::FormatString(szText,_countof(szText),L"%d",Info.GetTransportStreamID());
						m_ListView.SetItemText(i,COLUMN_TS_ID,szText);

						if (Info.GetFrequency()!=0) {
							TSTask::FormatString(szText,_countof(szText),L"%d",Info.GetFrequency());
							m_ListView.SetItemText(i,COLUMN_FREQUENCY,szText);
						}

						TSTask::FormatString(szText,_countof(szText),L"%d",Info.GetNetworkID());
						m_ListView.SetItemText(i,COLUMN_NETWORK_ID,szText);
					}

					EnableItem(IDC_CHANNELS_COPY_CH_TXT,fTvRockChannel);
				}

				m_ListView.AdjustColumnWidth();

				AddControl(IDC_CHANNELS_LIST,ALIGN_ALL);
				AddControl(IDC_CHANNELS_COPY_CH_TXT,ALIGN_BOTTOM);

				AdjustPos();
			}
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_CHANNELS_COPY_CH_TXT:
				{
					const int ItemCount=m_ListView.GetItemCount();
					if (ItemCount>0) {
						static const int CopyColumns[] = {
							COLUMN_NAME,COLUMN_TVROCK_CHANNEL,COLUMN_SERVICE_ID
						};
						TSTask::String List;

						for (int i=0;i<ItemCount;i++) {
							for (int j=0;j<_countof(CopyColumns);j++) {
								WCHAR szText[256];

								m_ListView.GetItemText(i,CopyColumns[j],szText,_countof(szText));
								if (CopyColumns[j]==COLUMN_NAME) {
									for (LPWSTR p=szText;*p!=L'\0';p++) {
										if (*p==L' ')
											*p=L'　';
									}
								}
								List+=szText;
								if (j+1<_countof(CopyColumns))
									List+=L" ";
								else
									List+=L"\r\n";
							}
						}

						if (::OpenClipboard(::GetParent(hDlg))) {
							HGLOBAL hData=::GlobalAlloc(GHND,(List.length()+1)*sizeof(WCHAR));
							if (hData!=nullptr) {
								WCHAR *pszData=static_cast<WCHAR*>(::GlobalLock(hData));
								::lstrcpyW(pszData,List.c_str());
								::GlobalUnlock(hData);
								::EmptyClipboard();
								if (::SetClipboardData(CF_UNICODETEXT,hData)==nullptr)
									::GlobalFree(hData);
							}
							::CloseClipboard();
						}
					}
				}
				return TRUE;

			case IDOK:
			case IDCANCEL:
				End(LOWORD(wParam));
				return TRUE;
			}
			return TRUE;

		case WM_DESTROY:
			m_ListView.Detach();
			return TRUE;
		}

		return FALSE;
	}

}
