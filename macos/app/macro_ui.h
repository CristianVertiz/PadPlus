// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Dialogs for the Macro menu: save the recorded macro (name + optional shortcut) and run a macro several times / to the end of the file.
#pragma once
#include <QDialog>
#include <QKeySequence>
#include <QStringList>
class QLineEdit; class QKeySequenceEdit; class QComboBox; class QSpinBox; class QRadioButton;

class MacroSaveDialog : public QDialog {
	Q_OBJECT
public:
	explicit MacroSaveDialog(QWidget* parent);
	QString name() const;
	QKeySequence key() const;
private:
	QLineEdit* _name; QKeySequenceEdit* _key;
};

class MacroRunDialog : public QDialog {
	Q_OBJECT
public:
	// macros: labels to choose from; entry 0 may be the unsaved "current" macro (see hasCurrent)
	MacroRunDialog(const QStringList& macros, bool hasCurrent, QWidget* parent);
	int source() const;          // -1 = current recorded macro, otherwise the index among saved macros
	int times() const;
	bool untilEnd() const;
private:
	QComboBox* _which; QSpinBox* _times; QRadioButton* _untilEnd; bool _hasCurrent;
};
