// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QDockWidget>
#include <QPrintDialog>
#include <QPrinter>
#include <QProgressDialog>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStyleHints>
#include <QStyleFactory>
#include <QToolButton>
#include <QStatusBar>
#include <QTabBar>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <map>
#include "ScintillaEdit.h"
#include "chrome.h"
#include "column_editor.h"
#include "find_dialog.h"
#include "extras.h"
#include "find_in_files.h"
#include "keymap.h"
#include "macro_ui.h"
#include "preferences_dialog.h"
#include "print_doc.h"
#include "shortcut_mapper.h"
#include "style_dialog.h"
#include "tools.h"
#include "main_window.h"
#include "npp_host.h"
#include "panels.h"
#include "qt_bridge.h"
#include "menuCmdID.h" // IDM_* command ids (plain macros) used by shortcuts.xml

static QSettings uiSettings(NppHost& h);
static QString q(const std::wstring& w) { return QString::fromStdWString(w); }
static std::wstring w(const QString& s) { return s.toStdWString(); }
static Buffer* asBuffer(void* p) { return static_cast<Buffer*>(p); }

MainWindow::MainWindow(NppHost& host) : _h(host) {
	_funcTimer = new QTimer(this); _funcTimer->setSingleShot(true); _funcTimer->setInterval(700);   // re-parse shortly after typing stops
	connect(_funcTimer, &QTimer::timeout, this, [this] { refreshFunctionList(); });
	_funcPanel = new FunctionListPanel(this); addDockWidget(Qt::RightDockWidgetArea, _funcPanel); _funcPanel->hide();
	_wsPanel = new WorkspacePanel(this); addDockWidget(Qt::LeftDockWidgetArea, _wsPanel); _wsPanel->hide();
	_docPanel = new DocListPanel(this); addDockWidget(Qt::LeftDockWidgetArea, _docPanel); _docPanel->hide();
	tabifyDockWidget(_wsPanel, _docPanel);
	_clipPanel = new ClipboardPanel(this); addDockWidget(Qt::LeftDockWidgetArea, _clipPanel); _clipPanel->hide();
	_ansiPanel = new AnsiPanel(this); addDockWidget(Qt::LeftDockWidgetArea, _ansiPanel); _ansiPanel->hide();
	tabifyDockWidget(_docPanel, _clipPanel); tabifyDockWidget(_clipPanel, _ansiPanel);
	_mapPanel = new DocMapPanel(this); addDockWidget(Qt::RightDockWidgetArea, _mapPanel); _mapPanel->hide();
	connect(_mapPanel, &QDockWidget::visibilityChanged, this, [this](bool v) { if (v) _mapPanel->follow(activeEditor()); });
	connect(_mapPanel, &DocMapPanel::scrollToLine, this, [this](int line) {
		if (ScintillaEdit* ed = activeEditor()) { long vis = ed->send(SCI_VISIBLEFROMDOCLINE, line), shown = ed->send(SCI_LINESONSCREEN);
			ed->send(SCI_SETFIRSTVISIBLELINE, vis > shown / 2 ? vis - shown / 2 : 0); } });
	auto insertText = [this](const QString& t) { if (ScintillaEdit* ed = activeEditor()) { QByteArray u = t.toUtf8(); ed->send(SCI_REPLACESEL, 0, (sptr_t)u.constData()); static_cast<QWidget*>(ed)->setFocus(); } };
	connect(_clipPanel, &ClipboardPanel::paste, this, insertText);
	connect(_ansiPanel, &AnsiPanel::insertChar, this, insertText);
	_monTimer = new QTimer(this); _monTimer->setInterval(1000);
	connect(_monTimer, &QTimer::timeout, this, [this] { pollMonitored(); });
	connect(_funcPanel, &QDockWidget::visibilityChanged, this, [this](bool v) { if (v) refreshFunctionList(); });
	connect(_funcPanel, &FunctionListPanel::jump, this, [this](long long pos) {
		if (ScintillaEdit* ed = activeEditor()) { ed->send(SCI_GOTOPOS, pos); ed->send(SCI_SCROLLCARET); static_cast<QWidget*>(ed)->setFocus(); } });
	connect(_wsPanel, &WorkspacePanel::openFile, this, [this](const QString& p) { openPath(p); });
	connect(_docPanel, &DocListPanel::activated, this, [this](int v, int i) { activateDoc(v, i); });

	_split = new QSplitter(Qt::Horizontal);
	for (int i = 0; i < 2; ++i) {
		Pane& p = _p[i];
		p.root = new QWidget;
		p.bar = new QTabBar; p.bar->setTabsClosable(false);   // custom per-tab close buttons (see addDoc) p.bar->setMovable(true); p.bar->setExpanding(false);
		p.bar->setStyle(QStyleFactory::create("Fusion"));   // the native macOS style paints the scroll-button area itself (black in dark mode); Fusion leaves it to the style sheet
		p.bar->setDocumentMode(true); p.bar->setUsesScrollButtons(true); p.bar->setElideMode(Qt::ElideNone);
		p.bar->setAttribute(Qt::WA_StyledBackground, true);   // native macOS document-mode tab bars paint their empty area from the system scheme (black in dark mode) unless the QSS background is honoured
		p.bar->setMinimumHeight(32); // QTabBar can be laid out at 0px height before its first tab exists
		p.ed = scintillaFromHwnd(_h.hwnd(i));
		auto* lay = new QVBoxLayout(p.root); lay->setContentsMargins(0, 0, 0, 0); lay->setSpacing(0);
		auto* strip = new QWidget; strip->setObjectName("tabStrip"); strip->setAttribute(Qt::WA_StyledBackground, true);   // the native tab bar paints its empty area itself (black in dark mode); the strip owns the background
		auto* sl = new QHBoxLayout(strip); sl->setContentsMargins(0, 0, 0, 0); sl->setSpacing(0); sl->addWidget(p.bar); sl->addStretch(1);
		p.strip = strip;
		lay->addWidget(strip); lay->addWidget(p.ed);
		_split->addWidget(p.root);

		connect(p.bar, &QTabBar::currentChanged, this, [this, i](int idx) { activate(i, idx); });
		connect(p.bar, &QTabBar::tabCloseRequested, this, [this, i](int idx) { closeTab(i, idx); });
		connect(p.bar, &QTabBar::tabMoved, this, [this, i](int from, int to) { std::swap(_p[i].docs[from], _p[i].docs[to]); });
		connect(p.ed, &ScintillaEditBase::savePointChanged, this, [this, i](bool dirty) { if (Buffer* b = current(i)) _h.setDirty(b, dirty); });
		connect(p.ed, &ScintillaEditBase::macroRecord, this, [this](Scintilla::Message m, Scintilla::uptr_t w, Scintilla::sptr_t l) { _h.macroRecord(static_cast<int>(m), w, l); });
		connect(p.ed, &ScintillaEditBase::verticalScrolled, this, [this, i](int) { syncScroll(i, true); });
		connect(p.ed, &ScintillaEditBase::horizontalScrolled, this, [this, i](int) { syncScroll(i, false); });
		connect(p.ed, &ScintillaEditBase::zoom, this, [this, i](int) { _h.updateLineNumberWidth(i); });
		connect(p.ed, &ScintillaEditBase::notifyChange, this, [this] { _funcTimer->start(); });
		connect(p.ed, &ScintillaEditBase::marginClicked, this, [this, i](Scintilla::Position pos, Scintilla::KeyMod, int margin) {
			ScintillaEdit* ed = _p[i].ed; long line = ed->send(SCI_LINEFROMPOSITION, pos);
			if (margin == 1) { setActivePane(i); ed->send(SCI_GOTOLINE, line); bookmark(0); }       // symbol margin: toggle a bookmark
			else if (margin == 3) ed->send(SCI_TOGGLEFOLD, line); });                                // fold margin
		connect(p.ed, &ScintillaEditBase::updateUi, this, [this](Scintilla::Update) { updateStatus(); });
		connect(p.ed, &ScintillaEditBase::focusChanged, this, [this, i](bool f) { if (f) setActivePane(i); });
		connect(p.ed, &ScintillaEditBase::uriDropped, this, [this](const QString& u) { openPath(QUrl(u).isLocalFile() ? QUrl(u).toLocalFile() : u); });
	}
	_p[1].root->hide();
	auto* central = new QWidget; auto* cl = new QVBoxLayout(central); cl->setContentsMargins(0, 0, 0, 0); cl->setSpacing(0);
	_inc = new IncSearchBar([this] { return activeEditor(); }, central);
	cl->addWidget(_split); cl->addWidget(_inc);
	setCentralWidget(central);

	_lblLang = new QLabel; _lblLen = new QLabel; _lblPos = new QLabel; _lblEol = new QLabel; _lblEnc = new QLabel; _lblIns = new QLabel;
	statusBar()->setSizeGripEnabled(false);
	statusBar()->addWidget(_lblLang);                 // Notepad++ order: doc type | length/lines | Ln/Col/Pos | EOL | encoding | INS/OVR
	statusBar()->addWidget(_lblLen, 1);
	for (QLabel* l : {_lblPos, _lblEol, _lblEnc, _lblIns}) statusBar()->addPermanentWidget(l);
	for (QLabel* l : {_lblLen, _lblPos, _lblEol, _lblEnc, _lblIns}) l->setProperty("sep", true);   // vertical separator on the left of each field
	_lblIns->setMinimumWidth(48);

	_find = new FindDialog([this] { return activeEditor(); }, this);
	buildMenus();
	buildToolbar();
	applyShortcuts();
	applyChromeTheme();
	setAcceptDrops(false);
	qApp->installEventFilter(this); // QFileOpenEvent: open files from Finder / `open -a`

	_h.onBufferChanged = [this](Buffer* b, int) { refreshTab(b); if (b == currentActive()) { updateStatus(); _funcTimer->start(); } };
	resize(1100, 750);
	{ QSettings ui = uiSettings(_h); restoreGeometry(ui.value("geometry").toByteArray()); restoreState(ui.value("windowState").toByteArray()); }
}

