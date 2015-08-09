#pragma once

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

// Winsock2 とヘッダが干渉しないようにする
#define _WINSOCKAPI_

#include <cstdlib>
#include <cstdio>
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlwapi.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

#undef _WINSOCKAPI_

// 警告の無効設定
#pragma warning(disable: 4355) // warning C4355: "'this' : ベース メンバ初期化リストで使用されました。"
//#pragma warning(disable: 4995) // warning C4995: 名前が避けられた #pragma として記述されています。
//#pragma warning(disable: 4996) // warning C4996: "This function or variable may be unsafe."

// ライブラリのリンク
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "ShLwApi.lib")

#ifndef _DEBUG
	#define _SECURE_SCL 0
#endif
