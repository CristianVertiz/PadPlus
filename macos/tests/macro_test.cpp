// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Macro recorder (record / playback / save / delete / run to end, persisted in shortcuts.xml) and synchronized scrolling.
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QTemporaryDir>
#include <QToolBar>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "main_window.h"
#include "npp_host.h"
#include "qt_bridge.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static QString text(ScintillaEdit* e) { return QString::fromUtf8(e->get_text_range(0, e->send(SCI_GETLENGTH))); }
static void setText(ScintillaEdit* e, const char* t) { e->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(t)); e->send(SCI_EMPTYUNDOBUFFER); }
static QString slurp(const QString& p) { QFile f(p); f.open(QIODevice::ReadOnly); return QString::fromUtf8(f.readAll()); }

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
	auto tb = [&](const QString& text) -> QAction* { for (QAction* a : win.toolbar()->actions()) if (!a->isSeparator() && a->text() == text) return a; printf("no toolbar action '%s'\n", qPrintable(text)); return nullptr; };
	auto menuAct = [&](const QString& text) -> QAction* { for (QAction* a : win.macroMenu()->actions()) if (a->text() == text) return a; return nullptr; };

	// ---------------- macro recorder ----------------
	CHECK(win.macroMenu() != nullptr);
	CHECK(menuAct("Start Recording")->isEnabled() && !menuAct("Stop Recording")->isEnabled() && !menuAct("Playback")->isEnabled() && !menuAct("Save Current Recorded Macro...")->isEnabled());
	CHECK(tb("Start Recording")->isEnabled() && !tb("Stop Recording")->isEnabled() && !tb("Playback")->isEnabled());

	setText(ed, "x"); ed->send(SCI_SETEOLMODE, SC_EOL_LF); ed->send(SCI_DOCUMENTEND);
	tb("Start Recording")->trigger(); pump();                                   // the toolbar button runs the menu command
	CHECK(host.macroRecording() && !menuAct("Start Recording")->isEnabled() && menuAct("Stop Recording")->isEnabled());
	CHECK(!tb("Playback")->isEnabled());                                        // nothing to play while recording
	ed->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>("ab"));                // what typing sends
	ed->send(SCI_NEWLINE);
	ed->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>("c"));
	CHECK(text(ed) == "xab\nc");
	tb("Stop Recording")->trigger(); pump();
	CHECK(!host.macroRecording() && host.macroLength() == 3);
	CHECK(menuAct("Playback")->isEnabled() && tb("Playback")->isEnabled() && tb("Save Current Recorded Macro")->isEnabled());

	tb("Playback")->trigger(); pump();                                          // replays: "ab", newline, "c"
	CHECK(text(ed) == "xab\ncab\nc");
	ed->send(SCI_UNDO);
	CHECK(text(ed) == "xab\nc");                                                // the whole playback was one undo step
	win.runMacro(-1, 3, false);
	CHECK(text(ed) == "xab\ncab\ncab\ncab\nc");

	// ---------------- save, shortcut, persistence, run saved, delete ----------------
	const int base = (int)host.savedMacros().size();                           // Notepad++ ships predefined macros (e.g. "Trim Trailing Space and Save")
	CHECK(base >= 1);
	QAction* predefined = menuAct("Trim Trailing Space and Save");
	CHECK(predefined != nullptr && predefined->shortcut().isEmpty());          // its Alt+S would shadow Option+S text input on a Mac, so it is not applied

	CHECK(win.saveMacro("append ab", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F8)));
	auto all = host.savedMacros();
	CHECK((int)all.size() == base + 1 && all.back().name == L"append ab" && all.back().steps == 3 && all.back().vk == 0x77 && all.back().ctrl && all.back().alt);
	CHECK(!menuAct("Save Current Recorded Macro...")->isEnabled());             // already saved
	QAction* saved = menuAct("append ab");
	CHECK(saved != nullptr && saved->shortcut() == QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F8));
	QString xml = slurp(settings + "/shortcuts.xml");
	CHECK(xml.contains("<Macro name=\"append ab\"") && xml.contains("Key=\"119\"") && xml.contains("message=\"2170\""));   // SCI_REPLACESEL, same format as Windows
	setText(ed, "q"); ed->send(SCI_DOCUMENTEND);
	saved->trigger();
	CHECK(text(ed) == "qab\nc");
	CHECK(!win.saveMacro("", QKeySequence()));                                  // a name is required
	CHECK(win.saveMacro("dup", QKeySequence()) && (int)host.savedMacros().size() == base + 2);   // the same macro may be saved again under another name
	CHECK(win.deleteMacro(base + 1) && (int)host.savedMacros().size() == base + 1 && menuAct("dup") == nullptr);
	CHECK(win.deleteMacro(base) && (int)host.savedMacros().size() == base && !slurp(settings + "/shortcuts.xml").contains("append ab"));
	CHECK(menuAct("append ab") == nullptr && !win.deleteMacro(base + 5));

	// ---------------- run until the end of the file ----------------
	setText(ed, "aa\nbb\ncc\ndd\nee"); ed->send(SCI_DOCUMENTSTART);
	win.macroStart();                                                           // position first: every cursor-movement command is recorded too
	ed->send(SCI_VCHOME);
	ed->send(SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>("> "));
	ed->send(SCI_LINEDOWN);
	win.macroStop();
	CHECK(host.macroLength() == 3);
	setText(ed, "aa\nbb\ncc\ndd\nee"); ed->send(SCI_DOCUMENTSTART);
	win.runMacro(-1, 1, true);
	CHECK(text(ed) == "> aa\n> bb\n> cc\n> dd\n> ee");                           // every line, including the last, exactly once
	setText(ed, "single line"); ed->send(SCI_DOCUMENTSTART);
	win.runMacro(-1, 1, true);                                                   // a macro that never moves down must not loop forever
	CHECK(text(ed) == "> single line");

	// ---------------- synchronized scrolling ----------------
	QByteArray big; for (int i = 0; i < 300; ++i) big += QByteArray(250, 'x') + "\n";
	setText(ed, big.constData());
	QAction* syncV = tb("Synchronize Vertical Scrolling"); QAction* syncH = tb("Synchronize Horizontal Scrolling");
	CHECK(syncV && syncH && !syncV->isEnabled() && !syncH->isEnabled());       // one view only: nothing to synchronize
	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Clone to Other View") { a->trigger(); break; }
	pump();
	ScintillaEdit* sub = win.editor(1);
	CHECK(syncV->isEnabled() && syncH->isEnabled());
	ed->send(SCI_SETFIRSTVISIBLELINE, 0); sub->send(SCI_SETFIRSTVISIBLELINE, 10);
	CHECK(!win.syncVertical());
	ed->send(SCI_SETFIRSTVISIBLELINE, 20);
	CHECK(sub->send(SCI_GETFIRSTVISIBLELINE) == 10);                            // off: the other view stays put
	ed->send(SCI_SETFIRSTVISIBLELINE, 0);

	syncV->trigger(); pump();                                                   // on: offset = 10 lines
	CHECK(win.syncVertical() && syncV->isChecked());
	ed->send(SCI_SETFIRSTVISIBLELINE, 30);
	CHECK(sub->send(SCI_GETFIRSTVISIBLELINE) == 40);                            // main + offset
	sub->send(SCI_SETFIRSTVISIBLELINE, 70);
	CHECK(ed->send(SCI_GETFIRSTVISIBLELINE) == 60);                             // and the other way round
	ed->send(SCI_SETXOFFSET, 90);
	CHECK(sub->send(SCI_GETXOFFSET) != 90 || !win.syncHorizontal());            // horizontal sync is still off

	ed->send(SCI_SETXOFFSET, 0); sub->send(SCI_SETXOFFSET, 0);                  // equal positions: offset 0
	syncH->trigger(); pump();
	CHECK(win.syncHorizontal());
	ed->send(SCI_SETXOFFSET, 120);
	CHECK(sub->send(SCI_GETXOFFSET) == 120);
	sub->send(SCI_SETXOFFSET, 40);
	CHECK(ed->send(SCI_GETXOFFSET) == 40);

	syncV->trigger(); pump();                                                   // off again
	CHECK(!win.syncVertical() && !syncV->isChecked());
	int before = (int)sub->send(SCI_GETFIRSTVISIBLELINE);
	ed->send(SCI_SETFIRSTVISIBLELINE, 5);
	CHECK(sub->send(SCI_GETFIRSTVISIBLELINE) == before);

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