// ------------------------------------------------------------------------------------------------ documents / tabs
Buffer* MainWindow::current(int which) const {
	int i = _p[which].bar->currentIndex();
	return i >= 0 && i < (int)_p[which].docs.size() ? _p[which].docs[i] : nullptr;
}

int MainWindow::paneOf(Buffer* b, int preferred) const {
	for (int k : {preferred, 1 - preferred})
		for (Buffer* d : _p[k].docs) if (d == b) return k;
	return -1;
}

void MainWindow::addDoc(int which, Buffer* b) {
	Pane& p = _p[which];
	const bool subAppears = which == 1 && p.root->isHidden();
	p.root->show();
	p.docs.push_back(b);
	int idx = p.bar->addTab(QString());
	auto* btn = new QToolButton; btn->setAutoRaise(true); btn->setIcon(_closeIc); btn->setIconSize(QSize(14, 14)); btn->setToolTip(tr("Close"));
	connect(btn, &QToolButton::clicked, this, [this, which, btn] {
		QTimer::singleShot(0, this, [this, which, btn] {   // after the click handler returns: removing the tab deletes the button
			for (int i = 0; i < _p[which].bar->count(); ++i) if (_p[which].bar->tabButton(i, QTabBar::RightSide) == btn) { closeTab(which, i); return; } }); });
	p.bar->setTabButton(idx, QTabBar::RightSide, btn);
	p.root->layout()->invalidate(); // the bar's height depends on its tabs
	refreshTab(b);
	p.bar->setCurrentIndex(idx);
	activate(which, idx);
	if (subAppears) resetSyncOffsets();
}

void MainWindow::activate(int which, int idx) {
	Pane& p = _p[which];
	if (idx < 0 || idx >= (int)p.docs.size()) return;
	_h.setActiveView(which);
	_active = which;
	_h.show(which, p.docs[idx]);
	updateStatus();
	refreshFunctionList();
	refreshDocList();
}

void MainWindow::setActivePane(int which) {
	if (_active == which) return;
	_active = which;
	_h.setActiveView(which);
	updateStatus();
}

static QString titleFor(const NppHost::DocInfo& d) { return QString("%1%2 - PadPlus").arg(d.dirty ? "*" : "", QString::fromStdWString(d.path.empty() ? d.name : d.path)); }

void MainWindow::refreshTab(Buffer* b) {
	NppHost::DocInfo d = _h.info(b);
	QString title = q(d.name) + (d.readOnly ? " [RO]" : "");
	for (int k = 0; k < 2; ++k)
		for (size_t i = 0; i < _p[k].docs.size(); ++i)
			if (_p[k].docs[i] == b) {
				_p[k].bar->setTabText((int)i, title);
				_p[k].bar->setTabToolTip((int)i, q(d.path));
				_p[k].bar->setTabIcon((int)i, d.dirty ? _pencil : QIcon());   // pencil = modified, like Notepad++'s tab icons
			}
	if (b == currentActive()) setWindowTitle(titleFor(d));
	refreshDocList();
}

void MainWindow::newDoc() { addDoc(_active, _h.newDocument()); }

void MainWindow::openPath(const QString& path, int which) {
	QString abs = QFileInfo(path).absoluteFilePath();
	if (which < 0) which = _active;
	for (int k : {which, 1 - which})
		for (size_t i = 0; i < _p[k].docs.size(); ++i)
			if (q(_h.info(_p[k].docs[i]).path) == abs) { _p[k].bar->setCurrentIndex((int)i); return; } // already open: switch to it
	Buffer* b = _h.open(w(abs));
	if (!b) { QMessageBox::warning(this, tr("Open"), tr("Cannot open %1").arg(abs)); return; }
	// replace a pristine "new 1" document, like Notepad++ does
	Pane& p = _p[which];
	if (p.docs.size() == 1) {
		NppHost::DocInfo d = _h.info(p.docs[0]);
		if (d.untitled && !d.dirty && _p[1 - which].docs.empty()) { Buffer* old = p.docs[0]; addDoc(which, b); closeTab(which, 0); (void)old; addRecent(abs); return; }
	}
	addDoc(which, b);
	addRecent(abs);
}

