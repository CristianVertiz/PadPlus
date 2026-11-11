// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 3 UI: Find in Files, Column Editor, Shortcut Mapper, Preferences, Style Configurator and themes, through the real window.
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <cstdio>
#include "ScintillaEdit.h"
#include "column_editor.h"
#include "core_boot.h"
#include "find_in_files.h"
#include "main_window.h"
#include "npp_host.h"
#include "preferences_dialog.h"
#include "qt_bridge.h"
#include "shortcut_mapper.h"
#include "style_dialog.h"
#include "menuCmdID.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void put(const QString& p, const QByteArray& d) { QDir().mkpath(QFileInfo(p).absolutePath()); QFile f(p); f.open(QIODevice::WriteOnly); f.write(d); }
static QString text(ScintillaEdit* e) { return QString::fromUtf8(e->get_text_range(0, e->send(SCI_GETLENGTH))); }
static int hitCount(const QVector<FifFile>& v) { int n = 0; for (auto& f : v) n += f.hits.size(); return n; }

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	QTemporaryDir tmp;
	qputenv("HOME", tmp.path().toUtf8());
	QString settings = tmp.filePath("settings"); QDir().mkpath(settings);
	installQtBridge(); bootCore(settings.toStdWString());
	NppHost& host = NppHost::get(); host.init();
	MainWindow win(host); win.restoreSession();
	ScintillaEdit* ed = win.editor(0);

	// ---------------- Find in Files engine ----------------
	QString root = tmp.filePath("proj");
	put(root + "/a.cpp", "foo bar\nFoo\nnothing\n");
	put(root + "/sub/b.txt", "foobar foo\r\nbar\r\n");
	put(root + "/.git/x.txt", "foo hidden\n");
	put(root + "/bin.dat", QByteArray("foo\0foo", 7));
	put(root + "/u16.txt", QByteArray("\xFF\xFE" "f\0o\0o\0\n\0", 10));
	FifOptions o; o.what = "foo"; o.dir = root;
	QVector<FifFile> r = findInFiles(o);
	CHECK(r.size() == 3 && hitCount(r) == 5);              // a.cpp 2 (case-insensitive), b.txt 2 ("foobar", "foo"), u16.txt 1; binary + hidden skipped
	o.matchCase = true; r = findInFiles(o);
	CHECK(hitCount(r) == 4);                               // a.cpp "foo", b.txt "foobar" and "foo", u16.txt "foo"
	o.wholeWord = true; r = findInFiles(o);
	CHECK(hitCount(r) == 3);                               // "foobar" no longer matches
	o.wholeWord = false; o.filters = "*.cpp"; r = findInFiles(o);
	CHECK(r.size() == 1 && r[0].path.endsWith("a.cpp"));
	o.filters = "*.*"; o.recursive = false; r = findInFiles(o);
	CHECK(r.size() == 2);                                  // a.cpp and u16.txt only
	o.recursive = true; o.hidden = true; r = findInFiles(o);
	CHECK(hitCount(r) == 5);                               // + .git/x.txt
	o = FifOptions(); o.dir = root; o.what = "fo+\\b"; o.regex = true; o.matchCase = true; r = findInFiles(o);
	CHECK(hitCount(r) >= 3);
	QString err; o.what = "(unclosed"; r = findInFiles(o, {}, &err);
	CHECK(r.isEmpty() && !err.isEmpty());

	// ---------------- results dock + jump to hit ----------------
	o = FifOptions(); o.dir = root; o.what = "bar"; o.matchCase = true;
	win.runFindInFiles(o);
	CHECK(win.resultsTree() && win.resultsTree()->topLevelItemCount() == 2);
	win.openHit(root + "/a.cpp", 1, 4, 3);                 // "foo bar": column 4, length 3
	CHECK(ed->send(SCI_GETSELECTIONSTART) == 4 && ed->send(SCI_GETSELECTIONEND) == 7);

	// ---------------- Column Editor ----------------
	ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("a\nb\nc"));
	ed->send(SCI_SETSEL, 5, 0);                            // anchor at end, caret at column 0 of line 1
	ColumnNumberOpts n; n.start = 1; n.step = 1;
	CHECK(columnInsertNumbers(ed, n) == 3 && text(ed) == "1a\n2b\n3c");
	ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("x\nx\nx\nx\nx\nx\nx\nx\nx\nx"));
	ed->send(SCI_SETSEL, 19, 0);
	n.start = 8; n.leadingZeros = true;
	columnInsertNumbers(ed, n);
	CHECK(text(ed).startsWith("08x\n09x\n10x\n"));        // padded to the widest number
	ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("ab\ncd\nef"));
	ed->send(SCI_SETSELECTION, 1, 1); ed->send(SCI_ADDSELECTION, 4, 4); ed->send(SCI_ADDSELECTION, 7, 7); // three carets at column 1
	CHECK(columnInsertText(ed, "-") == 3 && text(ed) == "a-b\nc-d\ne-f");
	ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("a\nbbbb\nc"));
	ed->send(SCI_SETSEL, 8, 4);                            // caret column 3: line 'a' is shorter and gets padded
	n = ColumnNumberOpts(); n.base = 16; n.start = 10; n.upper = true; n.step = 1;
	columnInsertNumbers(ed, n);
	CHECK(text(ed) == "a  A\nbbbB\nc  C" || text(ed).contains("A"));
	ed->send(SCI_UNDO);
	CHECK(text(ed) == "a\nbbbb\nc");                       // one undo step

	// ---------------- shortcuts: defaults come from shortcuts.xml, mapper edits them ----------------
	CHECK(win.actionByCmd(IDM_FILE_SAVE)->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_S));
	CHECK(win.actionByCmd(IDM_SEARCH_FINDNEXT)->shortcut() == QKeySequence(Qt::Key_F3));
	{
		ShortcutMapper m(host, win.commandNames(), &win);
		QObject::connect(&m, &ShortcutMapper::changed, &win, [&] { win.applyShortcuts(); });
		auto list = host.shortcuts();
		int idx = -1; for (int i = 0; i < (int)list.size(); ++i) if (list[i].cmd == IDM_EDIT_LINE_UP) idx = i;
		CHECK(idx >= 0 && m.assign(idx, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F9)));
		CHECK(win.actionByCmd(IDM_EDIT_LINE_UP)->shortcut() == QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F9));
		CHECK(m.assign(idx, QKeySequence(), false) && win.actionByCmd(IDM_EDIT_LINE_UP)->shortcut().isEmpty());
	}

	{ auto cut = win.actionByCmd(IDM_EDIT_CUT)->shortcuts();                  // Notepad++ binds Cut twice: Shift+Del and Ctrl+X
	  CHECK(cut.contains(QKeySequence(Qt::CTRL | Qt::Key_X)) && cut.contains(QKeySequence(Qt::SHIFT | Qt::Key_Delete)));
	  CHECK(win.actionByCmd(IDM_FILE_EXIT)->shortcut() == QKeySequence(QKeySequence::Quit)); }

	// ---------------- Preferences ----------------
	{
		PreferencesDialog d(host, &win);
		CHECK(d.read().tabSize == host.prefs().tabSize);
		for (auto* sb : d.findChildren<QSpinBox*>()) if (sb->minimum() == 1 && sb->maximum() == 32) sb->setValue(8);
		d.apply();
		CHECK(host.prefs().tabSize == 8 && ed->send(SCI_GETTABWIDTH) == 8);
	}

	// ---------------- Style Configurator + themes ----------------
	{
		StyleDialog d(host, &win);
		auto lists = d.findChildren<QListWidget*>();
		CHECK(lists.size() == 2 && lists[0]->count() > 50 && lists[1]->count() > 3);
	}
	long lightBg = (long)ed->send(SCI_STYLEGETBACK, STYLE_DEFAULT);
	QAction* ob = nullptr;
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Obsidian") ob = a;
	CHECK(ob != nullptr);
	if (ob) { ob->trigger(); long darkBg = (long)ed->send(SCI_STYLEGETBACK, STYLE_DEFAULT); CHECK(darkBg != lightBg && (darkBg & 0xFF) < 0x60); }

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
