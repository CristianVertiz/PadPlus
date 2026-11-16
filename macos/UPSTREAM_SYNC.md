# Keeping PadPlus in step with Notepad++

PadPlus compiles the Notepad++ core, Scintilla and Lexilla as they are, so most upstream fixes arrive for free, provided upstream is merged
regularly. This document says how, when, and what to do when a merge does not go cleanly.

## The two branches

| Branch | What it is | Rule |
|---|---|---|
| `master` | A mirror of [notepad-plus-plus/notepad-plus-plus](https://github.com/notepad-plus-plus/notepad-plus-plus) `master` | Never commit to it. It only ever fast-forwards to upstream. |
| `macos` | PadPlus: upstream plus the `macos/` folder and a few edits to upstream files | All work happens here, through pull requests. |

Upstream changes always travel `upstream master` → `master` → `macos`, by **merge**. Never rebase `macos` onto `master` and never squash a
sync: git must remember which upstream commits are already in `macos`, or every later sync shows the same conflicts again.

## When to sync

- **After each Notepad++ release** (upstream tags them `v8.x.y`). This is the main rhythm.
- **Before each PadPlus release**, if upstream has moved since the last sync.
- **At least once a month** otherwise. Small syncs are cheap; a sync after six months is a project.

## The procedure

The remotes on a working clone: `origin` is upstream Notepad++ (fetch only), `fork` is `CristianVertiz/PadPlus`.

```sh
# 1. Mirror upstream into master
git fetch origin
git fetch . origin/master:master            # fast-forward local master (fails if master was ever committed to: fix that first)
git push fork master                        # mirror on GitHub; optional, see "Notes" below

# 2. Merge into a sync branch
git switch macos && git pull --ff-only fork macos
git switch -c sync/upstream-$(date +%Y-%m-%d)
git merge master                            # resolve conflicts as described below, then: git commit

# 3. Check it
cmake --build macos/build
for t in core_test bridge_test host_test window_test dialogs_test settings_ui_test panels_test toolbar_test macro_test extras_test more_test; do
  printf '%-18s' $t; QT_QPA_PLATFORM=offscreen macos/build/$t 2>&1 | tail -1
done
cmake --build macos/build --target package_app && macos/tests/package_check.sh macos/build/PadPlus.app --packaged

# 4. Pull request against macos
git push -u fork HEAD
gh pr create -R CristianVertiz/PadPlus --base macos --title "Sync with Notepad++ <version or commit>"
```

**Merge the pull request with "Create a merge commit".** Not "Squash and merge", not "Rebase and merge": both throw away the record of
what was merged.

Before reading the diff, skim what upstream changed: `git log --oneline macos..master` and, for a release, its change log. Step 5 below
depends on it.

## Resolving conflicts

Only files that both sides changed can conflict. PadPlus changes these upstream files and nothing else outside `macos/`:

| Files | What PadPlus did | On conflict |
|---|---|---|
| `README.md`, `CONTRIBUTING.md`, `AGENTS.md`, `.github/ISSUE_TEMPLATE/*`, `.github/pull_request_template.md` | Replaced with PadPlus's own | **Keep ours:** `git checkout --ours <file> && git add <file>`. Read upstream's change anyway; adopt an idea by hand if it applies to PadPlus. |
| `.github/workflows/CI_build.yml` | Only the trigger block at the top (Windows jobs run for `master` only) | Take upstream's file, then put our trigger block and its dated comment back. |
| `PowerEditor/src/MISC/Common/Common.cpp`, `Common.h`, `Parameters.cpp`, `ScintillaComponent/Buffer.cpp`, `WinControls/FunctionList/functionParser.cpp` | A native path separator (`NPP_SEP`) in place of a hard-coded backslash | Take upstream's version of the conflicting lines, then re-apply the separator edit. Check new upstream code in these files for new hard-coded `\\` in paths. These files use **CRLF**: edit byte-exact. |
| `PowerEditor/src/pugixml/pugiconfig.hpp` | `PUGIXML_CHARCONV_FLOAT` is not defined on Apple toolchains | Take upstream's file (pugixml updates replace it), re-apply the three-line guard. |
| `scintilla/`, `lexilla/` | Nothing, ever | Cannot conflict. If they do, something was edited by mistake: take upstream's. |

Every edited upstream file carries a comment `Modified in <month year> for PadPlus (macOS): ...` under its licence header. Keep it, and
`grep -rn "for PadPlus" PowerEditor .github` lists them all. If a sync forces an edit to a new upstream file, add the same comment and add
the file to the table above and to [REUSE_PORT_NEW.md](REUSE_PORT_NEW.md).

Files upstream **deletes or renames** that PadPlus edits: follow upstream, then move the edit to wherever the code went.

## After a clean merge: what can still break

A merge without conflicts is not yet a working build. In order of likelihood:

1. **A compile error `undeclared identifier 'SomeWin32Name'`** in a core file: upstream started using a Win32 function, type or constant
   the compatibility layer does not have. Add it to `macos/compat/` (see "Troubleshooting" in the [developer guide](README.md)).
   A function whose result matters needs a real implementation in `win_impl.h` **and** a `#define NPP_HAVE_<name>`, or the catch-all stub
   in `win_auto.h` silently wins and returns 0.
2. **A link error `Undefined symbols`**: a compiled core file now calls into a file PadPlus does not compile. Either    the `npp_core` library in `macos/CMakeLists.txt` (if it is UI-free) or add a stand-in to `macos/compat/ui_stubs.cpp`.
3. **A test fails**: upstream changed behaviour the tests pin down (a default setting, a file format, a menu command id). Decide whether the
   test or the Qt code has to follow.
4. **Nothing fails, but a setting does nothing**: upstream added a preference or a command that the core now reads and the Qt interface
   does not expose. See the next section.

Data files need no attention: themes, `langs.model.xml`, `stylers.model.xml`, function-list rules and translations are copied from the
upstream tree at build time, so new languages, themes and translations appear by themselves.

## 5. Changes that do not arrive by merging

About three quarters of Notepad++ is its Windows user interface, which PadPlus does not compile. A fix there does nothing for PadPlus;
if the Qt side has the same bug or lacks the same feature, it has to be done again by hand. After each sync, go through upstream's changes
to these areas and ask whether the Qt counterpart needs the same:

| Upstream (not compiled) | PadPlus counterpart |
|---|---|
| `Notepad_plus*.cpp`, `NppCommands.cpp`, `NppIO.cpp`, `NppNotification.cpp`, `NppBigSwitch.cpp` | `macos/app/npp_host.*`, `main_window.*`, `main_window_extras.cpp` |
| `ScintillaComponent/FindReplaceDlg.*`, `GoToLineDlg.*`, `columnEditor.*` | `find_dialog.*`, `find_in_files.*`, `column_editor.*` |
| `WinControls/TabBar`, `DocTabView`, `StatusBar`, `ToolBar` | `main_window.*`, `chrome.*` |
| `WinControls/Preference`, `WinControls/ColourPicker`, `WordStyleDlg`, `shortcut/` (the mapper dialog) | `preferences_dialog.*`, `style_dialog.*`, `shortcut_mapper.*` |
| `WinControls/FunctionList` (the panel), `FileBrowser`, `VerticalFileSwitcher`, `DocumentMap`, `ClipboardHistory`, `AnsiCharPanel` | `panels.*`, `extras.*` |
| `ScintillaComponent/UserDefineDialog.*` (the dialog) | `udl_dialog.*` |
| `MISC/md5`, `sha*` dialogs, `Printer.cpp`, `WinControls/StaticDialog/RunDlg` | `tools.*`, `print_doc.*`, `main_window_extras.cpp` |
| `menuCmdID.h` (new `IDM_*` commands), `Notepad_plus.rc` (menu layout) | the menus in `main_window.cpp`; new ids also matter for macros and shortcuts |
| `NppDarkMode.cpp`, `DarkMode/` | `chrome.*` |

Record what was looked at in the pull request: "upstream fixed X in the tab bar; not applicable" is a useful line next year.

## Notes

- **Do not use GitHub's "Sync fork" button on `macos`.** It is fine on `master`, where it does exactly step 1.
- **Pushing `master` to the fork starts upstream's Windows workflow there**, because the workflow file on `master` is upstream's own.
  It is harmless and free on a public repository; disable the "CI_build" workflow in the fork's Actions tab to avoid it.
- **Tags:** upstream's `v*` tags come along with `git fetch origin`. PadPlus's own tags are `padplus-v*`; do not push upstream's tags to
  the fork unless you want them listed there.
- **A sync is a good moment for a release**, once the pull request is merged and green: `git tag padplus-v<version> && git push fork padplus-v<version>`.

## Sync log

Add a line per sync, newest first.

| Date | Upstream at | Conflicts | Follow-up on the Qt side |
|---|---|---|---|
| 2026-11-16 | `a3b459aa5` (after v8.9.9.1) | `AGENTS.md` (kept ours) | None needed: upstream's changes were DPI work in the Windows tab bar, Document List and Character panel, and command-line parsing in `winmain.cpp`; none has a Qt counterpart with the same problem |
| 2026-11-12 | `066c93ac0` (v8.9.9.1) | None | None: translations, themes, version bump |
| 2026-11-12 | `7fcd5b5a9` | None | None: translations, one Scintilla paste fix that arrives by itself |