bool MainWindow::maybeSave(int which, Buffer* b) {
	if (paneOf(b, 1 - which) >= 0 && std::count(_p[1 - which].docs.begin(), _p[1 - which].docs.end(), b)) return true; // still open elsewhere
	NppHost::DocInfo d = _h.info(b);
	if (!d.dirty) return true;
	auto r = QMessageBox::question(this, tr("Save changes?"), tr("Save changes to \"%1\" before closing?").arg(q(d.name)),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
	if (r == QMessageBox::Cancel) return false;
	return r == QMessageBox::Discard || save(b);
}

bool MainWindow::closeTab(int which, int idx) {
	Pane& p = _p[which];
	if (idx < 0 || idx >= (int)p.docs.size()) return true;
	Buffer* b = p.docs[idx];
	if (!maybeSave(which, b)) return false;
	p.docs.erase(p.docs.begin() + idx);
	p.bar->blockSignals(true); p.bar->removeTab(idx); p.bar->blockSignals(false);
	_h.close(which, b);
	if (p.docs.empty()) {
		if (which == 1) { p.root->hide(); setActivePane(0); }
		else if (_p[1].docs.empty()) { newDoc(); }       // keep one document open, like Notepad++
		else { _p[0].root->hide(); setActivePane(1); }
	} else activate(which, p.bar->currentIndex());
	updateStatus();
	refreshDocList();
	return true;
}

bool MainWindow::save(Buffer* b) {
	NppHost::DocInfo d = _h.info(b);
	if (d.untitled) return saveAs(b);
	if (!_h.save(b)) { QMessageBox::warning(this, tr("Save"), tr("Cannot save %1").arg(q(d.path))); return false; }
	_funcTimer->start();
	return true;
}

bool MainWindow::saveAs(Buffer* b) {
	NppHost::DocInfo d = _h.info(b);
	QString f = QFileDialog::getSaveFileName(this, tr("Save As"), q(d.path.empty() ? d.name : d.path));
	if (f.isEmpty()) return false;
	if (!_h.save(b, w(f))) { QMessageBox::warning(this, tr("Save As"), tr("Cannot save %1").arg(f)); return false; }
	addRecent(f);
	return true;
}

void MainWindow::moveOrClone(bool move) {
	Buffer* b = currentActive();
	if (!b) return;
	int to = 1 - _active, from = _active;
	if (paneOf(b, to) >= 0 && std::count(_p[to].docs.begin(), _p[to].docs.end(), b)) return; // already there
	addDoc(to, b);
	if (move) { int i = (int)(std::find(_p[from].docs.begin(), _p[from].docs.end(), b) - _p[from].docs.begin()); closeTab(from, i); }
}

// ------------------------------------------------------------------------------------------------ status / misc
QStringList MainWindow::statusFields() const {
	return { _lblLang->text(), _lblLen->text(), _lblPos->text(), _lblEol->text(), _lblEnc->text(), _lblIns->text() };
}

void MainWindow::updateStatus() {
	ScintillaEdit* ed = activeEditor();
	Buffer* b = currentActive();
	if (!ed || !b) return;
	long bg = (long)ed->send(SCI_STYLEGETBACK, STYLE_DEFAULT);
	if (bg != _chromeBg) { _chromeBg = bg; applyChromeTheme(); }              // theme changed: re-colour the window chrome
	_mapPanel->follow(ed);
	NppHost::DocInfo d = _h.info(b);
	long long pos = ed->send(SCI_GETCURRENTPOS), selS = ed->send(SCI_GETSELECTIONSTART), selE = ed->send(SCI_GETSELECTIONEND);
	_lblLang->setText(q(d.language));
	_lblLen->setText(tr("length : %1   lines : %2").arg(ed->send(SCI_GETLENGTH)).arg(ed->send(SCI_GETLINECOUNT)));
	QString p = tr("Ln : %1   Col : %2").arg(ed->send(SCI_LINEFROMPOSITION, pos) + 1).arg(ed->send(SCI_GETCOLUMN, pos) + 1);
	if (selS != selE) p += tr("   Sel : %1 | %2").arg(ed->send(SCI_COUNTCHARACTERS, selS, selE)).arg(ed->send(SCI_LINEFROMPOSITION, selE) - ed->send(SCI_LINEFROMPOSITION, selS) + 1);
	_lblPos->setText(p + tr("   Pos : %1").arg(pos));
	static const char* eol[] = { "Windows (CR LF)", "Old Mac (CR)", "Unix (LF)" };
	_lblEol->setText(d.eol >= 0 && d.eol < 3 ? eol[d.eol] : "?");
	static const std::map<int, const char*> enc = { { 0, "ANSI" }, { 1, "UTF-8-BOM" }, { 2, "UTF-16 BE BOM" }, { 3, "UTF-16 LE BOM" },
		{ 4, "UTF-8" }, { 5, "UTF-16 BE" }, { 6, "UTF-16 LE" } };
	auto it = enc.find(d.unicodeMode);
	_lblEnc->setText(it == enc.end() ? "?" : it->second);
	_lblIns->setText(ed->send(SCI_GETOVERTYPE) ? "OVR" : "INS");
	setWindowTitle(titleFor(d));
	syncActionStates();
}

// Dark theme -> dark colour scheme for the whole app (native menus, dialogs, scrollbars) plus the dark tab/status look; light theme follows the system.
void MainWindow::applyChromeTheme() {
	ScintillaEdit* ed = _p[0].ed;
	_dark = isDarkColor((long)ed->send(SCI_STYLEGETBACK, STYLE_DEFAULT));
	ChromePalette pal = chromePalette(_dark);
	QGuiApplication::styleHints()->setColorScheme(_dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Unknown);
	setStyleSheet(chromeStyleSheet(pal));
	_pencil = pencilIcon(pal.text); _closeIc = closeIcon(pal.dim);
	reloadToolbarIcons();
	for (int k = 0; k < 2; ++k) {
		QTabBar* bar = _p[k].bar;
		bar->setIconSize(QSize(14, 14));
		for (int i = 0; i < bar->count(); ++i) {
			if (auto* btn = qobject_cast<QToolButton*>(bar->tabButton(i, QTabBar::RightSide))) btn->setIcon(_closeIc);
			if (!bar->tabIcon(i).isNull()) bar->setTabIcon(i, _pencil);
		}
	}
}

void MainWindow::sendActive(unsigned msg, unsigned long long wp, long long lp) { if (activeEditor()) activeEditor()->send(msg, wp, lp); }

void MainWindow::gotoLine() {
	ScintillaEdit* ed = activeEditor();
	if (!ed) return;
	int max = (int)ed->send(SCI_GETLINECOUNT);
	bool ok = false;
	int line = QInputDialog::getInt(this, tr("Go To Line"), tr("Line (1-%1):").arg(max), (int)ed->send(SCI_LINEFROMPOSITION, ed->send(SCI_GETCURRENTPOS)) + 1, 1, max, 1, &ok);
	if (ok) { ed->send(SCI_GOTOLINE, line - 1); ed->send(SCI_SCROLLCARET); static_cast<QWidget*>(ed)->setFocus(); }
}

// UI state (recent files, window layout) lives in the profile directory next to config.xml, never in ~/Library/Preferences.
static QSettings uiSettings(NppHost& h) { return QSettings(QString::fromStdWString(h.profileDir()) + "/macos-ui.ini", QSettings::IniFormat); }

void MainWindow::addRecent(const QString& path) {
	QSettings s = uiSettings(_h);
	QStringList l = s.value("recent").toStringList();
	l.removeAll(path); l.prepend(path);
	while (l.size() > 15) l.removeLast();
	s.setValue("recent", l);
	rebuildRecent();
}

void MainWindow::rebuildRecent() {
	_recent->clear();
	QStringList l = uiSettings(_h).value("recent").toStringList();
	for (const QString& f : l) _recent->addAction(f, this, [this, f] { openPath(f); });
	_recent->setEnabled(!l.isEmpty());
}

// ------------------------------------------------------------------------------------------------ menus
void MainWindow::buildMenus() {
	auto act = [this](QMenu* m, const QString& t, auto fn, const QKeySequence& k = QKeySequence(), int idm = 0) {
		QAction* a = m->addAction(t, this, [this, fn, idm] { const int n = _h.macroLength(); fn(); recordCommand(idm, n); });
		if (!k.isEmpty()) a->setShortcut(k); if (idm) _byCmd[idm] = a; return a; };
	_h.onMenuCommand = [this](int idm) { if (QAction* a = _byCmd.value(idm)) a->trigger(); };   // macro playback of a recorded menu command

	QMenu* f = menuBar()->addMenu(tr("&File")); f->setObjectName("file");
	act(f, tr("New"), [this] { newDoc(); }, QKeySequence::New, IDM_FILE_NEW);
	act(f, tr("Open..."), [this] { for (const QString& p : QFileDialog::getOpenFileNames(this, tr("Open"))) openPath(p); }, QKeySequence::Open, IDM_FILE_OPEN);
	_recent = f->addMenu(tr("Open Recent")); _recent->setObjectName("file-recentFiles");
	act(f, tr("Open Folder as Workspace..."), [this] { QString d = QFileDialog::getExistingDirectory(this, tr("Open Folder as Workspace")); if (!d.isEmpty()) addWorkspace(d); });
	f->addSeparator();
	act(f, tr("Save"), [this] { if (Buffer* b = currentActive()) save(b); }, QKeySequence::Save, IDM_FILE_SAVE);
	act(f, tr("Save As..."), [this] { if (Buffer* b = currentActive()) saveAs(b); }, QKeySequence::SaveAs, IDM_FILE_SAVEAS);
	act(f, tr("Save All"), [this] { for (auto& p : _p) for (Buffer* b : p.docs) if (_h.info(b).dirty) save(b); }, QKeySequence(), IDM_FILE_SAVEALL);
	f->addSeparator();
	act(f, tr("Close"), [this] { closeTab(_active, _p[_active].bar->currentIndex()); }, QKeySequence::Close, IDM_FILE_CLOSE);
	f->addSeparator();
	act(f, tr("Print..."), [this] {
		ScintillaEdit* ed = activeEditor();
		if (!ed) return;
		QPrinter pr(QPrinter::HighResolution);
		QPrintDialog dlg(&pr, this);
		if (dlg.exec() == QDialog::Accepted) printDocument(ed, &pr); }, QKeySequence::Print, IDM_FILE_PRINT);
	f->addSeparator();
	act(f, tr("Close All"), [this] { closeAll(); }, QKeySequence(), IDM_FILE_CLOSEALL);
	act(f, tr("Quit"), [this] { close(); }, QKeySequence::Quit, IDM_FILE_EXIT);
	rebuildRecent();

	QMenu* e = menuBar()->addMenu(tr("&Edit")); e->setObjectName("edit");
	act(e, tr("Undo"), [this] { sendActive(SCI_UNDO); }, QKeySequence::Undo, IDM_EDIT_UNDO);
	act(e, tr("Redo"), [this] { sendActive(SCI_REDO); }, QKeySequence::Redo, IDM_EDIT_REDO);
	e->addSeparator();
	act(e, tr("Cut"), [this] { sendActive(SCI_CUT); }, QKeySequence::Cut, IDM_EDIT_CUT);
	act(e, tr("Copy"), [this] { sendActive(SCI_COPY); }, QKeySequence::Copy, IDM_EDIT_COPY);
	act(e, tr("Paste"), [this] { sendActive(SCI_PASTE); }, QKeySequence::Paste, IDM_EDIT_PASTE);
	act(e, tr("Select All"), [this] { sendActive(SCI_SELECTALL); }, QKeySequence::SelectAll, IDM_EDIT_SELECTALL);
	e->addSeparator();
	act(e, tr("Duplicate Line"), [this] { sendActive(SCI_SELECTIONDUPLICATE); }, QKeySequence(Qt::CTRL | Qt::Key_D), IDM_EDIT_DUP_LINE);
	act(e, tr("Move Line Up"), [this] { sendActive(SCI_MOVESELECTEDLINESUP); }, QKeySequence(), IDM_EDIT_LINE_UP);
	act(e, tr("Move Line Down"), [this] { sendActive(SCI_MOVESELECTEDLINESDOWN); }, QKeySequence(), IDM_EDIT_LINE_DOWN);
	act(e, tr("Join Lines"), [this] { sendActive(SCI_TARGETFROMSELECTION); sendActive(SCI_LINESJOIN); }, QKeySequence(), IDM_EDIT_JOIN_LINES);
	act(e, tr("UPPERCASE"), [this] { sendActive(SCI_UPPERCASE); }, QKeySequence(), IDM_EDIT_UPPERCASE);
	act(e, tr("lowercase"), [this] { sendActive(SCI_LOWERCASE); }, QKeySequence(), IDM_EDIT_LOWERCASE);
	act(e, tr("Column Editor..."), [this] { if (!_col) _col = new ColumnEditorDialog([this] { return activeEditor(); }, this); _col->show(); _col->raise(); }, QKeySequence(), IDM_EDIT_COLUMNMODETIP);
	e->addSeparator();
	QMenu* clip = e->addMenu(tr("Copy to Clipboard")); clip->setObjectName("edit-copyToClipboard");
	auto toClip = [this](auto pick) { if (Buffer* b = currentActive()) QGuiApplication::clipboard()->setText(pick(QFileInfo(q(_h.info(b).path.empty() ? _h.info(b).name : _h.info(b).path)))); };
	act(clip, tr("Copy Current Full File Path"), [toClip] { toClip([](const QFileInfo& f) { return f.filePath(); }); }, QKeySequence(), IDM_EDIT_FULLPATHTOCLIP);
	act(clip, tr("Copy Current Filename"), [toClip] { toClip([](const QFileInfo& f) { return f.fileName(); }); }, QKeySequence(), IDM_EDIT_FILENAMETOCLIP);
	act(clip, tr("Copy Current File Directory"), [toClip] { toClip([](const QFileInfo& f) { return f.absolutePath(); }); }, QKeySequence(), IDM_EDIT_CURRENTDIRTOCLIP);
	QMenu* eol = e->addMenu(tr("EOL Conversion")); eol->setObjectName("edit-eolConversion");
	act(eol, tr("Windows (CR LF)"), [this] { if (Buffer* b = currentActive()) _h.setEol(b, 0); }, QKeySequence(), IDM_FORMAT_TODOS);
	act(eol, tr("Unix (LF)"), [this] { if (Buffer* b = currentActive()) _h.setEol(b, 2); }, QKeySequence(), IDM_FORMAT_TOUNIX);
	act(eol, tr("Old Mac (CR)"), [this] { if (Buffer* b = currentActive()) _h.setEol(b, 1); }, QKeySequence(), IDM_FORMAT_TOMAC);

	QMenu* s = menuBar()->addMenu(tr("&Search")); s->setObjectName("search");
	act(s, tr("Find / Replace..."), [this] {
		ScintillaEdit* ed = activeEditor();
		QString seed = ed && ed->send(SCI_GETSELECTIONSTART) != ed->send(SCI_GETSELECTIONEND) ? QString::fromUtf8(ed->get_text_range(ed->send(SCI_GETSELECTIONSTART), ed->send(SCI_GETSELECTIONEND))) : QString();
		_find->showFind(seed); }, QKeySequence::Find, IDM_SEARCH_FIND);
	act(s, tr("Find Next"), [this] { _find->findNext(); }, QKeySequence::FindNext, IDM_SEARCH_FINDNEXT);
	act(s, tr("Find Previous"), [this] { _find->findPrev(); }, QKeySequence::FindPrevious, IDM_SEARCH_FINDPREV);
	act(s, tr("Replace..."), [this] { _find->showFind(QString()); }, QKeySequence(), IDM_SEARCH_REPLACE);
	act(s, tr("Find in Files..."), [this] {
		if (!_fif) { _fif = new FindInFilesDialog(this); connect(_fif, &FindInFilesDialog::searchRequested, this, [this](const FifOptions& o) { runFindInFiles(o); }); }
		QString dir = currentActive() ? QFileInfo(q(_h.info(currentActive()).path)).absolutePath() : QDir::homePath();
		ScintillaEdit* ed = activeEditor();
		QString seed = ed && ed->send(SCI_GETSELECTIONSTART) != ed->send(SCI_GETSELECTIONEND) ? QString::fromUtf8(ed->get_text_range(ed->send(SCI_GETSELECTIONSTART), ed->send(SCI_GETSELECTIONEND))) : QString();
		_fif->showDialog(seed, dir); }, QKeySequence(), IDM_SEARCH_FINDINFILES);
	act(s, tr("Go To Line..."), [this] { gotoLine(); }, QKeySequence(Qt::CTRL | Qt::Key_L), IDM_SEARCH_GOTOLINE);
	buildSearchExtras(s);

	QMenu* v = menuBar()->addMenu(tr("&View")); v->setObjectName("view");
	QAction* wrap = act(v, tr("Word Wrap"), [this] { if (auto* ed = activeEditor()) ed->send(SCI_SETWRAPMODE, ed->send(SCI_GETWRAPMODE) ? SC_WRAP_NONE : SC_WRAP_WORD); }, QKeySequence(), IDM_VIEW_WRAP);
	wrap->setCheckable(true);
	act(v, tr("Show Whitespace and Tabs"), [this] { if (auto* ed = activeEditor()) ed->send(SCI_SETVIEWWS, ed->send(SCI_GETVIEWWS) ? SCWS_INVISIBLE : SCWS_VISIBLEALWAYS); }, QKeySequence(), IDM_VIEW_TAB_SPACE)->setCheckable(true);
	act(v, tr("Show End of Line"), [this] { if (auto* ed = activeEditor()) ed->send(SCI_SETVIEWEOL, !ed->send(SCI_GETVIEWEOL)); }, QKeySequence(), IDM_VIEW_EOL)->setCheckable(true);
	act(v, tr("Show All Characters"), [this] { if (auto* ed = activeEditor()) { bool on = !ed->send(SCI_GETVIEWWS) && !ed->send(SCI_GETVIEWEOL); ed->send(SCI_SETVIEWWS, on ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE); ed->send(SCI_SETVIEWEOL, on); } }, QKeySequence(), IDM_VIEW_ALL_CHARACTERS)->setCheckable(true);
	act(v, tr("Show Indent Guide"), [this] { auto p = _h.prefs(); p.indentGuides = !p.indentGuides; _h.setPrefs(p); _h.savePrefs(); }, QKeySequence(), IDM_VIEW_INDENT_GUIDE)->setCheckable(true);
	v->addSeparator();
	act(v, tr("Zoom In"), [this] { sendActive(SCI_ZOOMIN); }, QKeySequence::ZoomIn, IDM_VIEW_ZOOMIN);
	act(v, tr("Zoom Out"), [this] { sendActive(SCI_ZOOMOUT); }, QKeySequence::ZoomOut, IDM_VIEW_ZOOMOUT);
	act(v, tr("Restore Default Zoom"), [this] { sendActive(SCI_SETZOOM, NppHost::kBaseZoom); }, QKeySequence(), IDM_VIEW_ZOOMRESTORE);
	v->addSeparator();
	act(v, tr("Synchronize Vertical Scrolling"), [this] { toggleSync(true); }, QKeySequence(), IDM_VIEW_SYNSCROLLV)->setCheckable(true);
	act(v, tr("Synchronize Horizontal Scrolling"), [this] { toggleSync(false); }, QKeySequence(), IDM_VIEW_SYNSCROLLH)->setCheckable(true);
	v->addSeparator();
	act(v, tr("Move to Other View"), [this] { moveOrClone(true); }, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Right), IDM_VIEW_GOTO_ANOTHER_VIEW);
	act(v, tr("Clone to Other View"), [this] { moveOrClone(false); }, QKeySequence(), IDM_VIEW_CLONE_TO_ANOTHER_VIEW);

	QMenu* panels = v->addMenu(tr("Panels"));
	panels->addAction(_funcPanel->toggleViewAction()); _byCmd[IDM_VIEW_FUNC_LIST] = _funcPanel->toggleViewAction();
	panels->addAction(_wsPanel->toggleViewAction()); _byCmd[IDM_VIEW_FILEBROWSER] = _wsPanel->toggleViewAction();
	panels->addAction(_docPanel->toggleViewAction());
	buildViewExtras(panels, v);
	QMenu* themeMenu = v->addMenu(tr("Theme"));
	auto* themeGroup = new QActionGroup(this);
	int curTheme = _h.currentTheme(), ti = 0;
	for (const std::wstring& t : _h.themes()) {
		int idx = ti++;
		QAction* a = themeMenu->addAction(q(t), this, [this, idx] { _h.setTheme(idx); _h.savePrefs(); updateStatus(); });
		a->setCheckable(true); a->setChecked(idx == curTheme); themeGroup->addAction(a);
	}

	QMenu* st = menuBar()->addMenu(tr("&Settings")); st->setObjectName("settings");
	act(st, tr("Preferences..."), [this] { PreferencesDialog d(_h, this); d.exec(); updateStatus(); }, QKeySequence(Qt::CTRL | Qt::Key_Comma), IDM_SETTING_PREFERENCE);
	act(st, tr("Style Configurator..."), [this] { StyleDialog d(_h, this); d.exec(); updateStatus(); }, QKeySequence(), IDM_LANGSTYLE_CONFIG_DLG);
	act(st, tr("Shortcut Mapper..."), [this] { ShortcutMapper d(_h, commandNames(), this); connect(&d, &ShortcutMapper::changed, this, [this] { applyShortcuts(); }); d.exec(); }, QKeySequence(), IDM_SETTING_SHORTCUT_MAPPER);

	QMenu* en = menuBar()->addMenu(tr("En&coding")); en->setObjectName("encoding");
	auto* encGroup = new QActionGroup(this);
	struct EncItem { const char* name; int mode; int idm; };
	for (const EncItem& it : { EncItem{ "ANSI", 0, IDM_FORMAT_ANSI }, EncItem{ "UTF-8", 4, IDM_FORMAT_AS_UTF_8 }, EncItem{ "UTF-8-BOM", 1, IDM_FORMAT_UTF_8 },
			EncItem{ "UTF-16 BE BOM", 2, IDM_FORMAT_UTF_16BE }, EncItem{ "UTF-16 LE BOM", 3, IDM_FORMAT_UTF_16LE } }) {
		QAction* a = act(en, tr(it.name), [this, m = it.mode] { if (Buffer* b = currentActive()) _h.setUnicodeMode(b, m); }, QKeySequence(), it.idm);
		a->setCheckable(true); a->setData(it.mode); encGroup->addAction(a);
	}
	connect(en, &QMenu::aboutToShow, this, [this, en] {          // tick the current document's encoding
		if (!currentActive()) return;
		int m = _h.info(currentActive()).unicodeMode;
		for (QAction* a : en->actions()) if (a->data().isValid()) a->setChecked(a->data().toInt() == m); });

	QMenu* l = menuBar()->addMenu(tr("&Language")); l->setObjectName("language");
	std::map<QChar, QMenu*> letters;
	for (const NppHost::Lang& lg : _h.languages()) {
		QString n = q(lg.name);
		QMenu* target = l;
		if (lg.id != 0) { QChar c = n.at(0).toUpper(); if (!letters[c]) letters[c] = l->addMenu(QString(c)); target = letters[c]; }
		int id = lg.id;
		act(target, n, [this, id] { if (Buffer* b = currentActive()) _h.setLanguage(b, id); });
		if (lg.id == 0) l->addSeparator();
	}

	buildLanguageExtras(l);
	menuBar()->removeAction(st->menuAction()); menuBar()->addMenu(st);   // Notepad++ order: ... Encoding, Language, Settings, Tools, ..., Window, ?

	QMenu* tools = menuBar()->addMenu(tr("&Tools")); tools->setObjectName("tools");
	struct HashItem { const char* name; QCryptographicHash::Algorithm algo; int idmText, idmFiles, idmClip; };
	for (const HashItem& h : { HashItem{ "MD5", QCryptographicHash::Md5, IDM_TOOL_MD5_GENERATE, IDM_TOOL_MD5_GENERATEFROMFILE, IDM_TOOL_MD5_GENERATEINTOCLIPBOARD },
			HashItem{ "SHA-1", QCryptographicHash::Sha1, IDM_TOOL_SHA1_GENERATE, IDM_TOOL_SHA1_GENERATEFROMFILE, IDM_TOOL_SHA1_GENERATEINTOCLIPBOARD },
			HashItem{ "SHA-256", QCryptographicHash::Sha256, IDM_TOOL_SHA256_GENERATE, IDM_TOOL_SHA256_GENERATEFROMFILE, IDM_TOOL_SHA256_GENERATEINTOCLIPBOARD },
			HashItem{ "SHA-512", QCryptographicHash::Sha512, IDM_TOOL_SHA512_GENERATE, IDM_TOOL_SHA512_GENERATEFROMFILE, IDM_TOOL_SHA512_GENERATEINTOCLIPBOARD } }) {
		QMenu* m = tools->addMenu(h.name); m->setObjectName(QString("tools-") + QString(h.name).remove('-').toLower());
		auto selection = [this]() -> QByteArray { ScintillaEdit* ed = activeEditor(); if (!ed) return {}; return ed->get_text_range(ed->send(SCI_GETSELECTIONSTART), ed->send(SCI_GETSELECTIONEND)); };
		act(m, tr("Generate..."), [this, h] {
			bool ok = false; QString t = QInputDialog::getMultiLineText(this, tr("%1 - Generate").arg(h.name), tr("Text:"), QString(), &ok);
			if (ok) HashDialog(tr("%1 digest").arg(h.name), hashHex(h.algo, t.toUtf8()), this).exec(); }, QKeySequence(), h.idmText);
		act(m, tr("Generate from files..."), [this, h] {
			QStringList files = QFileDialog::getOpenFileNames(this, tr("%1 - Generate from files").arg(h.name));
			if (files.isEmpty()) return;
			QString out; for (const HashResult& r : hashFiles(h.algo, files)) out += (r.hash.isEmpty() ? tr("(unreadable)") : r.hash) + "  " + r.file + "\n";
			HashDialog(tr("%1 digests").arg(h.name), out, this).exec(); }, QKeySequence(), h.idmFiles);
		act(m, tr("Generate from selection into Clipboard"), [h, selection] { QGuiApplication::clipboard()->setText(hashHex(h.algo, selection())); }, QKeySequence(), h.idmClip);
	}

	_macroMenu = menuBar()->addMenu(tr("&Macro")); _macroMenu->setObjectName("macro");
	_actRecStart = act(_macroMenu, tr("Start Recording"), [this] { macroStart(); }, QKeySequence(), IDM_MACRO_STARTRECORDINGMACRO);
	_actRecStop = act(_macroMenu, tr("Stop Recording"), [this] { macroStop(); }, QKeySequence(), IDM_MACRO_STOPRECORDINGMACRO);
	_actPlay = act(_macroMenu, tr("Playback"), [this] { runMacro(-1, 1, false); }, QKeySequence(), IDM_MACRO_PLAYBACKRECORDEDMACRO);
	_actRunMulti = act(_macroMenu, tr("Run a Macro Multiple Times..."), [this] {
		QStringList names; const bool hasCurrent = _h.macroLength() > 0 && !_h.macroRecording();
		if (hasCurrent) names << tr("Current recorded macro");
		for (const auto& m : _h.savedMacros()) names << q(m.name);
		if (names.isEmpty()) return;
		MacroRunDialog d(names, hasCurrent, this);
		if (d.exec() == QDialog::Accepted) runMacro(d.source(), d.times(), d.untilEnd()); }, QKeySequence(), IDM_MACRO_RUNMULTIMACRODLG);
	_actSaveMacro = act(_macroMenu, tr("Save Current Recorded Macro..."), [this] {
		MacroSaveDialog d(this);
		if (d.exec() == QDialog::Accepted) saveMacro(d.name(), d.key()); }, QKeySequence(), IDM_MACRO_SAVECURRENTMACRO);
	_macroMenu->addSeparator();
	_macroStatic = (int)_macroMenu->actions().size();
	rebuildMacroMenu();

	buildRunMenu();
	_winMenu = menuBar()->addMenu(tr("&Window")); _winMenu->setObjectName("Window");
	act(_winMenu, tr("Next Tab"), [this] { cycleTab(+1); }, QKeySequence(), IDM_VIEW_TAB_NEXT);
	act(_winMenu, tr("Previous Tab"), [this] { cycleTab(-1); }, QKeySequence(), IDM_VIEW_TAB_PREV);
	_winMenu->addSeparator();
	_winStatic = (int)_winMenu->actions().size();
	connect(_winMenu, &QMenu::aboutToShow, this, [this] { rebuildWindowMenu(); });

	QMenu* hm = menuBar()->addMenu("?");
	act(hm, tr("About PadPlus..."), [this] {
		QMessageBox box(this);
		box.setWindowTitle(tr("About PadPlus"));
		QPixmap logo(resourcesDir() + "/PadPlus.png");                      // 256 px, shown at 128 pt
		if (!logo.isNull()) { logo.setDevicePixelRatio(2); box.setIconPixmap(logo); }
		box.setTextFormat(Qt::RichText);
		box.setText(tr("PadPlus %1").arg(QCoreApplication::applicationVersion()));
		box.setInformativeText(tr("A macOS editor based on the Notepad++ source code (with Scintilla, Lexilla, Boost.Regex and Qt).<br>"
			"Not affiliated with or endorsed by the Notepad++ project.<br><br>"
			"Copyright (C) 2026 Cristian Vertiz. Notepad++ code: copyright (C) Don Ho and contributors.<br>"
			"This program comes with ABSOLUTELY NO WARRANTY. It is free software: you may redistribute it under the terms of the "
			"GNU General Public License, version 3 or later. The licence texts and the list of source code locations are in "
			"PadPlus.app/Contents/Resources/licenses."));
		box.exec(); }, QKeySequence(), IDM_ABOUT);
	buildLocalizationMenu(st);
	act(hm, tr("PadPlus Releases"), [] { QDesktopServices::openUrl(QUrl("https://github.com/CristianVertiz/PadPlus/releases")); });   // ponytail: no in-app updater; a real check needs QtNetwork in the bundle
	act(hm, tr("Notepad++ Home Page"), [] { QDesktopServices::openUrl(QUrl("https://notepad-plus-plus.org/")); }, QKeySequence(), IDM_HOMESWEETHOME);
}

