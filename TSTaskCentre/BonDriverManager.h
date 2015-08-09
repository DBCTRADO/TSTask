#ifndef TSTASKCENTRE_BONDRIVER_MANAGER_H
#define TSTASKCENTRE_BONDRIVER_MANAGER_H


#include <vector>


namespace TSTaskCentre
{

	class CBonDriverManager
	{
	public:
		typedef std::vector<TSTask::String> BonDriverFileList;

		CBonDriverManager();
		~CBonDriverManager();
		bool GetBonDriverFileList(LPCWSTR pszDirectory,BonDriverFileList *pList);
	};

}


#endif
