# PadPlus (Notepad++ on macOS) — Build / Port Plan

> **Name:** the app was called iNotepad until 2026-11-11, when it was renamed PadPlus (an older Mac editor had used the previous name). Earlier entries below were updated to the new name.

## 0. Scope

Personal-use fork. Not intended for upstream contribution, so the upstream rules in `AGENTS.md` (Windows-only, no PRs for other platforms) don't apply here.

Still worth following by choice:
- **Leave `scintilla/` and `lexilla/` untouched** — makes pulling upstream updates trivial.
- **Keep non-UI core files close to upstream** — lets you keep merging upstream bug fixes.

## Decisions (2026-11-10)

| # | Topic | Decision |
|---|---|---|
| 1 | Approach | **Native macOS app** (Option B). Option A kept below only as a reference/fallback. |
| 2 | UI toolkit | **Qt 6** — Scintilla ships `scintilla/qt/ScintillaEdit`; docking, dialogs, tabs built in. |
| 3 | Qt install | **Homebrew** (`brew install qt`). LGPL is a non-issue for personal use. |
| 4 | Build system | **New top-level CMake** in `macos/`. Existing Windows builds untouched. |
| 5 | Code layout | **Separate `macos/` tree** for all Qt UI code. Shared core files reused from `PowerEditor/src`, changed only where needed to remove Win32 deps. Minimal `#ifdef` in shared files. |
| 6 | First usable version (MVP) | Open/edit/save, tabs, two views, syntax highlighting, encodings, recent files, session restore **+ Find/Replace**. |
| 7 | Plugins | **None.** Windows DLL plugins can't load. Revisit only if a specific plugin is missed. |
| 8 | Shortcuts | **Cmd** for standard mac shortcuts (Cmd+S/O/W/F/Z…); keep Npp-specific bindings, Ctrl→Cmd where it doesn't clash with macOS. |
| 9 | Settings | **`~/Library/Application Support/Notepad++`**, same XML formats as Windows (`config.xml`, `stylers.xml`, `shortcuts.xml`, UDLs) so files can be copied across. |
| 10 | Dark mode | **Follow macOS appearance** for chrome; editor colors from Npp theme (`stylers.xml` / themes). |
| 11 | Mac integrations | **Only** open-from-Finder (`Info.plist` document types) and drag-and-drop. Nothing else until missed. |
| 12 | Upstream sync | **Monthly merge** of upstream; conflicts expected only in shared core files. |
| 13 | Architecture | **Apple Silicon (arm64) only.** |
| 14 | Signing | **Ad-hoc** (`codesign -s -`). No notarization, no DMG. |
| 15 | Updater | **None.** Rebuild from the fork. |

## 1. Codebase assessment

| Area | State |
|---|---|
| `scintilla/` | Portable. Ships a **Cocoa** backend (`scintilla/cocoa/`, Xcode project) and a **Qt** backend (`scintilla/qt/ScintillaEdit`). |
| `lexilla/` | Portable. Ships `lexilla/src/Lexilla/Lexilla.xcodeproj`. |
| `boostregex/`, `pugixml/`, `uchardet/`, `json/`, `MISC/sha*`, `MISC/md5`, `MISC/crc16` | Portable C++. |
| **`PowerEditor/src` (the app)** | **~160k LOC of pure Win32.** |
| Build | MSVC (`PowerEditor/visual.net/notepadPlus.sln`), MinGW (`PowerEditor/gcc/makefile`, already supports `CROSS_COMPILE=`), CMake (`PowerEditor/src/CMakeLists.txt`, Windows only). CI is `windows-latest` only. |

Win32 surface in `PowerEditor/src` (grep counts):

