#pragma once

#ifndef WINVER
#define WINVER 0x0601
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// Winsock2 とヘッダが干渉しないようにする
#define _WINSOCKAPI_

#define PSAPI_VERSION 1

#include <cstdlib>
#include <cstdio>
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>

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
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef _DEBUG
	#define _SECURE_SCL 0
#endif
