#ifndef TSTASKCENTRE_INFORMATION_BAR_ITMES_H
#define TSTASKCENTRE_INFORMATION_BAR_ITMES_H


#include "InformationBar.h"


namespace TSTaskCentre
{

	namespace InformationBarItems
	{

		class CCustomInformationItem : public CInformationBar::CItem
		{
		public:
			struct ParameterInfo
			{
				LPCWSTR pszParameter;
				LPCWSTR pszText;
			};

			CCustomInformationItem(int ID);
			~CCustomInformationItem();

		// CInformationBar::CItem
			void Draw(const DrawInfo &Info,const RECT &TextRect,const RECT &ClipRect) override;
			bool Update() override;

			bool SetFormat(LPCWSTR pszFormat);

			static bool GetText(LPCWSTR pszFormat,TSTask::String *pText);
			static bool GetParameterInfo(int Index,ParameterInfo *pInfo);

		private:
			TSTask::String m_Text;
			TSTask::String m_Format;
		};

	}

}


#endif
