#ifndef TSTASKCENTRE_MISC_DIALOG_H
#define TSTASKCENTRE_MISC_DIALOG_H


namespace TSTaskCentre
{

	bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult);
	bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf);
	bool BrowseFolderDialog(HWND hwndOwner,TSTask::String *pDirectory,LPCTSTR pszTitle);

}


#endif
