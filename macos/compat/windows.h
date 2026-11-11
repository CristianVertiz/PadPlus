// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Minimal Win32 type/constant compatibility layer so shared Notepad++ core sources parse and
// compile on macOS. Only what the core actually uses; grow on demand. No behaviour here.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <climits>
#include <ctime>
#include <string>

#ifndef __cdecl
#define __cdecl
#endif
#define __stdcall
#define WINAPI
#define CALLBACK
#define APIENTRY
#define PASCAL
#define FAR
#define NEAR
#define CONST const
#define VOID void
#define _MAX_PATH 260
#define MAX_PATH 260
#define _T(x) L##x
#define TEXT(x) L##x
#define __declspec(x)
#define _In_
#define _Out_
#define _Inout_
#define _In_opt_
#define _Out_opt_
#define __uuidof(x) 0

typedef int BOOL; typedef unsigned char BYTE; typedef unsigned short WORD; typedef uint32_t DWORD;
typedef uint32_t UINT; typedef int32_t INT; typedef int32_t LONG; typedef uint32_t ULONG;
typedef int64_t LONGLONG; typedef uint64_t ULONGLONG; typedef uint64_t DWORDLONG;
typedef intptr_t INT_PTR, LONG_PTR, LPARAM, LRESULT; typedef uintptr_t UINT_PTR, ULONG_PTR, WPARAM, DWORD_PTR, SIZE_T;
typedef wchar_t WCHAR, TCHAR; typedef char CHAR; typedef short SHORT; typedef unsigned short USHORT;
typedef unsigned char UCHAR; typedef float FLOAT; typedef void *PVOID, *LPVOID; typedef const void *LPCVOID;
typedef BYTE *LPBYTE, *PBYTE; typedef DWORD *LPDWORD, *PDWORD; typedef WORD *LPWORD; typedef UINT *PUINT;
typedef WCHAR *LPWSTR, *PWSTR, *LPTSTR; typedef const WCHAR *LPCWSTR, *PCWSTR, *LPCTSTR;
typedef CHAR *LPSTR, *PSTR; typedef const CHAR *LPCSTR, *PCSTR; typedef BOOL *LPBOOL, *PBOOL; typedef int *LPINT;
typedef LONG *PLONG, *LPLONG; typedef void *HANDLE, *HWND, *HINSTANCE, *HMODULE, *HDC, *HMENU, *HICON, *HCURSOR,
	*HBITMAP, *HBRUSH, *HPEN, *HFONT, *HGDIOBJ, *HRGN, *HIMAGELIST, *HHOOK, *HKEY, *HTREEITEM, *HACCEL, *HMONITOR,
	*HGLOBAL, *HLOCAL, *HRSRC, *HPALETTE, *HDWP, *HTHEME, *HDROP, *HRESULT_dummy;
typedef HANDLE *PHANDLE; typedef HINSTANCE HMODULE_t;
typedef long HRESULT; typedef WORD ATOM; typedef DWORD COLORREF, *LPCOLORREF; typedef UINT_PTR SOCKET;
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID);
typedef LRESULT (CALLBACK *HOOKPROC)(int, WPARAM, LPARAM);
typedef int (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);
typedef int (CALLBACK *FARPROC)();

#define TRUE 1
#define FALSE 0
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define NO_ERROR 0
#define ERROR_SUCCESS 0
#define S_OK 0
#define S_FALSE 1
#define E_FAIL ((HRESULT)0x80004005L)
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#define FAILED(hr) ((HRESULT)(hr) < 0)
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))
#define MAKEINTRESOURCE(i) ((LPWSTR)((ULONG_PTR)((WORD)(i))))
#define UNREFERENCED_PARAMETER(P) (void)(P)
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define _countof(a) ARRAYSIZE(a)
#define ZeroMemory(p, n) memset((p), 0, (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#define FILE_ATTRIBUTE_DIRECTORY 0x10
#define FILE_ATTRIBUTE_READONLY 0x1
#define FILE_ATTRIBUTE_HIDDEN 0x2
#define FILE_ATTRIBUTE_NORMAL 0x80

struct POINT { LONG x, y; }; typedef POINT *LPPOINT;
struct SIZE { LONG cx, cy; }; typedef SIZE *LPSIZE;
struct RECT { LONG left, top, right, bottom; }; typedef RECT *LPRECT; typedef const RECT *LPCRECT;
struct FILETIME { DWORD dwLowDateTime, dwHighDateTime; };
struct SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; };
struct GUID { uint32_t Data1; uint16_t Data2, Data3; uint8_t Data4[8]; };
union LARGE_INTEGER { struct { DWORD LowPart; LONG HighPart; }; LONGLONG QuadPart; };
struct LOGFONTW { LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight; BYTE lfItalic, lfUnderline, lfStrikeOut,
	lfCharSet, lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily; WCHAR lfFaceName[32]; };
typedef LOGFONTW LOGFONT;
struct WIN32_FIND_DATAW { DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
	DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1; WCHAR cFileName[MAX_PATH]; WCHAR cAlternateFileName[14]; };
typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
struct WIN32_FILE_ATTRIBUTE_DATA { DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
	DWORD nFileSizeHigh, nFileSizeLow; };

#define CP_ACP 0
#define CP_UTF8 65001
#define CP_OEMCP 1
union ULARGE_INTEGER { struct { DWORD LowPart; DWORD HighPart; }; ULONGLONG QuadPart; };
typedef void *_locale_t_dummy;
#define COINIT_MULTITHREADED 0x0
#define COINIT_APARTMENTTHREADED 0x2
#define RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)
inline HRESULT CoInitializeEx(void*, DWORD) { return S_OK; }
inline void CoUninitialize() {}
struct TOOLINFO { UINT cbSize, uFlags; HWND hwnd; UINT_PTR uId; RECT rect; HINSTANCE hinst; LPWSTR lpszText; LPARAM lParam; };

#include "win_consts.h"
#include "win_types.h"
#include "win_impl.h"
#if __has_include("win_auto.h")
#include "win_auto.h"
#endif
