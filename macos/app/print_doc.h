// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Printing: lays the document out with Scintilla's own page formatter onto any QPrinter (paper or PDF).
#pragma once
class ScintillaEdit; class QPrinter;
int printDocument(ScintillaEdit* ed, QPrinter* printer);   // returns the number of pages printed (0 for an empty document)