void MainWindow::cycleTab(int dir) {
	QTabBar* b = _p[_active].bar;
	int n = b->count();
	if (n > 1) b->setCurrentIndex((b->currentIndex() + dir + n) % n);
}

void MainWindow::rebuildWindowMenu() {            // the open documents of both views, current one ticked
	QList<QAction*> acts = _winMenu->actions();
	for (int i = _winStatic; i < acts.size(); ++i) { _winMenu->removeAction(acts[i]); acts[i]->deleteLater(); }
	int n = 0;
	for (int k = 0; k < 2; ++k)
		for (size_t i = 0; i < _p[k].docs.size(); ++i) {
			QString label = QString("%1  %2").arg(++n).arg(q(_h.info(_p[k].docs[i]).name));
			QAction* a = _winMenu->addAction(label, this, [this, k, i] { activateDoc(k, (int)i); });
			a->setCheckable(true); a->setChecked(k == _active && (int)i == _p[k].bar->currentIndex());
		}
}

void MainWindow::closeAll() {
	while (!_p[1].docs.empty()) if (!closeTab(1, 0)) return;
	for (int n = (int)_p[0].docs.size(); n > 0; --n) if (!closeTab(0, 0)) return;   // the last close opens a fresh untitled document
}

// ------------------------------------------------------------------------------------------------ macros
void MainWindow::macroStart() { _h.macroStart(); statusBar()->showMessage(tr("Recording macro...")); rebuildMacroMenu(); updateMacroActions(); }
void MainWindow::macroStop() { _h.macroStop(); statusBar()->clearMessage(); rebuildMacroMenu(); updateMacroActions(); }

