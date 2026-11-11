// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// PadPlus: a macOS editor, Qt front end over the shared Notepad++ core.
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QPrinter>
#include <QTimer>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "core_boot.h"
#include "main_window.h"
#include "tools.h"
#include "npp_host.h"
#include "print_doc.h"
#include "qt_bridge.h"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	app.setApplicationName("PadPlus");
	installQtBridge();
	const char* dir = getenv("NPP_SETTINGS_DIR"); // override for tests / portable use
	bootCore(dir ? QString::fromUtf8(dir).toStdWString() : std::wstring());
	NppHost& host = NppHost::get();
	host.init();

	MainWindow win(host);
	win.restoreSession();
	for (int i = 1; i < argc; ++i) win.openPath(QDir::current().absoluteFilePath(QString::fromLocal8Bit(argv[i])));
	if (const char* ws = getenv("NPP_WORKSPACE")) win.addWorkspace(QString::fromUtf8(ws)); // dev aid: preload a workspace folder
	if (const char* th = getenv("NPP_THEME")) {       // dev aid: start with this theme (by name, not saved)
		auto names = host.themes();
		for (size_t i = 0; i < names.size(); ++i) if (QString::fromStdWString(names[i]) == th) host.setTheme((int)i);
	}
	win.show();
	if (getenv("NPP_DEBUG")) { fprintf(stderr, "%ls\n", host.resourceSummary().c_str()); win.debugDump(); }

	if (const char* out = getenv("NPP_SNAPSHOT")) { // smoke test: render, save PNG, exit
		QString path = QString::fromUtf8(out);
		QTimer::singleShot(600, [&win, path] { win.grab().save(path); win.close(); });
	}
	// dev aid: NPP_PRINT_PDF=<file.pdf> prints the first view's document to a PDF and exits
	if (const char* pdf = getenv("NPP_PRINT_PDF")) {
		QString path = QString::fromUtf8(pdf);
		QTimer::singleShot(800, [&win, path] { QPrinter pr(QPrinter::HighResolution); pr.setOutputFormat(QPrinter::PdfFormat); pr.setOutputFileName(path); printDocument(win.editor(0), &pr); win.close(); });
	}
	// dev aid: NPP_HASH_DEMO=<text> shows the SHA-256 result dialog for that text (the real menu path needs typing into an input dialog first)
	if (const char* hd = getenv("NPP_HASH_DEMO")) {
		QString text = QString::fromUtf8(hd);
		QTimer::singleShot(1000, [&win, text] { HashDialog(QObject::tr("SHA-256 digest"), hashHex(QCryptographicHash::Sha256, text.toUtf8()), &win).exec(); });
	}
	// dev aid: NPP_OPEN_ACTION=<menu item text> triggers that item shortly after start and leaves the app running (for real-screen captures of native dialogs)
	if (const char* oa = getenv("NPP_OPEN_ACTION")) {
		QString text = QString::fromUtf8(oa);
		QTimer::singleShot(1200, [&win, text] { for (QAction* a : win.findChildren<QAction*>()) if (a->text() == text && a->isEnabled()) { a->trigger(); break; } });
	}
	// dev aid: NPP_TRIGGER=<menu item text> NPP_SNAPSHOT_DIALOG=<png> opens that item's dialog, saves it as a PNG and exits
	if (const char* act = getenv("NPP_TRIGGER")) {
		QString text = QString::fromUtf8(act), out = QString::fromUtf8(getenv("NPP_SNAPSHOT_DIALOG") ? getenv("NPP_SNAPSHOT_DIALOG") : "");
		QTimer::singleShot(400, [&win, text] {   // several items can be given, separated by '|'
			for (const QString& t : text.split('|')) for (QAction* a : win.findChildren<QAction*>()) if (a->text() == t) { a->trigger(); break; } });
		QTimer::singleShot(1500, [out] {
			for (QWidget* w : QApplication::topLevelWidgets()) if (w->isVisible() && qobject_cast<QDialog*>(w)) { w->grab().save(out); qobject_cast<QDialog*>(w)->reject(); }
			QApplication::quit(); });
	}
	return app.exec();
}
