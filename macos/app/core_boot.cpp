// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include "core_boot.h"
#include "Parameters.h"
#include "localization.h"

void bootCore(const std::wstring& settingsDir) {
	NppParameters& p = NppParameters::getInstance();
	if (!settingsDir.empty()) p.setCmdSettingsDir(settingsDir);
	(void)p.load(); // false on a fresh profile (optional files absent); Notepad++ ignores it too
	static NativeLangSpeaker nls;
	nls.init(p.getNativeLang());
	p.setNativeLangSpeaker(&nls);
}
