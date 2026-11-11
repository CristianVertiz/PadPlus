#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# Verifies PadPlus.app as a distributable bundle.
#   tests/package_check.sh [path/to/PadPlus.app]            checks the bundle (Info.plist, icon, resources, runs headless)
#   tests/package_check.sh path/to/PadPlus.app --packaged   also requires it to be self-contained (no Homebrew paths), signed, and runs it from another folder
APP="${1:-$(dirname "$0")/../build/PadPlus.app}"; MODE="${2:-}"
fails=0
ok()   { echo "ok    $*"; }
bad()  { echo "FAIL  $*"; fails=$((fails + 1)); }
check(){ if eval "$2"; then ok "$1"; else bad "$1"; fi; }

C="$APP/Contents"
check "Info.plist is valid"                     'plutil -lint "$C/Info.plist" >/dev/null'
check "executable name and bundle id"           '[ "$(/usr/libexec/PlistBuddy -c "Print :CFBundleExecutable" "$C/Info.plist")" = PadPlus ] && [ "$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$C/Info.plist")" = io.github.cristianvertiz.padplus ]'
check "document types declared (Finder Open With)" '/usr/libexec/PlistBuddy -c "Print :CFBundleDocumentTypes:0:LSItemContentTypes:0" "$C/Info.plist" >/dev/null'
check "icon file present and referenced"        '[ -f "$C/Resources/$(/usr/libexec/PlistBuddy -c "Print :CFBundleIconFile" "$C/Info.plist")" ]'
check "the Notepad++ logo is not shipped"         '[ ! -e "$C/Resources/icons/npp.ico" ]'
for r in langs.model.xml stylers.model.xml themes/Obsidian.xml functionList/cpp.xml localization/french.xml icons/dark/toolbar/regular/new_off.ico icons/light/toolbar/regular/new_off.ico; do
  check "resource $r"                           '[ -f "$C/Resources/$r" ]'
done

RUN="$APP"
if [ "$MODE" = "--packaged" ]; then
  check "no Homebrew paths in the executable"   '! otool -L "$C/MacOS/PadPlus" | grep -q /opt/homebrew'
  check "Qt frameworks embedded (incl. printing)" '[ -d "$C/Frameworks/QtCore.framework" ] && [ -d "$C/Frameworks/QtWidgets.framework" ] && [ -d "$C/Frameworks/QtPrintSupport.framework" ]'
  check "plugins embedded (cocoa, ico icons)"   '[ -f "$C/PlugIns/platforms/libqcocoa.dylib" ] && [ -f "$C/PlugIns/imageformats/libqico.dylib" ]'
  check "no Homebrew paths in embedded libraries" '! find "$C/Frameworks" "$C/PlugIns" -type f \( -name "*.dylib" -o -path "*.framework/Versions/A/Qt*" \) -print0 | xargs -0 otool -L 2>/dev/null | grep -q /opt/homebrew'
  for f in $(find "$C/Frameworks" "$C/PlugIns" -type f \( -name "*.dylib" -o -path "*.framework/Versions/A/Qt*" \)); do otool -L "$f" 2>/dev/null | grep /opt/homebrew | sed "s#^#      ${f#$C/} ->#"; done   # which ones, when the check above fails
  check "no reference to a missing embedded framework" 'for f in "$C/MacOS/PadPlus" "$C"/Frameworks/*.framework/Versions/A/Qt* "$C"/PlugIns/*/*.dylib; do otool -L "$f" 2>/dev/null | grep -o "@\(executable_path/\.\./Frameworks\|rpath\)/Qt[A-Za-z0-9]*\.framework" | sed -E "s#.*/##" | while read fw; do [ -d "$C/Frameworks/$fw" ] || { echo "$f -> $fw"; exit 1; }; done || exit 1; done'
  check "licence texts embedded"                 '[ -f "$C/Resources/licenses/LICENSE" ] && [ -f "$C/Resources/licenses/Scintilla-License.txt" ] && [ -d "$C/Resources/licenses/glib" ] && [ -f "$C/Resources/licenses/SOURCES.md" ] && [ -f "$C/Resources/licenses/Boost-LICENSE_1_0.txt" ]'
  check "declared minimum macOS matches the embedded Qt" '[ "$(/usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$C/Info.plist")" = "$(otool -l "$C/Frameworks/QtCore.framework/Versions/A/QtCore" | awk "/minos/{print \$2; exit}")" ] || [ "$(/usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$C/Info.plist")" = "$(otool -l "$C/MacOS/PadPlus" | awk "/minos/{print \$2; exit}")" ]'
  check "ad-hoc signature verifies"             'codesign --verify --deep --strict "$APP" 2>/dev/null'
  TMP="$(mktemp -d)"; cp -R "$APP" "$TMP/PadPlus.app"; RUN="$TMP/PadPlus.app"   # run from somewhere else: nothing may depend on the build folder
fi

# headless launch with a throw-away profile
T="$(mktemp -d)"; mkdir "$T/settings"; printf 'int main() { return 0; }\n' > "$T/a.cpp"
# dev bundles run on the headless test platform; a packaged bundle only embeds the real macOS platform plugin, so it opens (and closes) a real window
PLATFORM=offscreen; [ "$MODE" = "--packaged" ] && PLATFORM=cocoa
OUT="$(QT_QPA_PLATFORM=$PLATFORM HOME="$T" NPP_SETTINGS_DIR="$T/settings" NPP_DEBUG=1 NPP_SNAPSHOT="$T/s.png" "$RUN/Contents/MacOS/PadPlus" "$T/a.cpp" 2>&1)"
check "app starts and exits cleanly"            '[ -f "$T/s.png" ]'
check "core found its resources in Contents/Resources" 'echo "$OUT" | grep -q "nppPath=.*PadPlus.app/Contents/Resources langs=9[0-9] themes=2[0-9]"'
check "all toolbar icons load"                  'echo "$OUT" | grep -Eq "toolbar icons ([0-9]+)/\1 "'
check "profile created from the bundled models" '[ -f "$T/settings/langs.xml" ] && [ -f "$T/settings/stylers.xml" ]'
rm -rf "$T" ${TMP:+"$TMP"}

[ $fails -eq 0 ] && echo "OK" || { echo "FAILED ($fails)"; exit 1; }
