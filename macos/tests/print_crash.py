#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
"""Prints the crashed thread of the newest macOS crash report (.ips) of a program: print_crash.py <program name> [reports dir].
Used by the CI workflow when a test crashes, so the place of the crash is in the log."""
import glob, json, os, sys

name = sys.argv[1]
folder = sys.argv[2] if len(sys.argv) > 2 else os.path.expanduser("~/Library/Logs/DiagnosticReports")
reports = sorted(glob.glob(os.path.join(folder, name + "*.ips")), key=os.path.getmtime)
if not reports:
    sys.exit("no crash report for %s in %s" % (name, folder))
text = open(reports[-1]).read()
body = json.loads(text.split("\n", 1)[1])                      # line 1 is a header object, the rest is the report
images = body.get("usedImages", [])
exc = body.get("exception", {})
print("%s: %s %s %s" % (os.path.basename(reports[-1]), exc.get("type", "?"), exc.get("signal", ""), exc.get("subtype", "")))
for thread in body.get("threads", []):
    if not thread.get("triggered"):
        continue
    for i, f in enumerate(thread.get("frames", [])[:30]):
        image = images[f["imageIndex"]].get("name", "?") if f.get("imageIndex", -1) < len(images) else "?"
        print("  #%-2d %-28s %s + %s" % (i, image, f.get("symbol", "0x%x" % f.get("imageOffset", 0)), f.get("symbolLocation", "")))
