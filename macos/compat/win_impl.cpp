// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// POSIX-backed implementations of the Win32 subset declared in win_impl.h.
#include <windows.h>
#include <cerrno>
#include <cctype>
#include <climits>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glob.h>
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#include <atomic>
#include <string>
#include <vector>

static thread_local DWORD g_lastError = 0;
DWORD GetLastError() { return g_lastError; }
void SetLastError(DWORD e) { g_lastError = e; }
static DWORD errnoToWin(int e) {
	switch (e) { case ENOENT: return 2; case ENOTDIR: return 3; case EACCES: case EPERM: case EROFS: return 5;
		case EEXIST: return 80; case EINVAL: return 87; default: return 31; }
}
static bool traceOn() { static const bool on = getenv("NPP_COMPAT_TRACE") != nullptr; return on; }
static void failAt(const char* fn, const std::string& p) { g_lastError = errnoToWin(errno); if (traceOn()) fprintf(stderr, "[compat] %s('%s') failed: %s\n", fn, p.c_str(), strerror(errno)); }
static void fail() { g_lastError = errnoToWin(errno); }

int MulDiv(int a, int b, int c) { return c ? (int)(((long long)a * b + (c / 2)) / c) : -1; }

int _wcsicmp(const wchar_t* a, const wchar_t* b) {
	for (;; ++a, ++b) { wint_t x = towlower(*a), y = towlower(*b); if (x != y) return x < y ? -1 : 1; if (!*a) return 0; }
}
int _wcsnicmp(const wchar_t* a, const wchar_t* b, size_t n) {
	for (; n; --n, ++a, ++b) { wint_t x = towlower(*a), y = towlower(*b); if (x != y) return x < y ? -1 : 1; if (!*a) return 0; }
	return 0;
}
int lstrcmpW(const wchar_t* a, const wchar_t* b) { return wcscmp(a, b); }
int lstrcmpiW(const wchar_t* a, const wchar_t* b) { return _wcsicmp(a, b); }
int lstrlenW(const wchar_t* s) { return s ? (int)wcslen(s) : 0; }
int wcscpy_s(wchar_t* d, size_t n, const wchar_t* s) { if (!d || !s || wcslen(s) >= n) { if (d && n) d[0] = 0; return ERANGE; } wcscpy(d, s); return 0; }
int wcsncpy_s(wchar_t* d, size_t n, const wchar_t* s, size_t c) {
	size_t l = wcsnlen(s, c); if (!d) return EINVAL;
	if (l >= n) { if (c != (size_t)-1) { if (n) d[0] = 0; return ERANGE; } l = n ? n - 1 : 0; }   // _TRUNCATE: cut to fit
	wmemcpy(d, s, l); d[l] = 0; return 0;
}
int wcscat_s(wchar_t* d, size_t n, const wchar_t* s) { size_t l = wcslen(d); if (l + wcslen(s) >= n) return ERANGE; wcscpy(d + l, s); return 0; }

