#ifndef TS_TASK_H
#define TS_TASK_H


#ifndef RC_INVOKED
#include "../Common/TSTaskCommon.h"
#endif


#define APP_NAME	"TSTask"
#define APP_NAME_W	L"TSTask"

#define APP_VERSION_MAJOR		0
#define APP_VERSION_MINOR		2
#define APP_VERSION_REVISION	0
#define APP_VERSION_BUILD		0

#define APP_VERSION_TEXT		"0.2.0"
#define APP_VERSION_TEXT_W		L"0.2.0"

#if defined(_M_IX86)
#define APP_PLATFORM_W	L"x86"
#elif defined(_M_X64)
#define APP_PLATFORM_W	L"x64"
#endif


#endif
