// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// The "application" that the shared core expects (Notepad_plus on Windows): owns the main/sub/scratch Scintilla views,
// provides the services FileManager/Buffer call back into, and exposes simple document operations to the Qt UI.
#pragma once
#include <functional>
#include <string>
#include <vector>
class ScintillaEditView;
class Notepad_plus;
class Buffer;

class NppHost {
public:
	enum { MAIN = 0, SUB = 1 };
	static NppHost& get();
	void init();                                   // after installQtBridge() + bootCore()
	static constexpr int kBaseZoom = 3;            // +3pt: a Windows 10pt font looks ~33% smaller at macOS's 72dpi, so views start zoomed by this much
	void updateLineNumberWidth(int which);         // re-fit the line number margin (after zoom / many lines)
	void configureViews();                         // apply the user's view settings (Notepad_plus::init does this on Windows)

	ScintillaEditView& view(int which);
	void* hwnd(int which);                         // the view's window handle == its ScintillaEdit* (see qt_bridge.h)
	void setDirty(Buffer* b, bool dirty);          // Scintilla save-point notifications, as Notepad_plus handles them
	ScintillaEditView& activeView();
	void setActiveView(int which);
	int activeViewIndex() const { return _active; }

	Buffer* open(const std::wstring& path);        // load via FileManager; nullptr on failure
	Buffer* newDocument();                         // untitled
	Buffer* takeInitialDocument();                 // the untitled document the core gives the main view at start-up ("new 1"); returned once, then nullptr
	void show(int which, Buffer* b);               // make b the current document of that view
	bool save(Buffer* b, const std::wstring& path = std::wstring()); // empty path => its own path
	void close(int which, Buffer* b);
	void park(int which);                          // show the view's idle document (hidden, never listed): a view must not keep pointing at a closed buffer
	bool reload(Buffer* b);                        // re-read the file from disk (used by monitoring / tail -f)

	std::wstring resourceSummary();                // "nppPath=... langs=N themes=M" (diagnostics)
	std::wstring profileDir();                     // where config.xml, session.xml, ... live (tests redirect it)

	// ---- Qt-free document queries/commands (keeps Qt code away from the Win32 compat macros) ----
	struct DocInfo {
		std::wstring path, name, language;
		bool dirty = false, readOnly = false, untitled = false;
		int eol = 0;           // 0 = Windows (CRLF), 1 = old Mac (CR), 2 = Unix (LF); same numbering as SC_EOL_*
		int unicodeMode = 0;   // UniMode: 0 ANSI, 1 UTF-8 BOM, 2 UTF-16BE BOM, 3 UTF-16LE BOM, 4 UTF-8, ...
	};
	DocInfo info(Buffer* b);
	struct Lang { int id; std::wstring name; };
	std::vector<Lang> languages();                 // menu entries, "Normal Text" first
	void setLanguage(Buffer* b, int langId);
	std::vector<std::wstring> udlNames();          // user defined languages (userDefineLang.xml + userDefineLangs/*.xml)
	void setUserLanguage(Buffer* b, const std::wstring& udlName);
	int importUdl(const std::wstring& xmlPath);    // copies the file into userDefineLangs/; returns how many languages it added
	std::wstring udlFolder();
	int reloadUdl(const std::wstring& oldName, const std::wstring& xmlPath);   // after the UDL editor wrote xmlPath: replace the in-memory definition
	void removeUdl(const std::wstring& name);
	void setEol(Buffer* b, int eol);               // converts the text and the buffer's format
	void setUnicodeMode(Buffer* b, int mode);

	struct SessionFile { std::wstring path; long long caret = 0, firstLine = 0; };
	struct SessionData { std::vector<SessionFile> files[2]; int activeView = 0; int activeIndex[2] = { 0, 0 }; std::vector<std::wstring> workspaceRoots; };
	SessionData loadSession();                     // session.xml from the profile (same format as Windows)
	void saveSession(const std::vector<Buffer*> docs[2], int activeView, const int activeIndex[2], const std::vector<std::wstring>& workspaceRoots = {});
	void restoreCaret(int which, long long caret, long long firstLine);

