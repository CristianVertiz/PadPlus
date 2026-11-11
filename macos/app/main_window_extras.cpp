// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Bookmarks, mark styles, incremental search, the Run menu, file monitoring (tail -f), extra panels, user defined languages.
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QXmlStreamReader>
#include <QSet>
#include <algorithm>
#include "ScintillaEdit.h"
#include "chrome.h"
#include "extras.h"
#include "find_dialog.h"
#include "udl_dialog.h"
#include "main_window.h"
#include "npp_host.h"
#include "panels.h"
#include "menuCmdID.h"

static QString q(const std::wstring& w) { return QString::fromStdWString(w); }
static const int kBookmark = 20;                      // MARK_BOOKMARK (the core defines its look and the margin)
static const int kMarkIndic[5] = { 25, 24, 23, 22, 21 };   // Notepad++'s Mark Style 1..5 indicators
static const long kMarkColor[5] = { 0x00FFFF, 0x0080FF, 0x00C000, 0xFF00FF, 0xFFA000 };   // BGR

static QString selectionOrWord(ScintillaEdit* ed) {
	if (!ed) return QString();
	long s = ed->send(SCI_GETSELECTIONSTART), e = ed->send(SCI_GETSELECTIONEND);
	if (s == e) { long p = ed->send(SCI_GETCURRENTPOS); s = ed->send(SCI_WORDSTARTPOSITION, p, true); e = ed->send(SCI_WORDENDPOSITION, p, true); }
	return s < e ? QString::fromUtf8(ed->get_text_range(s, e)) : QString();
}

// ------------------------------------------------------------------------------------------------ Search menu
void MainWindow::buildSearchExtras(QMenu* s) {
	auto act = [this](QMenu* m, const QString& t, auto fn, const QKeySequence& k, int idm) {
		QAction* a = m->addAction(t, this, [this, fn, idm] { const int n = _h.macroLength(); fn(); recordCommand(idm, n); });
		if (!k.isEmpty()) a->setShortcut(k); if (idm) _byCmd[idm] = a; return a; };
	act(s, tr("Mark..."), [this] { _find->showFind(selectionOrWord(activeEditor())); }, QKeySequence(), IDM_SEARCH_MARK);
	act(s, tr("Find Characters in Range..."), [this] { if (!_chars) _chars = new FindCharsDialog([this] { return activeEditor(); }, this); _chars->show(); _chars->raise(); }, QKeySequence(), IDM_SEARCH_FINDCHARINRANGE);
	s->addSeparator();
	act(s, tr("Incremental Search"), [this] { _inc->open(selectionOrWord(activeEditor())); }, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_I), IDM_SEARCH_FINDINCREMENT);
	s->addSeparator();
	QMenu* bm = s->addMenu(tr("Bookmark")); bm->setObjectName("search-bookmark");
	act(bm, tr("Toggle Bookmark"), [this] { bookmark(0); }, QKeySequence(Qt::CTRL | Qt::Key_F2), IDM_SEARCH_TOGGLE_BOOKMARK);
	act(bm, tr("Next Bookmark"), [this] { bookmark(1); }, QKeySequence(Qt::Key_F2), IDM_SEARCH_NEXT_BOOKMARK);
	act(bm, tr("Previous Bookmark"), [this] { bookmark(2); }, QKeySequence(Qt::SHIFT | Qt::Key_F2), IDM_SEARCH_PREV_BOOKMARK);
	act(bm, tr("Clear All Bookmarks"), [this] { bookmark(3); }, QKeySequence(), IDM_SEARCH_CLEAR_BOOKMARKS);
	bm->addSeparator();
	act(bm, tr("Cut Bookmarked Lines"), [this] { bookmarkedLines(0); }, QKeySequence(), IDM_SEARCH_CUTMARKEDLINES);
	act(bm, tr("Copy Bookmarked Lines"), [this] { bookmarkedLines(1); }, QKeySequence(), IDM_SEARCH_COPYMARKEDLINES);
	act(bm, tr("Remove Bookmarked Lines"), [this] { bookmarkedLines(2); }, QKeySequence(), IDM_SEARCH_DELETEMARKEDLINES);
	QMenu* mk = s->addMenu(tr("Style All Occurrences of Token")); mk->setObjectName("search-markAll");
	for (int i = 1; i <= 5; ++i) act(mk, tr("Using Style %1").arg(i), [this, i] { markAll(i); }, QKeySequence(), IDM_SEARCH_MARKALLEXT1 + 2 * (i - 1));
	mk->addSeparator();
	act(mk, tr("Clear All Styles"), [this] { markAll(0); }, QKeySequence(), IDM_SEARCH_CLEARALLMARKS);
}