// ---- UTF-8 <-> UTF-32 (wchar_t). CP_ACP / CP_OEMCP / CP_UTF8 are all treated as UTF-8; CP_UTF16 is not supported here. ----
int MultiByteToWideChar(UINT, DWORD flags, const char* mb, int cb, wchar_t* wc, int cch) {
	if (!mb) { SetLastError(87); return 0; }
	size_t n = cb < 0 ? strlen(mb) + 1 : (size_t)cb;
	int out = 0; size_t i = 0;
	auto put = [&](uint32_t c) { if (wc) { if (out >= cch) return false; wc[out] = (wchar_t)c; } ++out; return true; };
	while (i < n) {
		uint8_t b = (uint8_t)mb[i]; uint32_t c; int len;
		if (b < 0x80) { c = b; len = 1; } else if ((b >> 5) == 6) { c = b & 0x1F; len = 2; } else if ((b >> 4) == 14) { c = b & 0x0F; len = 3; }
		else if ((b >> 3) == 30) { c = b & 7; len = 4; } else { c = 0xFFFD; len = 1; if (flags & MB_ERR_INVALID_CHARS) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; } }
		if (len > 1) {
			if (i + len > n) { c = 0xFFFD; len = (int)(n - i); if (flags & MB_ERR_INVALID_CHARS) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; } }
			else for (int k = 1; k < len; ++k) {
				uint8_t x = (uint8_t)mb[i + k];
				if ((x >> 6) != 2) { c = 0xFFFD; len = k; if (flags & MB_ERR_INVALID_CHARS) { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return 0; } break; }
				c = (c << 6) | (x & 0x3F);
			}
		}
		i += len;
		if (!put(c)) { SetLastError(ERROR_INSUFFICIENT_BUFFER); return 0; }
	}
	return out;
}
int WideCharToMultiByte(UINT, DWORD, const wchar_t* wc, int cch, char* mb, int cb, const char*, BOOL* used) {
	if (used) *used = FALSE;
	if (!wc) { SetLastError(87); return 0; }
	size_t n = cch < 0 ? wcslen(wc) + 1 : (size_t)cch;
	int out = 0;
	auto put = [&](char c) { if (mb) { if (out >= cb) return false; mb[out] = c; } ++out; return true; };
	for (size_t i = 0; i < n; ++i) {
		uint32_t c = (uint32_t)wc[i];
		bool ok = true;
		if (c < 0x80) ok = put((char)c);
		else if (c < 0x800) ok = put((char)(0xC0 | (c >> 6))) && put((char)(0x80 | (c & 0x3F)));
		else if (c < 0x10000) ok = put((char)(0xE0 | (c >> 12))) && put((char)(0x80 | ((c >> 6) & 0x3F))) && put((char)(0x80 | (c & 0x3F)));
		else ok = put((char)(0xF0 | (c >> 18))) && put((char)(0x80 | ((c >> 12) & 0x3F))) && put((char)(0x80 | ((c >> 6) & 0x3F))) && put((char)(0x80 | (c & 0x3F)));
		if (!ok) { SetLastError(ERROR_INSUFFICIENT_BUFFER); return 0; }
	}
	return out;
}

static std::string U8(const wchar_t* w) { int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr); std::string s(n > 0 ? n - 1 : 0, '\0'); if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr); return s; }
static std::wstring W(const char* s) { int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0); std::wstring w(n > 0 ? n - 1 : 0, L'\0'); if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s, -1, w.data(), n); return w; }
static std::string posixPath(const wchar_t* p) { std::string s = U8(p); for (auto& c : s) if (c == '\\') c = '/'; return s; }

