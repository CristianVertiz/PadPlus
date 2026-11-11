// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "ScintillaEdit.h"
#include "find_dialog.h"

FindDialog::FindDialog(std::function<ScintillaEdit*()> ed, QWidget* parent) : QDialog(parent), _editor(std::move(ed)) {
	setWindowTitle(tr("Find / Replace"));
	setWindowFlag(Qt::Tool);
	_what = new QLineEdit; _with = new QLineEdit;
	_case = new QCheckBox(tr("Match case")); _word = new QCheckBox(tr("Whole word"));
	_regex = new QCheckBox(tr("Regular expression")); _wrap = new QCheckBox(tr("Wrap around")); _wrap->setChecked(true);
	_status = new QLabel;
	auto* grid = new QGridLayout;
	grid->addWidget(new QLabel(tr("Find what:")), 0, 0); grid->addWidget(_what, 0, 1);
	grid->addWidget(new QLabel(tr("Replace with:")), 1, 0); grid->addWidget(_with, 1, 1);
	auto* opts = new QHBoxLayout;
	for (auto* c : {_case, _word, _regex, _wrap}) opts->addWidget(c);
	auto* btns = new QHBoxLayout;
	struct { const char* t; void (FindDialog::*f)(); } b[] = { { "Find Next", &FindDialog::findNext }, { "Find Previous", &FindDialog::findPrev },
		{ "Replace", &FindDialog::replaceOne }, { "Replace All", &FindDialog::replaceAll }, { "Count", &FindDialog::count } };
	for (auto& x : b) { auto* p = new QPushButton(tr(x.t)); connect(p, &QPushButton::clicked, this, x.f); btns->addWidget(p); }
	auto* mark = new QHBoxLayout;
	_bookmark = new QCheckBox(tr("Bookmark line"));
	auto* bMark = new QPushButton(tr("Mark All")); auto* bClear = new QPushButton(tr("Clear All Marks"));
	connect(bMark, &QPushButton::clicked, this, &FindDialog::markAll); connect(bClear, &QPushButton::clicked, this, &FindDialog::clearMarks);
	mark->addWidget(bMark); mark->addWidget(bClear); mark->addWidget(_bookmark); mark->addStretch(1);
	auto* lay = new QVBoxLayout(this);
	lay->addLayout(grid); lay->addLayout(opts); lay->addLayout(btns); lay->addLayout(mark); lay->addWidget(_status);
	connect(_what, &QLineEdit::returnPressed, this, &FindDialog::findNext);
	setMinimumWidth(560);
}

void FindDialog::showFind(const QString& seed) {
	if (!seed.isEmpty() && !seed.contains('\n')) _what->setText(seed);
	show(); raise(); activateWindow(); _what->setFocus(); _what->selectAll();
}

int FindDialog::flags() const {
	int f = 0;
	if (_case->isChecked()) f |= SCFIND_MATCHCASE;
	if (_word->isChecked()) f |= SCFIND_WHOLEWORD;
	if (_regex->isChecked()) f |= SCFIND_REGEXP;   // Notepad++'s Scintilla routes this to Boost.Regex
	return f;
}

// start > end searches backwards. Returns match start, or -1 (match end is left in SCI_GETTARGETEND).
long long FindDialog::search(ScintillaEdit* ed, long long start, long long end, const QByteArray& pat) {
	ed->send(SCI_SETTARGETSTART, start);
	ed->send(SCI_SETTARGETEND, end);
	ed->send(SCI_SETSEARCHFLAGS, flags());
	return ed->send(SCI_SEARCHINTARGET, pat.size(), reinterpret_cast<sptr_t>(pat.constData()));
}

bool FindDialog::find(bool forward) {
	ScintillaEdit* ed = _editor();
	QByteArray pat = _what->text().toUtf8();
	if (!ed || pat.isEmpty()) return false;
	long long selS = ed->send(SCI_GETSELECTIONSTART), selE = ed->send(SCI_GETSELECTIONEND), len = ed->send(SCI_GETLENGTH);
	long long pos = forward ? search(ed, selE, len, pat) : search(ed, selS, 0, pat);
	bool wrapped = false;
	if (pos < 0 && _wrap->isChecked()) { pos = forward ? search(ed, 0, len, pat) : search(ed, len, 0, pat); wrapped = pos >= 0; }
	if (pos < 0) { _status->setText(tr("Not found")); return false; }
	ed->send(SCI_SETSEL, pos, ed->send(SCI_GETTARGETEND));
	ed->send(SCI_SCROLLCARET);
	_status->setText(wrapped ? tr("Found (wrapped around)") : tr("Found"));
	return true;
}

