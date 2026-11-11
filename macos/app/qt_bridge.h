// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Connects the shared core's Win32-style Scintilla calls (SendMessage / CreateWindowEx) to Qt ScintillaEdit widgets.
#pragma once
class ScintillaEdit;
void installQtBridge();                       // call once, after QApplication exists
ScintillaEdit* scintillaFromHwnd(void* hwnd); // nullptr if hwnd is not a bridged Scintilla window
