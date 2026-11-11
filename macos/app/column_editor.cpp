// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <vector>
#include "ScintillaEdit.h"
#include "column_editor.h"

namespace {
struct Spot { long long start, end; int pad; };    // replace [start,end) (after `pad` spaces for short lines)

std::vector<Spot> collectSpots(ScintillaEdit* ed) {
	std::vector<Spot> spots;
	int n = (int)ed->send(SCI_GETSELECTIONS);
	if (n > 1) {                                           // rectangular or multiple selection
		for (int i = 0; i < n; ++i) {
			long long a = ed->send(SCI_GETSELECTIONNSTART, i), b = ed->send(SCI_GETSELECTIONNEND, i);
			spots.push_back({ a, b, 0 });
		}
		std::sort(spots.begin(), spots.end(), [](const Spot& x, const Spot& y) { return x.start < y.start; });
		return spots;
	}
	long long s = ed->send(SCI_GETSELECTIONSTART), e = ed->send(SCI_GETSELECTIONEND);
	long long l0 = ed->send(SCI_LINEFROMPOSITION, s), l1 = ed->send(SCI_LINEFROMPOSITION, e);
	if (l0 == l1) { spots.push_back({ s, e, 0 }); return spots; }
	long long col = ed->send(SCI_GETCOLUMN, ed->send(SCI_GETCURRENTPOS));       // stream selection over several lines: insert at the caret column
	for (long long l = l0; l <= l1; ++l) {
		long long end = ed->send(SCI_GETLINEENDPOSITION, l);
		long long endCol = ed->send(SCI_GETCOLUMN, end);
		long long pos = endCol >= col ? ed->send(SCI_FINDCOLUMN, l, col) : end;
		spots.push_back({ pos, pos, endCol >= col ? 0 : (int)(col - endCol) });
	}
	return spots;
}

int apply(ScintillaEdit* ed, const std::vector<Spot>& spots, const std::vector<QByteArray>& texts) {
	ed->send(SCI_BEGINUNDOACTION);
	for (int i = (int)spots.size() - 1; i >= 0; --i) {                         // back to front keeps earlier offsets valid
		QByteArray t = QByteArray(spots[i].pad, ' ') + texts[i];
		ed->send(SCI_SETTARGETRANGE, spots[i].start, spots[i].end);
		ed->send(SCI_REPLACETARGET, t.size(), reinterpret_cast<sptr_t>(t.constData()));
	}
	ed->send(SCI_ENDUNDOACTION);
	return (int)spots.size();
}
}

int columnInsertText(ScintillaEdit* ed, const QString& text) {
	if (!ed) return 0;
	auto spots = collectSpots(ed);
	return apply(ed, spots, std::vector<QByteArray>(spots.size(), text.toUtf8()));
}

int columnInsertNumbers(ScintillaEdit* ed, const ColumnNumberOpts& o) {
	if (!ed) return 0;
	auto spots = collectSpots(ed);
	int repeat = std::max(1, o.repeat);
	auto fmt = [&](long long v) { QString s = QString::number(v < 0 ? -v : v, o.base); return o.upper ? s.toUpper() : s; };
	long long last = o.start + (long long)((spots.size() - 1) / repeat) * o.step;
	int width = o.leadingZeros ? (int)std::max(fmt(o.start).size(), fmt(last).size()) : 0;
	std::vector<QByteArray> texts;
	for (size_t i = 0; i < spots.size(); ++i) {
		long long v = o.start + (long long)(i / repeat) * o.step;
		QString s = fmt(v);
		if (s.size() < width) s = QString(width - s.size(), '0') + s;
		texts.push_back(((v < 0 ? "-" : "") + s).toUtf8());
	}
	return apply(ed, spots, texts);
}

ColumnEditorDialog::ColumnEditorDialog(std::function<ScintillaEdit*()> ed, QWidget* parent) : QDialog(parent), _editor(std::move(ed)) {
	setWindowTitle(tr("Column Editor"));
	setWindowFlag(Qt::Tool);
	_textMode = new QRadioButton(tr("Text to insert")); _numMode = new QRadioButton(tr("Number to insert")); _textMode->setChecked(true);
	_text = new QLineEdit;
	_start = new QSpinBox; _start->setRange(-1000000000, 1000000000);
	_step = new QSpinBox; _step->setRange(-1000000, 1000000); _step->setValue(1);
	_repeat = new QSpinBox; _repeat->setRange(1, 100000); _repeat->setValue(1);
	_zeros = new QCheckBox(tr("Leading zeros"));
	_base = new QComboBox; for (auto [n, b] : std::vector<std::pair<QString, int>>{ { "Dec", 10 }, { "Hex", 16 }, { "Oct", 8 }, { "Bin", 2 } }) _base->addItem(n, b);
	auto* form = new QFormLayout;
	form->addRow(_textMode, _text);
	form->addRow(_numMode);
	form->addRow(tr("Initial number:"), _start); form->addRow(tr("Increase by:"), _step); form->addRow(tr("Repeat:"), _repeat);
	form->addRow(_zeros); form->addRow(tr("Format:"), _base);
	auto* ok = new QPushButton(tr("OK")); ok->setDefault(true); auto* close = new QPushButton(tr("Close"));
	auto* lay = new QVBoxLayout(this); lay->addLayout(form); lay->addWidget(ok); lay->addWidget(close);
	connect(close, &QPushButton::clicked, this, &QDialog::hide);
	connect(_text, &QLineEdit::textEdited, this, [this] { _textMode->setChecked(true); });
	connect(ok, &QPushButton::clicked, this, [this] {
		ScintillaEdit* e = _editor();
		if (_textMode->isChecked()) columnInsertText(e, _text->text());
		else { ColumnNumberOpts o; o.start = _start->value(); o.step = _step->value(); o.repeat = _repeat->value();
			o.leadingZeros = _zeros->isChecked(); o.base = _base->currentData().toInt(); o.upper = o.base == 16; columnInsertNumbers(e, o); }
		hide();
	});
}
