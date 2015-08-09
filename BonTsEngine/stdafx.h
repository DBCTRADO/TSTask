#pragma once

#ifndef WINVER
#define WINVER 0x0600
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

// Winsock2 �ƃw�b�_�������Ȃ��悤�ɂ���
#define _WINSOCKAPI_

#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#ifndef _ALLOW_KEYWORD_MACROS
#define _ALLOW_KEYWORD_MACROS
#endif
#endif

#include <stdio.h>
#include <process.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>
#ifdef _DEBUG
#undef strdup
#define strdup strdup
#define DEBUG_NEW new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif // _DEBUG

#undef _WINSOCKAPI_

// �x���̖����ݒ�
#pragma warning(disable: 4355) // warning C4355: "'this' : �x�[�X �����o���������X�g�Ŏg�p����܂����B"
#pragma warning(disable: 4995) // warning C4995: ���O��������ꂽ #pragma �Ƃ��ċL�q����Ă��܂��B
#pragma warning(disable: 4996) // warning C4996: "This function or variable may be unsafe."

// ���C�u�����̃����N
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "strmiids.lib")

// �g���[�X�o��
#ifdef _DEBUG
	#undef TRACE
	#define TRACE ::DebugTrace
	void DebugTrace(LPCTSTR szFormat, ...);
#else
	#define TRACE __noop
#endif

#ifndef _DEBUG
	#define _SECURE_SCL 0
#endif

// �A���C�������g�w��
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
#define alignas(n) __declspec(align(n))
#define alignof(t) __alignof(t)
#endif

#include "BonTsEngineConfig.h"
