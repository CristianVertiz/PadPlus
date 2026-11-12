# PadPlus for Mac: developer guide

How to build, package, run, test and release PadPlus, a macOS editor based on the Notepad++ source code. To just install the app, see the [main README](../README.md).

Related documents: [what is reused, ported and new](REUSE_PORT_NEW.md), [third-party notices](THIRD_PARTY_NOTICES.md), and the [port plan and history](MACOS_PORT_PLAN.md).

**Contents:** [Prerequisites](#prerequisites) · [Build](#build) · [Package](#package-a-self-contained-app) · [Run](#run-the-app) · [Features](#features) · [CI and releases](#continuous-integration-and-releases) · [Test](#test) · [Known gaps](#known-gaps) · [Where things live](#where-things-live) · [Troubleshooting](#troubleshooting)

All commands run from the **repository root**. Apple Silicon (arm64) only.

## Prerequisites

```sh
xcode-select --install          # Apple clang + SDK (skip if already installed)
brew install cmake ninja qt     # Qt 6 (tested with 6.12), CMake >= 3.21, Ninja
python3 --version               # any Python 3; used at configure time to generate ScintillaEdit.cpp
```

## Build

```sh
cmake -S macos -B macos/build -G Ninja -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build macos/build
```

Targets (build one with `cmake --build macos/build --target <name>`):

| Target | Output | What it is |
|---|---|---|
| `NotepadPP` | `macos/build/PadPlus.app` | **PadPlus**, the app: Qt main window over the shared Notepad++ core (the CMake target keeps its old name, `NotepadPP`) |
| `core_test` | `macos/build/core_test` | shared core: config XMLs, encodings, paths |
| `bridge_test` | `macos/build/bridge_test` | the core's `ScintillaEditView` running on a Qt Scintilla widget |
| `host_test` | `macos/build/host_test` | real `FileManager`: encodings, EOL, language, save |
| `window_test` | `macos/build/window_test` | the main window driven through its menu actions |
| `dialogs_test` | `macos/build/dialogs_test` | shortcuts, preferences, styles and themes (the logic behind the Settings dialogs) |
| `settings_ui_test` | `macos/build/settings_ui_test` | Find in Files, Column Editor, Shortcut Mapper, Preferences, Style Configurator, theme menu |
| `panels_test` | `macos/build/panels_test` | Function List, Folder as Workspace, Document List |
| `toolbar_test` | `macos/build/toolbar_test` | toolbar, Encoding / Window / ? menus, Close All |
| `macro_test` | `macos/build/macro_test` | macro recorder (record, playback, save, delete, run to end) and synchronized scrolling |
| `extras_test` | `macos/build/extras_test` | bookmarks, mark styles, incremental search, Run variables, monitoring, Clipboard / ANSI / Document Map panels, UDL import |
| `more_test` | `macos/build/more_test` | Mark All, Find Characters in Range, menu commands in macros, the User Defined Language editor, localization, tab scrolling |
| `npp_core`, `scintilla_qt`, `lexilla` | static libs | built automatically as dependencies |

First build takes a few minutes (Scintilla, ~130 Lexilla lexers, Boost.Regex, the core). Rebuilds are incremental.

Clean rebuild: `rm -rf macos/build` and run the two commands above again.

## Package a self-contained app

```sh
cmake --build macos/build --target package_app        # embeds Qt (macdeployqt), licence texts, signs ad hoc
macos/tests/package_check.sh macos/build/PadPlus.app --packaged
cp -R macos/build/PadPlus.app /Applications/          # install
```
The plain build (`PadPlus.app` straight from step 2) already has everything PadPlus reads at run time in `Contents/Resources` (`langs.model.xml`,
`stylers.model.xml`, `themes/`, `functionList/`, `icons/`, an empty `plugins/`, and the app icon `PadPlus.icns` from
`packaging/icon/`), but still loads Qt from Homebrew. `package_app` copies Qt into the bundle, so it runs on a Mac without Homebrew and can be
moved anywhere; `package_check.sh --packaged` proves it (no Homebrew paths, valid ad-hoc signature, starts from a copied location and finds its resources).
`package_app` prunes what PadPlus never uses (Qt's on-screen-keyboard plugin and with it Qt Quick/QML, SVG/PDF image plugins, icon engines, network/TLS
plugins). `macdeployqt` still prints about eight `Cannot resolve rpath` errors while it works (they concern exactly those unused plugins whose frameworks
Homebrew keeps in other prefixes); they are harmless because `package_app` removes them afterwards and `package_check.sh --packaged` verifies nothing is left dangling.
The bundle is about 85 MB (a 40 MB disk image). It is not notarized: a copy downloaded from elsewhere must be allowed once in System Settings > Privacy & Security.
`package_app` can be run repeatedly; it also rewrites leftover Homebrew install names (`packaging/fix_homebrew_refs.sh`), sets the minimum macOS the bundle declares (`packaging/set_min_macos.sh`) and collects licence texts (`packaging/collect_licenses.sh`). Finder integration comes from `packaging/Info.plist.in`:
PadPlus is offered under "Open With" for text, source, script, XML, JSON and other files (it never becomes the default), and files opened from Finder or
dropped on the Dock icon open in the running app.

## Run the app

```sh
macos/build/PadPlus.app/Contents/MacOS/PadPlus PowerEditor/src/Utf8.h     # opens a file
open macos/build/PadPlus.app                                                # opens an empty window
```

## Features

**Files and views**
- Tabs in two views (main and sub) with Move / Clone to Other View and synchronized scrolling (vertical and horizontal, independent). Many tabs scroll.
- Double-click the empty part of a tab strip to create a new document in that view.
- Open, Save, Save As, Save All, Close All, Open Recent, drag and drop, opening from Finder ("Open With", Dock icon), session restore (`session.xml`).
- Encodings: ANSI, UTF-8, UTF-8 BOM, UTF-16 BE / LE BOM, detected on open and preserved on save. EOL detection and conversion.
- Print (Cmd+P) with Scintilla's own formatter; dark themes print black on white.
- Monitoring (tail -f): re-reads a file when it changes on disk and follows its end.

**Editing**
- Notepad++'s lexers, keywords and folding for all its languages (Language menu), line numbers, fold and bookmark margins, change history.
- Duplicate / move / join lines, upper / lower case, Column Editor (text or counting numbers in dec / hex / oct / bin).
- Word wrap, whitespace and EOL display, indent guides, zoom. Text starts at zoom +3, because a Windows 10pt font looks about a third smaller at macOS's 72 dpi.
- Copy to Clipboard: full path, file name, directory.

**Search**
- Find / Replace (Cmd+F; next / previous Cmd+G): match case, whole word, regular expressions (Boost.Regex), wrap around, Replace All, Count.
- Mark All, Clear All Marks and Bookmark line, in the same dialog.
- Find in Files (Cmd+Shift+F): filters, sub-folders, hidden files; results dock, double-click to jump.
- Incremental Search (Cmd+Opt+I), Go To Line (Cmd+L), Find Characters in Range.
- Bookmarks (toggle Cmd+F2, next / previous F2 / Shift+F2, cut / copy / delete bookmarked lines; click the symbol margin to toggle).
- Style All Occurrences of Token, in five colours.

**Macros**
- Start / Stop Recording (Cmd+Shift+R), Playback (Cmd+Shift+P), Run a Macro Multiple Times (N times or to the end of the file), Save with a name and shortcut, Delete.
- Records typing, cursor movement, editing and menu commands; a playback is one undo step. Searches run from the Find dialog are not recorded.
- Stored in `shortcuts.xml` in Notepad++'s format, so macros are interchangeable with Windows. Shortcuts that use Option without Command are not applied (Option+letter types special characters on a Mac).

**Panels** (View > Panels; docked, layout remembered)
- Function List: functions and classes parsed with Notepad++'s own rules (`functionList/*.xml`), search box, A-Z sort, click to jump.
- Folder as Workspace: several roots; open, reveal in Finder, copy path, new file / folder, rename, move to Trash.
- Document List, Document Map (click or drag to scroll), Clipboard History, ANSI Chars.

**Tools and Run**
- MD5, SHA-1, SHA-256, SHA-512: from typed text, from files, or from the selection into the clipboard.
- Run... (F5) with `$(FULL_CURRENT_PATH)`, `$(CURRENT_DIRECTORY)`, `$(FILE_NAME)`, `$(NAME_PART)`, `$(EXT_PART)`, `$(CURRENT_WORD)`, `$(CURRENT_LINE)`, `$(CURRENT_COLUMN)` and a history; open the folder in Finder or Terminal; open the file in the default app, Safari, Chrome or Firefox; search the selection on Google or Wikipedia.

**Settings**
- Preferences (editing, tabs, new-document defaults, session), Style Configurator (pick a theme, edit any language or global style live), Shortcut Mapper.
- 22 Notepad++ themes (View > Theme). A dark theme switches the whole app, including native menus and dialogs, to the dark colour scheme; light themes follow the system appearance.
- Notepad++'s default shortcuts apply with Ctrl mapped to Command and Alt to Option (Quit stays Cmd+Q).
- User Defined Languages: editor (Language > User Defined Language > Define Your Language), import of definition files, list of installed languages.
- Localization (Settings > Localization): Notepad++'s translation files applied to menus, commands and toolbar tooltips.

**Look**
- Notepad++'s toolbar icons (dark or light set, following the theme) and menu order; buttons grey out and toggles stay in sync as in Notepad++.
- Tabs with a coloured bar on the active one, a close button each, and a pencil icon on modified documents.
- Status bar: `doc type | length, lines | Ln, Col, Pos (Sel while text is selected) | EOL | encoding | INS/OVR`.

### Settings and environment variables

Settings live in `~/Library/Application Support/Notepad++` (created on first run, same XML formats as Windows). UI-only state (recent files, window and panel layout) is in `macos-ui.ini` in the same folder; nothing is written to `~/Library/Preferences`.
Environment variables, mostly for development:

| Variable | Effect |
|---|---|
| `NPP_SETTINGS_DIR=<dir>` | Use another profile folder |
| `NPP_THEME=<name>` | Start with that theme (not saved) |
| `NPP_WORKSPACE=<dir>` | Preload a Folder as Workspace root |
| `NPP_DEBUG=1` | Print layout and resource facts to stderr |
| `NPP_SNAPSHOT=<png>` | Save the window as a PNG and exit |
| `NPP_OPEN_ACTION="<menu text>"` | Trigger that menu item shortly after start and keep running |
| `NPP_TRIGGER="<menu text>"` with `NPP_SNAPSHOT_DIALOG=<png>` | Open that item's dialog, save it as a PNG and exit (several items: `A\|B`) |
| `NPP_PRINT_PDF=<pdf>` | Print the first view's document to a PDF and exit |
| `NPP_HASH_DEMO=<text>` | Show the SHA-256 result dialog for that text |

### Headless smoke test (no display needed)

Use a throwaway profile so your real settings are untouched:
```sh
T=$(mktemp -d); mkdir $T/settings
QT_QPA_PLATFORM=offscreen HOME=$T NPP_SETTINGS_DIR=$T/settings NPP_SNAPSHOT=$T/shot.png \
  macos/build/PadPlus.app/Contents/MacOS/PadPlus PowerEditor/src/Utf8.h
```
(`NPP_SNAPSHOT` saves the window to a PNG and closes it, which also writes `session.xml`.)

## Continuous integration and releases

`.github/workflows/macos.yml` runs on every push and pull request to the `macos` branch (GitHub's Apple Silicon runner, Qt from Homebrew):
build, the 11 test programs, `package_app`, `package_check.sh --packaged`, then a disk image, kept as a build artifact.

To publish a release, push a tag named `padplus-v<version>`:

```sh
git tag padplus-v0.1.0 && git push fork padplus-v0.1.0
```

The version in the tag becomes the app's version, and the workflow creates a GitHub release with `PadPlus-<version>-arm64.dmg` and its SHA-256.
The same image can be made locally: `macos/packaging/make_dmg.sh macos/build/PadPlus.app PadPlus.dmg` (after `package_app`).
The app is signed ad hoc and not notarized, so people who download it must allow it once (System Settings > Privacy & Security > Open Anyway).

The upstream Windows workflow (`CI_build.yml`) is set to skip the `macos` branch and the `padplus-v*` tags; it still runs for `master`.

## Test

```sh
for t in core_test bridge_test host_test window_test dialogs_test settings_ui_test panels_test toolbar_test macro_test extras_test more_test; do
  printf '%-12s ' $t; QT_QPA_PLATFORM=offscreen macos/build/$t 2>&1 | tail -1
done
```
Each prints `OK` and exits 0 on success; on failure it prints `FAIL <file>:<line> <expression>`. All of them use temporary `HOME`/settings
folders and delete them, so your real profile is never touched.

| Test | Checks |
|---|---|
| `core_test` | `NppParameters::load()` parses `langs.xml` (96 languages), extension to language, UTF-8/32 and UTF-16 conversion, `Utf8_16` BOM decoding, path joining |
| `bridge_test` | the unmodified core `ScintillaEditView` initialises on a Qt Scintilla widget; `defineDocType(L_CPP)` sets the `cpp` lexer, keyword styles and the theme colours |
| `host_test` | real `FileManager`: UTF-8 / UTF-8 BOM / UTF-16LE BOM load, EOL and language detection, save keeps encoding and EOL, Save As, missing file |
| `window_test` | main window via its actions: open (replaces pristine untitled), already-open switch, dirty pencil icon + `*` window title, status bar fields and selection info, dark/light chrome following the theme, Save, Find/Replace incl. regex back-references and case rules, Clone/Move to other view, Language and EOL menus, `session.xml` written on close |
| `dialogs_test` | `shortcuts.xml` edits persisted (`Key="120"`), preferences applied to the views and written to `config.xml`, a style edit reaching the view and `stylers.xml`, theme switch (Obsidian goes dark) and remembered in `config.xml` |
| `panels_test` | Function List for C++ (class grouping), C and Python (signatures kept), jump to a function, filter, A-Z sort within groups; Document List rows, dirty `*`, click to switch; workspace tree (folders first, lazy expand, refresh), open from the tree, roots + UI state saved in the profile |
| `toolbar_test` | print to PDF (1 page / several pages / empty), MD5-SHA-512 test vectors and menu command, copy-path commands, base zoom, every toolbar button has a Notepad++ icon; menu order; undo/redo/cut/copy/paste/save enabled states follow the editor; buttons run their commands; wrap / all-characters / indent-guide toggles stay in sync; Encoding menu ticks and changes the encoding; Window menu lists documents and switches; Next/Previous Tab; per-tab close buttons; Close All leaves one untitled document; icons follow the theme |
| `extras_test` | bookmarks toggle / next / previous (both wrap) and cut-copy-delete of bookmarked lines; Style All Occurrences marks and clears; incremental search finds, wraps, reports "not found"; Run variables expand and a command runs in the file's folder; monitoring reloads an appended file and follows its end, then stops; Clipboard History order and de-duplication, paste; ANSI table values, insert; Document Map shares the document without changing it; Run menu present; a language-definition XML imports; the change-history margin stays visible (regression: hidden, Scintilla paints modified lines as an orange full-line background) |
| `more_test` | Find dialog Mark All (indicator 31) with Bookmark line, and Clear; Find Characters in Range forwards / backwards / wrapping, selecting whole UTF-8 characters; a menu command recorded as a macro step and replayed, without doubling operations Scintilla records itself; the User Defined Language editor creating a language (XML written, document lexed and coloured), editing it (open document restyled) and removing it; menus and toolbar translated from `french.xml` and restored; 40 tabs scroll instead of widening the window |
| `macro_test` | Start/Stop/Playback buttons and their enabled states; typed text + newline recorded and replayed (one undo step); save with a shortcut, persisted in `shortcuts.xml` (Windows format), run from the menu, delete; the predefined macros and why Option-only shortcuts are skipped; run-to-end hits every line once and cannot loop forever; vertical/horizontal sync with offsets, both directions, off and on, disabled with one view |
| `settings_ui_test` | Find in Files engine (case/whole word/regex/filters/recursion/hidden/binary/UTF-16/bad regex), results dock + jump to hit, Column Editor (numbers, padding, hex, multi-caret text, single undo step), Shortcut Mapper assign/clear re-applied to menu actions, all bindings of a command, Preferences, Style Configurator lists, theme menu |

### Debugging a failing test

```sh
NPP_COMPAT_TRACE=1 macos/build/core_test      # logs file-system calls and failures from the compat layer
lldb macos/build/window_test                  # the core is built with -g
```

## Known gaps

What Notepad++ on Windows has and PadPlus does not (the full reuse / port / left-behind breakdown is in [REUSE_PORT_NEW.md](REUSE_PORT_NEW.md)):

| Missing | Notes |
|---|---|
| Plugins, Plugins Admin | Notepad++ plugins are Windows DLLs and cannot load on macOS |
| Translated dialogs | Settings > Localization translates menus, commands and toolbar tooltips; the contents of dialogs stay English |
| Find / Replace steps in macros | Typing, editing and menu commands are recorded; searches run from the Find dialog are not |
| In-app update check | ? > PadPlus Releases opens the releases page; nothing checks for a newer version |
| Parts of the User Defined Language editor | No per-style font, nesting options or transparent colours (values already in a language file are kept when it is saved) |
| Window list and Task list dialogs | The Window menu and the Document List panel cover them |
| Notarization, Developer ID signature | Releases are signed ad hoc: macOS asks for confirmation on first launch |
| Intel Macs | Apple Silicon (arm64) only |
| Older macOS | A release runs on the macOS version of the machine that packaged it and later (`LSMinimumSystemVersion` in the bundle says which); GitHub-built releases need macOS 15 |

## Where things live

| Path | Purpose |
|---|---|
| `macos/CMakeLists.txt` | the whole macOS build (does not touch `scintilla/` or `lexilla/`) |
| `macos/app/` | the Qt app: `main.cpp`, `main_window.*` (tabs, menus, status bar), `find_dialog.*`, `npp_host.*` (the "Notepad_plus" services the core expects, Qt-free API), `core_boot.*`, `qt_bridge.*` (SendMessage/CreateWindowEx to Qt), `keymap.*` (Windows virtual keys <-> Qt), `shortcut_mapper.*`, `preferences_dialog.*`, `style_dialog.*`, `find_in_files.*`, `column_editor.*`, `panels.*` (Function List, Workspace, Document List), `chrome.*` (tab/status/toolbar look, palettes, icons), `print_doc.*`, `tools.*` (hashes), `macro_ui.*` (save/run dialogs), `extras.*` (Clipboard / ANSI / Document Map panels, incremental search bar), `main_window_extras.cpp` (bookmarks, marks, Run menu, monitoring, UDL menu, macro commands, localization), `udl_dialog.*` (User Defined Language editor) |
| `macos/compat/` | Win32 compatibility layer: `win_impl.cpp` (real), `win_consts.h` / `win_types.h` (real values), `win_auto.h` (no-op stubs), `ui_stubs.cpp` (UI class stand-ins) |
| `macos/shim/windows.h` | tiny shim so Lexilla's `LexUser.cxx` compiles |
| `macos/packaging/` | `Info.plist.in`, the app icon with its sources (`icon/`), and the scripts `package_app` runs (licence collection, install-name fix-up, minimum macOS, disk image) |
| `macos/docs/` | the screenshot and icon image used by the main README |
| `macos/tests/` | the eleven tests above and `package_check.sh` (their resources are staged once into `build/` by the `test_resources` target) |
| `macos/build/` | build output (git-ignored) |

`macos/tests/package_check.sh [app]` (without `--packaged`) checks the plain bundle: `Info.plist`, icon, resources, and a headless launch that must find
its resources in `Contents/Resources` and load all 22 toolbar icons. `window_test` also covers the Finder file-open event.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Could not find Qt6` | Pass `-DCMAKE_PREFIX_PATH="$(brew --prefix)"` |
| `ninja: command not found` | `brew install ninja` (or drop `-G Ninja` to use Make) |
| `WidgetGen.py failed` at configure | Needs `python3` on `PATH`; rerun CMake after fixing |
| Compile error `undeclared identifier 'SomeWin32Name'` in a shared core file | Add it to `compat/win_consts.h` (if its value matters), `compat/win_impl.{h,cpp}` (if behaviour matters), or `compat/win_auto.h` (placeholder or no-op) |
| `Undefined symbols ... SomeUiClass::...` at link | Add a stand-in to `compat/ui_stubs.cpp` |
| Editing `Common.cpp`, `Parameters.cpp`, `Buffer.cpp`, etc. produces a huge diff | They use **CRLF** line endings; edit byte-exact (the Edit tool is fine, text-mode Python rewrites are not) |
| `NotepadPP` window doesn't appear over SSH | Use the headless snapshot above |
| Tab bar missing / zero height | `QTabBar` can be laid out before it has tabs; the window sets a minimum height (see `main_window.cpp`) |