// ---- files. HANDLE for a file is (fd + 1) cast to a pointer; INVALID_HANDLE_VALUE is -1. ----
static int fdOf(HANDLE h) { return (int)(intptr_t)h - 1; }
static bool isThreadHandle(HANDLE h) { return (uintptr_t)h >= 0x70000000u && (uintptr_t)h < 0x7FFFFFFFu; }
BOOL CloseHandle(HANDLE h) { if (h == INVALID_HANDLE_VALUE || !h) return FALSE; if (isThreadHandle(h)) return TRUE; return close(fdOf(h)) == 0; }
// ponytail: synchronous "thread"; loses the hung-network-share timeout the Windows code relies on. Use a real
// std::thread + timed join here if a share ever hangs the UI.
HANDLE CreateThread(SECURITY_ATTRIBUTES*, SIZE_T, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD, DWORD* id) {
	static std::atomic<uintptr_t> next{0x70000000u}; if (id) *id = 0; fn(arg); return (HANDLE)next++;
}
DWORD WaitForSingleObject(HANDLE, DWORD) { return WAIT_OBJECT_0; }
BOOL TerminateThread(HANDLE, DWORD) { return TRUE; }
HANDLE CreateFileW(LPCWSTR path, DWORD access, DWORD, SECURITY_ATTRIBUTES*, DWORD disp, DWORD flags, HANDLE) {
	int of = (access & GENERIC_WRITE) ? ((access & GENERIC_READ) ? O_RDWR : O_WRONLY) : O_RDONLY;
	switch (disp) { case CREATE_NEW: of |= O_CREAT | O_EXCL; break; case CREATE_ALWAYS: of |= O_CREAT | O_TRUNC; break;
		case OPEN_ALWAYS: of |= O_CREAT; break; case TRUNCATE_EXISTING: of |= O_TRUNC; break; default: break; }
	std::string p = posixPath(path);
	int fd = open(p.c_str(), of, 0644);
	if (fd < 0) { failAt("CreateFileW", p); return INVALID_HANDLE_VALUE; }
	if (flags & FILE_FLAG_DELETE_ON_CLOSE) unlink(p.c_str());
	return (HANDLE)(intptr_t)(fd + 1);
}
BOOL ReadFile(HANDLE h, LPVOID buf, DWORD n, DWORD* got, void*) { ssize_t r = read(fdOf(h), buf, n); if (r < 0) { fail(); if (got) *got = 0; return FALSE; } if (got) *got = (DWORD)r; return TRUE; }
BOOL WriteFile(HANDLE h, const void* buf, DWORD n, DWORD* put, void*) { ssize_t r = write(fdOf(h), buf, n); if (r < 0) { fail(); if (put) *put = 0; return FALSE; } if (put) *put = (DWORD)r; return TRUE; }
BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER d, LARGE_INTEGER* np, DWORD m) {
	off_t r = lseek(fdOf(h), (off_t)d.QuadPart, m == FILE_BEGIN ? SEEK_SET : m == FILE_CURRENT ? SEEK_CUR : SEEK_END);
	if (r < 0) { fail(); return FALSE; } if (np) np->QuadPart = r; return TRUE;
}
BOOL FlushFileBuffers(HANDLE h) { return fsync(fdOf(h)) == 0; }
BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER* sz) { struct stat st; if (fstat(fdOf(h), &st)) { fail(); return FALSE; } sz->QuadPart = st.st_size; return TRUE; }

