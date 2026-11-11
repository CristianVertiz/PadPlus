// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 2 end-to-end: drive the real main window through its menu actions, headless (QT_QPA_PLATFORM=offscreen).
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QFileOpenEvent>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTextStream>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "find_dialog.h"
#include "main_window.h"
#include "npp_host.h"
#include "qt_bridge.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)

static QString slurp(const QString& p) { QFile f(p); f.open(QIODevice::ReadOnly); return QString::fromUtf8(f.readAll()); }
static void spit(const QString& p, const QString& t) { QFile f(p); f.open(QIODevice::WriteOnly); f.write(t.toUtf8()); }
static QAction* action(QWidget* w, const QString& text) {
	for (QAction* a : w->findChildren<QAction*>()) if (a->text() == text) return a;
	printf("no action '%s'\n", qPrintable(text)); return nullptr;
}
static void trigger(QWidget* w, const QString& text) { if (QAction* a = action(w, text)) a->trigger(); }

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	QTemporaryDir tmp;
	qputenv("HOME", tmp.path().toUtf8());
	QString settings = tmp.filePath("settings"); QDir().mkpath(settings);
	installQtBridge();
	bootCore(settings.toStdWString());
	NppHost& host = NppHost::get();
	host.init();

	QString a = tmp.filePath("a.cpp"), b = tmp.filePath("b.txt");
	spit(a, "int x;\r\nint y;\r\n"); spit(b, "foo123 bar45\n");

	MainWindow win(host);
	win.restoreSession();                                  // fresh profile: one untitled document
	QTabBar *main = win.tabBar(0), *sub = win.tabBar(1);
	ScintillaEdit* ed = win.editor(0);
	CHECK(main->count() == 1 && sub->count() == 0);

	// open: replaces the pristine untitled doc; language + EOL come from the real core
	win.openPath(a);
	CHECK(main->count() == 1 && main->tabText(0) == "a.cpp");
	CHECK(ed->send(SCI_GETLENGTH) == 16);
	win.openPath(b);
	CHECK(main->count() == 2 && main->currentIndex() == 1 && main->tabText(1) == "b.txt");
	win.openPath(a);                                       // already open: just switches
	CHECK(main->count() == 2 && main->currentIndex() == 0);

	// edit -> dirty marker; save -> clean, file changed on disk with CRLF preserved
	ed->send(SCI_APPENDTEXT, 8, reinterpret_cast<sptr_t>("int z;\r\n"));
	CHECK(main->tabText(0) == "a.cpp" && !main->tabIcon(0).isNull());   // pencil icon marks a modified document
	CHECK(win.windowTitle().startsWith("*") && win.windowTitle().endsWith(" - PadPlus"));   // Notepad++-style title: *path - PadPlus
	trigger(&win, "Save");
	CHECK(main->tabText(0) == "a.cpp" && main->tabIcon(0).isNull());
	CHECK(!win.windowTitle().startsWith("*"));
	CHECK(slurp(a) == "int x;\r\nint y;\r\nint z;\r\n");

	// find / replace (regex through Boost): "foo123" -> swap groups
	main->setCurrentIndex(1);
	FindDialog* fd = win.findDialog();
	fd->setQuery("bar", "", false, false, false); fd->findNext();
	CHECK(fd->status().startsWith("Found") && ed->send(SCI_GETSELECTIONSTART) == 7);
	fd->setQuery("([a-z]+)([0-9]+)", "\\2\\1", false, false, true); fd->replaceAll();
	CHECK(fd->status().contains("2") && QString::fromUtf8(ed->get_text_range(0, ed->send(SCI_GETLENGTH))) == "123foo 45bar\n");
	fd->setQuery("O", "0", true, false, false); fd->replaceAll();       // match case: "O" does not match "o"
	CHECK(fd->status().contains("0 occ"));
	fd->setQuery("o", "0", true, false, false); fd->count();
	CHECK(fd->status().startsWith("2 match"));
	trigger(&win, "Undo");

	// clone to the other view: same document in both panes
	trigger(&win, "Clone to Other View");
	CHECK(sub->count() == 1 && main->count() == 2);
	trigger(&win, "Move to Other View");                   // moves it back out of the active pane
	// language menu through the real menu: set b.txt to C++
	trigger(&win, "C++ source file");
	{ char lx[32] = {}; ed->send(SCI_GETLEXERLANGUAGE, 0, reinterpret_cast<sptr_t>(lx)); CHECK(QString(lx) == "cpp"); }
	// encoding + EOL commands
	trigger(&win, "Unix (LF)");

	// status bar: Notepad++'s fields (doc type | length/lines | Ln/Col/Pos | EOL | encoding | INS)
	main->setCurrentIndex(0);
	ed->send(SCI_SETSEL, 0, 0);
	QCoreApplication::processEvents();
	QStringList f = win.statusFields();
	CHECK(f.size() == 6 && f[0] == "C++ source file");
	CHECK(f[1].startsWith("length : 24") && f[1].contains("lines : 4"));
	CHECK(f[2] == "Ln : 1   Col : 1   Pos : 0" && f[3] == "Windows (CR LF)" && f[4] == "UTF-8" && f[5] == "INS");
	ed->send(SCI_SETSEL, 0, 5);
	QCoreApplication::processEvents();
	CHECK(win.statusFields()[2].contains("Sel : 5 | 1"));

	// chrome follows the theme: a dark theme makes the whole app dark, a light one returns to the system look
	CHECK(!win.isDarkChrome() && !win.styleSheet().contains("#262626"));
	trigger(&win, "Obsidian");
	CHECK(win.isDarkChrome() && win.styleSheet().contains("#262626") && win.styleSheet().contains("#7fd3b0"));   // dark tab bar + mint accent
	trigger(&win, "Default (stylers.xml)");
	CHECK(!win.isDarkChrome() && win.styleSheet().contains("#f5a33a"));                                           // light: orange accent

	// Finder: "Open With PadPlus", `open -a PadPlus file` and dropping a file on the Dock icon arrive as a QFileOpenEvent
	{
		QString fe = tmp.filePath("from_finder.txt"); spit(fe, "opened from Finder\n");
		QFileOpenEvent ev{ QUrl::fromLocalFile(fe) };
		QCoreApplication::sendEvent(&app, &ev);
		CHECK(main->tabText(main->currentIndex()) == "from_finder.txt" && QString::fromUtf8(ed->get_text_range(0, ed->send(SCI_GETLENGTH))) == "opened from Finder\n");
	}

		// close tab (discard-free: save first), then window close writes session.xml with the open files
	trigger(&win, "Save All");
	win.close();
	QString session = slurp(settings + "/session.xml");
	CHECK(session.contains("a.cpp"));
	printf("session.xml: %lld bytes\n", (long long)session.size());

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