void MainWindow::runMacro(int source, int times, bool untilEnd) {
	if (_h.macroRecording()) return;
	if (source < 0) { if (untilEnd) _h.macroPlayUntilEnd(); else _h.macroPlay(times); }
	else if (untilEnd) _h.macroPlaySavedUntilEnd(source);
	else _h.macroPlaySaved(source, times);
}

bool MainWindow::saveMacro(const QString& name, const QKeySequence& key) {
	bool ctrl = false, alt = false, shift = false; int vk = 0;
	if (!fromQtSequence(key, ctrl, alt, shift, vk)) vk = 0, ctrl = alt = shift = false;   // unusable key: save without a shortcut
	if (!_h.macroSave(w(name), ctrl, alt, shift, vk)) return false;
	rebuildMacroMenu(); updateMacroActions();
	return true;
}

bool MainWindow::deleteMacro(int index) {
	if (!_h.macroDelete(index)) return false;
	rebuildMacroMenu(); updateMacroActions();
	return true;
}

// Saved macros are listed under the fixed Macro items (with their shortcut); a "Delete Macro" submenu removes them.
void MainWindow::rebuildMacroMenu() {
	QList<QAction*> acts = _macroMenu->actions();
	for (int i = _macroStatic; i < acts.size(); ++i) { _macroMenu->removeAction(acts[i]); acts[i]->deleteLater(); }
	const auto saved = _h.savedMacros();
	const bool rec = _h.macroRecording();
	for (int i = 0; i < (int)saved.size(); ++i) {
		NppHost::ShortcutItem k; k.ctrl = saved[i].ctrl; k.alt = saved[i].alt; k.shift = saved[i].shift; k.vk = saved[i].vk;
		QAction* a = _macroMenu->addAction(q(saved[i].name), this, [this, i] { runMacro(i, 1, false); });
		if (!(k.alt && !k.ctrl)) a->setShortcut(toQtSequence(k));   // Option+letter types special characters on a Mac (Option+S = \u00df): never steal those
		a->setEnabled(!rec);
	}
	if (saved.empty()) return;
	_macroMenu->addSeparator();
	QMenu* del = _macroMenu->addMenu(tr("Delete Macro"));
	for (int i = 0; i < (int)saved.size(); ++i)
		del->addAction(q(saved[i].name), this, [this, i] {
			if (QMessageBox::question(this, tr("Delete Macro"), tr("Delete the macro \"%1\"?").arg(q(_h.savedMacros()[i].name))) == QMessageBox::Yes) deleteMacro(i); });
}

