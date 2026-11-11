// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 1 exit test: the shared core loads Notepad++ config XMLs and handles encodings on macOS.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unistd.h>
#include "Parameters.h"
#include "Common.h"
#include "Utf8_16.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

int main() {
	NppParameters& p = NppParameters::getInstance();
	char tmpl[] = "/tmp/npp_core_test_XXXXXX";          // throwaway settings dir: never touch the real profile
	CHECK(mkdtemp(tmpl) != nullptr);
	setenv("HOME", tmpl, 1);                           // APPDATA maps under $HOME: keep the whole run inside the temp dir
	p.setCmdSettingsDir(std::wstring(tmpl, tmpl + strlen(tmpl)));
	bool loaded = p.load();
	printf("settingsDir=%s\n", tmpl); 
	(void)loaded; // false on a fresh profile (optional files such as userDefineLang.xml are absent); the app ignores it too
	printf("nppPath=%ls\nappdata=%ls\nlangs=%d userLangs=%d\n", p.getNppPath().c_str(), p.getAppDataNppDir(), p.getNbLang(), p.getNbUserLang());
	CHECK(p.getNbLang() > 50);                       // langs.xml parsed
	CHECK(p.getLangFromExt(L"cpp") == L_CPP);        // extension -> language
	CHECK(p.getLangFromExt(L"py") == L_PYTHON);
	// --- encodings / strings (compat layer + shared Utf8_16) ---
	{
		const wchar_t* w = L"h\u00e9llo \u4e16\u754c \U0001F600";                 // 1-, 2-, 3- and 4-byte UTF-8 sequences
		char u8[64]; int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, u8, sizeof u8, nullptr, nullptr);
		CHECK(n == 19 && strlen(u8) == 18);                                              // h(1) \u00e9(2) llo(3) sp(1) \u4e16\u754c(6) sp(1) emoji(4)
		wchar_t back[32]; CHECK(MultiByteToWideChar(CP_UTF8, 0, u8, -1, back, 32) > 0 && wcscmp(back, w) == 0);
		CHECK(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, "\xC3\x28", -1, back, 32) == 0); // invalid UTF-8 rejected
		std::wstring pth = L"/a/b"; pathAppend(pth, L"c.txt"); CHECK(pth == L"/a/b/c.txt");
		pth = L"/a/b/"; pathAppend(pth, L"c.txt"); CHECK(pth == L"/a/b/c.txt");

		unsigned char u16[] = { 0xFF, 0xFE, 'h', 0, 0xE9, 0, 'i', 0 };                // UTF-16LE with BOM: "h\u00e9i"
		Utf8_16_Read r; size_t got = r.convert(reinterpret_cast<char*>(u16), sizeof u16);
		CHECK(r.getEncoding() == uni16LE);
		CHECK(got == 4 && std::string(r.getNewBuf(), got) == "h\xC3\xA9i");
	}
	std::filesystem::remove_all(tmpl);
	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
