// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 2 step A: the unmodified core ScintillaEditView runs on a Qt Scintilla widget.
#include <QApplication>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unistd.h>
#include "qt_bridge.h"
#include "core_boot.h"
#include "Parameters.h"
#include "ScintillaEditView.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	char tmpl[] = "/tmp/npp_bridge_test_XXXXXX";
	if (!mkdtemp(tmpl)) return 2;
	setenv("HOME", tmpl, 1);
	installQtBridge();
	bootCore(std::wstring(tmpl, tmpl + strlen(tmpl)));

	ScintillaEditView v;
	v.init(nullptr, nullptr);
	CHECK(v.getHSelf() != nullptr);

	const char* src = "int main() { return 0; } // done\n";
	v.execute(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(src));
	CHECK(v.execute(SCI_GETLENGTH) == static_cast<LRESULT>(strlen(src)));

	v.defineDocType(L_CPP);                                    // real Notepad++ lexer/keyword/theme setup
	char lexer[64] = {};
	v.execute(SCI_GETLEXERLANGUAGE, 0, reinterpret_cast<LPARAM>(lexer));
	CHECK(std::string(lexer) == "cpp");
	v.execute(SCI_COLOURISE, 0, -1);
	CHECK(v.execute(SCI_GETSTYLEAT, 0) == SCE_C_WORD2);         // 'int' is in the type keyword list
	CHECK(v.execute(SCI_GETSTYLEAT, 14) == SCE_C_WORD);         // 'return' is an instruction keyword
	CHECK(v.execute(SCI_GETSTYLEAT, 28) == SCE_C_COMMENTLINE);  // '// done'
	printf("keyword fg=0x%06lX  default fg=0x%06lX\n", (long)v.execute(SCI_STYLEGETFORE, SCE_C_WORD), (long)v.execute(SCI_STYLEGETFORE, STYLE_DEFAULT));
	CHECK(v.execute(SCI_STYLEGETFORE, SCE_C_WORD) != v.execute(SCI_STYLEGETFORE, STYLE_DEFAULT)); // theme applied

	std::filesystem::remove_all(tmpl);
	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