static FILETIME toFT(time_t s, long ns) { ULONGLONG t = ((ULONGLONG)s + 11644473600ULL) * 10000000ULL + ns / 100; FILETIME f; f.dwLowDateTime = (DWORD)t; f.dwHighDateTime = (DWORD)(t >> 32); return f; }
static timespec fromFT(const FILETIME& f) { ULONGLONG t = ((ULONGLONG)f.dwHighDateTime << 32) | f.dwLowDateTime; t -= 11644473600ULL * 10000000ULL; return { (time_t)(t / 10000000ULL), (long)((t % 10000000ULL) * 100) }; }
BOOL GetFileTime(HANDLE h, FILETIME* c, FILETIME* a, FILETIME* w) {
	struct stat st; if (fstat(fdOf(h), &st)) { fail(); return FALSE; }
	if (c) *c = toFT(st.st_birthtimespec.tv_sec, st.st_birthtimespec.tv_nsec);
	if (a) *a = toFT(st.st_atimespec.tv_sec, st.st_atimespec.tv_nsec);
	if (w) *w = toFT(st.st_mtimespec.tv_sec, st.st_mtimespec.tv_nsec); return TRUE;
}
BOOL SetFileTime(HANDLE h, const FILETIME*, const FILETIME* a, const FILETIME* w) {
	struct stat st; if (fstat(fdOf(h), &st)) { fail(); return FALSE; }
	timespec ts[2] = { a ? fromFT(*a) : st.st_atimespec, w ? fromFT(*w) : st.st_mtimespec };
	return futimens(fdOf(h), ts) == 0;
}
static DWORD attrsOf(const struct stat& st, const std::string& p) {
	DWORD a = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	if (!(st.st_mode & S_IWUSR)) a |= FILE_ATTRIBUTE_READONLY;
	size_t s = p.find_last_of('/'); if ((s == std::string::npos ? p[0] : p[s + 1]) == '.') a |= FILE_ATTRIBUTE_HIDDEN;
	return a;
}
DWORD GetFileAttributesW(LPCWSTR p) { std::string s = posixPath(p); struct stat st; if (stat(s.c_str(), &st)) { failAt("GetFileAttributesW", s); return INVALID_FILE_ATTRIBUTES; } if (traceOn()) fprintf(stderr, "[compat] GetFileAttributesW('%s') ok\n", s.c_str()); return attrsOf(st, s); }
BOOL SetFileAttributesW(LPCWSTR p, DWORD a) {
	std::string s = posixPath(p); struct stat st; if (stat(s.c_str(), &st)) { fail(); return FALSE; }
	mode_t m = (a & FILE_ATTRIBUTE_READONLY) ? (st.st_mode & ~(S_IWUSR | S_IWGRP | S_IWOTH)) : (st.st_mode | S_IWUSR);
	return chmod(s.c_str(), m) == 0;
}
BOOL GetFileAttributesExW(LPCWSTR p, GET_FILEEX_INFO_LEVELS, void* out) {
	std::string s = posixPath(p); struct stat st; if (stat(s.c_str(), &st)) { fail(); return FALSE; }
	auto* d = (WIN32_FILE_ATTRIBUTE_DATA*)out; d->dwFileAttributes = attrsOf(st, s);
	d->ftCreationTime = toFT(st.st_birthtimespec.tv_sec, st.st_birthtimespec.tv_nsec);
	d->ftLastAccessTime = toFT(st.st_atimespec.tv_sec, st.st_atimespec.tv_nsec);
	d->ftLastWriteTime = toFT(st.st_mtimespec.tv_sec, st.st_mtimespec.tv_nsec);
	d->nFileSizeHigh = (DWORD)((ULONGLONG)st.st_size >> 32); d->nFileSizeLow = (DWORD)st.st_size; return TRUE;
}
// Unlike Windows this creates missing parents too (mkdir -p): macOS profile dirs sit under ~/Library/Application Support.
BOOL CreateDirectoryW(LPCWSTR p, SECURITY_ATTRIBUTES*) {
	std::string s = posixPath(p);
	for (size_t i = 1; i < s.size(); ++i) if (s[i] == '/') { std::string parent = s.substr(0, i); mkdir(parent.c_str(), 0755); }
	if (mkdir(s.c_str(), 0755)) { fail(); return FALSE; } return TRUE;
}
BOOL DeleteFileW(LPCWSTR p) { if (unlink(posixPath(p).c_str())) { fail(); return FALSE; } return TRUE; }
BOOL RemoveDirectoryW(LPCWSTR p) { if (rmdir(posixPath(p).c_str())) { fail(); return FALSE; } return TRUE; }
BOOL MoveFileW(LPCWSTR a, LPCWSTR b) { std::string t = posixPath(b); struct stat st; if (!stat(t.c_str(), &st)) { SetLastError(183); return FALSE; } if (rename(posixPath(a).c_str(), t.c_str())) { fail(); return FALSE; } return TRUE; }
BOOL MoveFileExW(LPCWSTR a, LPCWSTR b, DWORD f) { if (rename(posixPath(a).c_str(), posixPath(b).c_str()) == 0) return TRUE; if (!(f & MOVEFILE_REPLACE_EXISTING)) { fail(); return FALSE; } fail(); return FALSE; }
BOOL CopyFileW(LPCWSTR a, LPCWSTR b, BOOL failIfExists) {
	std::string from = posixPath(a), to = posixPath(b); struct stat st;
	if (traceOn()) fprintf(stderr, "[compat] CopyFileW('%s' -> '%s')\n", from.c_str(), to.c_str());
	if (failIfExists && !stat(to.c_str(), &st)) { SetLastError(80); return FALSE; }
	int in = open(from.c_str(), O_RDONLY); if (in < 0) { failAt("CopyFileW(src)", from); return FALSE; }
	int out = open(to.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); if (out < 0) { failAt("CopyFileW(dst)", to); close(in); return FALSE; }
	char buf[65536]; ssize_t r; BOOL ok = TRUE;
	while ((r = read(in, buf, sizeof buf)) > 0) if (write(out, buf, r) != r) { ok = FALSE; break; }
	close(in); close(out); if (!ok) fail(); return ok;
}

