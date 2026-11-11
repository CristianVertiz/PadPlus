// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include "keymap.h"
#include "shortcut_mapper.h"

ShortcutMapper::ShortcutMapper(NppHost& host, const QMap<int, QString>& names, QWidget* parent) : QDialog(parent), _h(host), _names(names) {
	setWindowTitle(tr("Shortcut Mapper"));
	resize(640, 560);
	auto* search = new QLineEdit; search->setPlaceholderText(tr("Filter commands..."));
	_table = new QTableWidget(0, 2);
	_table->setHorizontalHeaderLabels({ tr("Command"), tr("Shortcut") });
	_table->horizontalHeader()->setStretchLastSection(true);
	_table->setColumnWidth(0, 380);
	_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	_table->setSelectionMode(QAbstractItemView::SingleSelection);
	_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	_edit = new QKeySequenceEdit;
	auto* assignBtn = new QPushButton(tr("Assign")), *clearBtn = new QPushButton(tr("Clear")), *close = new QPushButton(tr("Close"));
	auto* row = new QHBoxLayout;
	row->addWidget(new QLabel(tr("New shortcut:"))); row->addWidget(_edit, 1); row->addWidget(assignBtn); row->addWidget(clearBtn); row->addWidget(close);
	auto* lay = new QVBoxLayout(this);
	lay->addWidget(search); lay->addWidget(_table, 1); lay->addLayout(row);

	auto current = [this]() -> int { auto* it = _table->currentItem(); return it ? _table->item(it->row(), 0)->data(Qt::UserRole).toInt() : -1; };
	connect(search, &QLineEdit::textChanged, this, &ShortcutMapper::filter);
	connect(assignBtn, &QPushButton::clicked, this, [=] { int r = current(); if (r >= 0) assign(r, _edit->keySequence()); });
	connect(clearBtn, &QPushButton::clicked, this, [=] { int r = current(); if (r >= 0) assign(r, QKeySequence(), false); });
	connect(close, &QPushButton::clicked, this, &QDialog::accept);
	connect(_table, &QTableWidget::currentCellChanged, this, [=](int r) { if (r >= 0) _edit->setKeySequence(toQtSequence(_h.shortcuts()[_table->item(r, 0)->data(Qt::UserRole).toInt()])); });
	refresh();
}

void ShortcutMapper::refresh() {
	auto list = _h.shortcuts();
	_table->setRowCount(0);
	for (int i = 0; i < (int)list.size(); ++i) {
		if (!_names.contains(list[i].cmd)) continue;                          // only commands implemented in this app
		int r = _table->rowCount();
		_table->insertRow(r);
		auto* name = new QTableWidgetItem(_names[list[i].cmd] + (list[i].nth ? QString(" (%1)").arg(list[i].nth + 1) : QString()));   // 2nd binding of a command
		name->setData(Qt::UserRole, i);
		_table->setItem(r, 0, name);
		_table->setItem(r, 1, new QTableWidgetItem(toQtSequence(list[i]).toString(QKeySequence::NativeText)));
	}
}

void ShortcutMapper::filter(const QString& text) {
	for (int r = 0; r < _table->rowCount(); ++r)
		_table->setRowHidden(r, !text.isEmpty() && !_table->item(r, 0)->text().contains(text, Qt::CaseInsensitive));
}

bool ShortcutMapper::assign(int index, const QKeySequence& seq, bool askOnConflict) {
	bool ctrl, alt, shift; int vk;
	if (!fromQtSequence(seq, ctrl, alt, shift, vk)) { QMessageBox::warning(this, tr("Shortcut Mapper"), tr("That key cannot be used.")); return false; }
	auto list = _h.shortcuts();
	if (index < 0 || index >= (int)list.size()) return false;
	if (vk && askOnConflict)
		for (int i = 0; i < (int)list.size(); ++i)
			if (i != index && list[i].vk == vk && list[i].ctrl == ctrl && list[i].alt == alt && list[i].shift == shift)
				if (QMessageBox::question(this, tr("Shortcut Mapper"), tr("%1 is already used by \"%2\". Assign it anyway?").arg(seq.toString(QKeySequence::NativeText), QString::fromStdWString(list[i].name)))
					!= QMessageBox::Yes) return false;
	_h.setShortcut(index, ctrl, alt, shift, vk);
	_h.saveShortcuts();
	refresh();
	emit changed();
	return true;
}