void MainWindow::bookmark(int op) {
	ScintillaEdit* ed = activeEditor(); if (!ed) return;
	const int mask = 1 << kBookmark;
	long line = ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)), count = ed->send(SCI_GETLINECOUNT), hit = -1;
	switch (op) {
	case 0: if (ed->send(SCI_MARKERGET, line) & mask) ed->send(SCI_MARKERDELETE, line, kBookmark); else ed->send(SCI_MARKERADD, line, kBookmark); return;
	case 1: hit = ed->send(SCI_MARKERNEXT, line + 1, mask); if (hit < 0) hit = ed->send(SCI_MARKERNEXT, 0, mask); break;
	case 2: hit = line > 0 ? ed->send(SCI_MARKERPREVIOUS, line - 1, mask) : -1; if (hit < 0) hit = ed->send(SCI_MARKERPREVIOUS, count - 1, mask); break;
	case 3: ed->send(SCI_MARKERDELETEALL, kBookmark); return;
	}
	if (hit >= 0) { ed->send(SCI_ENSUREVISIBLE, hit); ed->send(SCI_GOTOLINE, hit); }
}

void MainWindow::bookmarkedLines(int op) {
	ScintillaEdit* ed = activeEditor(); if (!ed) return;
	const int mask = 1 << kBookmark;
	QList<QPair<long, long>> spans;
	for (long l = ed->send(SCI_MARKERNEXT, 0, mask); l >= 0; l = ed->send(SCI_MARKERNEXT, l + 1, mask))
		spans.append({ ed->send(SCI_POSITIONFROMLINE, l), l + 1 < ed->send(SCI_GETLINECOUNT) ? ed->send(SCI_POSITIONFROMLINE, l + 1) : ed->send(SCI_GETLENGTH) });
	if (spans.isEmpty()) return;
	if (op <= 1) { QByteArray t; for (auto& sp : spans) t += ed->get_text_range(sp.first, sp.second); QApplication::clipboard()->setText(QString::fromUtf8(t)); }
	if (op == 1 || ed->send(SCI_GETREADONLY)) return;
	ed->send(SCI_BEGINUNDOACTION);
	for (int i = spans.size() - 1; i >= 0; --i) { ed->send(SCI_SETTARGETRANGE, spans[i].first, spans[i].second); ed->send(SCI_REPLACETARGET, 0, (sptr_t)""); }
	ed->send(SCI_ENDUNDOACTION);
}

void MainWindow::markAll(int style) {
	ScintillaEdit* ed = activeEditor(); if (!ed) return;
	long len = ed->send(SCI_GETLENGTH);
	if (style == 0) { for (int i : kMarkIndic) { ed->send(SCI_SETINDICATORCURRENT, i); ed->send(SCI_INDICATORCLEARRANGE, 0, len); } return; }
	QByteArray pat = selectionOrWord(ed).toUtf8();
	if (pat.isEmpty()) return;
	int ind = kMarkIndic[style - 1];
	ed->send(SCI_SETINDICATORCURRENT, ind);
	ed->send(SCI_INDICSETSTYLE, ind, INDIC_ROUNDBOX); ed->send(SCI_INDICSETFORE, ind, kMarkColor[style - 1]); ed->send(SCI_INDICSETALPHA, ind, 130);
	ed->send(SCI_INDICATORCLEARRANGE, 0, len);
	ed->send(SCI_SETSEARCHFLAGS, SCFIND_MATCHCASE);
	for (long pos = 0; pos < len;) {
		ed->send(SCI_SETTARGETRANGE, pos, len);
		long hit = ed->send(SCI_SEARCHINTARGET, pat.size(), (sptr_t)pat.constData());
		if (hit < 0) break;
		ed->send(SCI_INDICATORFILLRANGE, hit, pat.size()); pos = hit + pat.size();
	}
}

// ------------------------------------------------------------------------------------------------ View menu: panels + monitoring
void MainWindow::buildViewExtras(QMenu* panels, QMenu* view) {
	panels->addAction(_clipPanel->toggleViewAction());
	panels->addAction(_ansiPanel->toggleViewAction());
	panels->addAction(_mapPanel->toggleViewAction()); _byCmd[IDM_VIEW_DOC_MAP] = _mapPanel->toggleViewAction();
	QAction* mon = view->addAction(tr("Monitoring (tail -f)"), this, [this] { toggleMonitor(); });
	mon->setCheckable(true); _byCmd[IDM_VIEW_MONITORING] = mon;
}

bool MainWindow::monitoring() const { return currentActive() && _tail.contains(currentActive()); }

