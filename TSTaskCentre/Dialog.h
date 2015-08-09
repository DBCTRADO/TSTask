#ifndef TSTASKCENTRE_DIALOG_H
#define TSTASKCENTRE_DIALOG_H


#include <vector>


namespace TSTaskCentre
{

	class CDialog abstract
	{
	public:
		CDialog();
		virtual ~CDialog();
		virtual bool Show(HWND hwndOwner,HINSTANCE hinst) { return false; }
		virtual bool Create(HWND hwndOwner,HINSTANCE hinst) { return false; }
		bool IsCreated() const { return m_hDlg!=nullptr; }
		bool Destroy();
		bool IsModeless() const { return m_fModeless; }
		bool ProcessMessage(LPMSG pMsg);
		bool IsVisible() const;
		bool SetVisible(bool fVisible);
		bool GetPosition(RECT *pPosition) const;
		bool GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
		bool SetPosition(const RECT &Position);
		bool SetPosition(int Left,int Top,int Width,int Height);

	protected:
		HWND m_hDlg;
		bool m_fModeless;
		struct Position {
			int x,y;
			int Width,Height;
			Position() : x(0), y(0), Width(0), Height(0) {}
			void Set(const RECT *pRect) {
				x=pRect->left;
				y=pRect->top;
				Width=pRect->right-x;
				Height=pRect->bottom-y;
			}
			void Get(RECT *pRect) const {
				pRect->left=x;
				pRect->top=y;
				pRect->right=x+Width;
				pRect->bottom=y+Height;
			}
		};
		Position m_Position;

		static INT_PTR CALLBACK DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
		int ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
		bool CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate);
		bool End(int Result);
		virtual INT_PTR HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
		virtual INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

		bool AdjustPos();
		void SetMessageResult(LRESULT Result);

		HWND GetItemHandle(int ID) const;
		bool GetItemString(int ID,TSTask::String *pString) const;
		bool SetItemString(int ID,const TSTask::String &Str);
		bool SetItemText(int ID,LPCWSTR pszText);
		int GetItemTextLength(int ID) const;
		int GetItemInt(int ID) const;
		bool SetItemInt(int ID,int Value);
		UINT GetItemUInt(int ID) const;
		bool SetItemUInt(int ID,UINT Value);
		bool EnableItem(int ID,bool fEnable);
		bool EnableItems(int FirstID,int LastID,bool fEnable);
		bool CheckItem(int ID,bool fCheck);
		bool IsItemChecked(int ID) const;
		bool MapDialogUnit(RECT *pRect) const;
		int MapDialogUnitX(int Value) const;
		int MapDialogUnitY(int Value) const;
	};

	class CResizableDialog abstract : public CDialog
	{
	public:
		CResizableDialog();
		virtual ~CResizableDialog();

	protected:
		struct LayoutItem {
			int ID;
			RECT rcOriginal;
			unsigned int Align;
		};
		enum {
			ALIGN_NONE		= 0x00000000,
			ALIGN_LEFT		= 0x00000001,
			ALIGN_TOP		= 0x00000002,
			ALIGN_RIGHT		= 0x00000004,
			ALIGN_BOTTOM	= 0x00000008,
			ALIGN_BOTTOM_RIGHT	= ALIGN_RIGHT | ALIGN_BOTTOM,
			ALIGN_HORZ			= ALIGN_LEFT | ALIGN_RIGHT,
			ALIGN_VERT			= ALIGN_TOP | ALIGN_BOTTOM,
			ALIGN_HORZ_TOP		= ALIGN_HORZ | ALIGN_TOP,
			ALIGN_HORZ_BOTTOM	= ALIGN_HORZ | ALIGN_BOTTOM,
			ALIGN_VERT_LEFT		= ALIGN_LEFT | ALIGN_VERT,
			ALIGN_VERT_RIGHT	= ALIGN_RIGHT | ALIGN_VERT,
			ALIGN_ALL			= ALIGN_HORZ | ALIGN_VERT
		};

		SIZE m_MinSize;
		SIZE m_OriginalClientSize;
		HWND m_hwndSizeGrip;
		std::vector<LayoutItem> m_ControlList;

	// CDialog
		virtual INT_PTR HandleMessage(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		void DoLayout();
		bool AddControl(int ID,unsigned int Align);
		bool AddControls(int FirstID,int LastID,unsigned int Align);
		void ApplyPosition();
	};

}


#endif
