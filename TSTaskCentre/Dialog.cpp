#include "stdafx.h"
#include "TSTaskCentre.h"
#include "Dialog.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	CDialog::CDialog()
		: m_hDlg(nullptr)
		, m_fModeless(false)
	{
	}

	CDialog::~CDialog()
	{
		Destroy();
	}

	int CDialog::ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
	{
		if (m_hDlg!=nullptr)
			return -1;

		m_fModeless=false;

		return (int)::DialogBoxParam(hinst,pszTemplate,hwndOwner,DialogProc,
									 reinterpret_cast<LPARAM>(this));
	}

	bool CDialog::CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
	{
		if (m_hDlg!=nullptr)
			return false;

		m_fModeless=true;

		if (::CreateDialogParam(hinst,pszTemplate,hwndOwner,DialogProc,
								reinterpret_cast<LPARAM>(this))==nullptr)
			return false;

		return true;
	}

	INT_PTR CALLBACK CDialog::DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		CDialog *pThis;

		if (uMsg==WM_INITDIALOG) {
			pThis=reinterpret_cast<CDialog*>(lParam);
			pThis->m_hDlg=hDlg;
			::SetProp(hDlg,TEXT("This"),pThis);
		} else {
			pThis=static_cast<CDialog*>(::GetProp(hDlg,TEXT("This")));
			if (uMsg==WM_NCDESTROY) {
				if (pThis!=nullptr) {
					pThis->HandleMessage(hDlg,uMsg,wParam,lParam);
					pThis->m_hDlg=nullptr;
				}
				::RemoveProp(hDlg,TEXT("This"));
				return TRUE;
			}
		}

		if (pThis!=nullptr)
			return pThis->HandleMessage(hDlg,uMsg,wParam,lParam);

		return FALSE;
	}

	bool CDialog::Destroy()
	{
		if (m_hDlg!=nullptr) {
			if (!::DestroyWindow(m_hDlg))
				return false;
			m_hDlg=nullptr;
		}

		return true;
	}

	bool CDialog::ProcessMessage(LPMSG pMsg)
	{
		if (m_hDlg==nullptr)
			return false;

		return ::IsDialogMessage(m_hDlg,pMsg)!=FALSE;
	}

	bool CDialog::IsVisible() const
	{
		return m_hDlg!=nullptr && ::IsWindowVisible(m_hDlg);
	}

	bool CDialog::SetVisible(bool fVisible)
	{
		if (m_hDlg==nullptr)
			return false;

		return ::ShowWindow(m_hDlg,fVisible?SW_SHOW:SW_HIDE)!=FALSE;
	}

	bool CDialog::GetPosition(RECT *pPosition) const
	{
		if (pPosition==nullptr)
			return false;

		if (m_hDlg==nullptr)
			m_Position.Get(pPosition);
		else
			::GetWindowRect(m_hDlg,pPosition);

		return true;
	}

	bool CDialog::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
	{
		RECT rc;

		GetPosition(&rc);
		if (pLeft!=nullptr)
			*pLeft=rc.left;
		if (pTop!=nullptr)
			*pTop=rc.top;
		if (pWidth!=nullptr)
			*pWidth=rc.right-rc.left;
		if (pHeight!=nullptr)
			*pHeight=rc.bottom-rc.top;

		return true;
	}

	bool CDialog::SetPosition(const RECT &Position)
	{
		return SetPosition(Position.left,Position.top,
						   Position.right-Position.left,
						   Position.bottom-Position.top);
	}

	bool CDialog::SetPosition(int Left,int Top,int Width,int Height)
	{
		if (Width<0 || Height<0)
			return false;

		if (m_hDlg==nullptr) {
			m_Position.x=Left;
			m_Position.y=Top;
			m_Position.Width=Width;
			m_Position.Height=Height;
		} else {
			::MoveWindow(m_hDlg,Left,Top,Width,Height,TRUE);
		}

		return true;
	}

	bool CDialog::End(int Result)
	{
		if (m_hDlg==nullptr)
			return false;

		if (m_fModeless) {
			Destroy();
		} else {
			::EndDialog(m_hDlg,Result);
		}

		return true;
	}

	INT_PTR CDialog::HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		return DlgProc(hDlg,uMsg,wParam,lParam);
	}

	INT_PTR CDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			return TRUE;
		}

		return FALSE;
	}

	bool CDialog::AdjustPos()
	{
		HWND hwndOwner=::GetParent(m_hDlg);
		if (hwndOwner!=nullptr && !::IsIconic(hwndOwner) && !::IsZoomed(hwndOwner)) {
			MONITORINFO mi;
			mi.cbSize=sizeof(mi);
			if (::GetMonitorInfo(::MonitorFromWindow(hwndOwner,MONITOR_DEFAULTTONEAREST),&mi)) {
				RECT rcOwner,rcDlg;
				int y;

				::GetWindowRect(hwndOwner,&rcOwner);
				::GetWindowRect(m_hDlg,&rcDlg);
				if (rcOwner.top-mi.rcWork.top>=mi.rcWork.bottom-rcOwner.bottom) {
					y=rcOwner.top-(rcDlg.bottom-rcDlg.top);
					if (y<mi.rcWork.top)
						y=mi.rcWork.top;
				} else {
					y=rcOwner.bottom;
					if (y+(rcDlg.bottom-rcDlg.top)>mi.rcWork.bottom)
						y=mi.rcWork.bottom-(rcDlg.bottom-rcDlg.top);
				}
				::SetWindowPos(m_hDlg,nullptr,rcDlg.left,y,0,0,
							   SWP_NOSIZE | SWP_NOZORDER);
			}
		}

		return true;
	}

	void CDialog::SetMessageResult(LRESULT Result)
	{
		::SetWindowLongPtr(m_hDlg,DWLP_MSGRESULT,Result);
	}

	HWND CDialog::GetItemHandle(int ID) const
	{
		return ::GetDlgItem(m_hDlg,ID);
	}

	bool CDialog::GetItemString(int ID,TSTask::String *pString) const
	{
		if (pString==nullptr)
			return false;

		pString->clear();

		HWND hwnd=::GetDlgItem(m_hDlg,ID);
		if (hwnd==nullptr)
			return false;

		int Length=::GetWindowTextLength(hwnd);
		if (Length>0) {
			LPWSTR pszText=new WCHAR[Length+1];
			if (::GetWindowText(hwnd,pszText,Length+1)!=Length) {
				delete [] pszText;
				return false;
			}
			*pString=pszText;
			delete [] pszText;
		}

		return true;
	}

	bool CDialog::SetItemString(int ID,const TSTask::String &Str)
	{
		return ::SetDlgItemText(m_hDlg,ID,Str.c_str())!=FALSE;
	}

	bool CDialog::SetItemText(int ID,LPCWSTR pszText)
	{
		return ::SetDlgItemText(m_hDlg,ID,pszText)!=FALSE;
	}

	int CDialog::GetItemInt(int ID) const
	{
		return ::GetDlgItemInt(m_hDlg,ID,nullptr,TRUE);
	}

	int CDialog::GetItemTextLength(int ID) const
	{
		return ::GetWindowTextLength(::GetDlgItem(m_hDlg,ID));
	}

	bool CDialog::SetItemInt(int ID,int Value)
	{
		return ::SetDlgItemInt(m_hDlg,ID,Value,TRUE)!=FALSE;
	}

	UINT CDialog::GetItemUInt(int ID) const
	{
		return ::GetDlgItemInt(m_hDlg,ID,nullptr,FALSE);
	}

	bool CDialog::SetItemUInt(int ID,UINT Value)
	{
		return ::SetDlgItemInt(m_hDlg,ID,Value,FALSE)!=FALSE;
	}

	bool CDialog::EnableItem(int ID,bool fEnable)
	{
		HWND hwnd=::GetDlgItem(m_hDlg,ID);
		if (hwnd==nullptr)
			return false;
		return ::EnableWindow(hwnd,fEnable)!=FALSE;
	}

	bool CDialog::EnableItems(int FirstID,int LastID,bool fEnable)
	{
		if (LastID<FirstID) {
			int i=FirstID;
			FirstID=LastID;
			LastID=i;
		}

		for (int i=FirstID;i<=LastID;i++)
			EnableItem(i,fEnable);

		return true;
	}

	bool CDialog::CheckItem(int ID,bool fCheck)
	{
		return ::CheckDlgButton(m_hDlg,ID,fCheck?BST_CHECKED:BST_UNCHECKED)!=FALSE;
	}

	bool CDialog::IsItemChecked(int ID) const
	{
		return ::IsDlgButtonChecked(m_hDlg,ID)==BST_CHECKED;
	}

	bool CDialog::MapDialogUnit(RECT *pRect) const
	{
		return ::MapDialogRect(m_hDlg,pRect)!=FALSE;
	}

	int CDialog::MapDialogUnitX(int Value) const
	{
		RECT rc={0,0,Value,0};
		::MapDialogRect(m_hDlg,&rc);
		return rc.right;
	}

	int CDialog::MapDialogUnitY(int Value) const
	{
		RECT rc={0,0,0,Value};
		::MapDialogRect(m_hDlg,&rc);
		return rc.bottom;
	}


	CResizableDialog::CResizableDialog()
		: m_hwndSizeGrip(nullptr)
	{
	}

	CResizableDialog::~CResizableDialog()
	{
		Destroy();
	}

	INT_PTR CResizableDialog::HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch (uMsg) {
		case WM_INITDIALOG:
			{
				RECT rc;

				::GetWindowRect(hDlg,&rc);
				m_MinSize.cx=rc.right-rc.left;
				m_MinSize.cy=rc.bottom-rc.top;
				::GetClientRect(hDlg,&rc);
				m_OriginalClientSize.cx=rc.right-rc.left;
				m_OriginalClientSize.cy=rc.bottom-rc.top;
				m_hwndSizeGrip=::CreateWindowEx(0,TEXT("SCROLLBAR"),NULL,
					WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
					0,0,rc.right,rc.bottom,m_hDlg,reinterpret_cast<HMENU>(0),
					reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(m_hDlg,GWLP_HINSTANCE)),NULL);
				::SetWindowPos(m_hwndSizeGrip,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
			}
			break;

		case WM_GETMINMAXINFO:
			{
				MINMAXINFO *pmmi=reinterpret_cast<MINMAXINFO*>(lParam);

				pmmi->ptMinTrackSize.x=m_MinSize.cx;
				pmmi->ptMinTrackSize.y=m_MinSize.cy;
			}
			return TRUE;

		case WM_SIZE:
			DoLayout();
			break;

		case WM_DESTROY:
			{
				RECT rc;

				::GetWindowRect(hDlg,&rc);
				m_Position.Set(&rc);
			}
			break;
		}

		return CDialog::HandleMessage(hDlg,uMsg,wParam,lParam);
	}

	void CResizableDialog::DoLayout()
	{
		RECT rc;
		int Width,Height;

		::GetClientRect(m_hDlg,&rc);
		Width=rc.right-rc.left;
		Height=rc.bottom-rc.top;
		for (size_t i=0;i<m_ControlList.size();i++) {
			rc=m_ControlList[i].rcOriginal;
			if ((m_ControlList[i].Align&ALIGN_RIGHT)!=0) {
				rc.right+=Width-m_OriginalClientSize.cx;
				if ((m_ControlList[i].Align&ALIGN_LEFT)==0)
					rc.left+=Width-m_OriginalClientSize.cx;
			}
			if ((m_ControlList[i].Align&ALIGN_BOTTOM)!=0) {
				rc.bottom+=Height-m_OriginalClientSize.cy;
				if ((m_ControlList[i].Align&ALIGN_TOP)==0)
					rc.top+=Height-m_OriginalClientSize.cy;
			}
			::MoveWindow(::GetDlgItem(m_hDlg,m_ControlList[i].ID),
						 rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
		}
		if (m_hwndSizeGrip!=nullptr) {
			::GetWindowRect(m_hwndSizeGrip,&rc);
			::OffsetRect(&rc,-rc.left,-rc.top);
			::MoveWindow(m_hwndSizeGrip,Width-rc.right,Height-rc.bottom,
						 rc.right,rc.bottom,TRUE);
		}
	}

	bool CResizableDialog::AddControl(int ID,unsigned int Align)
	{
		HWND hwnd=::GetDlgItem(m_hDlg,ID);
		if (hwnd==nullptr)
			return false;

		LayoutItem Item;

		Item.ID=ID;
		::GetWindowRect(hwnd,&Item.rcOriginal);
		::MapWindowPoints(nullptr,m_hDlg,reinterpret_cast<LPPOINT>(&Item.rcOriginal),2);
		Item.Align=Align;
		m_ControlList.push_back(Item);
		return true;
	}

	bool CResizableDialog::AddControls(int FirstID,int LastID,unsigned int Align)
	{
		if (FirstID>LastID)
			return false;
		for (int i=FirstID;i<=LastID;i++) {
			if (!AddControl(i,Align))
				return false;
		}
		return true;
	}

	void CResizableDialog::ApplyPosition()
	{
		if (m_Position.Width<m_MinSize.cx)
			m_Position.Width=m_MinSize.cx;
		if (m_Position.Height<m_MinSize.cy)
			m_Position.Height=m_MinSize.cy;

		RECT rc;
		m_Position.Get(&rc);
		HMONITOR hMonitor=::MonitorFromRect(&rc,MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi;
		mi.cbSize=sizeof(mi);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			if (rc.left<mi.rcMonitor.left)
				m_Position.x=mi.rcMonitor.left;
			else if (rc.right>mi.rcMonitor.right)
				m_Position.x=mi.rcMonitor.right-m_Position.Width;
			if (rc.top<mi.rcMonitor.top)
				m_Position.y=mi.rcMonitor.top;
			else if (rc.bottom>mi.rcMonitor.bottom)
				m_Position.y=mi.rcMonitor.bottom-m_Position.Height;
		}

		::MoveWindow(m_hDlg,m_Position.x,m_Position.y,
					 m_Position.Width,m_Position.Height,FALSE);
	}

}
