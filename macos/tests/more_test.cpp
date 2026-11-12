// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Mark (Find dialog), Find Characters in Range, menu commands in macros, the User Defined Language editor, localization, tab scrolling.
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMenuBar>
#include <QMouseEvent>
#include <QTabBar>
#include <QToolBar>
#include <QTemporaryDir>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "find_dialog.h"
#include "main_window.h"
#include "menuCmdID.h"
#include "npp_host.h"
#include "qt_bridge.h"
#include "udl_dialog.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void setText(ScintillaEdit* e, const char* t) { e->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(t)); e->send(SCI_EMPTYUNDOBUFFER); }
static QString text(ScintillaEdit* e) { return QString::fromUtf8(e->get_text_range(0, e->send(SCI_GETLENGTH))); }

int main(int argc, char** argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	QApplication app(argc, argv);
	QTemporaryDir tmp;
	qputenv("HOME", tmp.path().toUtf8());
	QString settings = tmp.filePath("settings"); QDir().mkpath(settings);
	installQtBridge(); bootCore(settings.toStdWString());
	NppHost& host = NppHost::get(); host.init();
	MainWindow win(host); win.restoreSession();
	CHECK(win.tabBar(0)->count() == 1 && win.tabBar(0)->tabText(0) == "new 1");      // a fresh start shows "new 1" (it was "new 2": the core's own first document was never shown)
	win.resize(1000, 600); win.show();
	ScintillaEdit* ed = win.editor(0);
	auto pump = [] { QCoreApplication::processEvents(); };
	auto has = [&](const wchar_t* n) { for (auto& u : host.udlNames()) if (u == n) return true; return false; };

	// ---- Mark All (Find dialog) ----
	setText(ed, "foo bar\nxx\nfoo baz\n");
	FindDialog* fd = win.findDialog();
	fd->setQuery("foo", "", true, false, false); fd->setBookmarkLine(true); fd->markAll();
	int hits = 0; for (int i = 0; i < 19; ++i) hits += ed->send(SCI_INDICATORVALUEAT, 31, i) != 0;
	CHECK(hits == 6);
	CHECK((ed->send(SCI_MARKERGET, 0) & (1 << 20)) && !(ed->send(SCI_MARKERGET, 1) & (1 << 20)) && (ed->send(SCI_MARKERGET, 2) & (1 << 20)));
	fd->clearMarks();
	hits = 0; for (int i = 0; i < 19; ++i) hits += ed->send(SCI_INDICATORVALUEAT, 31, i) != 0;
	CHECK(hits == 0 && ed->send(SCI_MARKERNEXT, 0, 1 << 20) < 0);
	CHECK(win.actionByCmd(IDM_SEARCH_MARK) && win.actionByCmd(IDM_SEARCH_FINDCHARINRANGE));

	// ---- Find Characters in Range ----
	setText(ed, "ab\xC3\xA9" "cd\xC3\xB1");                                     // ab é cd ñ
	ed->send(SCI_GOTOPOS, 0);
	CHECK(FindCharsDialog::findInRange(ed, 128, 255, true, true) && ed->send(SCI_GETSELECTIONSTART) == 2 && ed->send(SCI_GETSELECTIONEND) == 4);
	CHECK(FindCharsDialog::findInRange(ed, 128, 255, true, true) && ed->send(SCI_GETSELECTIONSTART) == 6);
	CHECK(FindCharsDialog::findInRange(ed, 128, 255, true, true) && ed->send(SCI_GETSELECTIONSTART) == 2);     // wraps
	CHECK(FindCharsDialog::findInRange(ed, 128, 255, false, true) && ed->send(SCI_GETSELECTIONSTART) == 6);    // up, wraps
	CHECK(FindCharsDialog::findInRange(ed, 'c', 'c', false, false) && ed->send(SCI_GETSELECTIONSTART) == 4);
	CHECK(!FindCharsDialog::findInRange(ed, 'z', 'z', true, true));

	// ---- macros record menu commands (once: not when Scintilla already recorded the operation) ----
	setText(ed, "one\ntwo\n"); ed->send(SCI_GOTOLINE, 1);
	win.macroStart();
	win.actionByCmd(IDM_SEARCH_TOGGLE_BOOKMARK)->trigger();
	CHECK(host.macroLength() == 1);
	win.actionByCmd(IDM_EDIT_DUP_LINE)->trigger();                              // recorded by Scintilla as a playable step: kept, no command step added
	CHECK(host.macroLength() == 2);
	win.actionByCmd(IDM_EDIT_SELECTALL)->trigger();                             // a playable Scintilla step: not recorded a second time as a command
	CHECK(host.macroLength() == 3);
	win.macroStop();
	setText(ed, "one\ntwo\n"); ed->send(SCI_MARKERDELETEALL, 20); ed->send(SCI_GOTOLINE, 0);
	win.runMacro(-1, 1, false);
	CHECK(ed->send(SCI_MARKERGET, 0) & (1 << 20));
	CHECK(text(ed).count("one") == 2 && text(ed).count("two") == 1);            // the duplicate uses the document's EOL
	ed->send(SCI_MARKERDELETEALL, 20);

	// ---- User Defined Language editor ----
	{
		UdlDialog d(host, &win);
		d.newLanguage("TestLang"); d.setExtensions("tl");
		d.setField("Keywords1", -1, "foo bar"); d.setField("Comments", 0, "// #"); d.setField("Comments", 3, "/*"); d.setField("Comments", 4, "*/");
		d.setField("Delimiters", 0, "\""); d.setField("Delimiters", 2, "\"");
		d.setStyle("KEYWORDS1", QColor(255, 0, 0), true);
		CHECK(d.save());
		CHECK(has(L"TestLang"));
		QFile x(QString::fromStdWString(host.udlFolder()) + "/TestLang.xml");
		CHECK(x.open(QIODevice::ReadOnly));
		const QByteArray xml = x.readAll();
		CHECK(xml.contains("00// 00# 01 02 03/* 04*/") && xml.contains("name=\"TestLang\"") && xml.contains("fgColor=\"FF0000\""));

		QString f = tmp.filePath("sample.tl"); { QFile o(f); o.open(QIODevice::WriteOnly); o.write("foo baz // c\n"); }
		win.openPath(f); pump();
		ScintillaEdit* cur = win.editor(0);
		CHECK(win.statusFields().at(0) == "User: TestLang");
		cur->send(SCI_COLOURISE, 0, -1);
		CHECK(cur->send(SCI_GETSTYLEAT, 0) == 4 && cur->send(SCI_GETSTYLEAT, 4) != 4);     // foo = KEYWORD1, baz is not
		CHECK(cur->send(SCI_GETSTYLEAT, 9) == 2);                                           // line comment
		CHECK(cur->send(SCI_STYLEGETFORE, 4) == 0x0000FF && cur->send(SCI_STYLEGETBOLD, 4));

		// edit it: the open document is restyled
		QObject::connect(&d, &UdlDialog::changed, &win, &MainWindow::udlChanged);
		CHECK(d.select("TestLang") && d.field("Comments", 0) == "// #" && d.field("Keywords1", -1) == "foo bar");
		d.setField("Keywords1", -1, "baz"); d.setStyle("KEYWORDS1", QColor(0, 0, 255), false);
		CHECK(d.save()); pump();
		CHECK(host.udlNames().size() == 1 && win.statusFields().at(0) == "User: TestLang");
		cur->send(SCI_COLOURISE, 0, -1);
		CHECK(cur->send(SCI_GETSTYLEAT, 0) != 4 && cur->send(SCI_GETSTYLEAT, 4) == 4);
		CHECK(cur->send(SCI_STYLEGETFORE, 4) == 0xFF0000);

		CHECK(d.removeCurrent()); pump();
		CHECK(!has(L"TestLang") && !x.exists() && win.statusFields().at(0) != "User: TestLang");
	}

	// ---- localization: Notepad++'s nativeLang files translate menus and commands ----
	CHECK(win.actionByCmd(IDM_FILE_NEW)->text() == "New");
	win.applyLocalization("french.xml");
	CHECK(win.actionByCmd(IDM_FILE_NEW)->text().remove('&') == "Nouveau");
	QStringList tops; for (QAction* a : win.menuBar()->actions()) tops << a->text().remove('&');
	CHECK(tops.contains("Fichier") && tops.contains("Recherche"));
	CHECK(win.toolbar()->actions().at(0)->text().remove('&') == "Nouveau");
	win.applyLocalization(QString());
	CHECK(win.actionByCmd(IDM_FILE_NEW)->text() == "New" && win.menuBar()->actions().at(0)->text() == "&File");

	// ---- double-click on the empty part of the tab strip: new document; on a tab: nothing ----
	{
		auto dbl = [](QWidget* w, QPoint p) { QMouseEvent e(QEvent::MouseButtonDblClick, QPointF(p), QPointF(w->mapToGlobal(p)), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier); QCoreApplication::sendEvent(w, &e); };
		QTabBar* bar = win.tabBar(0); QWidget* strip = bar->parentWidget();
		CHECK(strip->objectName() == "tabStrip");
		int n = bar->count();
		dbl(strip, QPoint(strip->width() - 5, strip->height() / 2));
		CHECK(bar->count() == n + 1 && bar->currentIndex() == n);
		{	// untitled documents are numbered: no two tabs share a name (they were all "new 1")
			QStringList names; for (int i = 0; i < bar->count(); ++i) names << bar->tabText(i);
			dbl(strip, QPoint(strip->width() - 5, strip->height() / 2));
			CHECK(!names.contains(bar->tabText(bar->count() - 1)) && bar->tabText(bar->count() - 1).startsWith("new "));
			win.actionByCmd(IDM_FILE_CLOSE)->trigger();
		}
		dbl(bar, bar->tabRect(0).center());
		CHECK(bar->count() == n + 1);
		win.actionByCmd(IDM_VIEW_CLONE_TO_ANOTHER_VIEW)->trigger(); pump();      // second view appears with one tab
		QTabBar* bar2 = win.tabBar(1); int n2 = bar2->count();
		dbl(bar2->parentWidget(), QPoint(bar2->parentWidget()->width() - 5, 5));
		CHECK(bar2->count() == n2 + 1 && bar->count() == n + 1);
		while (bar2->count()) win.actionByCmd(IDM_FILE_CLOSE)->trigger();                // close the second view again
	}

	// ---- many tabs scroll instead of widening the window ----
	for (int i = 0; i < 40; ++i) win.actionByCmd(IDM_FILE_NEW)->trigger();
	pump();
	CHECK(win.tabBar(0)->count() >= 40 && win.tabBar(0)->usesScrollButtons());
	CHECK(win.tabBar(0)->width() <= win.width() && win.tabBar(0)->tabRect(0).width() > 40);
	CHECK(win.width() <= 1000);

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
