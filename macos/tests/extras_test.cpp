// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Bookmarks, mark styles, incremental search, Run variables, monitoring (tail -f), Clipboard/ANSI/Document Map panels, user defined languages.
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenuBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "extras.h"
#include "main_window.h"
#include "menuCmdID.h"
#include "npp_host.h"
#include "qt_bridge.h"

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
	ScintillaEdit* ed = win.editor(0);
	auto pump = [] { QCoreApplication::processEvents(); };
	const int mask = 1 << 20;
	// regression: a hidden change-history margin makes Scintilla paint every modified line as a full-line background
	CHECK(ed->send(SCI_GETMARGINWIDTHN, 2) > 0);
	win.resize(1100, 640); win.show(); pump();
	setText(ed, "x\ny\n"); ed->send(SCI_APPENDTEXT, 1, (sptr_t)"z"); pump();
	CHECK((win.grab().toImage().pixel(500, 150) & 0xFFFFFF) == 0xFFFFFF);

	// ---- bookmarks ----
	setText(ed, "a\nb\nc\nd\ne\n");
	ed->send(SCI_GOTOLINE, 1); win.bookmark(0); ed->send(SCI_GOTOLINE, 3); win.bookmark(0);
	CHECK((ed->send(SCI_MARKERGET, 1) & mask) && (ed->send(SCI_MARKERGET, 3) & mask) && !(ed->send(SCI_MARKERGET, 2) & mask));
	ed->send(SCI_GOTOLINE, 0); win.bookmark(1); CHECK(ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)) == 1);
	win.bookmark(1); CHECK(ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)) == 3);
	win.bookmark(1); CHECK(ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)) == 1);      // wraps
	win.bookmark(2); CHECK(ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)) == 3);      // previous wraps back
	win.bookmarkedLines(1);
	CHECK(QApplication::clipboard()->text() == "b\nd\n");
	win.bookmarkedLines(2);
	CHECK(text(ed) == "a\nc\ne\n");
	win.bookmark(3); CHECK(ed->send(SCI_MARKERNEXT, 0, mask) < 0);
	CHECK(win.actionByCmd(IDM_SEARCH_NEXT_BOOKMARK) && !win.actionByCmd(IDM_SEARCH_NEXT_BOOKMARK)->shortcuts().isEmpty());

	// ---- mark styles ----
	setText(ed, "foo bar foo baz foo");
	ed->send(SCI_SETSEL, 0, 3); win.markAll(2);
	int hits = 0; for (int i = 0; i < 19; ++i) hits += ed->send(SCI_INDICATORVALUEAT, 24, i) != 0;
	CHECK(hits == 9);                                                           // 3 x "foo" (3 chars each)
	win.markAll(0);
	hits = 0; for (int i = 0; i < 19; ++i) hits += ed->send(SCI_INDICATORVALUEAT, 24, i) != 0;
	CHECK(hits == 0);

	// ---- incremental search ----
	setText(ed, "alpha beta gamma beta");
	ed->send(SCI_GOTOPOS, 0);
	IncSearchBar* inc = win.incSearch();
	inc->setText("beta");
	CHECK(ed->send(SCI_GETSELECTIONSTART) == 6 && ed->send(SCI_GETSELECTIONEND) == 10 && inc->state() == "found");
	inc->next(); CHECK(ed->send(SCI_GETSELECTIONSTART) == 17);
	inc->next(); CHECK(ed->send(SCI_GETSELECTIONSTART) == 6);                   // wraps
	inc->prev(); CHECK(ed->send(SCI_GETSELECTIONSTART) == 17);
	inc->setText("zzz"); CHECK(inc->state() == "not found");
	inc->closeBar(); CHECK(inc->isHidden());

	// ---- Run variables ----
	QString f = tmp.filePath("note.txt"); { QFile o(f); o.open(QIODevice::WriteOnly); o.write("one\ntwo\n"); }
	win.openPath(f); pump();
	CHECK(win.expandRunVars("$(FILE_NAME)|$(NAME_PART)|$(EXT_PART)|$(CURRENT_DIRECTORY)") == "note.txt|note|.txt|" + QFileInfo(f).absolutePath());
	QString marker = tmp.filePath("ran.txt");
	win.runCommand("echo $(FILE_NAME) > " + marker);
	for (int i = 0; i < 50 && !QFile::exists(marker); ++i) QThread::msleep(50);
	{ QFile o(marker); CHECK(o.open(QIODevice::ReadOnly) && o.readAll().trimmed() == "note.txt"); }

	// ---- monitoring (tail -f) ----
	ScintillaEdit* cur = win.editor(0);
	CHECK(!win.monitoring());
	win.actionByCmd(IDM_VIEW_MONITORING)->trigger(); pump();
	CHECK(win.monitoring() && win.actionByCmd(IDM_VIEW_MONITORING)->isChecked());
	QThread::msleep(1100);
	{ QFile o(f); o.open(QIODevice::Append); o.write("three\n"); }
	win.pollMonitored();
	CHECK(text(cur) == "one\ntwo\nthree\n");
	CHECK(cur->send(SCI_GETCURRENTPOS) == cur->send(SCI_GETLENGTH));            // follows the end
	win.actionByCmd(IDM_VIEW_MONITORING)->trigger(); pump();
	CHECK(!win.monitoring());

	// ---- Clipboard History ----
	ClipboardPanel* cp = win.clipboardPanel();
	QApplication::clipboard()->setText("first"); pump(); QApplication::clipboard()->setText("second"); pump();
	CHECK(cp->list()->count() >= 2 && cp->list()->item(0)->data(Qt::UserRole).toString() == "second");
	cp->record("first"); CHECK(cp->list()->item(0)->data(Qt::UserRole).toString() == "first");        // moved to the top, not duplicated
	int n = cp->list()->count(); cp->record("first"); CHECK(cp->list()->count() == n);
	setText(cur, ""); cur->send(SCI_GOTOPOS, 0);
	emit cp->paste("pasted");
	CHECK(text(cur) == "pasted");

	// ---- ANSI Chars ----
	AnsiPanel* ap = win.ansiPanel();
	CHECK(ap->table()->rowCount() == 256 && ap->table()->item(65, 2)->text() == "A" && ap->table()->item(10, 3)->text() == "LF" && ap->table()->item(233, 1)->text() == "E9");
	setText(cur, ""); emit ap->insertChar(QString(QChar(233))); CHECK(text(cur) == QString(QChar(233)));

	// ---- Document Map ----
	setText(cur, "l0\nl1\nl2\nl3\nl4\n");
	win.docMapPanel()->show(); win.show(); pump();
	win.docMapPanel()->follow(cur);
	ScintillaEdit* map = win.docMapPanel()->map();
	CHECK(map->send(SCI_GETDOCPOINTER) == cur->send(SCI_GETDOCPOINTER));         // same text, no copy
	CHECK(map->send(SCI_GETLENGTH) == cur->send(SCI_GETLENGTH));
	CHECK(win.docMapPanel()->band()->isVisible() && win.docMapPanel()->band()->height() > 0);   // the visible lines are shaded
	CHECK(!(cur->send(SCI_MARKERGET, 0) & (1 << 16)));                                     // and nothing was written into the shared document
	emit win.docMapPanel()->scrollToLine(3);
	CHECK(cur->send(SCI_GETFIRSTVISIBLELINE) >= 0);

	// ---- menus ----
	QStringList tops; for (QAction* a : win.menuBar()->actions()) tops << a->text().remove('&');
	CHECK(tops.contains("Run"));
	CHECK(win.actionByCmd(IDM_LANG_USER_DLG) != nullptr);

	// ---- user defined languages: import a minimal definition ----
	QString udl = tmp.filePath("mylang.xml");
	{ QFile o(udl); o.open(QIODevice::WriteOnly); o.write(
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<NotepadPlus><UserLang name=\"MyLang\" ext=\"mylang\" udlVersion=\"2.1\">"
		"<Settings><Global caseIgnored=\"no\" allowFoldOfComments=\"no\" foldCompact=\"no\" forceLineCommentsAtBOL=\"no\" /><Prefix Keywords1=\"no\" Keywords2=\"no\" Keywords3=\"no\" Keywords4=\"no\" Keywords5=\"no\" Keywords6=\"no\" Keywords7=\"no\" Keywords8=\"no\" /></Settings>"
		"<KeywordLists><Keywords name=\"Comments\">00// 01 02 03 04</Keywords><Keywords name=\"Keywords1\">hello</Keywords></KeywordLists>"
		"<Styles><WordsStyle name=\"DEFAULT\" fgColor=\"000000\" bgColor=\"FFFFFF\" fontName=\"\" fontStyle=\"0\" nesting=\"0\" /></Styles></UserLang></NotepadPlus>\n"); }
	int before = (int)host.udlNames().size();
	int added = host.importUdl(udl.toStdWString());
	CHECK(added == 1 && (int)host.udlNames().size() == before + 1);
	CHECK(host.udlNames().back() == L"MyLang");
	host.setUserLanguage(host.newDocument(), L"MyLang");

	if (const char* snap = getenv("EXTRAS_SNAPSHOT")) {                       // dev aid: render the new panels and bars
		win.resize(1100, 640); win.show(); pump(); pump();
		auto px = [&](const char* step) { QImage im = win.grab().toImage(); printf("%s: pixel(500,150)=%06x\n", step, im.pixel(500, 150) & 0xFFFFFF); };
		setText(cur, "int main() {\n\tfoo(1);\n\tbar(2);\n\tfoo(3);\n\treturn 0;\n}\n"); pump(); px("text");
		cur->send(SCI_GOTOLINE, 1); win.bookmark(0); ed->send(SCI_GOTOLINE, 3); win.bookmark(0); pump(); px("bookmarks");
		cur->send(SCI_SETSEL, 14, 17); win.markAll(1); pump(); px("mark");
		win.clipboardPanel()->show(); win.ansiPanel()->show(); pump(); px("panels");
		win.docMapPanel()->show(); pump(); px("map");
		win.incSearch()->setText("foo"); pump(); px("inc");
		win.grab().save(snap);
	}
	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
