// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// One-time setup of the shared core services that Notepad_plus's constructor performs on Windows.
#pragma once
#include <string>
// settingsDir empty => ~/Library/Application Support/Notepad++. Call after installQtBridge(), before creating views.
void bootCore(const std::wstring& settingsDir = std::wstring());
