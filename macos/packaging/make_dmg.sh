#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
# Wraps a packaged PadPlus.app in a compressed disk image with an /Applications shortcut.
#   make_dmg.sh path/to/PadPlus.app out.dmg
set -e
app="$1"; out="$2"
stage="$(mktemp -d)"; trap 'rm -rf "$stage"' EXIT
cp -R "$app" "$stage/"
ln -s /Applications "$stage/Applications"
hdiutil create -volname PadPlus -srcfolder "$stage" -ov -format UDZO "$out" >/dev/null
echo "$out"
