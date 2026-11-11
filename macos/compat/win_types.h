// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Win32 struct/handle types referenced by core headers (field layouts follow the Windows SDK).
#pragma once
#include <windows.h>
struct MSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; };
typedef MSG *LPMSG;
struct NMHDR { HWND hwndFrom; UINT_PTR idFrom; UINT code; }; typedef NMHDR *LPNMHDR;
struct ACCEL { BYTE fVirt; WORD key; WORD cmd; };
struct MINMAXINFO { POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; };
struct WINDOWPLACEMENT { UINT length, flags, showCmd; POINT ptMinPosition, ptMaxPosition; RECT rcNormalPosition; };
struct DRAWITEMSTRUCT { UINT CtlType, CtlID, itemID, itemAction, itemState; HWND hwndItem; HDC hDC; RECT rcItem; ULONG_PTR itemData; };
typedef DRAWITEMSTRUCT *LPDRAWITEMSTRUCT;
struct TCITEMW { UINT mask; DWORD dwState, dwStateMask; LPWSTR pszText; int cchTextMax, iImage; LPARAM lParam; }; typedef TCITEMW TCITEM;
struct TCHITTESTINFO { POINT pt; UINT flags; };
struct LVCOLUMNW { UINT mask; int fmt, cx; LPWSTR pszText; int cchTextMax, iSubItem, iImage, iOrder, cxMin, cxDefault, cxIdeal; }; typedef LVCOLUMNW LVCOLUMN;
struct REBARBANDINFOW { UINT cbSize, fMask, fStyle; COLORREF clrFore, clrBack; LPWSTR lpText; UINT cch; int iImage; HWND hwndChild;
	UINT cxMinChild, cyMinChild, cx; HBITMAP hbmBack; UINT wID, cyChild, cyMaxChild, cyIntegral, cxIdeal; LPARAM lParam; UINT cxHeader; }; typedef REBARBANDINFOW REBARBANDINFO;
struct TBBUTTON { int iBitmap, idCommand; BYTE fsState, fsStyle; BYTE bReserved[6]; DWORD_PTR dwData; INT_PTR iString; };
struct NMTOOLBARW { NMHDR hdr; int iItem; TBBUTTON tbButton; int cchText; LPWSTR pszText; RECT rcButton; }; typedef NMTOOLBARW *LPNMTOOLBARW;
struct NOTIFYICONDATAW { DWORD cbSize; HWND hWnd; UINT uID, uFlags, uCallbackMessage; HICON hIcon; WCHAR szTip[128]; DWORD dwState, dwStateMask;
	WCHAR szInfo[256]; UINT uTimeout; WCHAR szInfoTitle[64]; DWORD dwInfoFlags; }; typedef NOTIFYICONDATAW NOTIFYICONDATA;
struct SHELLEXECUTEINFOW { DWORD cbSize, fMask; HWND hwnd; LPCWSTR lpVerb, lpFile, lpParameters, lpDirectory; int nShow; HINSTANCE hInstApp;
	void* lpIDList; LPCWSTR lpClass; HKEY hkeyClass; DWORD dwHotKey; HANDLE hIcon; HANDLE hProcess; };
struct SHFILEOPSTRUCTW { HWND hwnd; UINT wFunc; LPCWSTR pFrom, pTo; WORD fFlags; BOOL fAnyOperationsAborted; LPVOID hNameMappings; LPCWSTR lpszProgressTitle; };
typedef SHFILEOPSTRUCTW SHFILEOPSTRUCT;
struct ITEMIDLIST { BYTE mkid[2]; };
struct OSVERSIONINFOEXW { DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId; WCHAR szCSDVersion[128];
	WORD wServicePackMajor, wServicePackMinor, wSuiteMask; BYTE wProductType, wReserved; }; typedef OSVERSIONINFOEXW *LPOSVERSIONINFOW;
struct VS_FIXEDFILEINFO { DWORD dwSignature, dwStrucVersion, dwFileVersionMS, dwFileVersionLS, dwProductVersionMS, dwProductVersionLS,
	dwFileFlagsMask, dwFileFlags, dwFileOS, dwFileType, dwFileSubtype, dwFileDateMS, dwFileDateLS; };