void MainWindow::toggleMonitor() {
	Buffer* b = currentActive(); if (!b) return;
	if (_tail.contains(b)) _tail.remove(b);
	else {
		QFileInfo fi(q(_h.info(b).path));
		if (!fi.exists()) { _byCmd[IDM_VIEW_MONITORING]->setChecked(false); return; }      // untitled documents cannot be followed
		_tail[b] = { fi.size(), fi.lastModified().toMSecsSinceEpoch() };
		pollMonitored();
	}
	if (_tail.isEmpty()) _monTimer->stop(); else _monTimer->start();
	syncActionStates();
}

void MainWindow::pollMonitored() {
	for (auto it = _tail.begin(); it != _tail.end();) {
		Buffer* b = it.key();
		bool open = false;
		for (auto& p : _p) open |= std::find(p.docs.begin(), p.docs.end(), b) != p.docs.end();
		if (!open) { it = _tail.erase(it); continue; }                                     // closed meanwhile
		QFileInfo fi(q(_h.info(b).path));
		QPair<qint64, qint64> now{ fi.size(), fi.lastModified().toMSecsSinceEpoch() };
		if (fi.exists() && now != it.value() && !_h.info(b).dirty) {
			it.value() = now;
			_h.reload(b);
			for (auto& p : _p) if (current(&p - _p) == b) p.ed->send(SCI_DOCUMENTEND);          // follow the end of the file
		}
		++it;
	}
	if (_tail.isEmpty()) _monTimer->stop();
}

// ------------------------------------------------------------------------------------------------ Run menu
QString MainWindow::expandRunVars(QString c) const {
	Buffer* b = currentActive(); ScintillaEdit* ed = activeEditor();
	QFileInfo fi(b ? q(_h.info(b).path) : QString());
	long pos = ed ? ed->send(SCI_GETCURRENTPOS) : 0;
	const QPair<const char*, QString> vars[] = {
		{ "FULL_CURRENT_PATH", fi.absoluteFilePath() }, { "CURRENT_DIRECTORY", fi.absolutePath() }, { "FILE_NAME", fi.fileName() },
		{ "NAME_PART", fi.completeBaseName() }, { "EXT_PART", fi.suffix().isEmpty() ? QString() : "." + fi.suffix() },
		{ "CURRENT_WORD", selectionOrWord(ed) }, { "CURRENT_LINE", ed ? QString::number(ed->send(SCI_LINEFROMPOSITION, pos) + 1) : QString() },
		{ "CURRENT_COLUMN", ed ? QString::number(ed->send(SCI_GETCOLUMN, pos) + 1) : QString() }, { "NPP_DIRECTORY", QCoreApplication::applicationDirPath() } };
	for (auto& v : vars) c.replace(QString("$(%1)").arg(v.first), v.second);
	return c;
}

void MainWindow::runCommand(const QString& cmd) {
	Buffer* b = currentActive();
	QString dir = b ? QFileInfo(q(_h.info(b).path)).absolutePath() : QDir::homePath();
	QProcess::startDetached("/bin/sh", { "-c", expandRunVars(cmd) }, QDir(dir).exists() ? dir : QDir::homePath());
}

