// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 3: shortcuts, preferences, styles and themes (the logic behind the Settings dialogs), persisted like Windows.
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
#include "menuCmdID.h"

namespace fs = std::filesystem;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static std::string slurp(const fs::path& p) { std::ifstream f(p, std::ios::binary); std::stringstream s; s << f.rdbuf(); return s.str(); }

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	char tmpl[] = "/tmp/npp_dialogs_test_XXXXXX";
	if (!mkdtemp(tmpl)) return 2;
	fs::path dir = tmpl, settings = dir / "settings";
	fs::create_directories(settings);
	setenv("HOME", tmpl, 1);
	installQtBridge();
	bootCore(settings.wstring());
	NppHost& host = NppHost::get();
	host.init();

	// ---- shortcuts ----
	auto sc = host.shortcuts();
	CHECK(sc.size() > 100);
	size_t save = sc.size(), other = sc.size();
	for (size_t i = 0; i < sc.size(); ++i) { if (sc[i].cmd == IDM_FILE_SAVE) save = i; if (sc[i].cmd == IDM_EDIT_LINE_UP) other = i; }
	CHECK(save < sc.size() && sc[save].ctrl && !sc[save].alt && !sc[save].shift && sc[save].vk == 'S');   // Ctrl+S by default
	CHECK(other < sc.size());
	if (other < sc.size()) {
		host.setShortcut(other, true, true, false, 0x78);                      // Ctrl+Alt+F9 (VK_F9 = 0x78 = 120)
		host.saveShortcuts();
		auto again = host.shortcuts();
		CHECK(again[other].ctrl && again[other].alt && again[other].vk == 0x78);
		std::string xml = slurp(settings / "shortcuts.xml");
		CHECK(xml.find("Key=\"120\"") != std::string::npos);
	}

	// ---- preferences ----
	NppHost::Prefs p = host.prefs();
	CHECK(p.tabSize == 4 && p.foldStyle == 4 && !p.wrap);
	p.tabSize = 2; p.tabsToSpaces = true; p.foldStyle = 3; p.wrap = true; p.lineNumbers = false;
	host.setPrefs(p);
	ScintillaEditView& v = host.view(NppHost::MAIN);
	CHECK(v.execute(SCI_GETTABWIDTH) == 2);
	CHECK(v.execute(SCI_GETUSETABS) == 0);                                      // tabs replaced by spaces
	CHECK(v.execute(SCI_GETWRAPMODE) != SC_WRAP_NONE);
	CHECK(host.prefs().foldStyle == 3 && !host.prefs().lineNumbers);
	host.savePrefs();
	std::string cfg = slurp(settings / "config.xml");
	CHECK(cfg.find("size=\"2\"") != std::string::npos);

	// ---- styles ----
	Buffer* b = host.open((dir / "a.cpp").wstring().empty() ? L"" : L"");     // placeholder: created below
	(void)b;
	{ std::ofstream f(dir / "a.cpp"); f << "int x; // c\n"; }
	b = host.open((dir / "a.cpp").wstring());
	CHECK(b != nullptr);
	host.show(NppHost::MAIN, b);
	v.execute(SCI_COLOURISE, 0, -1);
	auto groups = host.styleGroups();
	CHECK(groups.size() > 50 && groups[0] == L"Global Styles");
	int cppGroup = -1;
	for (size_t i = 0; i < groups.size(); ++i) if (groups[i].rfind(L"C++", 0) == 0 || groups[i] == L"cpp") { cppGroup = (int)i; break; }
	CHECK(cppGroup > 0);
	if (cppGroup > 0) {
		auto items = host.styleItems(cppGroup);
		int idx = -1;
		for (size_t i = 0; i < items.size(); ++i) if (items[i].id == SCE_C_COMMENTLINE) idx = (int)i;
		CHECK(idx >= 0);
		if (idx >= 0) {
			NppHost::StyleItem it = items[idx];
			it.fg = 0x0000FF;                                                   // COLORREF 0x0000FF == RGB(255,0,0)
			it.bold = true;
			host.setStyleItem(cppGroup, idx, it);
			host.applyStyles();
			CHECK(v.execute(SCI_STYLEGETFORE, SCE_C_COMMENTLINE) == 0x0000FF);
			CHECK(v.execute(SCI_STYLEGETBOLD, SCE_C_COMMENTLINE) != 0);
			CHECK(host.saveStyles());
			CHECK(slurp(settings / "stylers.xml").find("fgColor=\"FF0000\"") != std::string::npos);
		}
	}

	// ---- themes ----
	auto themes = host.themes();
	CHECK(themes.size() > 15);
	int obsidian = -1, current = host.currentTheme();
	for (size_t i = 0; i < themes.size(); ++i) if (themes[i] == L"Obsidian") obsidian = (int)i;
	CHECK(obsidian > 0);
	if (obsidian > 0) {
		long beforeBg = (long)v.execute(SCI_STYLEGETBACK, STYLE_DEFAULT);
		CHECK(host.setTheme(obsidian));
		long afterBg = (long)v.execute(SCI_STYLEGETBACK, STYLE_DEFAULT);
		printf("default background: 0x%06lX -> 0x%06lX (Obsidian)\n", beforeBg, afterBg);
		CHECK(afterBg != beforeBg && (afterBg & 0xFF) < 0x60);                  // Obsidian is a dark theme
		CHECK(host.currentTheme() == obsidian);
		host.savePrefs();
		CHECK(slurp(settings / "config.xml").find("lightThemeName=\"Obsidian.xml\"") != std::string::npos);   // remembered for the next launch
		CHECK(host.setTheme(current));
	}

	fs::remove_all(dir);
	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
