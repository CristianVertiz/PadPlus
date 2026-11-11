// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Style Configurator: pick a theme, then edit colours / font of any language style or global style (stylers.xml / themes).
#pragma once
#include <QDialog>
#include "npp_host.h"
class QComboBox; class QListWidget; class QPushButton; class QCheckBox; class QFontComboBox; class QSpinBox;

class StyleDialog : public QDialog {
	Q_OBJECT
public:
	StyleDialog(NppHost& host, QWidget* parent);
private:
	void loadGroups();
	void loadStyles();
	void loadItem();
	void commit();                       // widgets -> host (live preview in the editor)
	void pickColor(bool foreground);
	NppHost& _h;
	QComboBox* _theme;
	QListWidget *_groups, *_styles;
	QPushButton *_fg, *_bg;
	QFontComboBox* _font;
	QSpinBox* _size;
	QCheckBox *_bold, *_italic, *_underline;
	NppHost::StyleItem _item;
	bool _loading = false;
};
