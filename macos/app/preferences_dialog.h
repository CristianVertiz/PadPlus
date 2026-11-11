// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Preferences: the editing / tab / new-document / startup settings that this Qt front end honours (stored in config.xml).
#pragma once
#include <QDialog>
#include "npp_host.h"
class QCheckBox; class QComboBox; class QSpinBox;

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	PreferencesDialog(NppHost& host, QWidget* parent);
	NppHost::Prefs read() const;       // values currently shown in the widgets
	void apply();                      // push to the views and write config.xml
private:
	NppHost& _h;
	QCheckBox *_lineNumbers, *_bookmark, *_guides, *_wrap, *_eol, *_ws, *_beyond, *_caretLine, *_spaces, *_session;
	QComboBox *_fold, *_newEol, *_newEnc;
	QSpinBox *_tab, *_caret;
};
