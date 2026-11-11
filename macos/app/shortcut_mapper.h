// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Shortcut Mapper: lists every Notepad++ command with its shortcut (shortcuts.xml) and lets you change it.
#pragma once
#include <QDialog>
#include <QKeySequence>
#include <QMap>
class QTableWidget; class QLineEdit; class QKeySequenceEdit; class NppHost;

class ShortcutMapper : public QDialog {
	Q_OBJECT
public:
	// names: command id (IDM_*) -> label, for the commands this app actually implements (only those are listed)
	ShortcutMapper(NppHost& host, const QMap<int, QString>& names, QWidget* parent);
	bool assign(int row, const QKeySequence& seq, bool askOnConflict = true); // row = index into host.shortcuts()
signals:
	void changed();   // shortcuts were modified and saved: re-apply them
private:
	void refresh();
	void filter(const QString& text);
	NppHost& _h;
	QMap<int, QString> _names;
	QTableWidget* _table;
	QKeySequenceEdit* _edit;
};
