// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include "npp_host.h"
#include "Parameters.h"
#include "ScintillaEditView.h"
#include "Buffer.h"
#include "host_hooks.h"
#include "functionParser.h"
#include "resource.h"      // ID_MACRO
#include <algorithm>
#include <new>
// Notepad_plus keeps the few fields FileManager reads private (FileManager is a friend on Windows). Open them up for this
// translation unit only; every other header is already included so nothing else is affected.
#define private public
#include "Notepad_plus.h"
#undef private

NppHost& NppHost::get() { static NppHost h; return h; }

void NppHost::init() {
	// FileManager/Buffer only read a few public members of Notepad_plus (_pEditView, _pNonEditView, _activeView), so a
	// zeroed block of the right size stands in for the real window class, which is not part of the macOS build.
	alignas(Notepad_plus) static unsigned char storage[sizeof(Notepad_plus)] = {};
	_npp = reinterpret_cast<Notepad_plus*>(storage);

	_main = new ScintillaEditView(true);
	_sub = new ScintillaEditView(false);
	_scratch = new ScintillaEditView(false);
	_npp->_pEditView = _main;
	_npp->_pNonEditView = _sub;
	_npp->_activeView = MAIN_VIEW;

	_main->init(nullptr, nullptr);
	_sub->init(nullptr, nullptr);
	_scratch->init(nullptr, nullptr);
	_scratch->execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
	MainFileManager.init(_npp, _scratch);

	configureViews();

	hostHooks().onBufferChanged = [this](Buffer* b, int mask) {
		_main->bufferUpdated(b, mask);
		_sub->bufferUpdated(b, mask);
		if (onBufferChanged) onBufferChanged(b, mask);
	};
}

ScintillaEditView& NppHost::view(int which) { return which == SUB ? *_sub : *_main; }
ScintillaEditView& NppHost::activeView() { return view(_active); }
void NppHost::setActiveView(int which) {
	_active = which;
	_npp->_activeView = which;
	_npp->_pEditView = which == SUB ? _sub : _main;
	_npp->_pNonEditView = which == SUB ? _main : _sub;
}

Buffer* NppHost::open(const std::wstring& path) {
	BufferID id = MainFileManager.loadFile(path.c_str());
	return id == BUFFER_INVALID ? nullptr : MainFileManager.getBufferByID(id);
}
Buffer* NppHost::newDocument() { return MainFileManager.getBufferByID(MainFileManager.newEmptyDocument()); }

void NppHost::show(int which, Buffer* b) {
	ScintillaEditView& v = view(which);
	MainFileManager.addBufferReference(b->getID(), &v);
	v.activateBuffer(b->getID(), false);
}

bool NppHost::save(Buffer* b, const std::wstring& path) {
	return MainFileManager.saveBuffer(b->getID(), path.empty() ? b->getFullPathName() : path.c_str()) == SaveOK;
}

bool NppHost::reload(Buffer* b) { return MainFileManager.reloadBuffer(b->getID()); }
void NppHost::close(int which, Buffer* b) { MainFileManager.closeBuffer(b->getID(), &view(which)); }

// ---------------------------------------------------------------------------------------------------------------------
NppHost::DocInfo NppHost::info(Buffer* b) {
	DocInfo d;
	d.path = b->getFullPathName();
	d.name = b->getFileName();
	d.dirty = b->isDirty();
	d.readOnly = b->isReadOnly();
	d.untitled = b->isUntitled();
	d.eol = static_cast<int>(b->getEolFormat());
	d.unicodeMode = static_cast<int>(b->getUnicodeMode());
	LangType lt = b->getLangType();
	if (lt == L_USER) d.language = std::wstring(L"User: ") + b->getUserDefineLangName();
	else if (lt >= 0 && lt <= L_EXTERNAL) {
		const wchar_t* n = ScintillaEditView::_langNameInfoArray[lt]._longName;
		d.language = n ? n : ScintillaEditView::_langNameInfoArray[lt]._langName;
	}
	return d;
}

