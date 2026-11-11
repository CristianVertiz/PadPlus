// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Column Editor: insert text, or a counting number sequence, at every selection / at the caret column of each selected line.
#pragma once
#include <QDialog>
#include <QString>
#include <functional>
class ScintillaEdit; class QRadioButton; class QLineEdit; class QSpinBox; class QCheckBox; class QComboBox;

struct ColumnNumberOpts { long long start = 0, step = 1; int repeat = 1; bool leadingZeros = false; int base = 10; bool upper = false; };
int columnInsertText(ScintillaEdit* ed, const QString& text);            // returns number of insertions
int columnInsertNumbers(ScintillaEdit* ed, const ColumnNumberOpts& o);

class ColumnEditorDialog : public QDialog {
	Q_OBJECT
public:
	ColumnEditorDialog(std::function<ScintillaEdit*()> editor, QWidget* parent);
private:
	std::function<ScintillaEdit*()> _editor;
	QRadioButton *_textMode, *_numMode;
	QLineEdit* _text;
	QSpinBox *_start, *_step, *_repeat;
	QCheckBox* _zeros;
	QComboBox* _base;
};
