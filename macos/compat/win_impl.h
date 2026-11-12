// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Real (POSIX-backed) implementations of the non-UI Win32 APIs the shared core depends on:
// string helpers, last-error, code-page conversion, file I/O, file attributes, directory enumeration, paths.
// Implemented in win_impl.cpp. wchar_t is UTF-32 on macOS; "ANSI" code page == UTF-8.
#pragma once
#include <windows.h>

DWORD GetLastError(); void SetLastError(DWORD e);
int MulDiv(int a, int b, int c);
int _wcsicmp(const wchar_t*, const wchar_t*); int _wcsnicmp(const wchar_t*, const wchar_t*, size_t);
int lstrcmpW(const wchar_t*, const wchar_t*); int lstrcmpiW(const wchar_t*, const wchar_t*); int lstrlenW(const wchar_t*);
#define lstrcmp lstrcmpW
#define lstrcmpi lstrcmpiW
#define lstrlen lstrlenW
inline int lstrcmpA(const char* a, const char* b) { return strcmp(a, b); }
int wcscpy_s(wchar_t* d, size_t n, const wchar_t* s); int wcsncpy_s(wchar_t* d, size_t n, const wchar_t* s, size_t c);
int wcscat_s(wchar_t* d, size_t n, const wchar_t* s);
inline unsigned short _byteswap_ushort(unsigned short v) { return (unsigned short)((v << 8) | (v >> 8)); }
inline unsigned long _byteswap_ulong(unsigned long v) { return __builtin_bswap32((uint32_t)v); }

int MultiByteToWideChar(UINT cp, DWORD flags, const char* mb, int cbMb, wchar_t* wc, int cchWc);
int WideCharToMultiByte(UINT cp, DWORD flags, const wchar_t* wc, int cchWc, char* mb, int cbMb, const char* defChar, BOOL* usedDef);

#define MAX_PATH_W 260
enum GET_FILEEX_INFO_LEVELS { GetFileExInfoStandard };
BOOL CloseHandle(HANDLE h);
HANDLE CreateFileW(LPCWSTR path, DWORD access, DWORD share, SECURITY_ATTRIBUTES* sa, DWORD disp, DWORD flags, HANDLE tmpl);
BOOL ReadFile(HANDLE h, LPVOID buf, DWORD n, DWORD* read, void* ov);
BOOL WriteFile(HANDLE h, const void* buf, DWORD n, DWORD* written, void* ov);
BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER dist, LARGE_INTEGER* newPos, DWORD method);
BOOL FlushFileBuffers(HANDLE h);
BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* size);
BOOL GetFileTime(HANDLE h, FILETIME* c, FILETIME* a, FILETIME* w);
BOOL SetFileTime(HANDLE h, const FILETIME* c, const FILETIME* a, const FILETIME* w);
DWORD GetFileAttributesW(LPCWSTR p); BOOL SetFileAttributesW(LPCWSTR p, DWORD a);
BOOL GetFileAttributesExW(LPCWSTR p, GET_FILEEX_INFO_LEVELS lvl, void* data);
BOOL CreateDirectoryW(LPCWSTR p, SECURITY_ATTRIBUTES*); BOOL DeleteFileW(LPCWSTR p); BOOL RemoveDirectoryW(LPCWSTR p);
BOOL MoveFileW(LPCWSTR from, LPCWSTR to); BOOL CopyFileW(LPCWSTR from, LPCWSTR to, BOOL failIfExists);
#define MOVEFILE_REPLACE_EXISTING 1
BOOL MoveFileExW(LPCWSTR from, LPCWSTR to, DWORD flags);
HANDLE FindFirstFileW(LPCWSTR pattern, WIN32_FIND_DATAW* fd); BOOL FindNextFileW(HANDLE h, WIN32_FIND_DATAW* fd); BOOL FindClose(HANDLE h);
DWORD GetFullPathNameW(LPCWSTR p, DWORD n, LPWSTR buf, LPWSTR* filePart);
DWORD GetCurrentDirectoryW(DWORD n, LPWSTR buf); DWORD GetModuleFileNameW(HMODULE m, LPWSTR buf, DWORD n);
DWORD GetTempPathW(DWORD n, LPWSTR buf); DWORD ExpandEnvironmentStringsW(LPCWSTR src, LPWSTR dst, DWORD n);
void GetSystemTimeAsFileTime(FILETIME* ft);
BOOL FileTimeToSystemTime(const FILETIME* ft, SYSTEMTIME* st);
#define GetFileAttributes GetFileAttributesW
#define SetFileAttributes SetFileAttributesW
#define GetFullPathName GetFullPathNameW
#define GetCurrentDirectory GetCurrentDirectoryW
#define GetModuleFileName GetModuleFileNameW
#define FindFirstFile FindFirstFileW
#define FindNextFile FindNextFileW
#define CreateFile CreateFileW

