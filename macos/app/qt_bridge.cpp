// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QWidget>
#include <unordered_set>
#include "ScintillaEdit.h"
#include "qt_bridge.h"
#include <windows.h> // compat layer (after Qt headers: it #defines Win32 macro names)

static std::unordered_set<void*> g_scis; // live bridged Scintilla widgets; an HWND is the widget pointer

static sptr_t direct(void* p, unsigned m, uptr_t w, sptr_t l) { return static_cast<ScintillaEdit*>(p)->send(m, w, l); }

ScintillaEdit* scintillaFromHwnd(void* h) { return g_scis.count(h) ? static_cast<ScintillaEdit*>(h) : nullptr; }

static LRESULT sendHook(HWND h, UINT m, WPARAM w, LPARAM l) {
	ScintillaEdit* ed = scintillaFromHwnd(h);
	if (!ed) return 0; // non-Scintilla windows: nothing to do (their UI is rebuilt in Qt)
	if (m == SCI_GETDIRECTFUNCTION) return reinterpret_cast<LRESULT>(&direct);
	if (m == SCI_GETDIRECTPOINTER) return reinterpret_cast<LRESULT>(h);
	return ed->send(m, w, l);
}

static HWND createHook(LPCWSTR cls, HWND) {
	if (!cls || wcscmp(cls, L"Scintilla") != 0) return nullptr;
	auto* ed = new ScintillaEdit(nullptr);
	g_scis.insert(ed);
	return ed;
}

void installQtBridge() { g_nppSendMessageHook = sendHook; g_nppCreateWindowHook = createHook; }
