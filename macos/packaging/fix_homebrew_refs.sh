#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# macdeployqt can leave an embedded library pointing at another Homebrew library by absolute path (seen on GitHub's runners).
# This rewrites every such reference to the bundle's Frameworks folder, copying the library in first if it is not there yet,
# and repeats until nothing changes (a copied library can bring references of its own).
#   fix_homebrew_refs.sh path/to/PadPlus.app
C="$1/Contents"; prefix="$(brew --prefix 2>/dev/null || echo /opt/homebrew)"
changed=1; rounds=0
while [ $changed = 1 ] && [ $rounds -lt 10 ]; do
  changed=0; rounds=$((rounds + 1))
  for f in "$C"/Frameworks/*.dylib "$C"/Frameworks/*.framework/Versions/A/Qt* "$C"/PlugIns/*/*.dylib; do
    [ -f "$f" ] || continue
    for dep in $(otool -L "$f" | tail -n +2 | awk '{print $1}' | grep "^$prefix/"); do
      base="$(basename "$dep")"
      if [ "$(otool -D "$f" | tail -n +2)" = "$dep" ]; then          # the file's own install name (macdeployqt leaves some frameworks' as built)
        case "$dep" in *.framework/*) id="@rpath/${dep#*/lib/}" ;; *) id="@executable_path/../Frameworks/$base" ;; esac
        install_name_tool -id "$id" "$f" 2>/dev/null; changed=1; continue
      fi
      case "$dep" in
        *.framework/*) echo "fix_homebrew_refs: $f still needs the framework $dep"; continue ;;
      esac
      [ -f "$C/Frameworks/$base" ] || { cp -L "$dep" "$C/Frameworks/$base"; chmod u+w "$C/Frameworks/$base"; echo "fix_homebrew_refs: embedded $base"; }
      install_name_tool -change "$dep" "@executable_path/../Frameworks/$base" "$f" 2>/dev/null; changed=1
    done
  done
done
exit 0
