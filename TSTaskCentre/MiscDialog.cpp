#include "stdafx.h"
#include "TSTaskCentre.h"
#include "MiscDialog.h"
#include "../Common/DebugDef.h"


namespace TSTaskCentre
{

	bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult)
	{
		if (pcrResult==nullptr)
			return false;

		static COLORREF crCustomColors[16] = {
			RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
			RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
			RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
			RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
			RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
			RGB(0xFF,0xFF,0xFF)
		};
		CHOOSECOLOR cc;

		cc.lStructSize=sizeof(CHOOSECOLOR);
		cc.hwndOwner=hwndOwner;
		cc.hInstance=NULL;
		cc.rgbResult=*pcrResult;
		cc.lpCustColors=crCustomColors;
		cc.Flags=CC_RGBINIT | CC_FULLOPEN;
		if (!::ChooseColor(&cc))
			return false;
		*pcrResult=cc.rgbResult;
		return true;
	}

	bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf)
	{
		if (plf==nullptr)
			return false;

		CHOOSEFONT cf;

		cf.lStructSize=sizeof(CHOOSEFONT);
		cf.hwndOwner=hwndOwner;
		cf.lpLogFont=plf;
		cf.Flags=CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
		return ::ChooseFont(&cf)!=FALSE;
	}

#if 0

	static int CALLBACK BrowseFolderCallback(HWND hwnd,UINT uMsg,LPARAM lpData,LPARAM lParam)
	{
		switch (uMsg) {
		case BFFM_INITIALIZED:
			if (((LPTSTR)lParam)[0]!=_T('\0')) {
				TCHAR szDirectory[MAX_PATH];

				::lstrcpy(szDirectory,(LPTSTR)lParam);
				::PathRemoveBackslash(szDirectory);
				::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szDirectory);
			}
			break;
		}
		return 0;
	}

	bool BrowseFolderDialog(HWND hwndOwner,TSTask::String *pDirectory,LPCTSTR pszTitle)
	{
		if (pDirectory==nullptr)
			return false;

		BROWSEINFO bi;
		TCHAR szDirectory[MAX_PATH];
		PIDLIST_ABSOLUTE pidl;
		BOOL fRet;

		::lstrcpyn(szDirectory,pDirectory->c_str(),_countof(szDirectory));
		bi.hwndOwner=hwndOwner;
		bi.pidlRoot=NULL;
		bi.pszDisplayName=szDirectory;
		bi.lpszTitle=pszTitle;
		bi.ulFlags=BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		bi.lpfn=BrowseFolderCallback;
		bi.lParam=reinterpret_cast<LPARAM>(szDirectory);
		pidl=::SHBrowseForFolder(&bi);
		if (pidl==NULL)
			return false;
		fRet=::SHGetPathFromIDList(pidl,szDirectory);
		::CoTaskMemFree(pidl);
		if (!fRet)
			return false;
		*pDirectory=szDirectory;
		return true;
	}

#else

	bool BrowseFolderDialog(HWND hwndOwner,TSTask::String *pDirectory,LPCTSTR pszTitle)
	{
		if (pDirectory==nullptr)
			return false;

		IFileOpenDialog *pFileOpenDialog;
		HRESULT hr;

		hr=::CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,
							  IID_PPV_ARGS(&pFileOpenDialog));
		if (FAILED(hr))
			return false;

		FILEOPENDIALOGOPTIONS Options;
		pFileOpenDialog->GetOptions(&Options);
		pFileOpenDialog->SetOptions(Options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

		if (!pDirectory->empty()) {
			IShellItem *psiFolder;
			hr=::SHCreateItemFromParsingName(pDirectory->c_str(),nullptr,IID_PPV_ARGS(&psiFolder));
			if (SUCCEEDED(hr)) {
				IShellItem *psiParent;
				hr=psiFolder->GetParent(&psiParent);
				if (SUCCEEDED(hr)) {
					pFileOpenDialog->SetFolder(psiParent);
					LPWSTR pszName;
					hr=psiFolder->GetDisplayName(SIGDN_NORMALDISPLAY,&pszName);
					if (SUCCEEDED(hr)) {
						pFileOpenDialog->SetFileName(pszName);
						::CoTaskMemFree(pszName);
					}
					psiParent->Release();
				}
				psiFolder->Release();
			}
		}

		if (!TSTask::IsStringEmpty(pszTitle))
			pFileOpenDialog->SetTitle(pszTitle);

		bool fOK=false;
		hr=pFileOpenDialog->Show(hwndOwner);
		if (SUCCEEDED(hr)) {
			LPWSTR pszPath;
			IShellItem *psi;
			hr=pFileOpenDialog->GetResult(&psi);
			if (SUCCEEDED(hr)) {
				hr=psi->GetDisplayName(SIGDN_FILESYSPATH,&pszPath);
				if (SUCCEEDED(hr)) {
					*pDirectory=pszPath;
					fOK=true;
					::CoTaskMemFree(pszPath);
				}
				psi->Release();
			}
		}

		pFileOpenDialog->Release();

		return fOK;
	}

#endif

}