// FindFirstFile: simple wildcard on the last path component. HANDLE = heap-allocated list of matches.
struct FindState { std::vector<std::string> names; size_t next = 0; };
static BOOL fillFind(FindState* fs, const std::string& dir, WIN32_FIND_DATAW* fd) {
	if (fs->next >= fs->names.size()) { SetLastError(18); return FALSE; }
	const std::string& n = fs->names[fs->next++]; struct stat st; std::string full = dir + "/" + n;
	memset(fd, 0, sizeof *fd); if (stat(full.c_str(), &st) == 0) { fd->dwFileAttributes = attrsOf(st, n); fd->nFileSizeLow = (DWORD)st.st_size; fd->nFileSizeHigh = (DWORD)((ULONGLONG)st.st_size >> 32);
		fd->ftLastWriteTime = toFT(st.st_mtimespec.tv_sec, st.st_mtimespec.tv_nsec); }
	std::wstring w = W(n.c_str()); wcsncpy(fd->cFileName, w.c_str(), MAX_PATH - 1); return TRUE;
}
static thread_local std::string g_findDir;
HANDLE FindFirstFileW(LPCWSTR pattern, WIN32_FIND_DATAW* fd) {
	std::string p = posixPath(pattern); size_t s = p.find_last_of('/');
	std::string dir = s == std::string::npos ? "." : p.substr(0, s), pat = s == std::string::npos ? p : p.substr(s + 1);
	DIR* d = opendir(dir.c_str()); if (!d) { fail(); return INVALID_HANDLE_VALUE; }
	auto* fs = new FindState; g_findDir = dir;
	while (dirent* e = readdir(d)) if (fnmatch(pat.c_str(), e->d_name, FNM_CASEFOLD) == 0) fs->names.push_back(e->d_name);
	closedir(d);
	if (!fillFind(fs, dir, fd)) { SetLastError(2); delete fs; return INVALID_HANDLE_VALUE; }
	return fs;
}
BOOL FindNextFileW(HANDLE h, WIN32_FIND_DATAW* fd) { return fillFind((FindState*)h, g_findDir, fd); }
BOOL FindClose(HANDLE h) { delete (FindState*)h; return TRUE; }

