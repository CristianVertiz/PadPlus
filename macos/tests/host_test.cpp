// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 2 step B: real FileManager loads/saves files (encodings, EOL, language) into real ScintillaEditViews on Qt.
#include <QApplication>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include "qt_bridge.h"
#include "core_boot.h"
#include "npp_host.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "Buffer.h"

namespace fs = std::filesystem;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

static std::string slurp(const fs::path& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }
static void spit(const fs::path& p, const std::string& d) { std::ofstream f(p, std::ios::binary); f.write(d.data(), d.size()); }
static std::string text(ScintillaEditView& v) { std::string s(v.execute(SCI_GETLENGTH), '\0'); v.execute(SCI_GETTEXT, s.size() + 1, reinterpret_cast<LPARAM>(s.data())); return s; }

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	char tmpl[] = "/tmp/npp_host_test_XXXXXX";
	if (!mkdtemp(tmpl)) return 2;
	fs::path dir = tmpl;
	setenv("HOME", tmpl, 1);
	installQtBridge();
	bootCore((dir / "settings").wstring());
	NppHost& host = NppHost::get();
	host.init();

	// UTF-8 (no BOM) with LF endings and a non-ASCII char
	spit(dir / "u8.txt", "h\xC3\xA9llo\nworld\n");
	Buffer* b = host.open((dir / "u8.txt").wstring());
	CHECK(b != nullptr);
	if (b) {
		host.show(NppHost::MAIN, b);
		CHECK(text(host.view(NppHost::MAIN)) == "h\xC3\xA9llo\nworld\n");
		CHECK(b->getEolFormat() == EolType::unix);
		CHECK(b->getUnicodeMode() == uniUTF8_NoBOM);
		printf("u8.txt: unicodeMode=%d encoding=%d eol=%d lang=%d\n", (int)b->getUnicodeMode(), b->getEncoding(), (int)b->getEolFormat(), (int)b->getLangType());
	}

	// UTF-8 with BOM
	spit(dir / "bom.txt", "\xEF\xBB\xBF" "abc\n");
	b = host.open((dir / "bom.txt").wstring());
	CHECK(b && b->getUnicodeMode() == uniUTF8);
	if (b) { host.show(NppHost::MAIN, b); CHECK(text(host.view(NppHost::MAIN)) == "abc\n"); }

	// UTF-16LE with BOM -> Scintilla holds UTF-8; saving writes UTF-16LE again
	spit(dir / "u16.txt", std::string("\xFF\xFE" "h\0i\0\n\0", 8));
	b = host.open((dir / "u16.txt").wstring());
	CHECK(b && b->getUnicodeMode() == uni16LE);
	if (b) {
		host.show(NppHost::MAIN, b);
		CHECK(text(host.view(NppHost::MAIN)) == "hi\n");
		host.view(NppHost::MAIN).execute(SCI_APPENDTEXT, 1, reinterpret_cast<LPARAM>("!"));
		CHECK(host.save(b));
		CHECK(slurp(dir / "u16.txt") == std::string("\xFF\xFE" "h\0i\0\n\0!\0", 10));
	}

	// CRLF C++ source: EOL + language by extension + highlighting through real defineDocType
	spit(dir / "a.cpp", "int a;\r\nint b;\r\n");
	b = host.open((dir / "a.cpp").wstring());
	CHECK(b && b->getEolFormat() == EolType::windows);
	CHECK(b && b->getLangType() == L_CPP);
	if (b) {
		host.show(NppHost::MAIN, b);
		char lexer[64] = {};
		host.view(NppHost::MAIN).execute(SCI_GETLEXERLANGUAGE, 0, reinterpret_cast<LPARAM>(lexer));
		CHECK(std::string(lexer) == "cpp");
		host.view(NppHost::MAIN).execute(SCI_COLOURISE, 0, -1);
		CHECK(host.view(NppHost::MAIN).execute(SCI_GETSTYLEAT, 0) == SCE_C_WORD2);
	}

	// save round-trip keeps CRLF and content
	if (b) {
		host.view(NppHost::MAIN).execute(SCI_APPENDTEXT, 8, reinterpret_cast<LPARAM>("int c;\r\n"));
		CHECK(host.save(b));
		CHECK(slurp(dir / "a.cpp") == "int a;\r\nint b;\r\nint c;\r\n");
	}

	// new untitled doc + Save As
	b = host.newDocument();
	CHECK(b != nullptr);
	if (b) {
		host.show(NppHost::SUB, b);
		host.view(NppHost::SUB).execute(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>("new file\n"));
		CHECK(host.save(b, (dir / "new.txt").wstring()));
		CHECK(slurp(dir / "new.txt") == "new file\n");
	}

	// missing file
	CHECK(host.open((dir / "nope.txt").wstring()) == nullptr);

	fs::remove_all(dir);
	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
