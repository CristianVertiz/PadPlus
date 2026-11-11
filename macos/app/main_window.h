// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// The Notepad++ main window: two panes (main/sub), each a tab bar over a Scintilla view, plus menus and a status bar.
#pragma once
#include <QMainWindow>
#include <vector>
#include <QIcon>
#include <QMap>
#include <QHash>
#include <QPair>
#include <QStringList>
class QKeySequence; class QTabBar; class QLabel; class QSplitter; class QMenu; class QAction; class QDockWidget; class QTreeWidget; class ScintillaEdit; class FindDialog;
class QToolBar; class FunctionListPanel; class WorkspacePanel; class DocListPanel; class QTimer;
class FindInFilesDialog; class ColumnEditorDialog; class FindCharsDialog; struct FifOptions; struct FifFile;
class Buffer;
class NppHost;
class ClipboardPanel; class AnsiPanel; class DocMapPanel; class IncSearchBar;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(NppHost& host);
	void openPath(const QString& path, int which = -1);
	void restoreSession();
	void debugDump() const;
	FindDialog* findDialog() const { return _find; }
	QAction* actionByCmd(int idm) const { return _byCmd.value(idm); }   // Notepad++ command id (IDM_*) -> menu action
	QDockWidget* resultsDock() const { return _results; }
	QTreeWidget* resultsTree() const { return _resultsTree; }
	void runFindInFiles(const FifOptions& o);
	void openHit(const QString& path, int line, int column, int length);
	QMap<int, QString> commandNames() const;     // IDM_* -> menu text, for the Shortcut Mapper
	void applyShortcuts();                       // (re)apply shortcuts.xml to the menu actions
	FunctionListPanel* functionList() const { return _funcPanel; }
	WorkspacePanel* workspace() const { return _wsPanel; }
	DocListPanel* docList() const { return _docPanel; }
	void refreshFunctionList();
	void refreshDocList();
	void activateDoc(int which, int index);
	void addWorkspace(const QString& dir);
	// macros and synchronized scrolling (also reachable from the Macro / View menus and the toolbar)
	void macroStart();
	void macroStop();
	void runMacro(int source, int times, bool untilEnd);     // source: -1 = the current recorded macro, otherwise an index among the saved ones
	bool saveMacro(const QString& name, const QKeySequence& key);
	bool deleteMacro(int index);
	QMenu* macroMenu() const { return _macroMenu; }
	bool syncVertical() const { return _syncV; }
	bool syncHorizontal() const { return _syncH; }
	QToolBar* toolbar() const { return _toolbar; }
	QStringList statusFields() const;            // texts of the status bar fields, left to right
	bool isDarkChrome() const { return _dark; }
	void applyChromeTheme();                     // tab/status bar look + light/dark colour scheme, from the active theme
	// bookmarks, marks, incremental search, Run menu, monitoring (tail -f), extra panels
	void bookmark(int op);                       // 0 toggle, 1 next, 2 previous, 3 clear all
	void bookmarkedLines(int op);                // 0 cut, 1 copy, 2 delete
	void markAll(int style);                     // style 1-5: highlight every occurrence of the selection / word at the caret; 0 clears all styles
	void toggleMonitor();
	bool monitoring() const;
	void pollMonitored();                        // also driven by a timer
	QString expandRunVars(QString cmd) const;    // $(FULL_CURRENT_PATH), $(CURRENT_DIRECTORY), ...
	void runCommand(const QString& cmd);
	void applyLocalization(const QString& file);  // a nativeLang XML file name (menus and commands only); empty = English
	void udlChanged(const QString& name);         // a user defined language was edited or removed: restyle the documents using it
	IncSearchBar* incSearch() const { return _inc; }
	ClipboardPanel* clipboardPanel() const { return _clipPanel; }
	AnsiPanel* ansiPanel() const { return _ansiPanel; }
	DocMapPanel* docMapPanel() const { return _mapPanel; }
	QTabBar* tabBar(int which) const { return _p[which].bar; }        // 0 = main view, 1 = sub view
	ScintillaEdit* editor(int which) const { return _p[which].ed; }