// shlwapi path helpers (backslash- and slash-tolerant)
BOOL PathFileExistsW(LPCWSTR); BOOL PathIsDirectoryW(LPCWSTR); BOOL PathIsRelativeW(LPCWSTR);
LPWSTR PathFindFileNameW(LPCWSTR); LPWSTR PathFindExtensionW(LPCWSTR); BOOL PathRemoveFileSpecW(LPWSTR);
BOOL PathAppendW(LPWSTR path, LPCWSTR more); LPWSTR PathCombineW(LPWSTR dst, LPCWSTR dir, LPCWSTR file);
BOOL PathMatchSpecW(LPCWSTR file, LPCWSTR spec);
void PathRemoveExtensionW(LPWSTR path);
#define PathRemoveExtension PathRemoveExtensionW
#define PathFileExists PathFileExistsW
#define PathIsDirectory PathIsDirectoryW
#define PathIsRelative PathIsRelativeW
#define PathFindFileName PathFindFileNameW
#define PathFindExtension PathFindExtensionW
#define PathRemoveFileSpec PathRemoveFileSpecW
#define PathAppend PathAppendW
#define PathCombine PathCombineW
#define PathMatchSpec PathMatchSpecW

template<size_t N> inline int wcscpy_s(wchar_t (&d)[N], const wchar_t* s) { return wcscpy_s(d, N, s); }
template<size_t N> inline int wcsncpy_s(wchar_t (&d)[N], const wchar_t* s, size_t c) { return wcsncpy_s(d, N, s, c); }
template<size_t N> inline int wcscat_s(wchar_t (&d)[N], const wchar_t* s) { return wcscat_s(d, N, s); }
FILE* _wfopen(const wchar_t* path, const wchar_t* mode);
int GetSystemMetrics(int idx);

#include <strings.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define stricmp strcasecmp
#define _strdup strdup
#define _wcsdup wcsdup

HRESULT SHGetFolderPathW(HWND, int csidl, HANDLE, DWORD, LPWSTR out); // APPDATA -> ~/Library/Application Support, etc.
#define SHGetFolderPath SHGetFolderPathW

// ANSI-less names map to the W implementations (Notepad++ builds as UNICODE)
#define CopyFile CopyFileW
#define CreateDirectory CreateDirectoryW
#define DeleteFile DeleteFileW
#define ExpandEnvironmentStrings ExpandEnvironmentStringsW
#define GetFileAttributesEx GetFileAttributesExW
#define MoveFileEx MoveFileExW

// Threads: CreateThread runs the routine synchronously on the caller (see win_impl.cpp); good enough for the core's
// short "worker with timeout" helpers, not a general threading API.
HANDLE CreateThread(SECURITY_ATTRIBUTES*, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, DWORD*);
DWORD WaitForSingleObject(HANDLE, DWORD);
BOOL TerminateThread(HANDLE, DWORD);

// ---- window-message bridge -------------------------------------------------------------------------------------
// The core drives Scintilla through ::SendMessage(hwnd, SCI_*, ...) and creates it via CreateWindowEx("Scintilla").
// On macOS the Qt app installs these hooks; an HWND for a Scintilla window is simply a pointer to its Qt widget.
typedef LRESULT (*NppSendMessageHook)(HWND, UINT, WPARAM, LPARAM);
typedef HWND (*NppCreateWindowHook)(LPCWSTR className, HWND parent);
extern NppSendMessageHook g_nppSendMessageHook;
extern NppCreateWindowHook g_nppCreateWindowHook;
LRESULT SendMessageW(HWND, UINT, WPARAM, LPARAM);
#define SendMessage SendMessageW
HWND CreateWindowExW(DWORD, LPCWSTR cls, LPCWSTR name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU, HINSTANCE, LPVOID);
#define CreateWindowEx CreateWindowExW
inline BOOL Scintilla_RegisterClasses(void*) { return TRUE; }
inline BOOL Scintilla_ReleaseResources() { return TRUE; }