void MainWindow::buildRunMenu() {
	QMenu* r = menuBar()->addMenu(tr("&Run")); r->setObjectName("run");
	auto file = [this]() -> QString { Buffer* b = currentActive(); return b ? q(_h.info(b).path) : QString(); };
	auto onDisk = [file](auto fn) { return [file, fn] { QString p = file(); if (!p.isEmpty() && QFileInfo::exists(p)) fn(p); }; };
	QAction* run = r->addAction(tr("Run..."), this, [this] {
		QSettings ui(QString::fromStdWString(_h.profileDir()) + "/macos-ui.ini", QSettings::IniFormat);
		QStringList hist = ui.value("runHistory").toStringList();
		QInputDialog d(this);
		d.setWindowTitle(tr("Run")); d.setInputMode(QInputDialog::TextInput); d.setComboBoxEditable(true); d.setComboBoxItems(hist);
		d.setLabelText(tr("Command to run in the document's folder.\nVariables: $(FULL_CURRENT_PATH)  $(CURRENT_DIRECTORY)  $(FILE_NAME)  $(NAME_PART)  $(EXT_PART)  $(CURRENT_WORD)  $(CURRENT_LINE)  $(CURRENT_COLUMN)"));
		for (QLabel* l : d.findChildren<QLabel*>()) l->setWordWrap(true);                 // one long line made the dialog wider than the screen
		d.setMinimumWidth(520);
		bool ok = d.exec() == QDialog::Accepted; QString c = d.textValue();
		if (!ok || c.trimmed().isEmpty()) return;
		hist.removeAll(c); hist.prepend(c); while (hist.size() > 20) hist.removeLast();
		ui.setValue("runHistory", hist);
		runCommand(c); });
	run->setShortcut(QKeySequence(Qt::Key_F5));
	r->addSeparator();
	r->addAction(tr("Open Containing Folder in Finder"), this, onDisk([](const QString& p) { QProcess::startDetached("open", { "-R", p }); }));
	r->addAction(tr("Open Containing Folder in Terminal"), this, onDisk([](const QString& p) { QProcess::startDetached("open", { "-a", "Terminal", QFileInfo(p).absolutePath() }); }));
	r->addAction(tr("Open in Default Application"), this, onDisk([](const QString& p) { QDesktopServices::openUrl(QUrl::fromLocalFile(p)); }));
	QMenu* br = r->addMenu(tr("Open in Browser"));
	for (const char* app : { "Safari", "Google Chrome", "Firefox" })
		br->addAction(app, this, onDisk([app](const QString& p) { QProcess::startDetached("open", { "-a", app, p }); }));
	r->addSeparator();
	auto web = [this](const QString& base) { return [this, base] { QString w = selectionOrWord(activeEditor()); if (!w.isEmpty()) QDesktopServices::openUrl(QUrl(base + QString::fromUtf8(QUrl::toPercentEncoding(w)))); }; };
	r->addAction(tr("Search Selection on Google"), this, web("https://www.google.com/search?q="));
	r->addAction(tr("Search Selection on Wikipedia"), this, web("https://en.wikipedia.org/wiki/Special:Search?search="));
}

// ------------------------------------------------------------------------------------------------ Language menu: user defined languages
void MainWindow::buildLanguageExtras(QMenu* lang) {
	lang->addSeparator();
	QMenu* udl = lang->addMenu(tr("User Defined Language")); udl->setObjectName("language-userDefinedLanguage");
	_byCmd[IDM_LANG_USER_DLG] = udl->addAction(tr("Define Your Language..."), this, [this] {      // also the toolbar's "udl" button
		UdlDialog d(_h, this); connect(&d, &UdlDialog::changed, this, &MainWindow::udlChanged); d.exec(); });
	udl->addAction(tr("Import Language Definition..."), this, [this] {
		QString f = QFileDialog::getOpenFileName(this, tr("Import User Defined Language"), QString(), tr("XML files (*.xml)"));
		if (f.isEmpty()) return;
		int n = _h.importUdl(f.toStdWString());
		QDir d(q(_h.udlFolder())); d.mkpath(".");
		QFile::copy(f, d.filePath(QFileInfo(f).fileName()));                          // loaded again at the next start
		QMessageBox::information(this, tr("User Defined Language"), n > 0 ? tr("%n language(s) imported.", nullptr, n) : tr("Nothing imported: the file has no new language definitions.")); });
	udl->addAction(tr("Open User Defined Language Folder"), this, [this] { QDir d(q(_h.udlFolder())); d.mkpath("."); QDesktopServices::openUrl(QUrl::fromLocalFile(d.path())); });
	udl->addSeparator();
	const int fixed = udl->actions().size();
	connect(udl, &QMenu::aboutToShow, this, [this, udl, fixed] {                      // the installed languages, rebuilt on every show
		for (QAction* a : udl->actions().mid(fixed)) { udl->removeAction(a); a->deleteLater(); }
		for (const std::wstring& n : _h.udlNames()) udl->addAction(q(n), this, [this, n] { if (Buffer* b = currentActive()) _h.setUserLanguage(b, n); });
		if (udl->actions().size() == fixed) udl->addAction(tr("(none installed)"))->setEnabled(false);
	});
}

void MainWindow::udlChanged(const QString& name) {
	const bool exists = [&] { for (const std::wstring& n : _h.udlNames()) if (q(n) == name) return true; return false; }();
	for (auto& p : _p) for (Buffer* b : p.docs) if (q(_h.info(b).language) == "User: " + name) { _h.setLanguage(b, 0); if (exists) _h.setUserLanguage(b, name.toStdWString()); }
	updateStatus();
}