std::vector<NppHost::Lang> NppHost::languages() {
	std::vector<Lang> out;
	for (int i = 0; i <= L_EXTERNAL; ++i) {
		const LanguageNameInfo& li = ScintillaEditView::_langNameInfoArray[i];
		if (!li._langName || !li._lexerID && li._langID != L_TEXT) continue;   // slots without a lexer
		if (li._langID == L_EXTERNAL || li._langID == L_USER) continue;
		out.push_back({ static_cast<int>(li._langID), li._longName ? li._longName : li._langName }); // same text as the status bar
	}
	std::sort(out.begin(), out.end(), [](const Lang& a, const Lang& b) {
		if (a.id == L_TEXT || b.id == L_TEXT) return a.id == L_TEXT && b.id != L_TEXT;
		return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
	return out;
}

void NppHost::setLanguage(Buffer* b, int langId) { b->setLangType(static_cast<LangType>(langId)); }

std::vector<std::wstring> NppHost::udlNames() {
	NppParameters& p = NppParameters::getInstance();
	std::vector<std::wstring> n;
	for (int i = 0; i < p.getNbUserLang(); ++i) n.push_back(p.getULCFromIndex(i)->getName());
	return n;
}
void NppHost::setUserLanguage(Buffer* b, const std::wstring& name) { b->setLangType(L_USER, name.c_str()); }
int NppHost::importUdl(const std::wstring& xml) {
	auto r = NppParameters::getInstance().importUDLFromFile(xml);
	return int(r.second) - int(r.first);
}
// (Re)loads the languages of an edited file: the old definition is dropped first because the importer skips names it already has.
int NppHost::reloadUdl(const std::wstring& oldName, const std::wstring& xml) {
	removeUdl(oldName);
	return xml.empty() ? 0 : importUdl(xml);
}
void NppHost::removeUdl(const std::wstring& name) {
	NppParameters& p = NppParameters::getInstance();
	for (int i = 0; i < p.getNbUserLang(); ++i) if (name == p.getULCFromIndex(i)->getName()) { p.removeUserLang(i); return; }
}
std::wstring NppHost::udlFolder() { return NppParameters::getInstance().getUserDefineLangFolderPath(); }

void NppHost::setEol(Buffer* b, int eol) {
	if (b->isReadOnly()) return;
	b->setEolFormat(static_cast<EolType>(eol));
	activeView().execute(SCI_CONVERTEOLS, eol); // Scintilla's SC_EOL_* use the same numbering
}

void NppHost::setUnicodeMode(Buffer* b, int mode) { if (!b->isReadOnly()) b->setUnicodeMode(static_cast<UniMode>(mode)); }

NppHost::SessionData NppHost::loadSession() {
	SessionData out;
	const Session& s = NppParameters::getInstance().getSession(); // already read from session.xml by NppParameters::load()
	out.activeView = static_cast<int>(s._activeView);
	out.activeIndex[0] = static_cast<int>(s._activeMainIndex);
	out.activeIndex[1] = static_cast<int>(s._activeSubIndex);
	for (const auto& r : s._fileBrowserRoots) out.workspaceRoots.push_back(r._root);
	for (int w = 0; w < 2; ++w)
		for (const auto& f : (w == 0 ? s._mainViewFiles : s._subViewFiles))
			out.files[w].push_back({ f._fileName, static_cast<long long>(f._endPos), static_cast<long long>(f._firstVisibleLine) });
	return out;
}

void NppHost::saveSession(const std::vector<Buffer*> docs[2], int activeView, const int activeIndex[2], const std::vector<std::wstring>& roots) {
	Session s;
	for (const std::wstring& r : roots) s._fileBrowserRoots.emplace_back(r);
	s._includeFileBrowser = !roots.empty();
	s._activeView = activeView;
	s._activeMainIndex = activeIndex[0];
	s._activeSubIndex = activeIndex[1];
	for (int w = 0; w < 2; ++w) {
		ScintillaEditView& v = view(w);
		for (Buffer* b : docs[w]) {
			if (b->isUntitled() || !doesFileExist(b->getFullPathName())) continue; // unsaved docs are not restored (no backup yet)
			Position pos = b->getPosition(&v);
			if (v.getCurrentBuffer() == b) { // live values for the shown document
				pos._startPos = v.execute(SCI_GETANCHOR);
				pos._endPos = v.execute(SCI_GETCURRENTPOS);
				pos._firstVisibleLine = v.execute(SCI_GETFIRSTVISIBLELINE);
			}
			sessionFileInfo sfi(b->getFullPathName(), ScintillaEditView::_langNameInfoArray[b->getLangType()]._langName,
				b->getEncoding(), false, false, false, pos, nullptr, FILETIME{}, MapPosition{});
			(w == 0 ? s._mainViewFiles : s._subViewFiles).push_back(sfi);
		}
	}
	NppParameters::getInstance().writeSession(s);
}

void NppHost::restoreCaret(int which, long long caret, long long firstLine) {
	ScintillaEditView& v = view(which);
	v.execute(SCI_GOTOPOS, caret);
	v.execute(SCI_SETFIRSTVISIBLELINE, firstLine);
}

void* NppHost::hwnd(int which) { return view(which).getHSelf(); }
void NppHost::setDirty(Buffer* b, bool dirty) { if (b->isDirty() != dirty) b->setDirty(dirty); }

void NppHost::configureViews() {
	const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
	for (ScintillaEditView* v : { _main, _sub }) {
		v->showMargin(ScintillaEditView::_SC_MARGE_LINENUMBER, svp._lineNumberMarginShow);
		v->showMargin(ScintillaEditView::_SC_MARGE_SYMBOL, svp._bookMarkMarginShow);
		v->showChangeHistoryMargin(svp._isChangeHistoryMarginEnabled);   // hidden margin = Scintilla paints modified lines as a full-line background
		v->showIndentGuideLine(svp._indentGuideLineShow);
		v->setMakerStyle(svp._folderStyle);
		v->defineDocType(v->getCurrentBuffer()->getLangType());
		v->setWrapMode(svp._lineWrapMethod);
		v->execute(SCI_SETENDATLASTLINE, !svp._scrollBeyondLastLine);
		v->setBorderEdge(svp._showBorderEdge);
		v->execute(SCI_SETCARETLINEVISIBLEALWAYS, true);
		v->wrap(svp._doWrap);
		v->showEOL(svp._eolShow);
		v->showWSAndTab(svp._whiteSpaceShow);
		v->showWrapSymbol(svp._wrapSymbolShow);
		v->performGlobalStyles();
		v->execute(SCI_SETZOOM, svp._zoom + kBaseZoom);
		v->execute(SCI_SETADDITIONALSELECTIONTYPING, true);
		v->execute(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
		v->execute(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, true);
		v->execute(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE);
		v->execute(SCI_SETMARGINLEFT, 0, svp._paddingLeft);
		v->execute(SCI_SETMARGINRIGHT, 0, svp._paddingRight);
		v->execute(SCI_STYLESETCHECKMONOSPACED, STYLE_DEFAULT, true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// shortcuts
std::vector<NppHost::ShortcutItem> NppHost::shortcuts() {
	std::vector<ShortcutItem> out;
	for (const CommandShortcut& c : NppParameters::getInstance().getUserShortcuts()) {
		const KeyCombo& k = c.getKeyCombo();
		out.push_back({ static_cast<int>(c.getID()), c.getNth(), c.getShortcutName(), k._isCtrl, k._isAlt, k._isShift, k._key });
	}
	return out;
}

void NppHost::setShortcut(size_t i, bool ctrl, bool alt, bool shift, int vk) {
	NppParameters& p = NppParameters::getInstance();
	auto& v = p.getUserShortcuts();
	if (i >= v.size()) return;
	v[i] = Shortcut(v[i].getName(), ctrl, alt, shift, static_cast<UCHAR>(vk)); // CommandShortcut::operator= keeps the names
	p.addUserModifiedIndex(i);
	p.setShortcutDirty();
}

void NppHost::saveShortcuts() { NppParameters::getInstance().writeShortcuts(); }

// ---------------------------------------------------------------------------------------------------------------------
// preferences
static ScintillaViewParams& svpRW() { return const_cast<ScintillaViewParams&>(NppParameters::getInstance().getSVP()); }

NppHost::Prefs NppHost::prefs() {
	NppParameters& p = NppParameters::getInstance();
	const ScintillaViewParams& s = p.getSVP();
	const NppGUI& g = p.getNppGUI();
	Prefs r;
	r.lineNumbers = s._lineNumberMarginShow; r.bookmarkMargin = s._bookMarkMarginShow; r.indentGuides = s._indentGuideLineShow;
	r.wrap = s._doWrap; r.showEol = s._eolShow; r.showWhitespace = s._whiteSpaceShow; r.scrollBeyondLastLine = s._scrollBeyondLastLine;
	r.currentLineHighlight = s._currentLineHiliteMode != LINEHILITE_NONE; r.foldStyle = static_cast<int>(s._folderStyle);
	r.caretWidth = g._caretWidth; r.tabSize = g._tabSize; r.tabsToSpaces = g._tabReplacedBySpace;
	r.newDocEol = static_cast<int>(g._newDocDefaultSettings._format); r.newDocUnicode = static_cast<int>(g._newDocDefaultSettings._unicodeMode);
	r.rememberSession = g._rememberLastSession;
	return r;
}

void NppHost::setPrefs(const Prefs& r) {
	NppParameters& p = NppParameters::getInstance();
	ScintillaViewParams& s = svpRW();
	NppGUI& g = p.getNppGUI();
	s._lineNumberMarginShow = r.lineNumbers; s._bookMarkMarginShow = r.bookmarkMargin; s._indentGuideLineShow = r.indentGuides;
	s._doWrap = r.wrap; s._eolShow = r.showEol; s._whiteSpaceShow = r.showWhitespace; s._scrollBeyondLastLine = r.scrollBeyondLastLine;
	s._currentLineHiliteMode = r.currentLineHighlight ? LINEHILITE_HILITE : LINEHILITE_NONE; s._folderStyle = static_cast<folderStyle>(r.foldStyle);
	g._caretWidth = r.caretWidth; g._tabSize = r.tabSize; g._tabReplacedBySpace = r.tabsToSpaces;
	g._newDocDefaultSettings._format = static_cast<EolType>(r.newDocEol);
	g._newDocDefaultSettings._unicodeMode = static_cast<UniMode>(r.newDocUnicode);
	g._rememberLastSession = r.rememberSession;
	configureViews();
	for (ScintillaEditView* v : { _main, _sub }) {
		v->execute(SCI_SETCARETWIDTH, g._caretWidth);
		v->execute(SCI_SETCARETLINEFRAME, 0);
		v->execute(SCI_SETCARETLINEVISIBLE, r.currentLineHighlight);
		if (Buffer* b = v->getCurrentBuffer()) v->defineDocType(b->getLangType()); // re-applies tab settings
	}
}

void NppHost::savePrefs() {
	NppParameters& p = NppParameters::getInstance();
	p.writeScintillaParams();
	p.createXmlTreeFromGUIParams();
	p.saveConfig_xml();
}

// ---------------------------------------------------------------------------------------------------------------------
// styles and themes
std::vector<std::wstring> NppHost::styleGroups() {
	NppParameters& p = NppParameters::getInstance();
	std::vector<std::wstring> out{ L"Global Styles" };
	for (size_t i = 0; i < p.getLStylerArray().getNbLexer(); ++i) {
		const wchar_t* d = p.getLStylerArray().getLexerDescFromIndex(i);
		out.push_back(d && *d ? d : p.getLStylerArray().getLexerNameFromIndex(i));
	}
	return out;
}

static StyleArray& styleArray(int group) {
	NppParameters& p = NppParameters::getInstance();
	return group == 0 ? p.getGlobalStylers() : static_cast<StyleArray&>(p.getLStylerArray().getLexerFromIndex(group - 1));
}

std::vector<NppHost::StyleItem> NppHost::styleItems(int group) {
	std::vector<StyleItem> out;
	for (const Style& st : styleArray(group)) {
		StyleItem it;
		it.id = st._styleID; it.desc = st._styleDesc;
		it.fg = st._fgColor == static_cast<COLORREF>(STYLE_NOT_USED) ? -1 : static_cast<long>(st._fgColor);
		it.bg = st._bgColor == static_cast<COLORREF>(STYLE_NOT_USED) ? -1 : static_cast<long>(st._bgColor);
		it.font = st._fontName; it.size = st._fontSize;
		if (st._fontStyle != STYLE_NOT_USED) { it.bold = st._fontStyle & FONTSTYLE_BOLD; it.italic = st._fontStyle & FONTSTYLE_ITALIC; it.underline = st._fontStyle & FONTSTYLE_UNDERLINE; }
		out.push_back(it);
	}
	return out;
}

void NppHost::setStyleItem(int group, int index, const StyleItem& it) {
	Style& st = styleArray(group).getStyler(static_cast<size_t>(index));
	st._fgColor = it.fg < 0 ? static_cast<COLORREF>(STYLE_NOT_USED) : static_cast<COLORREF>(it.fg);
	st._bgColor = it.bg < 0 ? static_cast<COLORREF>(STYLE_NOT_USED) : static_cast<COLORREF>(it.bg);
	st._fontName = it.font; st._fontSize = it.size;
	st._fontStyle = (it.bold ? FONTSTYLE_BOLD : 0) | (it.italic ? FONTSTYLE_ITALIC : 0) | (it.underline ? FONTSTYLE_UNDERLINE : 0);
}

bool NppHost::saveStyles() {
	NppParameters& p = NppParameters::getInstance();
	std::wstring saved = p.writeStyles(p.getLStylerArray(), p.getGlobalStylers()); // "" = saved in place, else the new (user) path
	if (!saved.empty()) p.getNppGUI()._themeName = saved;
	p.saveConfig_xml();
	return true;
}

void NppHost::applyStyles() {
	for (ScintillaEditView* v : { _main, _sub }) {
		v->performGlobalStyles();
		if (Buffer* b = v->getCurrentBuffer()) v->defineDocType(b->getLangType());
	}
}

static void scanThemes() {
	NppParameters& p = NppParameters::getInstance();
	ThemeSwitcher& ts = p.getThemeSwitcher();
	std::wstring dir = p.getNppPath();
	pathAppend(dir, L"themes");
	ts.setThemeDirPath(dir);
	std::vector<std::wstring> files;
	getFilesInFolder(files, L"*.xml", dir);
	std::sort(files.begin(), files.end());
	for (const std::wstring& f : files)
		if (!ts.themeNameExists(ThemeSwitcher::getThemeFromXmlFileName(f.c_str()).c_str())) ts.addThemeFromXml(f);
}

std::vector<std::wstring> NppHost::themes() {
	static bool scanned = false;
	if (!scanned) { scanThemes(); scanned = true; }
	ThemeSwitcher& ts = NppParameters::getInstance().getThemeSwitcher();
	std::vector<std::wstring> out;
	for (size_t i = 0; i < ts.size(); ++i) out.push_back(ts.getElementFromIndex(i).first.c_str()); // c_str(): PathRemoveExtension leaves a NUL inside the string
	return out;
}

int NppHost::currentTheme() {
	(void)themes();
	NppParameters& p = NppParameters::getInstance();
	ThemeSwitcher& ts = p.getThemeSwitcher();
	for (size_t i = 0; i < ts.size(); ++i) if (ts.getElementFromIndex(i).second == p.getNppGUI()._themeName) return static_cast<int>(i);
	return 0;
}

bool NppHost::setTheme(int index) {
	NppParameters& p = NppParameters::getInstance();
	ThemeSwitcher& ts = p.getThemeSwitcher();
	(void)themes();
	if (index < 0 || static_cast<size_t>(index) >= ts.size()) return false;
	const std::wstring path = ts.getElementFromIndex(index).second;
	p.getNppGUI()._themeName = path;
	// config.xml remembers the theme as a file name inside the DarkMode section (light or dark slot, whichever is active)
	NppGUI& g = p.getNppGUI();
	auto& slot = g._darkmode._isEnabled ? g._darkmode._advOptions._darkDefaults : g._darkmode._advOptions._lightDefaults;
	slot._xmlFileName = index == 0 ? std::wstring() : std::wstring(PathFindFileName(path.c_str())); // index 0 = Default (stylers.xml)
	if (!p.reloadStylers(path.c_str())) return false;
	applyStyles();
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// function list
std::vector<NppHost::FuncEntry> NppHost::functions(int which) {
	static FunctionParsersManager mgr;
	static ScintillaEditView* target = nullptr;   // the parsers read the document through this pointer
	static bool ready = false;
	NppParameters& p = NppParameters::getInstance();
	if (!ready) {
		std::wstring user = p.getUserPath(), installed = p.getNppPath();
		pathAppend(user, L"functionList"); pathAppend(installed, L"functionList");
		ready = mgr.init(user, installed, &target);
	}
	std::vector<FuncEntry> out;
	if (!ready) return out;
	target = &view(which);
	Buffer* b = target->getCurrentBuffer();
	if (!b) return out;
	LangType lang = b->getLangType();
	if (lang == L_JS_EMBEDDED) lang = L_JAVASCRIPT;
	const wchar_t* udl = lang == L_USER ? b->getUserDefineLangName() : nullptr;
	std::vector<foundInfo> found;
	if (!mgr.parse(found, AssociationInfo(-1, lang, PathFindExtension(b->getFileName()), udl))) return out;
	const UINT cp = static_cast<UINT>(target->execute(SCI_GETCODEPAGE));
	for (const foundInfo& f : found) out.push_back({ string2wstring(f._data2, cp), string2wstring(f._data, cp), static_cast<long long>(f._pos) });
	return out;
}

std::wstring NppHost::profileDir() { return NppParameters::getInstance().getUserPath(); }

void NppHost::updateLineNumberWidth(int which) { view(which).updateLineNumberWidth(); }

std::wstring NppHost::resourceSummary() {
	NppParameters& p = NppParameters::getInstance();
	return L"nppPath=" + p.getNppPath() + L" langs=" + std::to_wstring(p.getNbLang()) + L" themes=" + std::to_wstring(themes().size());
}

// ---------------------------------------------------------------------------------------------------------------------
// macros
static Macro g_macro;
static bool g_recording = false, g_macroSaved = false;

bool NppHost::macroRecording() const { return g_recording; }
int NppHost::macroLength() const { return static_cast<int>(g_macro.size()); }
bool NppHost::macroIsSaved() const { return g_macroSaved; }

void NppHost::macroStart() {
	g_macro.clear();
	_main->execute(SCI_STARTRECORD); _sub->execute(SCI_STARTRECORD);
	g_recording = true; g_macroSaved = false;
}

void NppHost::macroStop() {
	_main->execute(SCI_STOPRECORD); _sub->execute(SCI_STOPRECORD);
	g_recording = false;
}

// called while Scintilla's notification is being delivered, so lParam still points at the text: recordedMacroStep copies it
void NppHost::macroRecord(int message, unsigned long long w, long long l) {
	if (g_recording) g_macro.emplace_back(message, static_cast<uptr_t>(w), static_cast<uptr_t>(l));
}

// Scintilla reports more operations than Notepad++ can play back (recordedMacroStep::isMacroable); when the command produced
// only such steps, or none, the command itself is the step - as on Windows, where commands do not go through Scintilla's recorder.
void NppHost::macroRecordCommand(int idm, int stepsBefore) {
	if (!g_recording || stepsBefore < 0 || stepsBefore > static_cast<int>(g_macro.size())) return;
	for (size_t i = stepsBefore; i < g_macro.size(); ++i) if (g_macro[i].isMacroable()) return;
	g_macro.erase(g_macro.begin() + stepsBefore, g_macro.end());
	g_macro.emplace_back(idm);
}

static void playOnce(ScintillaEditView& v, Macro macro) {
	for (recordedMacroStep& step : macro) {
		if (step._macroType == recordedMacroStep::mtMenuCommand) { if (auto& f = NppHost::get().onMenuCommand) f(static_cast<int>(step._wParameter)); }   // WM_COMMAND on Windows
		else if (step.isScintillaMacro()) step.PlayBack(&v, &v);   // PlayBack only uses the Window argument for a notification
	}
}

void NppHost::macroPlay(int times) {
	ScintillaEditView& v = activeView();
	v.execute(SCI_BEGINUNDOACTION);                          // the whole playback is a single undo step
	for (int i = 0; i < times; ++i) playOnce(v, g_macro);
	v.execute(SCI_ENDUNDOACTION);
}

// Repeats `macro` until it has been run on the last line (a run that does not move down also ends the loop, so it can never spin forever).
static int playUntilEnd(ScintillaEditView& v, const Macro& macro) {
	int runs = 0;
	v.execute(SCI_BEGINUNDOACTION);
	for (;;) {
		auto lineOf = [&] { return v.execute(SCI_LINEFROMPOSITION, v.execute(SCI_GETCURRENTPOS)); };
		const auto startLine = lineOf(), lastLine = v.execute(SCI_GETLINECOUNT) - 1;
		playOnce(v, macro);
		++runs;
		if (startLine >= lastLine || lineOf() <= startLine || runs >= 100000) break;
	}
	v.execute(SCI_ENDUNDOACTION);
	return runs;
}

int NppHost::macroPlayUntilEnd() { return playUntilEnd(activeView(), g_macro); }

bool NppHost::macroSave(const std::wstring& name, bool ctrl, bool alt, bool shift, int vk) {
	if (g_recording || g_macro.empty() || name.empty()) return false;
	NppParameters& p = NppParameters::getInstance();
	auto& list = p.getMacroList();
	const int cmd = ID_MACRO + static_cast<int>(list.size());
	const std::string n = wstring2string(name, CP_UTF8);
	list.emplace_back(Shortcut(n.c_str(), ctrl, alt, shift, static_cast<UCHAR>(vk)), g_macro, cmd);
	p.getMacroMenuItems().push_back(MenuItemUnit(cmd, name));   // writeShortcuts() walks both lists in parallel
	p.setShortcutDirty();
	p.writeShortcuts();
	g_macroSaved = true;
	return true;
}

std::vector<NppHost::SavedMacro> NppHost::savedMacros() {
	std::vector<SavedMacro> out;
	for (MacroShortcut& m : NppParameters::getInstance().getMacroList()) {
		const KeyCombo& k = m.getKeyCombo();
		out.push_back({ string2wstring(m.getName(), CP_UTF8), static_cast<int>(m.getID()), static_cast<int>(m.getMacro().size()), k._isCtrl, k._isAlt, k._isShift, k._key });
	}
	return out;
}

void NppHost::macroPlaySaved(int index, int times) {
	auto& list = NppParameters::getInstance().getMacroList();
	if (index < 0 || static_cast<size_t>(index) >= list.size()) return;
	ScintillaEditView& v = activeView();
	Macro m = list[index].getMacro();
	v.execute(SCI_BEGINUNDOACTION);
	for (int i = 0; i < times; ++i) playOnce(v, m);
	v.execute(SCI_ENDUNDOACTION);
}

int NppHost::macroPlaySavedUntilEnd(int index) {
	auto& list = NppParameters::getInstance().getMacroList();
	if (index < 0 || static_cast<size_t>(index) >= list.size()) return 0;
	return playUntilEnd(activeView(), list[index].getMacro());
}

bool NppHost::macroDelete(int index) {
	NppParameters& p = NppParameters::getInstance();
	auto& list = p.getMacroList();
	if (index < 0 || static_cast<size_t>(index) >= list.size()) return false;
	list.erase(list.begin() + index);
	p.getMacroMenuItems().erase(static_cast<size_t>(index));
	p.setShortcutDirty();
	p.writeShortcuts();
	return true;
}