DWORD GetFullPathNameW(LPCWSTR p, DWORD n, LPWSTR buf, LPWSTR* filePart) {
	std::string s = posixPath(p);
	if (s.empty() || s[0] != '/') { char cwd[PATH_MAX]; if (getcwd(cwd, sizeof cwd)) s = std::string(cwd) + "/" + s; }
	std::vector<std::string> parts; size_t i = 0; // collapse . and ..
	while (i <= s.size()) { size_t j = s.find('/', i); if (j == std::string::npos) j = s.size(); std::string c = s.substr(i, j - i);
		if (c == "..") { if (!parts.empty()) parts.pop_back(); } else if (!c.empty() && c != ".") parts.push_back(c); i = j + 1; }
	std::string r; for (auto& c : parts) r += "/" + c; if (r.empty()) r = "/";
	std::wstring w = W(r.c_str()); if (w.size() + 1 > n) return (DWORD)w.size() + 1;
	wcscpy(buf, w.c_str()); if (filePart) { size_t sl = w.find_last_of(L'/'); *filePart = buf + (sl == std::wstring::npos ? 0 : sl + 1); } return (DWORD)w.size();
}
DWORD GetCurrentDirectoryW(DWORD n, LPWSTR buf) { char cwd[PATH_MAX]; if (!getcwd(cwd, sizeof cwd)) return 0; std::wstring w = W(cwd); if (w.size() + 1 > n) return (DWORD)w.size() + 1; wcscpy(buf, w.c_str()); return (DWORD)w.size(); }
DWORD GetModuleFileNameW(HMODULE, LPWSTR buf, DWORD n) {
	char p[PATH_MAX]; uint32_t sz = sizeof p; if (_NSGetExecutablePath(p, &sz)) return 0;
	char real[PATH_MAX]; if (!realpath(p, real)) strcpy(real, p);
	// Inside an .app bundle the core's "install dir" (langs.model.xml, themes/, functionList/ ...) is Contents/Resources, not Contents/MacOS.
	std::string exe = real; size_t m = exe.rfind(".app/Contents/MacOS/");
	if (m != std::string::npos) exe = exe.substr(0, m) + ".app/Contents/Resources/" + exe.substr(m + 20);
	std::wstring w = W(exe.c_str()); if (w.size() + 1 > n) return 0; wcscpy(buf, w.c_str()); return (DWORD)w.size();
}
DWORD GetTempPathW(DWORD n, LPWSTR buf) { const char* t = getenv("TMPDIR"); std::string s = t ? t : "/tmp/"; if (s.back() != '/') s += '/'; std::wstring w = W(s.c_str()); if (w.size() + 1 > n) return (DWORD)w.size() + 1; wcscpy(buf, w.c_str()); return (DWORD)w.size(); }
DWORD ExpandEnvironmentStringsW(LPCWSTR src, LPWSTR dst, DWORD n) { // expands %NAME% from the process environment
	std::wstring in = src, out; size_t i = 0;
	while (i < in.size()) { if (in[i] == L'%') { size_t j = in.find(L'%', i + 1); if (j != std::wstring::npos) { const char* v = getenv(U8(in.substr(i + 1, j - i - 1).c_str()).c_str());
		if (v) { out += W(v); i = j + 1; continue; } } } out += in[i++]; }
	if (out.size() + 1 > n) return (DWORD)out.size() + 1; wcscpy(dst, out.c_str()); return (DWORD)out.size() + 1;
}
void GetSystemTimeAsFileTime(FILETIME* ft) { timespec ts; clock_gettime(CLOCK_REALTIME, &ts); *ft = toFT(ts.tv_sec, ts.tv_nsec); }
BOOL FileTimeToSystemTime(const FILETIME* ft, SYSTEMTIME* st) {
	timespec ts = fromFT(*ft); struct tm tm; time_t t = ts.tv_sec; gmtime_r(&t, &tm);
	st->wYear = tm.tm_year + 1900; st->wMonth = tm.tm_mon + 1; st->wDayOfWeek = tm.tm_wday; st->wDay = tm.tm_mday;
	st->wHour = tm.tm_hour; st->wMinute = tm.tm_min; st->wSecond = tm.tm_sec; st->wMilliseconds = ts.tv_nsec / 1000000; return TRUE;
}