void MainWindow::updateMacroActions() {
	if (!_actRecStart) return;
	const bool rec = _h.macroRecording(), has = _h.macroLength() > 0, saved = _h.macroIsSaved();
	const bool anySaved = !_h.savedMacros().empty();
	const bool canPlay = !rec && has, canRunMulti = canPlay || (!rec && anySaved), canSave = canPlay && !saved;
	auto set = [this](QAction* menuAction, const char* icon, bool on) {
		if (menuAction) menuAction->setEnabled(on);
		if (QAction* t = _tbNamed.value(icon)) t->setEnabled(on); };
	set(_actRecStart, "startrecord", !rec);
	set(_actRecStop, "stoprecord", rec);
	set(_actPlay, "playrecord", canPlay);
	set(_actRunMulti, "playrecord_m", canRunMulti);
	set(_actSaveMacro, "saverecord", canSave);
}

// ------------------------------------------------------------------------------------------------ synchronized scrolling
// While both views are shown, scrolling one scrolls the other by the same amount; the offset between them is taken when it is switched on.
void MainWindow::resetSyncOffsets() {
	_syncOffV = (int)(_p[1].ed->send(SCI_GETFIRSTVISIBLELINE) - _p[0].ed->send(SCI_GETFIRSTVISIBLELINE));
	_syncOffH = (int)(_p[1].ed->send(SCI_GETXOFFSET) - _p[0].ed->send(SCI_GETXOFFSET));
}

