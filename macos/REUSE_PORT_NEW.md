# PadPlus: what is reused, what is ported, what is new

PadPlus is a Qt 6 front end (arm64 macOS) over the real Notepad++ source. This document sorts every piece into four groups:

| Group | Meaning |
|---|---|
| **Reused** | Upstream source compiled as it is. Behaviour is identical to Windows because it is the same code. |
| **Reused, tiny edits** | Upstream source with a handful of changed lines. |
| **Ported** | A Windows feature that exists in Notepad++ and was re-created for macOS (Win32 call replaced by a POSIX or Qt equivalent, or the Windows UI rebuilt in Qt). "Ported" means the same behaviour, not a line-by-line translation. |
| **New** | Has no Notepad++ counterpart: it only exists because this is a Mac app. |

Plus a fifth list: what was **left behind**.

Line counts are approximate (`wc -l`). Paths are relative to the repository root.

## At a glance

| | Lines | Share of the shipped code |
|---|---|---|
| Reused unchanged: Scintilla (core 33.7k, Qt backend 3.6k), Lexilla (76.5k), Boost.Regex glue (0.7k), Notepad++ core (41.2k, of which pugixml 13.8k, vendored) + uchardet (4.8k) | about 160k | the large majority |
| Reused with edits (5 upstream files) | 24 added / 16 removed | negligible |
| Win32 compatibility layer (`macos/compat/`) | 2.3k (1.25k of it hand-kept stub tables) | small |
| Qt app: ported features and new features (`macos/app/`) | 3.4k | small |
| Tests (`macos/tests/`, 11 programs) | 1.4k | not shipped |

Notepad++ itself is about 159k lines in `PowerEditor/src`. Only the non-UI core (about 41k) is compiled. The rest is the Windows UI layer (`Notepad_plus*.cpp`, `NppBigSwitch.cpp`, `WinControls/`, and so on), which is what the Qt app replaces.

---

## 1. Reused (compiled unmodified)

