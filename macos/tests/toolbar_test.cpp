// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Toolbar (Notepad++ icons, live enable/check state) and the Encoding / Window / ? menus, Close All.
#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QPrinter>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QTabBar>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QToolBar>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "main_window.h"
#include "npp_host.h"
#include "print_doc.h"
#include "tools.h"
#include "qt_bridge.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void put(const QString& p, const QByteArray& d) { QFile f(p); f.open(QIODevice::WriteOnly); f.write(d); }
static QString strip(QString s) { return s.remove('&'); }

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
	QTabBar* tabs = win.tabBar(0);

	auto tbAction = [&](const QString& text) -> QAction* { for (QAction* a : win.toolbar()->actions()) if (!a->isSeparator() && a->text() == text) return a; printf("no toolbar action '%s'\n", qPrintable(text)); return nullptr; };
	auto menuNamed = [&](const QString& title) -> QMenu* { for (QAction* a : win.menuBar()->actions()) if (a->menu() && strip(a->text()) == title) return a->menu(); return nullptr; };
	auto pump = [] { QCoreApplication::processEvents(); };

	// ---- toolbar: Notepad++ icons for every button ----
	int buttons = 0;
	for (QAction* a : win.toolbar()->actions()) if (!a->isSeparator()) { ++buttons; if (a->icon().isNull()) printf("  no icon: %s\n", qPrintable(a->text())); CHECK(!a->icon().isNull()); }
	CHECK(buttons >= 20);

	// ---- menu order matches Notepad++ ----
	QStringList menus; for (QAction* a : win.menuBar()->actions()) menus << strip(a->text());
	CHECK(menus == (QStringList{ "File", "Edit", "Search", "View", "Encoding", "Language", "Settings", "Tools", "Macro", "Run", "Window", "?" }));

	// ---- font: views start zoomed so Windows point sizes look the same on macOS ----
	CHECK(ed->send(SCI_GETZOOM) == NppHost::kBaseZoom);
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Zoom In") { a->trigger(); break; }   // menu action (the toolbar proxy would trigger it again)
	CHECK(ed->send(SCI_GETZOOM) == NppHost::kBaseZoom + 1);
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Restore Default Zoom") a->trigger();
	CHECK(ed->send(SCI_GETZOOM) == NppHost::kBaseZoom);
	CHECK(tbAction("Print") != nullptr);

	// ---- live enabled state on a fresh untitled document ----
	pump();
	CHECK(!tbAction("Undo")->isEnabled() && !tbAction("Redo")->isEnabled());
	CHECK(!tbAction("Cut")->isEnabled() && !tbAction("Copy")->isEnabled());
	CHECK(tbAction("Save")->isEnabled());                   // an untitled document can be saved
	CHECK(!tbAction("Save All")->isEnabled());

	QString f1 = tmp.filePath("one.txt"), f2 = tmp.filePath("two.txt"), f3 = tmp.filePath("three.txt");
	put(f1, "hello world\n"); put(f2, "two\n"); put(f3, "three\n");
	win.openPath(f1); pump();
	CHECK(tabs->count() == 1 && !tbAction("Save")->isEnabled());      // clean file: nothing to save
	ed->send(SCI_APPENDTEXT, 3, reinterpret_cast<sptr_t>("abc")); pump();
	CHECK(tbAction("Undo")->isEnabled() && tbAction("Save")->isEnabled() && tbAction("Save All")->isEnabled());
	ed->send(SCI_SETSEL, 0, 5); pump();
	CHECK(tbAction("Cut")->isEnabled() && tbAction("Copy")->isEnabled());
	ed->send(SCI_SETSEL, 0, 0); pump();
	CHECK(!tbAction("Cut")->isEnabled() && !tbAction("Copy")->isEnabled());

	// clicking a button runs the menu command
	tbAction("Save")->trigger(); pump();
	QFile rf(f1); rf.open(QIODevice::ReadOnly);
	CHECK(rf.readAll() == "hello world\nabc" && !tbAction("Save")->isEnabled() && !tbAction("Save All")->isEnabled());

	// checkable buttons mirror the editor state
	QAction* wrap = tbAction("Word Wrap");
	CHECK(!wrap->isChecked());
	wrap->trigger(); pump();
	CHECK(wrap->isChecked() && ed->send(SCI_GETWRAPMODE) != 0);
	wrap->trigger(); pump();
	CHECK(!wrap->isChecked() && ed->send(SCI_GETWRAPMODE) == 0);
	QAction* all = tbAction("Show All Characters");
	all->trigger(); pump();
	CHECK(all->isChecked() && ed->send(SCI_GETVIEWWS) != 0 && ed->send(SCI_GETVIEWEOL) != 0);
	all->trigger(); pump();
	CHECK(!all->isChecked() && ed->send(SCI_GETVIEWWS) == 0);
	QAction* guide = tbAction("Show Indent Guide");
	bool guideBefore = guide->isChecked();
	guide->trigger(); pump();
	CHECK(guide->isChecked() != guideBefore);
	guide->trigger(); pump();

	// ---- Encoding menu: ticks the current encoding, and changes it ----
	QMenu* enc = menuNamed("Encoding");
	CHECK(enc != nullptr);
	if (enc) {
		enc->aboutToShow();
		bool u8 = false; for (QAction* a : enc->actions()) if (a->text() == "UTF-8" && a->isChecked()) u8 = true;
		CHECK(u8);
		for (QAction* a : enc->actions()) if (a->text() == "UTF-8-BOM") a->trigger();
		pump();
		CHECK(win.statusFields()[4] == "UTF-8-BOM");
		enc->aboutToShow();
		bool bom = false; for (QAction* a : enc->actions()) if (a->text() == "UTF-8-BOM" && a->isChecked()) bom = true;
		CHECK(bom);
	}

	// ---- Window menu lists the open documents; Next/Previous Tab ----
	win.openPath(f2); win.openPath(f3); pump();
	CHECK(tabs->count() == 3 && tabs->currentIndex() == 2);
	QMenu* wm = menuNamed("Window");
	CHECK(wm != nullptr);
	if (wm) {
		wm->aboutToShow();
		QStringList docs; QString current;
		for (QAction* a : wm->actions()) if (a->isCheckable()) { docs << a->text(); if (a->isChecked()) current = a->text(); }
		CHECK(docs == (QStringList{ "1  one.txt", "2  two.txt", "3  three.txt" }) && current == "3  three.txt");
		for (QAction* a : wm->actions()) if (a->text() == "1  one.txt") a->trigger();
		CHECK(tabs->currentIndex() == 0);
		for (QAction* a : wm->actions()) if (a->text() == "Next Tab") a->trigger();
		CHECK(tabs->currentIndex() == 1);
		for (QAction* a : wm->actions()) if (a->text() == "Previous Tab") a->trigger();
		CHECK(tabs->currentIndex() == 0);
	}

	// ---- Edit > Copy to Clipboard (current document is one.txt) ----
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Copy Current Filename") a->trigger();
	CHECK(QGuiApplication::clipboard()->text() == "one.txt");
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Copy Current Full File Path") a->trigger();
	CHECK(QGuiApplication::clipboard()->text() == QFileInfo(f1).absoluteFilePath());
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Copy Current File Directory") a->trigger();
	CHECK(QGuiApplication::clipboard()->text() == QFileInfo(f1).absolutePath());

	// ---- Tools: hashes (known test vectors) and the menu command ----
	CHECK(hashHex(QCryptographicHash::Md5, "abc") == "900150983cd24fb0d6963f7d28e17f72");
	CHECK(hashHex(QCryptographicHash::Sha1, "abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
	CHECK(hashHex(QCryptographicHash::Sha256, "abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(hashHex(QCryptographicHash::Sha512, "abc").startsWith("ddaf35a193617aba"));
	auto hf = hashFiles(QCryptographicHash::Sha1, { f2, tmp.filePath("missing.bin") });
	CHECK(hf.size() == 2 && hf[0].hash == hashHex(QCryptographicHash::Sha1, "two\n") && hf[1].hash.isEmpty());
	if (QMenu* tools = menuNamed("Tools")) {
		QMenu* md5 = nullptr; for (QAction* a : tools->actions()) if (a->menu() && a->text() == "MD5") md5 = a->menu();
		CHECK(md5 != nullptr);
		if (md5) {
			ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("abc")); ed->send(SCI_SETSEL, 0, 3);
			for (QAction* a : md5->actions()) if (a->text() == "Generate from selection into Clipboard") a->trigger();
			CHECK(QGuiApplication::clipboard()->text() == "900150983cd24fb0d6963f7d28e17f72");
			ed->send(SCI_UNDO);
		}
	} else CHECK(false);

	// ---- Print: lay out onto a PDF "printer" (one page for a short file, several for a long one) ----
	auto pdfPages = [&](const QString& path) {
		QFile pf(path); pf.open(QIODevice::ReadOnly); QByteArray b = pf.readAll();
		int n = 0; for (int i = b.indexOf("/Type /Page"); i >= 0; i = b.indexOf("/Type /Page", i + 1)) if (b.mid(i + 11, 1) != "s") ++n;
		return QPair<bool, int>(b.startsWith("%PDF"), n);
	};
	{
		QPrinter pr(QPrinter::HighResolution); pr.setOutputFormat(QPrinter::PdfFormat); pr.setOutputFileName(tmp.filePath("short.pdf"));
		ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>("int main() { return 0; }\n"));
		CHECK(printDocument(ed, &pr) == 1);
		auto r = pdfPages(tmp.filePath("short.pdf")); CHECK(r.first && r.second == 1);
	}
	{
		QPrinter pr(QPrinter::HighResolution); pr.setOutputFormat(QPrinter::PdfFormat); pr.setOutputFileName(tmp.filePath("long.pdf"));
		QByteArray big; for (int i = 0; i < 400; ++i) big += "line " + QByteArray::number(i) + " of a long document\n";
		ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(big.constData()));
		int pages = printDocument(ed, &pr);
		auto r = pdfPages(tmp.filePath("long.pdf"));
		CHECK(pages >= 3 && r.first && r.second == pages);
		ed->send(SCI_UNDO);
	}
	{
		QPrinter pr(QPrinter::HighResolution); pr.setOutputFormat(QPrinter::PdfFormat); pr.setOutputFileName(tmp.filePath("empty.pdf"));
		ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(""));
		CHECK(printDocument(ed, &pr) == 0);
		ed->send(SCI_UNDO); ed->send(SCI_UNDO);
	}

	// ---- per-tab close button and Close All ----
	CHECK(qobject_cast<QWidget*>(tabs->tabButton(0, QTabBar::RightSide)) != nullptr);
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Close All") a->trigger();
	pump();
	CHECK(tabs->count() == 1 && win.tabBar(1)->count() == 0);
	CHECK(tabs->tabText(0).startsWith("new"));                         // all closed: a fresh untitled document remains

	// ---- icons follow the theme ----
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Obsidian") a->trigger();
	CHECK(win.isDarkChrome());
	for (QAction* a : win.toolbar()->actions()) if (!a->isSeparator()) CHECK(!a->icon().isNull());

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
