#ifndef TSTASKCENTRE_LIST_VIEW_H
#define TSTASKCENTRE_LIST_VIEW_H


namespace TSTaskCentre
{

	class CListView
	{
	public:
		CListView();
		~CListView();
		bool Attach(HWND hwnd);
		void Detach();
		void SetExtendedStyle(DWORD Style);
		bool InitCheckList();
		bool SetTheme(LPCWSTR pszName);
		int InsertItem(int Index,LPCTSTR pszText,LPARAM Param=0);
		bool DeleteItem(int Index);
		void DeleteAllItems();
		bool SetItemText(int Index,LPCTSTR pszText);
		bool SetItemText(int Index,int SubItem,LPCTSTR pszText);
		bool GetItemText(int Index,LPTSTR pszText,int MaxText) const;
		bool GetItemText(int Index,int SubItem,LPTSTR pszText,int MaxText) const;
		bool SetItemState(int Index,UINT State,UINT Mask);
		bool CheckItem(int Index,bool fCheck);
		bool IsItemChecked(int Index) const;
		bool SetItemParam(int Index,LPARAM Param);
		LPARAM GetItemParam(int Index) const;
		bool SetItemImage(int Index,int SubItem,int Image);
		int GetItemCount() const;
		void ReserveItemCount(int Count);
		int GetSelectedItem() const;
		bool MoveItem(int From,int To);
		bool EnsureItemVisible(int Index,bool fPartialOK=false);
		int InsertColumn(int Index,LPCTSTR pszText,int Format=LVCFMT_LEFT);
		void SetColumnWidth(int Column,int Width);
		void AdjustColumnWidth(bool fUseHeader=true);
		void AdjustColumnWidth(int Column,bool fUseHeader=true);
		int GetColumnCount() const;
		bool SetImageList(HIMAGELIST himl,int Type);

	protected:
		HWND m_hwnd;
	};

}


#endif
