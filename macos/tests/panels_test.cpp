// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Phase 4: Function List, Folder as Workspace and Document List panels.
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QTabBar>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTreeWidget>
#include <cstdio>
#include "ScintillaEdit.h"
#include "core_boot.h"
#include "main_window.h"
#include "npp_host.h"
#include "panels.h"
#include "qt_bridge.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
static void put(const QString& p, const QByteArray& d) { QDir().mkpath(QFileInfo(p).absolutePath()); QFile f(p); f.open(QIODevice::WriteOnly); f.write(d); }
static QStringList leaves(QTreeWidgetItem* n, bool onlyVisible = false) {
	QStringList out;
	for (int i = 0; i < n->childCount(); ++i) { auto* c = n->child(i); if (onlyVisible && c->isHidden()) continue; if (c->childCount()) out << leaves(c, onlyVisible); else out << c->text(0); }
	return out;
}

int main(int argc, char** argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	QApplication app(argc, argv);
	QTemporaryDir tmp;
	qputenv("HOME", tmp.path().toUtf8());
	QString settings = tmp.filePath("settings"); QDir().mkpath(settings);
	installQtBridge(); bootCore(settings.toStdWString());
	NppHost& host = NppHost::get(); host.init();
	MainWindow win(host); win.restoreSession();

	QString cpp = tmp.filePath("calc.cpp"), py = tmp.filePath("tool.py");
	put(cpp, "class Calc {\npublic:\n    int add(int a, int b) { return a + b; }\n    int sub(int a, int b) { return a - b; }\n};\n\nstatic int helper(int x) { return x * 2; }\n\nint main() { return helper(1); }\n");
	put(py, "def top():\n    pass\n\nclass Thing:\n    def method(self):\n        pass\n");

	// ---------------- function list ----------------
	win.openPath(cpp);
	auto fl = host.functions(NppHost::MAIN);
	CHECK(fl.size() >= 3);
	win.functionList()->show();
	win.refreshFunctionList();
	QTreeWidget* tree = win.functionList()->tree();
	CHECK(tree->topLevelItemCount() == 1 && tree->topLevelItem(0)->text(0) == "calc.cpp");
	QStringList names = leaves(tree->topLevelItem(0));
	CHECK(names.contains("add") && names.contains("sub") && names.contains("helper") && names.contains("main"));

	// jump: activating an entry moves the caret to that function
	QTreeWidgetItem* mainItem = nullptr;
	for (QTreeWidgetItemIterator it(tree); *it; ++it) if ((*it)->text(0) == "main") mainItem = *it;
	CHECK(mainItem != nullptr);
	if (mainItem) {
		tree->itemActivated(mainItem, 0);
		ScintillaEdit* ed = win.editor(0);
		long long pos = ed->send(SCI_GETCURRENTPOS);
		CHECK(pos == mainItem->data(0, Qt::UserRole).toLongLong());
		CHECK(ed->send(SCI_LINEFROMPOSITION, pos) == 8);          // "int main()" is on line 9 (0-based 8)
	}

	// filter and sort
	auto* filter = win.functionList()->findChild<QLineEdit*>();
	filter->setText("su");
	CHECK(leaves(tree->topLevelItem(0)) == QStringList{ "sub" });
	filter->clear();
	QString zc = tmp.filePath("order.c");
	put(zc, "int zeta(void) { return 1; }\n\nint alpha(void) { return 2; }\n\nint mid(void) { return 3; }\n");
	win.openPath(zc);
	win.refreshFunctionList();
	QStringList unsorted = leaves(tree->topLevelItem(0));
	CHECK(unsorted.size() == 3 && unsorted[0].startsWith("zeta") && unsorted[1].startsWith("alpha"));   // source order
	win.functionList()->setSorted(true);
	QStringList sorted = leaves(tree->topLevelItem(0));
	CHECK(sorted.size() == 3 && sorted[0].startsWith("alpha") && sorted[1].startsWith("mid") && sorted[2].startsWith("zeta"));
	win.functionList()->setSorted(false);

	// another language: Python (class + method)
	win.openPath(py);
	win.refreshFunctionList();
	CHECK(tree->topLevelItem(0)->text(0) == "tool.py");
	names = leaves(tree->topLevelItem(0));
	CHECK(names.contains("top()") && names.contains("method(self)"));   // the Python rules keep the signature
	{ bool classed = false; for (QTreeWidgetItemIterator it(tree); *it; ++it) if ((*it)->text(0) == "Thing" && (*it)->childCount() == 1) classed = true; CHECK(classed); }  // methods grouped under their class

	// ---------------- document list ----------------
	win.docList()->show();
	win.refreshDocList();
	QTreeWidget* dl = win.docList()->tree();
	CHECK(dl->topLevelItemCount() == 3);
	CHECK(dl->topLevelItem(0)->text(0) == "calc.cpp" && dl->topLevelItem(1)->text(0) == "order.c" && dl->topLevelItem(2)->text(0) == "tool.py");
	win.editor(0)->send(SCI_APPENDTEXT, 1, reinterpret_cast<sptr_t>("#"));            // dirty the current doc (tool.py)
	win.refreshDocList();
	CHECK(dl->topLevelItem(2)->text(0) == "tool.py *");
	dl->itemClicked(dl->topLevelItem(0), 0);                                          // click calc.cpp -> its tab becomes current
	CHECK(win.tabBar(0)->currentIndex() == 0);

	// ---------------- workspace ----------------
	QString root = tmp.filePath("proj");
	put(root + "/src/main.c", "int main(void){return 0;}\n"); put(root + "/README.md", "# hi\n"); put(root + "/.DS_Store", "x");
	win.addWorkspace(root);
	QTreeWidget* ws = win.workspace()->tree();
	CHECK(ws->topLevelItemCount() == 1 && win.workspace()->roots() == QStringList{ QDir(root).absolutePath() });
	QTreeWidgetItem* r = ws->topLevelItem(0);
	CHECK(r->childCount() == 2 && r->child(0)->text(0) == "src" && r->child(1)->text(0) == "README.md");   // folders first, .DS_Store hidden
	r->child(0)->setExpanded(true);                                                                           // lazy listing on expand
	CHECK(r->child(0)->childCount() == 1 && r->child(0)->child(0)->text(0) == "main.c");
	ws->itemActivated(r->child(0)->child(0), 0);
	bool opened = false;
	for (int i = 0; i < win.tabBar(0)->count(); ++i) if (win.tabBar(0)->tabText(i) == "main.c") opened = true;
	CHECK(opened);
	put(root + "/new.txt", "n");
	win.workspace()->refresh(r);
	CHECK(r->childCount() == 3);

	for (QAction* a : win.findChildren<QAction*>()) if (a->text() == "Save All") a->trigger();   // so closing does not ask

	// everything is remembered: workspace root in session.xml, panels' layout and recents in the profile (not ~/Library/Preferences)
	win.close();
	QFile sf(settings + "/session.xml"); sf.open(QIODevice::ReadOnly);
	CHECK(QString::fromUtf8(sf.readAll()).contains(QDir(root).absolutePath()));
	CHECK(QFileInfo::exists(settings + "/macos-ui.ini"));

	printf(fails ? "FAILED (%d)\n" : "OK\n", fails);
	return fails;
}
