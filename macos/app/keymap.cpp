// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QMap>
#include "keymap.h"

namespace {
struct Pair { int vk; Qt::Key key; };
const Pair kTable[] = {
	{ 0x08, Qt::Key_Backspace }, { 0x09, Qt::Key_Tab }, { 0x0D, Qt::Key_Return }, { 0x13, Qt::Key_Pause }, { 0x14, Qt::Key_CapsLock },
	{ 0x1B, Qt::Key_Escape }, { 0x20, Qt::Key_Space }, { 0x21, Qt::Key_PageUp }, { 0x22, Qt::Key_PageDown }, { 0x23, Qt::Key_End },
	{ 0x24, Qt::Key_Home }, { 0x25, Qt::Key_Left }, { 0x26, Qt::Key_Up }, { 0x27, Qt::Key_Right }, { 0x28, Qt::Key_Down },
	{ 0x2D, Qt::Key_Insert }, { 0x2E, Qt::Key_Delete },
	{ 0xBA, Qt::Key_Semicolon }, { 0xBB, Qt::Key_Equal }, { 0xBC, Qt::Key_Comma }, { 0xBD, Qt::Key_Minus }, { 0xBE, Qt::Key_Period },
	{ 0xBF, Qt::Key_Slash },                                  // main-keyboard keys first: the reverse mapping picks the first match
	{ 0x6A, Qt::Key_Asterisk }, { 0x6B, Qt::Key_Plus }, { 0x6D, Qt::Key_Minus }, { 0x6E, Qt::Key_Period }, { 0x6F, Qt::Key_Slash }, { 0xC0, Qt::Key_QuoteLeft }, { 0xDB, Qt::Key_BracketLeft }, { 0xDC, Qt::Key_Backslash },
	{ 0xDD, Qt::Key_BracketRight }, { 0xDE, Qt::Key_Apostrophe },
};
}

static int vkToQt(int vk) {
	if (vk >= 'A' && vk <= 'Z') return Qt::Key_A + (vk - 'A');
	if (vk >= '0' && vk <= '9') return Qt::Key_0 + (vk - '0');
	if (vk >= 0x70 && vk <= 0x87) return Qt::Key_F1 + (vk - 0x70);   // F1..F24
	if (vk >= 0x60 && vk <= 0x69) return Qt::Key_0 + (vk - 0x60);    // numpad digits
	for (const Pair& p : kTable) if (p.vk == vk) return p.key;
	return 0;
}

static int qtToVk(int key) {
	if (key >= Qt::Key_A && key <= Qt::Key_Z) return 'A' + (key - Qt::Key_A);
	if (key >= Qt::Key_0 && key <= Qt::Key_9) return '0' + (key - Qt::Key_0);
	if (key >= Qt::Key_F1 && key <= Qt::Key_F24) return 0x70 + (key - Qt::Key_F1);
	for (const Pair& p : kTable) if (p.key == key) return p.vk;      // first match wins
	return 0;
}

QKeySequence toQtSequence(const NppHost::ShortcutItem& s) {
	int key = vkToQt(s.vk);
	if (!key) return QKeySequence();
	int mods = (s.ctrl ? Qt::CTRL : 0) | (s.alt ? Qt::ALT : 0) | (s.shift ? Qt::SHIFT : 0); // Qt::CTRL is Command on macOS
	return QKeySequence(mods | key);
}

bool fromQtSequence(const QKeySequence& seq, bool& ctrl, bool& alt, bool& shift, int& vk) {
	if (seq.isEmpty()) { ctrl = alt = shift = false; vk = 0; return true; }
	int combo = seq[0].toCombined();
	ctrl = combo & Qt::CTRL; alt = combo & Qt::ALT; shift = combo & Qt::SHIFT;
	vk = qtToVk(combo & ~(Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::META | Qt::KeypadModifier));
	return vk != 0;
}
