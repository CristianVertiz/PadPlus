// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "macro_ui.h"

MacroSaveDialog::MacroSaveDialog(QWidget* parent) : QDialog(parent) {
	setWindowTitle(tr("Save Current Recorded Macro"));
	_name = new QLineEdit; _key = new QKeySequenceEdit;
	auto* form = new QFormLayout; form->addRow(tr("Name:"), _name); form->addRow(tr("Shortcut (optional):"), _key);
	auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	auto* lay = new QVBoxLayout(this); lay->addLayout(form); lay->addWidget(box);
	connect(box, &QDialogButtonBox::accepted, this, [this] { if (!_name->text().trimmed().isEmpty()) accept(); });
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
QString MacroSaveDialog::name() const { return _name->text().trimmed(); }
QKeySequence MacroSaveDialog::key() const { return _key->keySequence(); }

MacroRunDialog::MacroRunDialog(const QStringList& macros, bool hasCurrent, QWidget* parent) : QDialog(parent), _hasCurrent(hasCurrent) {
	setWindowTitle(tr("Run a Macro Multiple Times"));
	_which = new QComboBox; _which->addItems(macros);
	_times = new QSpinBox; _times->setRange(1, 100000); _times->setValue(2);
	auto* nTimes = new QRadioButton(tr("Run")); nTimes->setChecked(true);
	_untilEnd = new QRadioButton(tr("Run until the end of the file"));
	auto* row = new QHBoxLayout; row->addWidget(nTimes); row->addWidget(_times); row->addWidget(new QLabel(tr("times"))); row->addStretch();
	auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	auto* lay = new QVBoxLayout(this);
	lay->addWidget(new QLabel(tr("Macro to run:"))); lay->addWidget(_which); lay->addLayout(row); lay->addWidget(_untilEnd); lay->addWidget(box);
	connect(_times, &QSpinBox::valueChanged, this, [nTimes] { nTimes->setChecked(true); });
	connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
int MacroRunDialog::source() const { return _hasCurrent ? _which->currentIndex() - 1 : _which->currentIndex(); }
int MacroRunDialog::times() const { return _times->value(); }
bool MacroRunDialog::untilEnd() const { return _untilEnd->isChecked(); }