	// ---- function list (Notepad++'s own regex rules in functionList/*.xml) ----
	struct FuncEntry { std::wstring parent, name; long long pos = 0; };   // parent = class/struct name (may be empty)
	std::vector<FuncEntry> functions(int which);                         // parse the document currently shown in that view

	// ---- keyboard shortcuts (shortcuts.xml; Windows virtual-key codes, converted to Qt by the UI) ----
	struct ShortcutItem { int cmd = 0, nth = 0; std::wstring name; bool ctrl = false, alt = false, shift = false; int vk = 0; };
	std::vector<ShortcutItem> shortcuts();
	void setShortcut(size_t index, bool ctrl, bool alt, bool shift, int vk); // vk 0 = no shortcut
	void saveShortcuts();

	// ---- preferences (config.xml) ----
	struct Prefs {
		bool lineNumbers = true, bookmarkMargin = true, indentGuides = true, wrap = false, showEol = false, showWhitespace = false;
		bool scrollBeyondLastLine = true, currentLineHighlight = true;
		int foldStyle = 4;            // 1 simple, 2 arrow, 3 circle, 4 box, 5 none
		int caretWidth = 1, tabSize = 4; bool tabsToSpaces = false;
		int newDocEol = 0, newDocUnicode = 4; // EOL numbering as DocInfo::eol; UniMode numbering as DocInfo::unicodeMode
		bool rememberSession = true;
	};
	Prefs prefs();
	void setPrefs(const Prefs&);   // takes effect immediately in both views
	void savePrefs();

	// ---- styles and themes (stylers.xml / themes) ----
	struct StyleItem { int id = 0; std::wstring desc; long fg = -1, bg = -1; std::wstring font; int size = -1; bool bold = false, italic = false, underline = false; }; // -1 = inherit
	std::vector<std::wstring> styleGroups();       // [0] "Global Styles", then one per language
	std::vector<StyleItem> styleItems(int group);
	void setStyleItem(int group, int index, const StyleItem&);
	bool saveStyles();
	void applyStyles();            // re-style both views from the in-memory styles
	std::vector<std::wstring> themes(); int currentTheme(); bool setTheme(int index);

	// ---- macros: Scintilla records user operations; steps and persistence use Notepad++'s own classes (saved in shortcuts.xml like on Windows) ----
	struct SavedMacro { std::wstring name; int cmd = 0, steps = 0; bool ctrl = false, alt = false, shift = false; int vk = 0; };
	bool macroRecording() const;
	void macroStart();                                       // clears the current macro and starts recording in both views
	void macroStop();
	void macroRecord(int message, unsigned long long wParam, long long lParam); // feed a Scintilla macro-record notification (copies text arguments)
	void macroRecordCommand(int idm, int stepsBefore);       // a menu command (IDM_*) as a macro step, unless it already left playable steps since stepsBefore
	std::function<void(int)> onMenuCommand;                  // playback of such a step: the UI triggers the menu action
	int macroLength() const;                                 // steps in the current recorded macro
	bool macroIsSaved() const;                               // current macro already saved under a name
	void macroPlay(int times = 1);                           // current macro on the active view, one undo step
	int macroPlayUntilEnd();                                 // repeat until the last line has been processed; returns the number of runs
	bool macroSave(const std::wstring& name, bool ctrl, bool alt, bool shift, int vk);
	std::vector<SavedMacro> savedMacros();
	void macroPlaySaved(int index, int times = 1);
	int macroPlaySavedUntilEnd(int index);
	bool macroDelete(int index);

	std::function<void(Buffer*, int)> onBufferChanged; // UI hook: title/dirty/language changed

private:
	ScintillaEditView *_main = nullptr, *_sub = nullptr, *_scratch = nullptr;
	Notepad_plus* _npp = nullptr;
	Buffer* _initial = nullptr;
	int _active = MAIN;
};
