#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# Declares the oldest macOS the packaged app can really start on: the highest "minos" among the executable and the embedded Qt
# (Homebrew's Qt is built for the macOS it was installed on, so this depends on the packaging machine).
#   set_min_macos.sh path/to/PadPlus.app
C="$1/Contents"
min="$(for f in "$C/MacOS/PadPlus" "$C"/Frameworks/Qt*.framework/Versions/A/Qt*; do otool -l "$f" | awk '/minos/{print $2; exit}'; done | sort -t. -k1,1n -k2,2n | tail -1)"
[ -n "$min" ] && /usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion $min" "$C/Info.plist" && echo "minimum macOS: $min"
exit 0