void MainWindow::toggleSync(bool vertical) {
	(vertical ? _syncV : _syncH) = !(vertical ? _syncV : _syncH);
	resetSyncOffsets();
}

void MainWindow::syncScroll(int from, bool vertical) {
	if (_syncing || _p[1].root->isHidden() || !(vertical ? _syncV : _syncH)) return;
	const int to = 1 - from;
	_syncing = true;                                        // setting the other view fires its own scroll signal
	if (vertical) {
		const long long top = _p[from].ed->send(SCI_GETFIRSTVISIBLELINE);
		_p[to].ed->send(SCI_SETFIRSTVISIBLELINE, qMax(0LL, top + (from == 0 ? _syncOffV : -_syncOffV)));
	} else {
		const long long x = _p[from].ed->send(SCI_GETXOFFSET);
		_p[to].ed->send(SCI_SETXOFFSET, qMax(0LL, x + (from == 0 ? _syncOffH : -_syncOffH)));
	}
	_syncing = false;
}

// ------------------------------------------------------------------------------------------------ toolbar
// A toolbar button is a proxy for a menu action: same command, but its own icon and enabled state (cut/copy/undo/... grey out like in Notepad++).
QAction* MainWindow::tbProxy(QAction* src, const char* icon) {
	if (!src) return nullptr;
	auto* a = new QAction(src->text().remove("..."), this);
	a->setCheckable(src->isCheckable()); a->setChecked(src->isChecked());
	connect(a, &QAction::triggered, this, [src, a] { src->trigger(); if (src->isCheckable()) a->setChecked(src->isChecked()); });
	connect(src, &QAction::toggled, a, &QAction::setChecked);
	connect(src, &QAction::changed, a, [src, a] { const QString t = src->text().remove("..."); if (a->text() != t) a->setText(t); });   // localization
	_tb.push_back({ a, icon });
	_tbNamed[icon] = a;
	_toolbar->addAction(a);
	return a;
}

void MainWindow::buildToolbar() {
	_toolbar = addToolBar(tr("Toolbar"));
	_toolbar->setObjectName("mainToolbar"); _toolbar->setMovable(false); _toolbar->setFloatable(false);
	_toolbar->setIconSize(QSize(20, 20)); _toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
	auto cmd = [this](int idm, const char* icon) { return tbProxy(_byCmd.value(idm), icon); };
	cmd(IDM_FILE_NEW, "new"); cmd(IDM_FILE_OPEN, "open"); _tbSave = cmd(IDM_FILE_SAVE, "save"); _tbSaveAll = cmd(IDM_FILE_SAVEALL, "saveall");
	cmd(IDM_FILE_CLOSE, "close"); cmd(IDM_FILE_CLOSEALL, "closeall"); cmd(IDM_FILE_PRINT, "print");
	_toolbar->addSeparator();
	_tbCut = cmd(IDM_EDIT_CUT, "cut"); _tbCopy = cmd(IDM_EDIT_COPY, "copy"); _tbPaste = cmd(IDM_EDIT_PASTE, "paste");
	_toolbar->addSeparator();
	_tbUndo = cmd(IDM_EDIT_UNDO, "undo"); _tbRedo = cmd(IDM_EDIT_REDO, "redo");
	_toolbar->addSeparator();
	cmd(IDM_SEARCH_FIND, "find"); cmd(IDM_SEARCH_REPLACE, "findrep");
	_toolbar->addSeparator();
	cmd(IDM_VIEW_ZOOMIN, "zoomIn"); cmd(IDM_VIEW_ZOOMOUT, "zoomOut");
	_toolbar->addSeparator();
	cmd(IDM_VIEW_SYNSCROLLV, "syncV"); cmd(IDM_VIEW_SYNSCROLLH, "syncH");
	_toolbar->addSeparator();
	cmd(IDM_VIEW_WRAP, "wrap"); cmd(IDM_VIEW_ALL_CHARACTERS, "allChars"); cmd(IDM_VIEW_INDENT_GUIDE, "indentGuide");
	_toolbar->addSeparator();
	cmd(IDM_LANG_USER_DLG, "udl"); cmd(IDM_VIEW_DOC_MAP, "docMap");
	tbProxy(_docPanel->toggleViewAction(), "docList"); cmd(IDM_VIEW_FUNC_LIST, "funcList"); cmd(IDM_VIEW_FILEBROWSER, "fileBrowser");
	cmd(IDM_VIEW_MONITORING, "monitoring");
	_toolbar->addSeparator();
	cmd(IDM_MACRO_STARTRECORDINGMACRO, "startrecord"); cmd(IDM_MACRO_STOPRECORDINGMACRO, "stoprecord"); cmd(IDM_MACRO_PLAYBACKRECORDEDMACRO, "playrecord");
	cmd(IDM_MACRO_RUNMULTIMACRODLG, "playrecord_m"); cmd(IDM_MACRO_SAVECURRENTMACRO, "saverecord");
	reloadToolbarIcons();
}

void MainWindow::reloadToolbarIcons() {
	const QString dir = resourcesDir();
	for (const TbItem& t : _tb) t.proxy->setIcon(toolbarIcon(dir, _dark, t.icon));
}

// Enable/check state that Notepad++ keeps live: undo/redo, cut/copy/paste, save, and the view toggles.
void MainWindow::syncActionStates() {
	ScintillaEdit* ed = activeEditor();
	Buffer* b = currentActive();
	if (!ed || !b || !_toolbar) return;
	NppHost::DocInfo d = _h.info(b);
	bool sel = ed->send(SCI_GETSELECTIONSTART) != ed->send(SCI_GETSELECTIONEND), anyDirty = false;
	for (auto& p : _p) for (Buffer* x : p.docs) anyDirty |= _h.info(x).dirty;
	_tbUndo->setEnabled(ed->send(SCI_CANUNDO)); _tbRedo->setEnabled(ed->send(SCI_CANREDO));
	_tbCut->setEnabled(sel && !d.readOnly); _tbCopy->setEnabled(sel); _tbPaste->setEnabled(ed->send(SCI_CANPASTE) && !d.readOnly);
	_tbSave->setEnabled(d.dirty || d.untitled); _tbSaveAll->setEnabled(anyDirty);
	auto check = [this](int idm, bool on) { if (QAction* a = _byCmd.value(idm)) if (a->isChecked() != on) a->setChecked(on); };
	check(IDM_VIEW_WRAP, ed->send(SCI_GETWRAPMODE) != 0);
	check(IDM_VIEW_TAB_SPACE, ed->send(SCI_GETVIEWWS) != 0);
	check(IDM_VIEW_EOL, ed->send(SCI_GETVIEWEOL) != 0);
	check(IDM_VIEW_ALL_CHARACTERS, ed->send(SCI_GETVIEWWS) != 0 && ed->send(SCI_GETVIEWEOL) != 0);
	check(IDM_VIEW_INDENT_GUIDE, _h.prefs().indentGuides);
	check(IDM_VIEW_MONITORING, monitoring());
	const bool twoViews = !_p[1].root->isHidden();               // synchronized scrolling needs both views
	for (const char* n : { "syncV", "syncH" }) if (QAction* a = _tbNamed.value(n)) a->setEnabled(twoViews);
	updateMacroActions();
}

QMap<int, QString> MainWindow::commandNames() const {
	QMap<int, QString> m;
	for (auto it = _byCmd.begin(); it != _byCmd.end(); ++it) m[it.key()] = it.value()->text().remove('&').remove("...");
	return m;
}

void MainWindow::applyShortcuts() {
	QMap<int, QList<QKeySequence>> seqs;                  // a command can have several bindings (e.g. Cut: Shift+Del and Ctrl+X)
	for (const NppHost::ShortcutItem& it : _h.shortcuts()) {
		if (it.cmd == IDM_FILE_EXIT) continue;            // keep Cmd+Q: Alt+F4 is not a Mac shortcut
		QKeySequence k = toQtSequence(it);
		if (_byCmd.contains(it.cmd)) { auto& l = seqs[it.cmd]; if (!k.isEmpty()) l << k; }
	}
	for (auto it = _byCmd.begin(); it != _byCmd.end(); ++it)
		if (it.key() != IDM_FILE_EXIT && _h.shortcuts().size()) it.value()->setShortcuts(seqs.value(it.key())); // empty list = unassigned
	// Start/Stop are never enabled together, so both can share Cmd+Shift+R (Notepad++'s own Ctrl+Shift+R toggle is a Scintilla key binding outside the table)
	for (QAction* a : { _actRecStart, _actRecStop }) if (a && a->shortcuts().isEmpty()) a->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
}

