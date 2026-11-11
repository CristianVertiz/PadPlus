// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include "preferences_dialog.h"

PreferencesDialog::PreferencesDialog(NppHost& host, QWidget* parent) : QDialog(parent), _h(host) {
	setWindowTitle(tr("Preferences"));
	NppHost::Prefs p = _h.prefs();
	auto check = [](const QString& t, bool v) { auto* c = new QCheckBox(t); c->setChecked(v); return c; };

	auto* editing = new QWidget; auto* ef = new QFormLayout(editing);
	ef->addRow(_lineNumbers = check(tr("Show line numbers"), p.lineNumbers));
	ef->addRow(_bookmark = check(tr("Show bookmark margin"), p.bookmarkMargin));
	ef->addRow(_guides = check(tr("Show indent guides"), p.indentGuides));
	ef->addRow(_caretLine = check(tr("Highlight current line"), p.currentLineHighlight));
	ef->addRow(_wrap = check(tr("Word wrap"), p.wrap));
	ef->addRow(_ws = check(tr("Show whitespace and tabs"), p.showWhitespace));
	ef->addRow(_eol = check(tr("Show end of line"), p.showEol));
	ef->addRow(_beyond = check(tr("Allow scrolling past the last line"), p.scrollBeyondLastLine));
	_fold = new QComboBox; _fold->addItems({ tr("Simple"), tr("Arrow"), tr("Circle tree"), tr("Box tree"), tr("None") });
	_fold->setCurrentIndex(p.foldStyle - 1);                                        // enum values 1..5
	ef->addRow(tr("Fold margin style:"), _fold);
	_caret = new QSpinBox; _caret->setRange(1, 3); _caret->setValue(p.caretWidth);
	ef->addRow(tr("Caret width:"), _caret);

	auto* tabs = new QWidget; auto* tf = new QFormLayout(tabs);
	_tab = new QSpinBox; _tab->setRange(1, 32); _tab->setValue(p.tabSize);
	tf->addRow(tr("Tab size:"), _tab);
	tf->addRow(_spaces = check(tr("Replace tab by spaces"), p.tabsToSpaces));

	auto* newdoc = new QWidget; auto* nf = new QFormLayout(newdoc);
	_newEol = new QComboBox; _newEol->addItems({ tr("Windows (CR LF)"), tr("Old Mac (CR)"), tr("Unix (LF)") }); _newEol->setCurrentIndex(p.newDocEol);
	_newEnc = new QComboBox;
	for (auto [n, m] : std::vector<std::pair<QString, int>>{ { "ANSI", 0 }, { "UTF-8", 4 }, { "UTF-8 with BOM", 1 }, { "UTF-16 BE BOM", 2 }, { "UTF-16 LE BOM", 3 } }) _newEnc->addItem(n, m);
	_newEnc->setCurrentIndex(_newEnc->findData(p.newDocUnicode));
	nf->addRow(tr("Default line ending:"), _newEol);
	nf->addRow(tr("Default encoding:"), _newEnc);

	auto* general = new QWidget; auto* gf = new QFormLayout(general);
	gf->addRow(_session = check(tr("Remember the files that were open when quitting"), p.rememberSession));

	auto* tw = new QTabWidget;
	tw->addTab(general, tr("General")); tw->addTab(editing, tr("Editing")); tw->addTab(tabs, tr("Tabs")); tw->addTab(newdoc, tr("New Document"));
	auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
	auto* lay = new QVBoxLayout(this); lay->addWidget(tw); lay->addWidget(box);
	connect(box, &QDialogButtonBox::accepted, this, [this] { apply(); accept(); });
	connect(box->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::apply);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

NppHost::Prefs PreferencesDialog::read() const {
	NppHost::Prefs p;
	p.lineNumbers = _lineNumbers->isChecked(); p.bookmarkMargin = _bookmark->isChecked(); p.indentGuides = _guides->isChecked();
	p.currentLineHighlight = _caretLine->isChecked(); p.wrap = _wrap->isChecked(); p.showWhitespace = _ws->isChecked(); p.showEol = _eol->isChecked();
	p.scrollBeyondLastLine = _beyond->isChecked(); p.foldStyle = _fold->currentIndex() + 1; p.caretWidth = _caret->value();
	p.tabSize = _tab->value(); p.tabsToSpaces = _spaces->isChecked();
	p.newDocEol = _newEol->currentIndex(); p.newDocUnicode = _newEnc->currentData().toInt();
	p.rememberSession = _session->isChecked();
	return p;
}

void PreferencesDialog::apply() { _h.setPrefs(read()); _h.savePrefs(); }