void FindDialog::replaceOne() {
	ScintillaEdit* ed = _editor();
	QByteArray pat = _what->text().toUtf8(), rep = _with->text().toUtf8();
	if (!ed || pat.isEmpty()) return;
	long long s = ed->send(SCI_GETSELECTIONSTART), e = ed->send(SCI_GETSELECTIONEND);
	if (search(ed, s, e, pat) == s && ed->send(SCI_GETTARGETEND) == e) { // selection is a current match: replace it
		ed->send(_regex->isChecked() ? SCI_REPLACETARGETRE : SCI_REPLACETARGET, rep.size(), reinterpret_cast<sptr_t>(rep.constData()));
		ed->send(SCI_SETSEL, ed->send(SCI_GETTARGETEND), ed->send(SCI_GETTARGETEND));
	}
	find(true);
}

void FindDialog::replaceAll() {
	ScintillaEdit* ed = _editor();
	QByteArray pat = _what->text().toUtf8(), rep = _with->text().toUtf8();
	if (!ed || pat.isEmpty()) return;
	int n = 0; long long pos = 0;
	ed->send(SCI_BEGINUNDOACTION);
	while (pos <= ed->send(SCI_GETLENGTH)) {
		long long hit = search(ed, pos, ed->send(SCI_GETLENGTH), pat);
		if (hit < 0) break;
		long long hitEnd = ed->send(SCI_GETTARGETEND);
		ed->send(_regex->isChecked() ? SCI_REPLACETARGETRE : SCI_REPLACETARGET, rep.size(), reinterpret_cast<sptr_t>(rep.constData()));
		long long after = ed->send(SCI_GETTARGETEND);
		pos = (after == hit && hitEnd == hit) ? after + 1 : after; // step over empty matches
		++n;
	}
	ed->send(SCI_ENDUNDOACTION);
	_status->setText(tr("Replaced %1 occurrence(s)").arg(n));
}

void FindDialog::count() {
	ScintillaEdit* ed = _editor();
	QByteArray pat = _what->text().toUtf8();
	if (!ed || pat.isEmpty()) return;
	int n = 0; long long pos = 0, len = ed->send(SCI_GETLENGTH);
	while (pos <= len) {
		long long hit = search(ed, pos, len, pat);
		if (hit < 0) break;
		long long end = ed->send(SCI_GETTARGETEND);
		pos = end == hit ? end + 1 : end; ++n;
	}
	_status->setText(tr("%1 match(es)").arg(n));
}

static const int kMarkIndic = 31, kBookmark = 20;   // SCE_UNIVERSAL_FOUND_STYLE and MARK_BOOKMARK: the core sets up how both look

void FindDialog::markAll() {
	ScintillaEdit* ed = _editor();
	QByteArray pat = _what->text().toUtf8();
	if (!ed || pat.isEmpty()) return;
	int n = 0; long long pos = 0, len = ed->send(SCI_GETLENGTH);
	ed->send(SCI_SETINDICATORCURRENT, kMarkIndic);
	while (pos <= len) {
		long long hit = search(ed, pos, len, pat);
		if (hit < 0) break;
		long long end = ed->send(SCI_GETTARGETEND);
		if (end > hit) ed->send(SCI_INDICATORFILLRANGE, hit, end - hit);
		if (_bookmark->isChecked()) { long long line = ed->send(SCI_LINEFROMPOSITION, hit); if (!(ed->send(SCI_MARKERGET, line) & (1 << kBookmark))) ed->send(SCI_MARKERADD, line, kBookmark); }
		pos = end == hit ? end + 1 : end; ++n;
	}
	_status->setText(tr("Mark: %1 match(es)").arg(n));
}