// ------------------------------------------------------------------------------------------------ macros: menu commands
// The commands Notepad_plus::command() records while a macro is being recorded, limited to the ones this menu has.
void MainWindow::recordCommand(int idm, int stepsBefore) {
	static const QSet<int> ok = { IDM_FILE_NEW, IDM_FILE_CLOSE, IDM_FILE_CLOSEALL, IDM_FILE_SAVE, IDM_FILE_SAVEALL, IDM_EDIT_UNDO, IDM_EDIT_REDO, IDM_EDIT_SELECTALL, IDM_EDIT_DUP_LINE,
		IDM_EDIT_JOIN_LINES, IDM_EDIT_UPPERCASE, IDM_EDIT_LOWERCASE, IDM_EDIT_FULLPATHTOCLIP, IDM_EDIT_FILENAMETOCLIP, IDM_EDIT_CURRENTDIRTOCLIP, IDM_SEARCH_FINDNEXT, IDM_SEARCH_FINDPREV,
		IDM_SEARCH_TOGGLE_BOOKMARK, IDM_SEARCH_NEXT_BOOKMARK, IDM_SEARCH_PREV_BOOKMARK, IDM_SEARCH_CLEAR_BOOKMARKS, IDM_SEARCH_CUTMARKEDLINES, IDM_SEARCH_COPYMARKEDLINES,
		IDM_SEARCH_DELETEMARKEDLINES, IDM_SEARCH_MARKALLEXT1, IDM_SEARCH_MARKALLEXT2, IDM_SEARCH_MARKALLEXT3, IDM_SEARCH_MARKALLEXT4, IDM_SEARCH_MARKALLEXT5, IDM_SEARCH_CLEARALLMARKS,
		IDM_FORMAT_TODOS, IDM_FORMAT_TOUNIX, IDM_FORMAT_TOMAC, IDM_VIEW_WRAP, IDM_VIEW_GOTO_ANOTHER_VIEW, IDM_VIEW_CLONE_TO_ANOTHER_VIEW, IDM_VIEW_SYNSCROLLV, IDM_VIEW_SYNSCROLLH,
		IDM_VIEW_TAB_NEXT, IDM_VIEW_TAB_PREV };
	if (_h.macroRecording() && ok.contains(idm)) { _h.macroRecordCommand(idm, stepsBefore); updateMacroActions(); }
}

// ------------------------------------------------------------------------------------------------ localization
// Notepad++'s nativeLang files name every menu and command by id, so the menus translate without a table of our own. Dialogs stay English.
void MainWindow::applyLocalization(const QString& file) {
	QList<QAction*> all = _byCmd.values();
	QHash<QString, QAction*> menus;
	for (QMenu* m : menuBar()->findChildren<QMenu*>()) if (!m->objectName().isEmpty()) { menus[m->objectName()] = m->menuAction(); all << m->menuAction(); }
	for (QAction* a : all) { if (!a->property("en").isValid()) a->setProperty("en", a->text()); a->setText(a->property("en").toString()); }
	QFile f(resourcesDir() + "/localization/" + file);
	if (file.isEmpty() || !f.open(QIODevice::ReadOnly)) return;
	QXmlStreamReader x(&f);
	for (bool inMain = false; !x.atEnd();) {
		const auto t = x.readNext();
		if (x.name() == QLatin1String("Main")) { if (t == QXmlStreamReader::EndElement) break; inMain = t == QXmlStreamReader::StartElement; }
		if (!inMain || t != QXmlStreamReader::StartElement || x.name() != QLatin1String("Item")) continue;
		const auto at = x.attributes();
		QAction* a = at.hasAttribute("id") ? _byCmd.value(at.value("id").toInt()) : menus.value(at.value(at.hasAttribute("menuId") ? "menuId" : "subMenuId").toString());
		if (a && !at.value("name").isEmpty()) a->setText(at.value("name").toString());
	}
}

void MainWindow::buildLocalizationMenu(QMenu* settings) {
	QMenu* m = settings->addMenu(tr("Localization"));
	auto* group = new QActionGroup(this);
	const QString cur = QSettings(q(_h.profileDir()) + "/macos-ui.ini", QSettings::IniFormat).value("localization").toString();
	QStringList files = QDir(resourcesDir() + "/localization").entryList({ "*.xml" }, QDir::Files, QDir::Name);
	files.removeIf([](const QString& n) { return n.startsWith("english"); });
	files.prepend(QString());                                                        // the built-in English texts
	for (const QString& file : files) {
		QString label = file.isEmpty() ? QString("English") : QFileInfo(file).completeBaseName(); label[0] = label[0].toUpper();
		QAction* a = m->addAction(label, this, [this, file] {
			QSettings(q(_h.profileDir()) + "/macos-ui.ini", QSettings::IniFormat).setValue("localization", file); applyLocalization(file); });
		a->setCheckable(true); a->setChecked(file == cur); group->addAction(a);
	}
	applyLocalization(cur);
}