### Scintilla and Lexilla (untouched, as required)
- `scintilla/src/*` (editor engine) and `scintilla/qt/ScintillaEditBase/*` (Scintilla's own Qt backend).
- `scintilla/qt/ScintillaEdit` wrapper: generated at build time by Scintilla's own `WidgetGen.py` from its template. The build patches a *copy* of the template (not the repository file) to the 64-bit `*FULL` messages.
- `lexilla/` all lexers and `lexlib`, including `LexUser.cxx` (user defined languages), compiled with an empty `windows.h` shim.
- `boostregex/` (Notepad++'s regex search glue) with standalone Boost.Regex.

### Notepad++ core (`PowerEditor/src`)
| File | What PadPlus gets from it |
|---|---|
| `Parameters.cpp/.h` | Reads and writes `config.xml`, `session.xml`, `shortcuts.xml` (shortcuts and macros), `stylers.xml`, themes, language definitions, UDL loading and import |
| `ScintillaComponent/Buffer.cpp/.h` (`FileManager`, `Buffer`) | Loading and saving files, encoding detection and conversion, EOL handling, language detection, undo state, session buffers |
| `ScintillaComponent/ScintillaEditView.cpp/.h` | All Scintilla set-up: margins, bookmark and fold markers, change history, lexer and style application, indent guides, zoom, wrap, macro recording hooks, tab handling |
| `WinControls/shortcut/shortcut.cpp` | The shortcut and macro data model (`Shortcut`, `MacroShortcut`, `recordedMacroStep`) and macro playback |
| `WinControls/FunctionList/functionParser.cpp` | The Function List regex parsers (rules in `functionList/*.xml`) |
| `ScintillaComponent/UserDefineDialog.cpp` | User defined language data structures and import |
| `Utf8_16.cpp`, `EncodingMapper.cpp`, `uchardet/` | Encoding conversion and detection |
| `localization.cpp` | Initialised so the core runs (UI strings in the Qt app are English) |
| `MISC/Common/*`, `dpiManagerV2.cpp`, `MISC/sha2`, `pugixml/` | Utilities and XML |

### Data files used as they are
`langs.model.xml`, `stylers.model.xml`, the 22 themes in `PowerEditor/installer/themes`, the 48 `functionList/*.xml` parser definitions, and Notepad++'s own toolbar icons (`icons/{dark,light}/toolbar/regular/*.ico`, loaded through Qt's `.ico` plugin). The settings keep Windows' XML formats, so profile files can be copied between Windows and macOS.

---

## 2. Reused with tiny edits

Five upstream files changed, 24 lines added and 16 removed in total, all CRLF-preserving:

| File | Edit |
|---|---|
| `MISC/Common/Common.cpp`, `Common.h` | A native path separator constant (`NPP_SEP`) in place of a hard-coded backslash |
| `Parameters.cpp` | The same separator change, in the profile and UDL path building |
| `ScintillaComponent/Buffer.cpp` | One separator change |
| `WinControls/FunctionList/functionParser.cpp` | Separator and one `std::filesystem::path` `.wstring()` conversion |

---

## 3. Ported (a Notepad++ feature, rebuilt for macOS)

### Platform layer: Win32 replaced by POSIX (`macos/compat/`)
The core still calls Win32 APIs. These are provided with real implementations where behaviour matters:
- **Files and paths:** `CreateFile`/`ReadFile`/`WriteFile`/`CopyFile`/`CreateDirectory` (creates parents), `FindFirstFile`, `SHGetFolderPath` (to `~/Library/Application Support`), `GetModuleFileNameW` (redirected to `Contents/Resources` inside a bundle).
- **Strings and code pages:** `MultiByteToWideChar`/`WideCharToMultiByte` (wchar_t is UTF-32 here; "ANSI" means UTF-8), the CRT "safe" functions, `lstrcpynA` and the other ANSI helpers.
- **Threads:** `CreateThread` runs synchronously.
- **Messaging:** `SendMessage` and `CreateWindowEx` route through hooks to Qt (`qt_bridge`), so an `HWND` for a Scintilla view is a pointer to a Qt `ScintillaEdit` and the unmodified `ScintillaEditView` drives it.
- **Everything else the core merely mentions:** Win32 types and constants and no-op stubs (`win_types.h`, `win_consts.h`, `win_auto.h`, `ui_stubs.cpp`).

### The application controller (`Notepad_plus`, `NppIO`, `NppCommands`, `NppNotification`) to `NppHost`
`macos/app/npp_host.*` plays the role of `Notepad_plus` for the core: two views and a scratch view, open / save / close, session load and save (including workspace roots), shortcut, preference, style and theme services, language and EOL and encoding changes, function list, macros (record, play, save, run to end), file reload.

### Windows UI rebuilt in Qt (`macos/app/`)
| Notepad++ feature | PadPlus |
|---|---|
| Main window, two views, tab bars, status bar, toolbar | `main_window.*`, `chrome.*`; Notepad++ icons, orange or mint active-tab bar |
| Menus and the shortcut system | Menus follow the Notepad++ order; `keymap.*` converts Windows virtual keys to Qt keys (Ctrl becomes Cmd, Alt becomes Option) |
| Find / Replace, Find in Files, Go To Line, Column Editor | `find_dialog.*`, `find_in_files.*`, `column_editor.*` (search uses Scintilla and the Boost.Regex glue) |
| Preferences, Style Configurator, Shortcut Mapper | `preferences_dialog.*`, `style_dialog.*`, `shortcut_mapper.*`, with themes |
| Dark mode (`DarkMode/`, `NppDarkMode.cpp`) | A theme's darkness selects Qt's colour scheme plus style sheets |
| Function List, Folder as Workspace (`FileBrowser`), Document List (`VerticalFileSwitcher`) | `panels.*` |
| Clipboard History, ANSI Chars, Document Map | `extras.*` |
| Incremental Search, Bookmarks, Mark (Style All Occurrences) | `extras.*`, `main_window_extras.cpp` |
| Run menu (`RunDlg`), Monitoring (tail -f), UDL import and list | `main_window_extras.cpp` |
| User Defined Language editor (`UserDefineDialog`) | `udl_dialog.*`: one dialog with tabs for keywords, comments and numbers, folding, delimiters and styles. It edits the language's XML file and has the core re-import it |
| Mark tab of the Find dialog, Find Characters in Range | `find_dialog.*` (Mark All / Clear All Marks / Bookmark line sit in the one Find dialog) |
| Translations (`nativeLang/*.xml`) | Settings > Localization applies a Notepad++ translation file to the menus, commands and toolbar tooltips (`main_window_extras.cpp`) |
| Macro menu, Save Current Macro, Run a Macro Multiple Times, menu commands recorded in macros | `macro_ui.*`, `main_window.cpp`, `main_window_extras.cpp` |
| Synchronized scrolling, Window menu, Encoding menu, tab context | `main_window.cpp` |
| Printing, MD5 / SHA-1 / SHA-256 / SHA-512 | `print_doc.*` (QPrinter, black on white), `tools.*` (QCryptographicHash) |
| Open recent files, session restore | `main_window.cpp`, `npp_host.*` |

---

## 4. New (no Notepad++ counterpart)

- **The macOS app bundle:** `Info.plist` template, document types for Finder "Open With", its own app icon (`packaging/icon/`), `QFileOpenEvent` handling for Finder and `open -a`.
- **Packaging:** the `package_app` target (macdeployqt, pruning unused Qt plugins, a dangling-framework check, ad-hoc code signing) and `tests/package_check.sh`, which verifies the bundle is self-contained and relocatable.
- **The build system:** a CMake build for the whole stack (Windows uses MSVC project files and makefiles).
- **The Qt bridge** (`qt_bridge.*`, `core_boot.*`, `compat/host_hooks.h`): the glue that lets the Win32-shaped core drive Qt widgets.
- **macOS-specific behaviour:**
  - Option-only shortcuts for saved macros are skipped (they would hijack Option+letter characters).
  - Run menu items for Finder, Terminal, Safari, Chrome and Firefox.
  - Reveal in Finder, Move to Trash and Copy Path in the workspace panel.
  - UI-only state (recent files, window and panel layout) lives in `macos-ui.ini`, never in `~/Library/Preferences`.
  - The tab bars use Qt's Fusion style, so the style sheet (not the native macOS style) paints them, including the scroll buttons.
  - Licence texts of everything embedded are collected into the packaged bundle (`packaging/collect_licenses.sh`).
- **A fix for a latent gap:** the change-history margin is shown at start-up (without it, Scintilla paints modified lines as a full-line background).
- **The test suite and dev hooks:** 11 headless test programs, plus `NPP_SETTINGS_DIR`, `NPP_SNAPSHOT`, `NPP_THEME`, `NPP_OPEN_ACTION`, `NPP_TRIGGER`, `NPP_SNAPSHOT_DIALOG`, `NPP_PRINT_PDF`, `NPP_HASH_DEMO`, `NPP_WORKSPACE`, `NPP_DEBUG`.

---

## 5. Left behind

| What | Why |
|---|---|
| Plugin system (`PluginsManager`, `PluginsAdmin`, Plugins menu) | Plugins are Windows DLLs |
| `Notepad_plus_Window`, `winmain`, docking framework, splitter container, `DoubleBuffer` | Replaced by Qt widgets and layouts |
| Installer (NSIS, MSI), auto-update, tray icon, shell registration (`RegExt`) | Windows deployment |
| `DarkMode/` IAT hooks, UAH menu painting | Windows-specific; macOS handles it natively |
| Window list and Task list dialogs | Not rebuilt: the Window menu and Document List cover them |
| Translated dialogs | Menus, commands and toolbar tooltips are translated; dialog contents stay English (the Qt dialogs do not share Windows' control ids) |
| Parts of the UDL editor | No per-style font or nesting options, no "transparent" colours, no docking; existing values for these are kept when a language is saved |
| Find / Replace steps in macros | Typing, editing and menu commands are recorded; searches run from the Find dialog are not |

## Not part of the port
Notarization, a DMG, Developer ID signing and auto-update are not done (the app is ad-hoc signed for local use).