| Pattern | Hits |
|---|---|
| files including `windows.h` | 146 |
| `SendMessage` | ~1200 |
| `WM_*` | ~1500 |
| `HWND` | ~1000 |
| GDI (`HDC`, `BitBlt`, `SelectObject`) | ~400 |
| `wchar_t` / `wstring` / `TCHAR` | ~3600 |
| Dialogs in `.rc` files (`DIALOGEX`) | 70 |
| Registry / INI APIs | 32 |
| `CreateFile`/`ReadFile`/`WriteFile` | 29 |
| Shell / COM (`SHGet*`, `IFileDialog`, `IShell*`) | ~45 |
| `LoadLibrary` / `GetProcAddress` | 41 |
| `ReadDirectoryChangesW` | 11 |
| `CreateThread` / `WaitForSingleObject` | 19 |

Files already free of Win32: `EncodingMapper`, `ScintillaComponent/FunctionCallTip.cpp`, hash libs, `*_rc.h` ID headers, `menuCmdID.h`, `keys.h`.
Even core classes (`Buffer`, `Parameters`, `MISC/Common`) take `HWND`/`wstring` in their interfaces.

Largest files: `Parameters.cpp` (9.6k), `Notepad_plus.cpp` (9.6k), `NppDarkMode.cpp` (5.1k), `NppCommands.cpp` (4.6k), `NppBigSwitch.cpp` (4.6k), `NppIO.cpp` (3.0k).

## 2. Two very different goals

"Compile for macOS" means one of:

- **Option A — build on macOS, run under Wine.** Days of work. Produces a Windows `.exe`.
- **Option B — native macOS app.** Months to a year+. A UI rewrite, not a port.

---

## 3. Option A — Cross-compile on macOS, run under Wine/CrossOver

No changes to `PowerEditor/src`.

1. Toolchain:
   ```sh
   brew install mingw-w64
   brew install --cask wine-stable   # or CrossOver / Whisky
   ```
2. Fix Windows-host assumptions in `PowerEditor/gcc/makefile`:
   - Lines ~219 / ~231: prebuild runs `cmd //C NppLibsVersionH-generator.bat`. Add a `sh` equivalent producing the same header when `$(OS)` is not `Windows_NT`.
   - Lines ~134–141: i686 build checks `%windir%/system32/SensApi.dll`. Target x86_64 only (or skip the check when cross-compiling).
3. Build:
   ```sh
   make -C PowerEditor/gcc CROSS_COMPILE=x86_64-w64-mingw32-
   ```
4. Run:
   ```sh
   wine PowerEditor/bin.x86_64/notepad++.exe
   ```
5. Optional: `.app` wrapper with a Wine launcher script; `macos-latest` CI job doing the cross-compile.

Limits: non-native look, x86_64 via Rosetta + Wine on Apple Silicon, partial dark mode / DPI.

---

## 4. Option B — Native macOS app

### 4.1 Toolkit choice

| Toolkit | Pros | Cons |
|---|---|---|
| **Qt 6 (recommended)** | Scintilla already has `scintilla/qt/ScintillaEdit`. Docking, dialogs, tabs, trees, file watching built in. Could later replace the Win32 UI on Windows too. | Large dependency, LGPL. |
| Cocoa / Objective-C++ | Most native. Uses `scintilla/cocoa` directly. | macOS-only; every dialog hand-built in AppKit. |
| wxWidgets | Win32-like API, more mechanical porting. | `wxStyledTextCtrl` bundles its own Scintilla fork — clashes with vendored one. |
| Win32 shim layer | Minimal source churn. | You'd be rewriting Wine. Don't. |

The phases below assume Qt 6.

### Phase 0 — Build skeleton (~1 week)

- New top-level CMake that builds, **without editing vendored trees**:
  - Scintilla (Qt backend sources from `scintilla/qt/ScintillaEdit` + `scintilla/src`)
  - Lexilla (`lexilla/src`, `lexilla/lexlib`, `lexilla/lexers`)
  - Boost.Regex (`boostregex/`) with the same defines used on Windows
- Hello-world `QMainWindow` with one `ScintillaEdit`: open file, Lexilla highlighting.
- CI: `macos-latest`, produce a `.app`.