void FindDialog::clearMarks() {
	ScintillaEdit* ed = _editor(); if (!ed) return;
	ed->send(SCI_SETINDICATORCURRENT, kMarkIndic); ed->send(SCI_INDICATORCLEARRANGE, 0, ed->send(SCI_GETLENGTH));
	if (_bookmark->isChecked()) ed->send(SCI_MARKERDELETEALL, kBookmark);
	_status->clear();
}

void FindDialog::setBookmarkLine(bool on) { _bookmark->setChecked(on); }
void FindDialog::setQuery(const QString& what, const QString& with, bool matchCase, bool wholeWord, bool regex) {
	_what->setText(what); _with->setText(with);
	_case->setChecked(matchCase); _word->setChecked(wholeWord); _regex->setChecked(regex);
}
QString FindDialog::status() const { return _status->text(); }

// ------------------------------------------------------------------------------------------------ find characters in range
bool FindCharsDialog::findInRange(ScintillaEdit* ed, int lo, int hi, bool down, bool wrap) {
	const long long len = ed->send(SCI_GETLENGTH), step = down ? 1 : -1;
	long long start = down ? ed->send(SCI_GETSELECTIONEND) : ed->send(SCI_GETSELECTIONSTART) - 1;
	for (int pass = 0; pass < (wrap ? 2 : 1); ++pass, start = down ? 0 : len - 1)
		for (long long p = start; p >= 0 && p < len; p += step) {
			const int c = static_cast<unsigned char>(ed->send(SCI_GETCHARAT, p));
			if (c < lo || c > hi) continue;
			const long long s = ed->send(SCI_POSITIONBEFORE, ed->send(SCI_POSITIONAFTER, p));   // whole character, also when p is inside a UTF-8 sequence
			ed->send(SCI_SETSEL, s, ed->send(SCI_POSITIONAFTER, s)); ed->send(SCI_SCROLLCARET);
			return true;
		}
	return false;
}

FindCharsDialog::FindCharsDialog(std::function<ScintillaEdit*()> ed, QWidget* parent) : QDialog(parent), _editor(std::move(ed)) {
	setWindowTitle(tr("Find Characters in Range")); setWindowFlag(Qt::Tool); setMinimumWidth(380);   // the direction row was clipped at the natural width
	_nonAscii = new QRadioButton(tr("Non-ASCII characters (128-255)")); _ascii = new QRadioButton(tr("ASCII characters (0-127)")); _custom = new QRadioButton(tr("My range:"));
	_nonAscii->setChecked(true);
	_from = new QSpinBox; _to = new QSpinBox; for (QSpinBox* b : { _from, _to }) b->setRange(0, 255);
	_to->setValue(255);
	auto* range = new QHBoxLayout; range->addWidget(_custom); range->addWidget(_from); range->addWidget(new QLabel("-")); range->addWidget(_to); range->addStretch(1);
	_up = new QRadioButton(tr("Up")); auto* down = new QRadioButton(tr("Down")); down->setChecked(true);
	_wrap = new QCheckBox(tr("Wrap around")); _wrap->setChecked(true);
	auto* dirBox = new QWidget; auto* dir = new QHBoxLayout(dirBox); dir->setContentsMargins(0, 0, 0, 0);   // its own parent: a separate radio group
	dir->addWidget(new QLabel(tr("Direction:"))); dir->addWidget(_up); dir->addWidget(down); dir->addWidget(_wrap); dir->addStretch(1);
	auto* find = new QPushButton(tr("Find")); find->setDefault(true);
	_status = new QLabel;
	auto* lay = new QVBoxLayout(this);
	lay->addWidget(_nonAscii); lay->addWidget(_ascii); lay->addLayout(range); lay->addWidget(dirBox); lay->addWidget(find); lay->addWidget(_status);
	connect(find, &QPushButton::clicked, this, [this] {
		ScintillaEdit* e = _editor(); if (!e) return;
		const int lo = _nonAscii->isChecked() ? 128 : _ascii->isChecked() ? 0 : _from->value(), hi = _nonAscii->isChecked() ? 255 : _ascii->isChecked() ? 127 : _to->value();
		_status->setText(findInRange(e, lo, hi, !_up->isChecked(), _wrap->isChecked()) ? QString() : tr("Not found")); });
}