// ------------------------------------------------------------------------------------------------ find in files
void MainWindow::runFindInFiles(const FifOptions& o) {
	if (!_results) {
		_results = new QDockWidget(tr("Search results"), this);
		_resultsTree = new QTreeWidget; _resultsTree->setHeaderHidden(true);
		_results->setWidget(_resultsTree);
		addDockWidget(Qt::BottomDockWidgetArea, _results);
		connect(_resultsTree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* it) {
			if (it->parent()) openHit(it->data(0, Qt::UserRole).toString(), it->data(0, Qt::UserRole + 1).toInt(), it->data(0, Qt::UserRole + 2).toInt(), it->data(0, Qt::UserRole + 3).toInt());
		});
	}
	QProgressDialog prog(tr("Searching..."), tr("Cancel"), 0, 0, this);
	prog.setMinimumDuration(400); prog.setWindowModality(Qt::WindowModal);
	QString err;
	auto files = findInFiles(o, [&prog](const QString& f) { prog.setLabelText(f); QCoreApplication::processEvents(); return !prog.wasCanceled(); }, &err);
	if (!err.isEmpty()) { QMessageBox::warning(this, tr("Find in Files"), tr("Invalid regular expression: %1").arg(err)); return; }
	int hits = 0;
	_resultsTree->clear();
	for (const FifFile& f : files) {
		auto* top = new QTreeWidgetItem(_resultsTree, QStringList(QString("%1  (%2 hit%3)").arg(f.path).arg(f.hits.size()).arg(f.hits.size() == 1 ? "" : "s")));
		for (const FifHit& h : f.hits) {
			auto* c = new QTreeWidgetItem(top, QStringList(QString("Line %1: %2").arg(h.line).arg(h.lineText.trimmed().left(200))));
			c->setData(0, Qt::UserRole, f.path); c->setData(0, Qt::UserRole + 1, h.line); c->setData(0, Qt::UserRole + 2, h.column); c->setData(0, Qt::UserRole + 3, h.length);
			c->setData(0, Qt::UserRole + 4, h.lineText);
			++hits;
		}
	}
	_resultsTree->expandAll();
	_results->setWindowTitle(tr("Search \"%1\" (%2 hits in %3 files)").arg(o.what).arg(hits).arg(files.size()));
	_results->show();
}

void MainWindow::openHit(const QString& path, int line, int column, int length) {
	openPath(path);
	ScintillaEdit* ed = activeEditor();
	if (!ed) return;
	QString text;
	if (_resultsTree) for (QTreeWidgetItemIterator it(_resultsTree); *it; ++it)
		if ((*it)->parent() && (*it)->data(0, Qt::UserRole).toString() == path && (*it)->data(0, Qt::UserRole + 1).toInt() == line && (*it)->data(0, Qt::UserRole + 2).toInt() == column) { text = (*it)->data(0, Qt::UserRole + 4).toString(); break; }
	long long pos = ed->send(SCI_POSITIONFROMLINE, line - 1) + text.left(column).toUtf8().size();
	ed->send(SCI_SETSEL, pos, pos + text.mid(column, length).toUtf8().size());
	ed->send(SCI_SCROLLCARET);
	static_cast<QWidget*>(ed)->setFocus();
}

// ------------------------------------------------------------------------------------------------ panels
void MainWindow::refreshFunctionList() {
	if (_funcPanel->isHidden()) return;
	Buffer* b = currentActive();
	if (!b) { _funcPanel->setEntries(QString(), {}); return; }
	_funcPanel->setEntries(q(_h.info(b).name), _h.functions(_active));
}

void MainWindow::refreshDocList() {
	if (_docPanel->isHidden()) return;
	std::vector<DocListPanel::Doc> docs;
	for (int k = 0; k < 2; ++k)
		for (size_t i = 0; i < _p[k].docs.size(); ++i) {
			NppHost::DocInfo d = _h.info(_p[k].docs[i]);
			docs.push_back({ k, (int)i, q(d.name), q(d.path), d.dirty });
		}
	_docPanel->setDocs(docs, _active, _p[_active].bar->currentIndex());
}

void MainWindow::activateDoc(int which, int index) {
	if (which < 0 || which > 1 || index < 0 || index >= (int)_p[which].docs.size()) return;
	setActivePane(which);
	_p[which].bar->setCurrentIndex(index);
	activate(which, index);
}

void MainWindow::addWorkspace(const QString& dir) { _wsPanel->addRoot(dir); _wsPanel->show(); _wsPanel->raise(); }

// ------------------------------------------------------------------------------------------------ session / lifecycle
void MainWindow::restoreSession() {
	NppHost::SessionData sd = _h.loadSession();
	Buffer* first = _h.takeInitialDocument();      // the main view's own "new 1": the first tab when nothing is restored, like Notepad++
	for (const std::wstring& r : sd.workspaceRoots) if (QDir(q(r)).exists()) addWorkspace(q(r));
	for (int k = 0; k < 2; ++k) {
		for (const auto& f : sd.files[k]) {
			Buffer* b = _h.open(f.path);
			if (!b) continue;
			addDoc(k, b);
			_h.restoreCaret(k, f.caret, f.firstLine);
		}
		if (!_p[k].docs.empty()) {
			int i = sd.activeIndex[k];
			_p[k].bar->setCurrentIndex(i >= 0 && i < (int)_p[k].docs.size() ? i : 0);
		}
	}
	if (_p[0].docs.empty() && _p[1].docs.empty()) { if (first) addDoc(0, first); else newDoc(); }
	else if (first && !_p[0].docs.empty()) _h.close(0, first);   // not needed: drop it, or it would keep the name "new 1" taken without being shown
	else if (_p[sd.activeView].docs.empty()) setActivePane(_p[0].docs.empty() ? 1 : 0);
	else setActivePane(sd.activeView);
	updateStatus();
}

void MainWindow::closeEvent(QCloseEvent* e) {
	for (int k = 0; k < 2; ++k)
		for (Buffer* b : _p[k].docs)
			if (!maybeSave(k, b)) { e->ignore(); return; }
	std::vector<Buffer*> docs[2] = { _p[0].docs, _p[1].docs };
	int idx[2] = { _p[0].bar->currentIndex(), _p[1].bar->currentIndex() };
	std::vector<std::wstring> roots;
	for (const QString& r : _wsPanel->roots()) roots.push_back(w(r));
	_h.saveSession(docs, _active, idx, roots);
	{ QSettings ui = uiSettings(_h); ui.setValue("windowState", saveState()); ui.setValue("geometry", saveGeometry()); }
	e->accept();
}

bool MainWindow::eventFilter(QObject* o, QEvent* ev) {
	if (ev->type() == QEvent::FileOpen) { openPath(static_cast<QFileOpenEvent*>(ev)->file()); return true; }
	if (ev->type() == QEvent::MouseButtonDblClick && static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton)
		for (int i = 0; i < 2; ++i)      // double-click on the empty part of a tab strip: new document in that view (as in Notepad++ and Sublime Text)
			if (o == _p[i].strip || (o == _p[i].bar && _p[i].bar->tabAt(static_cast<QMouseEvent*>(ev)->position().toPoint()) < 0)) { setActivePane(i); newDoc(); return true; }
	return QMainWindow::eventFilter(o, ev);
}

#include <cstdio>
void MainWindow::debugDump() const { // NPP_DEBUG=1: layout facts for headless runs
	int icons = 0, buttons = 0;
	for (QAction* a : _toolbar->actions()) if (!a->isSeparator()) { ++buttons; icons += !a->icon().isNull(); }
	fprintf(stderr, "toolbar icons %d/%d resources=%s\n", icons, buttons, qPrintable(resourcesDir()));
	for (int i = 0; i < 2; ++i) {
		const Pane& p = _p[i];
		fprintf(stderr, "pane %d: visible=%d tabs=%d docs=%zu bar=%dx%d barHint=%dx%d tab0=%dx%d ed=%dx%d\n", i, p.root->isVisible(), p.bar->count(), p.docs.size(),
			p.bar->width(), p.bar->height(), p.bar->sizeHint().width(), p.bar->sizeHint().height(),
			p.bar->count() ? p.bar->tabRect(0).width() : -1, p.bar->count() ? p.bar->tabRect(0).height() : -1, p.ed->width(), p.ed->height());
	}
}