**Exit criteria:** a `.app` opens a C++ file with syntax highlighting.

**Status: DONE (2026-11-10).** `macos/` builds `build/PadPlus.app` (arm64):
```sh
brew install qt ninja
cmake -S macos -B macos/build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build macos/build
macos/build/PadPlus.app/Contents/MacOS/PadPlus some_file.cpp
# headless smoke test: QT_QPA_PLATFORM=offscreen NPP_SNAPSHOT=out.png <binary> file
```
Findings (all handled in `macos/CMakeLists.txt` without touching `scintilla/` or `lexilla/`):
- `ScintillaEdit.{cpp,h}` are generated by `WidgetGen.py` into the build dir (run from a mirror dir because it uses `../..` paths).
- The Qt template predates this Scintilla's removal of `SCI_FINDTEXT/GETTEXTRANGE/FORMATRANGE`; the build-dir copy of the template is patched to the `*FULL` variants.
- `lexilla/lexers/LexUser.cxx` (Notepad++'s UDL lexer) includes `windows.h` and uses `_itoa`; `macos/shim/windows.h` covers this for that file only.
- Boost.Regex search (`SCI_OWNREGEX`) compiles and links; not yet exercised at runtime (needs Find/Replace).

### Phase 1 — Platform abstraction of the core (2–4 weeks)

- **Strings:** `wchar_t` is 32-bit on macOS. Internal `wstring` use is fine; audit every conversion point to UTF-8/UTF-16 (Scintilla, file I/O, XML, clipboard). Reuse `WcharMbcsConvertor` in `MISC/Common` rather than adding a new helper. `L"..."` literals keep working.
- Put OS calls in `MISC/Common`, `Parameters.cpp`, `NppIO.cpp`, `ScintillaComponent/Buffer.cpp` behind thin free functions:
  - File I/O (`CreateFile` & co.) → `std::filesystem` / POSIX
  - Paths: `%APPDATA%\Notepad++` → `~/Library/Application Support/Notepad++`
  - Registry → drop or `QSettings`
  - Code pages (Win32 `MultiByteToWideChar` with CP ids) → iconv + mapping table (uchardet already detects)
  - Threads → `std::thread`, `std::mutex`, `std::condition_variable`
  - `WinControls/ReadDirectoryChanges` → `QFileSystemWatcher` (or FSEvents)
- **Goal:** `Buffer`, `FileManager`, `Parameters` (config.xml, langs.xml, stylers.xml, shortcuts.xml via pugixml), `localization.cpp` compile without `windows.h`. This is the reusable core.

**Exit criteria:** core library compiles on macOS; loads/saves config XMLs; unit test loads a file through `FileManager` with encoding detection.

**Status: Phase 1 PARTIAL (2026-11-10) — core compiles, links and runs on macOS; file loading not yet done.**

Approach taken (differs from the original plan, which assumed moving Win32 calls behind wrappers file by file): compile the shared core
sources **unmodified** against a Win32 compatibility layer in `macos/compat/`, so upstream merges stay easy.
- `compat/win_consts.h`, `win_types.h`: real Win32 types and constants (key codes, file flags, struct layouts).
- `compat/win_impl.{h,cpp}`: **real POSIX implementations** of the non-UI APIs the core depends on: code-page conversion (UTF-8 <-> UTF-32 `wchar_t`),
  file I/O/attributes/times, directory enumeration, path helpers (`shlwapi`), `SHGetFolderPath` (APPDATA -> `~/Library/Application Support`), last-error,
  string helpers. `NPP_COMPAT_TRACE=1` logs file-system calls/failures.
- `compat/win_auto.h`: placeholder constants and no-op stubs for UI/system calls the core mentions but never needs (`SendMessage`, GDI, ...). Hand-maintained.
- `compat/ui_stubs.cpp`: link-time stand-ins for Win32 UI classes (StaticDialog, TabBar, CustomFileDialog, NppDarkMode, ...). Phase 2/3 replace these.

Built as `npp_core` (static lib): `Common.cpp`, `FileInterface.cpp`, `Utf8_16.cpp`, `EncodingMapper.cpp`, `Parameters.cpp`, `localization.cpp`, `Buffer.cpp`,
`ScintillaEditView.cpp`, `UserDefineDialog.cpp`, `shortcut.cpp`, `dpiManagerV2.cpp`, `sha-256.cpp`, pugixml, uchardet.
`macos/tests/core_test.cpp` (target `core_test`, hermetic: temp `HOME` + settings dir) passes: loads `langs.xml` (96 languages), ext->language lookup,
UTF-8/UTF-16 conversion and BOM handling, path joining.

Edits to shared sources (4 files, ~21 lines, Windows behaviour unchanged): native path separator `kPathSep`/`NPP_SEP` in `Common.h` used by `pathAppend` and
the few places that built paths with a literal `\`; `testFile.wstring()` in `Common.cpp` (`std::filesystem::path::c_str()` is `char*` on macOS).
**These files are CRLF: edit them byte-exact (don't round-trip through text-mode tools).**

Known limits / still to do for Phase 1:
- `FileManager::loadFile` (in `Buffer.cpp`) needs a live `Notepad_plus` + Scintilla view, so open/save of real files is exercised in Phase 2 (window MVP).
  The "load a file through FileManager with encoding detection" exit test is therefore **not done yet**.
- Config *saving* (`config.xml`, `shortcuts.xml`, ...) and `stylers.xml`/theme/UDL loading are compiled but not asserted in the test.
- `CreateThread` is synchronous (loses the hung-network-share timeout); stubs silently do nothing, so a UI-ish call reaching them fails quietly.
- Not compiled: `NppIO.cpp`, `NppCommands.cpp`, `NppBigSwitch.cpp`, `Notepad_plus.cpp` (the application window; Phase 2).
- `NppDarkMode.cpp` is stubbed (it needs real DWM/UxTheme); Qt palette replaces it.

### Phase 2 — Main window MVP (4–8 weeks)

- Replace `winmain.cpp`, `Notepad_plus_Window`, message loop → `QApplication` / `QMainWindow`.
- `NppBigSwitch.cpp` (the `WM_*` dispatcher): convert handlers to direct methods / slots. Keep an internal `sendMessage(id, wParam, lParam)` for `NPPM_*` messages (plugin compatibility path, see Phase 5).
- `ScintillaEditView`: replace `::SendMessage(hwnd, SCI_*, ...)` with `ScintillaEdit::send(SCI_*, ...)` — SCI API is identical, mostly mechanical.
- Menus (`Notepad_plus.rc` + `menuCmdID.h`) → `QMenuBar`; map Ctrl→Cmd.
- Toolbar → `QToolBar`; `DocTabView` / `TabBar` → `QTabBar`; `StatusBar` → `QStatusBar`; `SplitterContainer` → `QSplitter` (clone/move to other view).
- `NppCommands.cpp`: mostly logic; port once Phase 1 helpers exist.

**Exit criteria:** open/edit/save, multiple tabs, two views, syntax highlighting, encodings, EOL conversion, recent files, session restore.

**Status: Phase 2 MVP DONE except the items listed below (2026-11-10).** The decisive trick: instead of replacing `Notepad_plus_Window`,
the unmodified core `ScintillaEditView` and `FileManager`/`Buffer` run on Qt.
- `compat` routes `SendMessage`/`CreateWindowEx("Scintilla")` to hooks (`win_impl.h`); `app/qt_bridge.cpp` answers them with Qt `ScintillaEdit`
  widgets (an `HWND` is the widget pointer; `SCI_GETDIRECTFUNCTION` returns a trampoline). So all of Notepad++'s lexer/keyword/theme/fold logic runs as is.
- `app/npp_host.cpp` plays `Notepad_plus` for the core: main/sub/scratch views, a zero-filled stand-in object for the few fields `FileManager` reads,
  `notifyBufferChanged` forwarded to the views and the Qt tabs (`compat/host_hooks.h`), and `configureViews()` (the view-settings block of `Notepad_plus::init`).
- `app/main_window.cpp`: two panes of tab bar + Scintilla, File/Edit/Search/View/Language menus, status bar, dirty tracking via Scintilla save points,
  Open Recent (QSettings), drag/drop + Finder open, session save/restore (`session.xml`), Move/Clone to other view.
- `app/find_dialog.cpp`: Find/Replace with match case, whole word, regex (Boost), wrap, replace all, count.
- Tests: `core_test`, `bridge_test`, `host_test`, `window_test` (see `macos/README.md`).

Still open from the Phase 2 list: unsaved/untitled documents are not restored by the session (no backup/snapshot yet); `NppCommands.cpp` is not ported
(only the commands listed above exist); no Cmd/Ctrl shortcut remapping layer for Notepad++'s own bindings (`shortcuts.xml` is loaded but not applied);
find-in-files, mark, incremental search; file-change-on-disk detection and the reload prompt; dark appearance for the editor chrome.

### Phase 3 — Dialogs (8–16 weeks, the bulk)

- Rebuild the 70 `.rc` dialogs as Qt `.ui` files or code. **Keep control IDs from `*_rc.h`** so `nativeLang.xml` localization keeps mapping.
- Re-base `WinControls/Window.h` and `WinControls/StaticDialog` on `QWidget`/`QDialog` so subclasses stay structurally similar.
- Priority:
  1. `ScintillaComponent/FindReplaceDlg` (Find / Replace / Find in Files / Mark — largest, most important)
  2. `GoToLineDlg`, `columnEditor`
  3. `WinControls/Preference` (24 dialogs)
  4. `WinControls/ColourPicker/WordStyleDlg` (Style Configurator)
  5. `WinControls/Grid/ShortcutMapper`, `WinControls/shortcut`
  6. `ScintillaComponent/UserDefineDialog` (UDL, 7 dialogs)
  7. Remaining: About, Run, macros, WindowsDlg, TaskList, md5/sha dialogs, FindCharsInRange.

**Status: Phase 3 PARTIAL (2026-11-10): the dialogs that matter for daily use are done and tested; the rest of the 70 are deliberately not rebuilt.**
Done (Qt, over the core's own data and writers so the files stay Windows-compatible):
- Shortcuts: `shortcuts.xml` bindings applied to the menu actions (`keymap.*` converts Windows virtual keys; Ctrl->Command, Alt->Option; multiple
  bindings per command), Shortcut Mapper with conflict warning, saved via the core's `writeShortcuts`.
- Preferences: view/editing, tab size/spaces, new-document EOL/encoding, session memory; applied live and written to `config.xml`.
- Style Configurator + themes: the ~20 shipped themes (`PowerEditor/installer/themes`, copied next to the executable), per-language/global style editing
  with live preview, saved via `writeStyles`; the theme is remembered through the `DarkMode` section (`lightThemeName`) that this Notepad++ version uses.
- Find in Files with a results dock; Column Editor; Find/Replace (Phase 2); Go To Line.
- Tests: `dialogs_test`, `settings_ui_test` (plus the four earlier ones).

Not rebuilt (Windows-only or low value for a personal Mac build): Mark/Bookmark dialogs, Find Chars In Range, Run/Run macro dialogs, Windows list, MD5/SHA
dialogs, UDL editor (`UserDefineDialog` still compiles with stubs; existing UDLs load but cannot be edited), Plugins Admin, About, "Open as workspace" tree,
tray icon / registry extension dialogs, DPI dialogs. Dark mode is theme-based (pick a dark theme); macOS menus and dialogs follow the system appearance.

### Phase 4 — Docking panels (3–6 weeks)

- `WinControls/DockingWnd` framework → `QDockWidget`.
- Panels: `FunctionList`, `DocumentMap`, `FileBrowser` (Folder as Workspace), `ProjectPanel`, `ClipboardHistory`, `AnsiCharPanel`, `VerticalFileSwitcher` (Document List).

**Status: Phase 4 PARTIAL (2026-11-10): Function List, Folder as Workspace and Document List are done and tested.**
- Function List reuses the core's `functionParser.cpp` unchanged (Notepad++'s XML regex rules, run through Boost on the Qt Scintilla view); the Qt panel groups
  by class, filters, sorts and jumps. Rules ship in `functionList/` next to the binary.
- Folder as Workspace: multi-root lazy tree with a file context menu; roots persisted through the core's own `session.xml` writer/reader.
- Document List: both views, dirty marker, click to switch. Dock layout/geometry persisted in the profile (`macos-ui.ini`).
- Portability fixes found on the way (all in shared code, tiny): two more hard-coded `\` path joins now use `NPP_SEP`; real implementations replaced silent
  stubs for `strncpy_s`, `sprintf_s`, `swprintf_s`/`wsprintf` (with MSVC `%s` = wide string semantics), `lstrcpy`, `itoa`, `CharUpper/Lower`, `CompareStringOrdinal`.
- UI state (recent files, layout) no longer goes to `~/Library/Preferences`; my earlier test runs had written temp paths there (removed).
Not done: Document Map, Clipboard History, ANSI Chars panel.

**Chrome pass (2026-11-10):** `app/chrome.*` gives the Notepad++ look around the editor: dark/light palettes, tab bar QSS (mint/orange top bar on the active
tab, no expansion), painted pencil and close icons, per-tab close buttons, the six status bar fields with separators, and the Notepad++-style window title.
The app colour scheme follows the active theme via `QStyleHints::setColorScheme` (Dark for dark themes, system otherwise); verified headless, but the native
macOS menu bar cannot be screenshotted, so its darkening is untested. Toolbar (Notepad++'s own `icons/{dark,light}/toolbar/regular` icons, proxy actions with live enabled/checked state) and the Encoding / Window / ? menus
followed. Printing (`print_doc.*`), the Tools menu (hashes) and a +3 base zoom (so Windows point sizes look right at macOS's 72 dpi) followed. Still missing versus
the reference screenshot: synchronized-scroll, monitoring and macro-recording buttons, and the Macro / Run / Plugins menus.

**Packaging (2026-11-11, part of Phase 5):** `PadPlus.app` is a real bundle. `packaging/Info.plist.in` (document types for Finder, high-DPI, category),
the app icon (generated at build time until 2026-11-11, now `packaging/icon/PadPlus.icns`), all run-time files in `Contents/Resources` (the compat layer's
`GetModuleFileNameW` reports `Contents/Resources/<exe>` for bundled executables, so the core's "install dir" is Resources; an empty `plugins/` ships so the
core never writes into the signed bundle), and `package_app` (macdeployqt + `codesign --sign -`) for a relocatable build. `tests/package_check.sh` verifies it.
Bundle id `local.PadPlus` (changed to `io.github.cristianvertiz.padplus` on 2026-11-11), macOS 13+, arm64. Not done: notarization, DMG, auto-update, Developer ID signing.

**Macros and synchronized scrolling (2026-11-11):** the macro recorder drives Scintilla's `SCI_STARTRECORD` and the `macroRecord` notification; steps, playback and
persistence are the core's own (`recordedMacroStep`, `MacroShortcut`, `writeShortcuts`), so macros round-trip with Windows `shortcuts.xml`. Synchronized scrolling
follows `verticalScrolled` / `horizontalScrolled` of the two Qt Scintilla views with a re-entrancy guard. Bugs found on the way: the compat layer's ANSI string
helpers (`lstrcpynA`, ...) were silent stubs, which left every core `Shortcut` name empty (fixed with real implementations); parallel per-test resource copies
raced in the build (replaced by one `test_resources` target).

**Remaining features (2026-11-11):** bookmarks (core marker 20, the margin and its icon are the core's), mark styles (indicators 21-25), incremental search, Run menu,
monitoring (`FileManager::reloadBuffer` on a 1 s poll), Clipboard History, ANSI Chars, Document Map (a second Scintilla view on the same document pointer; the visible
region is an overlay widget, because a marker would be written into the shared document) and UDL import/list. Plugins are not possible (Windows DLLs); the UDL editor
dialog is not rebuilt. Bug found on the way, present since the first phases: `configureViews` never showed the change-history margin, so Scintilla painted every
modified line as an orange full-line background (markers that are in no visible margin are drawn in the text area); fixed with `showChangeHistoryMargin`.

**Second pass (2026-11-11):** the User Defined Language editor (`udl_dialog.*`; it edits the XML with pugixml and reloads it through `importUDLFromFile`, so no
private core state is touched), Mark All / Bookmark line in the Find dialog, Find Characters in Range, menu commands as macro steps (`mtMenuCommand`, played back by
triggering the menu action), menu translation from `nativeLang/*.xml` (Settings > Localization), and scrolling tab bars (Fusion style on the tab bars only).
Packaging collects licence texts into `Contents/Resources/licenses`. Still open: translated dialogs, Find/Replace steps in macros, notarization, auto-update.

### Phase 5 — Platform features & packaging

| Windows feature | macOS action |
|---|---|
| `DarkMode/` (IAT hooks, UAH menu), `NppDarkMode.cpp` | Drop; use Qt palette / native appearance, map Npp theme colors |
| `TrayIcon` | `QSystemTrayIcon` |
| `MISC/RegExt` (file associations) | `Info.plist` UTIs / document types |
| `MISC/Exception/Win32Exception` | Drop or use a crash reporter |
| `Printer.cpp` | `QPrinter` + Scintilla `SCI_FORMATRANGE` |
| `dpiManagerV2` | Drop (Retina handled by Qt) |
| `OpenSaveFileDialog` | `QFileDialog` |
| `MISC/Process` | `QProcess` |
| Plugins (`MISC/PluginsManager`, `PluginInterface.h`) | Existing Windows DLLs receive `HWND` and **can never load**. Ship v1 without plugins; optionally define a `.dylib` ABI later. Plugin Admin install flow is Windows-specific. |
| Updater (WinGUp) | Drop or Sparkle |

Packaging: `.app` bundle, `macdeployqt`, codesign, notarize, DMG. Universal binary (arm64 + x86_64).

### 4.2 Risks

- **Upstream merges:** once the UI is rewritten, merging upstream becomes painful. Keep the Phase 1 core structurally close to upstream; rebase only non-UI files.
- **Behavior drift:** Boost regex integration in `FindReplaceDlg`, encoding edge cases. Use `PowerEditor/Test` (FunctionList, UrlDetection, xmlValidator) as a regression harness — run on Windows first, then macOS.
- **Keyboard:** Npp's Ctrl-heavy defaults vs. macOS conventions; needs a remap layer in `WinControls/shortcut`.
- **Scope:** 70 dialogs + docking framework is the dominant cost; underestimating Phase 3 is the most likely failure.

---

## 5. Execution order

Native with Qt 6 was chosen (see Decisions). Order:

1. **Phase 0** — CMake skeleton in `macos/`, Scintilla(Qt) + Lexilla + Boost.Regex, hello-world `.app`.
2. **Phase 1** — strip Win32 from the shared core.
3. **Phase 2 + Find/Replace from Phase 3** — daily-driver MVP. Go/no-go checkpoint.
4. **Phases 3–5** — remaining dialogs, docking panels, platform polish, in the priority order listed.

Adjustments from the decisions above:
- Phase 5: plugins, updater, notarization, DMG and universal binary are **out of scope**.
- Packaging is just: `macdeployqt` + `codesign -s -` on an arm64 `.app`.