protected:
	void closeEvent(QCloseEvent*) override;
	bool eventFilter(QObject*, QEvent*) override;
private:
	struct Pane { QWidget* root = nullptr; QTabBar* bar = nullptr; ScintillaEdit* ed = nullptr; std::vector<Buffer*> docs; };

	void buildMenus();
	void buildSearchExtras(QMenu* search);
	void buildViewExtras(QMenu* panels, QMenu* view);
	void buildLanguageExtras(QMenu* lang);
	void buildRunMenu();
	void buildLocalizationMenu(QMenu* settings);
	void recordCommand(int idm, int stepsBefore);   // while a macro is recorded: keep the menu command unless Scintilla already recorded what it did
	void buildToolbar();
	QAction* tbProxy(QAction* src, const char* icon);
	void reloadToolbarIcons();
	void syncActionStates();
	void closeAll();
	void cycleTab(int dir);
	void rebuildWindowMenu();
	void rebuildMacroMenu();
	void updateMacroActions();
	void toggleSync(bool vertical);
	void syncScroll(int from, bool vertical);
	void resetSyncOffsets();
	Buffer* current(int which) const;
	Buffer* currentActive() const { return current(_active); }
	ScintillaEdit* activeEditor() const { return _p[_active].ed; }
	int paneOf(Buffer* b, int preferred) const;
	void addDoc(int which, Buffer* b);
	void activate(int which, int index);
	void setActivePane(int which);
	bool closeTab(int which, int index);
	bool maybeSave(int which, Buffer* b);
	bool save(Buffer* b);
	bool saveAs(Buffer* b);
	void newDoc();
	void moveOrClone(bool move);
	void refreshTab(Buffer* b);
	void updateStatus();
	void addRecent(const QString& path);
	void rebuildRecent();
	void gotoLine();
	void sendActive(unsigned msg, unsigned long long w = 0, long long l = 0);

	NppHost& _h;
	Pane _p[2];
	int _active = 0;
	QSplitter* _split = nullptr;
	FindDialog* _find = nullptr;
	QMenu* _recent = nullptr;
	QMap<int, QAction*> _byCmd;
	FindInFilesDialog* _fif = nullptr;
	ColumnEditorDialog* _col = nullptr;
	FindCharsDialog* _chars = nullptr;
	QDockWidget* _results = nullptr;
	FunctionListPanel* _funcPanel = nullptr;
	WorkspacePanel* _wsPanel = nullptr;
	DocListPanel* _docPanel = nullptr;
	QTimer* _funcTimer = nullptr;
	ClipboardPanel* _clipPanel = nullptr; AnsiPanel* _ansiPanel = nullptr; DocMapPanel* _mapPanel = nullptr;
	IncSearchBar* _inc = nullptr;
	QTimer* _monTimer = nullptr;
	QHash<Buffer*, QPair<qint64, qint64>> _tail;   // monitored buffers -> last seen (size, mtime)
	QTreeWidget* _resultsTree = nullptr;
	QLabel *_lblLang, *_lblPos, *_lblLen, *_lblEol, *_lblEnc, *_lblIns;
	QToolBar* _toolbar = nullptr;
	struct TbItem { QAction* proxy; QString icon; };
	std::vector<TbItem> _tb;
	QAction *_tbUndo = nullptr, *_tbRedo = nullptr, *_tbCut = nullptr, *_tbCopy = nullptr, *_tbPaste = nullptr, *_tbSave = nullptr, *_tbSaveAll = nullptr;
	QMenu* _macroMenu = nullptr;
	int _macroStatic = 0;
	QAction *_actRecStart = nullptr, *_actRecStop = nullptr, *_actPlay = nullptr, *_actRunMulti = nullptr, *_actSaveMacro = nullptr;
	QMap<QString, QAction*> _tbNamed;            // toolbar buttons by icon name
	bool _syncV = false, _syncH = false, _syncing = false;
	int _syncOffV = 0, _syncOffH = 0;           // second view's position minus the first's, kept while scrolling together
	QMenu* _winMenu = nullptr;
	int _winStatic = 0;
	bool _dark = false;
	long _chromeBg = -2;
	QIcon _pencil, _closeIc;
	int _untitledCount = 0;
};
