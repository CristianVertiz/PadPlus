# Third-party notices

PadPlus is a macOS editor based on the source code of [Notepad++](https://notepad-plus-plus.org/). It is not affiliated with or endorsed by the Notepad++ project.
PadPlus as a whole is distributed under the **GNU General Public License v3** (see [`LICENSE`](../LICENSE)), the licence of Notepad++, from which it is derived.
The files under `macos/` are the port's own additions and are released under the same licence.

What is reused, ported and new is described in [`REUSE_PORT_NEW.md`](REUSE_PORT_NEW.md).

## Source code in this repository

| Component | Where | Licence | Copyright |
|---|---|---|---|
| Notepad++ | `PowerEditor/` | GPL v3, [`LICENSE`](../LICENSE) | Don Ho and the Notepad++ contributors |
| Scintilla | `scintilla/` | Scintilla licence, [`scintilla/License.txt`](../scintilla/License.txt) | Neil Hodgson |
| Lexilla | `lexilla/` | Lexilla licence, [`lexilla/License.txt`](../lexilla/License.txt) | Neil Hodgson |
| Boost.Regex (standalone headers) | `boostregex/boost/` | Boost Software License 1.0 (stated in each file; text in `macos/packaging/licenses/`) | John Maddock and the Boost authors |
| pugixml | `PowerEditor/src/pugixml/` | MIT, [`LICENSE.md`](../PowerEditor/src/pugixml/LICENSE.md) | Arseny Kapoulkine |
| uchardet | `PowerEditor/src/uchardet/` | MPL 1.1 / GPL 2.0 or later / LGPL 2.1 or later (stated in each file); used here under the GPL option | Mozilla and the uchardet authors |

`scintilla/` and `lexilla/` are unmodified. Five files under `PowerEditor/src` carry small portability edits; the git history shows them exactly.

Other material shipped from the Notepad++ tree is used as distributed by Notepad++ under its licence (GPL v3):

- **Toolbar icons** (`PowerEditor/src/icons/{dark,light}/toolbar`): the "Fluent UI" icon sets that Notepad++ added in 2021 (upstream commits
  `7d2539424` and `db27172ca`, issues #9742, #9808 and #9831). The repository records no separate licence or designer for them, so they are taken
  as part of Notepad++.
- **Themes** (`PowerEditor/installer/themes`, 22 files): each file's header comment names its author and terms. Bespin, Black board, Choco,
  DansLeRuSH-Dark, Mono Industrial, Monokai, Plastic Code Wrap and Twilight carry an MIT-style permission notice; DarkModeDefault states GPL 2;
  Deep Black, HotFudgeSundae, khaki, MossyLawn, Navajo, Solarized, Solarized-light, Zenburn, Ruby Blue, Vibrant Ink and Obsidian name their
  authors and the colour scheme they derive from, most with a "free to modify and re-release" statement; Hello Kitty and vim Dark Blue have no
  header. The files are shipped unmodified, headers included.
- **Language, function-list and translation definitions** (`langs.model.xml`, `stylers.model.xml`, `installer/functionList`,
  `installer/nativeLang`): part of Notepad++; the translation files name their translators in the header.

## Additional software inside a packaged `PadPlus.app`

The `package_app` build target copies these into the bundle. They are not in this repository; they come from the machine that builds the package
(Homebrew). The same target runs `packaging/collect_licenses.sh`, which puts the GPL v3, the Scintilla and Lexilla licences, this file and each
library's own licence files into `Contents/Resources/licenses`; `tests/package_check.sh --packaged` checks they are there.

Licences as declared by Homebrew for the versions built here (November 2026). All of them permit distribution together with a GPL v3 program:

| Library (Homebrew formula) | Licence |
|---|---|
| Qt 6: QtCore, QtGui, QtWidgets, QtPrintSupport, QtDBus, cocoa / macstyle / ico plugins (`qtbase`), QtCore5Compat (`qt5compat`) | LGPL 3.0 or GPL 2.0 or GPL 3.0 (used under GPL 3.0), with BSD parts |
| ICU (`icu4c`) | Unicode-3.0 |
| GLib (`glib`) | LGPL 2.1 or later |
| gettext runtime, libintl (`gettext`) | LGPL 2.1 or later (the formula also contains GPL 3.0 or later tools, not embedded) |
| D-Bus (`dbus`) | AFL 2.1 or GPL 2.0 or later |
| OpenSSL (`openssl`) | Apache 2.0 |
| FreeType (`freetype`) | FreeType License |
| HarfBuzz (`harfbuzz`), brotli, md4c | MIT |
| Graphite (`graphite2`) | MIT or MPL 2.0 or LGPL 2.1 or later or GPL 2.0 or later |
| PCRE2, double-conversion, WebP (`webp`) | BSD 3-Clause |
| zstd | BSD 3-Clause or GPL 2.0, with BSD 2-Clause and MIT parts |
| xz, liblzma | 0BSD (the formula also contains GPL 2.0 or later tools, not embedded) |
| libjpeg-turbo (`jpeg-turbo`) | IJG, zlib and BSD 3-Clause |
| libpng | libpng-2.0 |
| libtiff | libtiff |
| libb2 | CC0 1.0 |

This is a reading of the declared licences, not legal advice. The exact set follows the Qt build on the packaging machine: after upgrading
Homebrew, check `ls PadPlus.app/Contents/Frameworks` and `Contents/Resources/licenses` again. Sharing only the source avoids redistributing them.

Where the source code of each embedded library is, in the exact version of a given build, is listed in `Contents/Resources/licenses/SOURCES.md`
inside the app and attached to each GitHub release.

## Names and marks

"Notepad++" and its logo belong to the Notepad++ project. PadPlus uses its own name and its own icon, and refers to Notepad++ only to
describe where the code comes from.

## AI assistance

The macOS port (everything under `macos/` and the edits to the upstream files, each marked with a dated comment) was written with AI assistance (Claude Code, Anthropic).
