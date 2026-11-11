#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# Puts the licence texts a redistributed PadPlus.app must carry into Contents/Resources/licenses:
# the GPL v3 (PadPlus, Notepad++; Qt is used under its GPL v3 option), Scintilla, Lexilla, the notices file,
# and the licence files of every Homebrew library macdeployqt embedded (found through the Cellar, so the list follows the bundle).
#   collect_licenses.sh path/to/PadPlus.app path/to/repository
app="$1"; root="$2"; out="$app/Contents/Resources/licenses"
rm -rf "$out"; mkdir -p "$out"
cp "$root/LICENSE" "$out/LICENSE"
cp "$root/scintilla/License.txt" "$out/Scintilla-License.txt"
cp "$root/lexilla/License.txt" "$out/Lexilla-License.txt"
cp "$root/macos/THIRD_PARTY_NOTICES.md" "$out/"
cp "$root/macos/packaging/licenses/Boost-LICENSE_1_0.txt" "$out/"
cp "$root/PowerEditor/src/pugixml/LICENSE.md" "$out/pugixml-LICENSE.md"
cellar="$(brew --cellar 2>/dev/null)"; [ -d "$cellar" ] || exit 0
missing=""
for lib in "$app"/Contents/Frameworks/*; do
  hit="$(find "$cellar" -maxdepth 4 -name "$(basename "$lib")" | head -1)"      # <cellar>/<formula>/<version>/lib/<name>
  [ -n "$hit" ] || continue
  keg="$(echo "$hit" | cut -d/ -f1-$(( $(echo "$cellar" | tr -cd / | wc -c) + 3 )))"
  formula="$(basename "$(dirname "$keg")")"
  [ -d "$out/$formula" ] && continue
  mkdir "$out/$formula"
  find "$keg" -maxdepth 1 -type f \( -iname 'LICEN[CS]E*' -o -iname 'COPYING*' -o -iname 'COPYRIGHT*' -o -iname 'NOTICE*' -o -iname '*GPL*' -o -iname 'UNLICENSE*' \) -exec cp {} "$out/$formula/" \;
  [ -n "$(ls "$out/$formula")" ] || { rmdir "$out/$formula"; missing="$missing $formula"; }
done
# SOURCES.md: where the source of every embedded library is (the GPL counts libraries shipped with the app as part of its source)
formulas="$(for lib in "$app"/Contents/Frameworks/*; do find "$cellar" -maxdepth 4 -name "$(basename "$lib")" | head -1 | sed "s#^$cellar/##" | cut -d/ -f1; done | sort -u)"
{
  echo "# Source code of what is inside PadPlus.app"
  echo
  echo "PadPlus itself, with the Notepad++, Scintilla, Lexilla, Boost.Regex, pugixml and uchardet code it is built from:"
  echo "https://github.com/CristianVertiz/PadPlus (the tag of this release)."
  echo
  echo "Libraries embedded from Homebrew, in the versions used for this build. Homebrew's build recipes are at"
  echo "https://github.com/Homebrew/homebrew-core (Formula/<first letter>/<name>.rb)."
  echo
  echo "| Library | Version | Source |"
  echo "|---|---|---|"
  brew info --json=v2 $formulas 2>/dev/null | python3 -c 'import json,sys
for f in json.load(sys.stdin)["formulae"]:
    stable = f["versions"]["stable"]; used = (f.get("installed") or [{}])[-1].get("version", stable).split("_")[0]   # the embedded one may be older than the current recipe
    print("| %s | %s | %s |" % (f["name"], used, f["urls"]["stable"]["url"].replace(stable, used).replace(".".join(stable.split(".")[:2]) + "/", ".".join(used.split(".")[:2]) + "/")))'
} > "$out/SOURCES.md"
# Qt's kegs carry no licence file; Qt is covered by the GPL v3 text above
missing="$(echo "$missing" | tr ' ' '\n' | grep -v '^qt' | tr '\n' ' ')"
[ -z "$(echo "$missing" | tr -d ' ')" ] || echo "collect_licenses: no licence file found for:$missing (add them by hand before distributing)"
exit 0
