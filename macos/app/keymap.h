// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Windows virtual-key shortcuts (as stored in shortcuts.xml) <-> Qt key sequences. Ctrl maps to Command on macOS.
#pragma once
#include <QKeySequence>
#include "npp_host.h"
QKeySequence toQtSequence(const NppHost::ShortcutItem& s);              // empty if the shortcut is disabled / unmappable
bool fromQtSequence(const QKeySequence& seq, bool& ctrl, bool& alt, bool& shift, int& vk);