// ---- CRT "safe" string functions and a few Win32 string helpers (real, not stubs) ----
#include <cerrno>
#include <cwctype>
#include <string>
#include <time.h>
#define _TRUNCATE ((size_t)-1)
inline int strncpy_s(char* d, size_t n, const char* s, size_t c) {
	if (!d || !n) return EINVAL;
	if (!s) { d[0] = 0; return EINVAL; }
	size_t l = strlen(s); if (c != _TRUNCATE && c < l) l = c;
	if (l >= n) { if (c != _TRUNCATE) { d[0] = 0; return ERANGE; } l = n - 1; }   // _TRUNCATE: cut to fit
	memcpy(d, s, l); d[l] = 0; return 0;
}
template<size_t N> inline int strncpy_s(char (&d)[N], const char* s, size_t c) { return strncpy_s(d, N, s, c); }
inline int strcpy_s(char* d, size_t n, const char* s) { return strncpy_s(d, n, s, strlen(s ? s : "") + 1) ; }
template<size_t N> inline int strcpy_s(char (&d)[N], const char* s) { return strcpy_s(d, N, s); }
inline int memcpy_s(void* d, size_t dn, const void* s, size_t n) { if (n > dn) return ERANGE; memcpy(d, s, n); return 0; }
template<class... A> inline int sprintf_s(char* b, size_t n, const char* f, A... a) { return snprintf(b, n, f, a...); }
template<size_t N, class... A> inline int sprintf_s(char (&b)[N], const char* f, A... a) { return snprintf(b, N, f, a...); }
// MSVC wide printf: %s is a wide string and %S a narrow one; POSIX is the opposite. Translate the format.
inline std::wstring npp_fixWideFmt(const wchar_t* f) {
	std::wstring o;
	for (; *f; ++f) {
		if (*f != L'%') { o += *f; continue; }
		o += *f++;
		if (*f == L'%') { o += *f; continue; }
		while (*f && wcschr(L"-+ #0123456789.*", *f)) o += *f++;
		if (*f == L's') o += L"ls"; else if (*f == L'S') o += L's'; else if (*f) o += *f; else break;
	}
	return o;
}
template<class... A> inline int swprintf_s(wchar_t* b, size_t n, const wchar_t* f, A... a) { return swprintf(b, n, npp_fixWideFmt(f).c_str(), a...); }
template<size_t N, class... A> inline int swprintf_s(wchar_t (&b)[N], const wchar_t* f, A... a) { return swprintf(b, N, npp_fixWideFmt(f).c_str(), a...); }
template<class... A> inline int wsprintfW(wchar_t* b, const wchar_t* f, A... a) { return swprintf(b, 1024, npp_fixWideFmt(f).c_str(), a...); }
#define wsprintf wsprintfW
inline wchar_t* lstrcpyW(wchar_t* d, const wchar_t* s) { return wcscpy(d, s); }
inline wchar_t* lstrcpynW(wchar_t* d, const wchar_t* s, int n) { if (n <= 0) return d; wcsncpy(d, s, n - 1); d[n - 1] = 0; return d; }
#define lstrcpy lstrcpyW
#define lstrcpyn lstrcpynW
inline char* itoa(int v, char* buf, int radix) {
	if (radix == 10) { snprintf(buf, 34, "%d", v); return buf; }
	char tmp[34]; int i = 0; unsigned u = (unsigned)v; do { int d = u % radix; tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); u /= radix; } while (u);
	for (int k = 0; k < i; ++k) buf[k] = tmp[i - 1 - k]; buf[i] = 0; return buf;
}
inline wchar_t* CharUpperW(wchar_t* p) { if ((uintptr_t)p < 0x10000) return (wchar_t*)(uintptr_t)towupper((wint_t)(uintptr_t)p); for (wchar_t* c = p; *c; ++c) *c = towupper(*c); return p; }
inline wchar_t* CharLowerW(wchar_t* p) { if ((uintptr_t)p < 0x10000) return (wchar_t*)(uintptr_t)towlower((wint_t)(uintptr_t)p); for (wchar_t* c = p; *c; ++c) *c = towlower(*c); return p; }
inline BOOL IsCharAlphaW(wchar_t c) { return iswalpha(c) != 0; }
inline BOOL IsCharAlphaNumericW(wchar_t c) { return iswalnum(c) != 0; }
inline int CompareStringOrdinal(const wchar_t* a, int la, const wchar_t* b, int lb, BOOL ic) {   // 1 less, 2 equal, 3 greater
	std::wstring x(a, la < 0 ? wcslen(a) : la), y(b, lb < 0 ? wcslen(b) : lb);
	if (ic) { for (auto& c : x) c = towlower(c); for (auto& c : y) c = towlower(c); }
	return x < y ? 1 : x == y ? 2 : 3;
}
inline ULONGLONG GetTickCount64() { timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (ULONGLONG)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; }

// ANSI string helpers (the core's Shortcut class stores its names with lstrcpynA; as a no-op stub every command name was empty)
inline char* lstrcpynA(char* d, const char* s, int n) { if (n <= 0) return d; strncpy(d, s, n - 1); d[n - 1] = 0; return d; }
inline char* lstrcpyA(char* d, const char* s) { return strcpy(d, s); }
inline char* lstrcatA(char* d, const char* s) { return strcat(d, s); }
inline int lstrlenA(const char* s) { return s ? (int)strlen(s) : 0; }
// A view that exists counts as visible. As a no-op stub (always "hidden"), FileManager ignored every open untitled document when it
// numbered a new one, so each was called "new 1"; looking an untitled document up by name failed for the same reason.
#define NPP_HAVE_IsWindowVisible
inline BOOL IsWindowVisible(HWND h) { return h != nullptr; }
#define NPP_HAVE__wtoi   // without this the catch-all stub template in win_auto.h wins overload resolution for a non-const argument
inline int _wtoi(const wchar_t* s) { return s ? static_cast<int>(wcstol(s, nullptr, 10)) : 0; }   // was a stub returning 0: the number of "new N" was never read back
