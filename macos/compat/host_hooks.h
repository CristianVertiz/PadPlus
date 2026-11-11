// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Callbacks the app installs so Notepad_plus-shaped calls made by the shared core (FileManager/Buffer) reach the Qt UI.
#pragma once
#include <functional>
class Buffer;
struct HostHooks {
	std::function<void(Buffer*, int)> onBufferChanged; // Notepad_plus::notifyBufferChanged
	std::function<void()> onSaveSession;               // Notepad_plus::saveCurrentSession
};
HostHooks& hostHooks();