struct WIN32_FIND_STREAM_DATA { LARGE_INTEGER StreamSize; WCHAR cStreamName[MAX_PATH + 36]; };
typedef struct tagCHOOSECOLORW *LPCHOOSECOLORW;
typedef int DPI_AWARENESS_CONTEXT_t; typedef void* DPI_AWARENESS_CONTEXT;
typedef void *HCERTSTORE, *HCRYPTMSG;
typedef _locale_t_dummy _locale_t_unused;
typedef void* _locale_t;
#define __inout
typedef void *PSECURITY_DESCRIPTOR; struct SECURITY_ATTRIBUTES { DWORD nLength; LPVOID lpSecurityDescriptor; BOOL bInheritHandle; };
typedef void *BCRYPT_ALG_HANDLE, *BCRYPT_HASH_HANDLE; typedef UCHAR *PUCHAR;
struct SYSTEM_INFO { WORD wProcessorArchitecture, wReserved; DWORD dwPageSize; LPVOID lpMinimumApplicationAddress, lpMaximumApplicationAddress;
	DWORD_PTR dwActiveProcessorMask; DWORD dwNumberOfProcessors, dwProcessorType, dwAllocationGranularity; WORD wProcessorLevel, wProcessorRevision; };
typedef SYSTEM_INFO *LPSYSTEM_INFO;
struct TEXTMETRICW { LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading, tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang,
	tmDigitizedAspectX, tmDigitizedAspectY; WCHAR tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar; BYTE tmItalic, tmUnderlined, tmStruckOut,
	tmPitchAndFamily, tmCharSet; }; typedef TEXTMETRICW TEXTMETRIC;
struct ENUMLOGFONTEXW { LOGFONTW elfLogFont; WCHAR elfFullName[64], elfStyle[32], elfScript[32]; }; typedef ENUMLOGFONTEXW ENUMLOGFONTEX;
typedef OSVERSIONINFOEXW OSVERSIONINFOEX; typedef OSVERSIONINFOEXW OSVERSIONINFOW, OSVERSIONINFO;
struct MONITORINFO { DWORD cbSize; RECT rcMonitor, rcWork; DWORD dwFlags; }; typedef MONITORINFO *LPMONITORINFO;
struct CRYPT_INTEGER_BLOB { DWORD cbData; BYTE* pbData; }; typedef CRYPT_INTEGER_BLOB CERT_NAME_BLOB, CRYPT_DATA_BLOB;
struct CERT_INFO { DWORD dwVersion; CRYPT_INTEGER_BLOB SerialNumber; CERT_NAME_BLOB Issuer; };
struct CERT_CONTEXT { CERT_INFO* pCertInfo; }; typedef const CERT_CONTEXT *PCCERT_CONTEXT;
struct CMSG_SIGNER_INFO { DWORD dwVersion; CRYPT_INTEGER_BLOB Issuer, SerialNumber; };
typedef CMSG_SIGNER_INFO *PCMSG_SIGNER_INFO;
enum STREAM_INFO_LEVELS { FindStreamInfoStandard };
template<class... A> inline PCCERT_CONTEXT CertFindCertificateInStore(A&&...) { return nullptr; }
inline HANDLE FindFirstStreamW(LPCWSTR, STREAM_INFO_LEVELS, LPVOID, DWORD) { return INVALID_HANDLE_VALUE; } // APFS has no alternate data streams
struct MEASUREITEMSTRUCT { UINT CtlType, CtlID, itemID, itemWidth, itemHeight; ULONG_PTR itemData; };
typedef WORD LANGID; typedef WORD CLIPFORMAT; struct POINTS { SHORT x, y; };
struct NONCLIENTMETRICSW { UINT cbSize; int iBorderWidth, iScrollWidth, iScrollHeight, iCaptionWidth, iCaptionHeight; LOGFONTW lfCaptionFont;
	int iSmCaptionWidth, iSmCaptionHeight; LOGFONTW lfSmCaptionFont; int iMenuWidth, iMenuHeight; LOGFONTW lfMenuFont, lfStatusFont, lfMessageFont; int iPaddingdWidth; };
typedef NONCLIENTMETRICSW NONCLIENTMETRICS;
struct RECONVERTSTRING { DWORD dwSize, dwVersion, dwStrLen, dwStrOffset, dwCompStrLen, dwCompStrOffset, dwTargetStrLen, dwTargetStrOffset; };
inline FARPROC GetProcAddress(HMODULE, const char*) { return nullptr; } // no DLLs to resolve on macOS
struct SCROLLINFO { UINT cbSize, fMask; int nMin, nMax; UINT nPage; int nPos, nTrackPos; };