// ---- shlwapi paths ----
static bool isSep(wchar_t c) { return c == L'/' || c == L'\\'; }
BOOL PathFileExistsW(LPCWSTR p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }
BOOL PathIsDirectoryW(LPCWSTR p) { DWORD a = GetFileAttributesW(p); return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY); }
BOOL PathIsRelativeW(LPCWSTR p) { return !(p && (isSep(p[0]) || (p[0] && p[1] == L':'))); }
LPWSTR PathFindFileNameW(LPCWSTR p) { LPCWSTR r = p; for (LPCWSTR c = p; *c; ++c) if (isSep(*c)) r = c + 1; return (LPWSTR)r; }
LPWSTR PathFindExtensionW(LPCWSTR p) { LPCWSTR f = PathFindFileNameW(p); LPCWSTR dot = nullptr; for (LPCWSTR c = f; *c; ++c) if (*c == L'.') dot = c; return (LPWSTR)(dot ? dot : f + wcslen(f)); }
BOOL PathRemoveFileSpecW(LPWSTR p) { LPWSTR f = PathFindFileNameW(p); if (f == p) { bool had = *p; *p = 0; return had; } if (f - 1 > p) f[-1] = 0; else *f = 0; return TRUE; }
BOOL PathAppendW(LPWSTR path, LPCWSTR more) { size_t l = wcslen(path); if (l && !isSep(path[l - 1])) path[l++] = L'/'; while (isSep(*more)) ++more; wcscpy(path + l, more); return TRUE; }
LPWSTR PathCombineW(LPWSTR dst, LPCWSTR dir, LPCWSTR file) { if (!PathIsRelativeW(file) || !dir || !*dir) wcscpy(dst, file); else { wcscpy(dst, dir); PathAppendW(dst, file); } return dst; }
BOOL PathMatchSpecW(LPCWSTR file, LPCWSTR spec) { // spec may hold several patterns separated by ';'
	std::string f = U8(file), s = U8(spec); size_t i = 0;
	while (i <= s.size()) { size_t j = s.find(';', i); if (j == std::string::npos) j = s.size(); std::string pat = s.substr(i, j - i); while (!pat.empty() && pat[0] == ' ') pat.erase(0, 1);
		if (!pat.empty() && fnmatch(pat.c_str(), f.c_str(), FNM_CASEFOLD) == 0) return TRUE; i = j + 1; }
	return FALSE;
}

FILE* _wfopen(const wchar_t* path, const wchar_t* mode) { return fopen(posixPath(path).c_str(), U8(mode).c_str()); }
int GetSystemMetrics(int i) { // no display queried here; plausible desktop numbers keep core layout maths sane
	switch (i) { case SM_CXSCREEN: case SM_CXVIRTUALSCREEN: return 1920; case SM_CYSCREEN: case SM_CYVIRTUALSCREEN: return 1080;
		case SM_CXSMICON: case SM_CYSMICON: return 16; default: return 0; }
}

HRESULT SHGetFolderPathW(HWND, int csidl, HANDLE, DWORD, LPWSTR out) {
	const char* home = getenv("HOME"); std::string h = home ? home : "/tmp", p;
	switch (csidl) {
		case CSIDL_APPDATA: case CSIDL_LOCAL_APPDATA: p = h + "/Library/Application Support"; break;
		case CSIDL_PROGRAM_FILES: case CSIDL_PROGRAM_FILESX86: p = "/Applications"; break;
		case CSIDL_PERSONAL: p = h + "/Documents"; break;
		case CSIDL_PROFILE: p = h; break;
		case CSIDL_WINDOWS: p = "/System"; break;
		default: return E_FAIL;
	}
	std::wstring w = W(p.c_str()); if (w.size() >= MAX_PATH) return E_FAIL; wcscpy(out, w.c_str()); return S_OK;
}

NppSendMessageHook g_nppSendMessageHook = nullptr;
NppCreateWindowHook g_nppCreateWindowHook = nullptr;
LRESULT SendMessageW(HWND h, UINT m, WPARAM w, LPARAM l) { return g_nppSendMessageHook ? g_nppSendMessageHook(h, m, w, l) : 0; }
HWND CreateWindowExW(DWORD, LPCWSTR cls, LPCWSTR, DWORD, int, int, int, int, HWND parent, HMENU, HINSTANCE, LPVOID) {
	return g_nppCreateWindowHook ? g_nppCreateWindowHook(cls, parent) : nullptr;
}

void PathRemoveExtensionW(LPWSTR p) { LPWSTR e = PathFindExtensionW(p); if (*e == L'.') *e = 0; }
