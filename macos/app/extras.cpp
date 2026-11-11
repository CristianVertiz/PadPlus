// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include "ScintillaEdit.h"
#include "extras.h"

// ================================================================================================== Clipboard History
ClipboardPanel::ClipboardPanel(QWidget* parent) : QDockWidget(tr("Clipboard History"), parent) {
	setObjectName("clipboardDock");
	_list = new QListWidget; _list->setUniformItemSizes(true); _list->setTextElideMode(Qt::ElideRight);
	setWidget(_list);
	connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this] { record(QApplication::clipboard()->text()); });
	connect(_list, &QListWidget::itemActivated, this, [this](QListWidgetItem* it) { emit paste(it->data(Qt::UserRole).toString()); });
}

void ClipboardPanel::record(const QString& text) {
	if (text.isEmpty()) return;
	for (int i = 0; i < _list->count(); ++i)                                   // a repeated entry moves to the top
		if (_list->item(i)->data(Qt::UserRole).toString() == text) delete _list->takeItem(i--);
	auto* it = new QListWidgetItem(text.simplified().left(200));
	it->setData(Qt::UserRole, text); it->setToolTip(text.left(1000));
	_list->insertItem(0, it);
	while (_list->count() > 50) delete _list->takeItem(_list->count() - 1);
}

// ================================================================================================== ANSI Chars
AnsiPanel::AnsiPanel(QWidget* parent) : QDockWidget(tr("ANSI Chars"), parent) {
	setObjectName("ansiDock");
	static const char* ctl[32] = { "NUL","SOH","STX","ETX","EOT","ENQ","ACK","BEL","BS","HT","LF","VT","FF","CR","SO","SI",
		"DLE","DC1","DC2","DC3","DC4","NAK","SYN","ETB","CAN","EM","SUB","ESC","FS","GS","RS","US" };
	_table = new QTableWidget(256, 4);
	_table->setHorizontalHeaderLabels({ tr("Value"), tr("Hex"), tr("Char"), tr("Name") });
	_table->verticalHeader()->hide(); _table->setEditTriggers(QAbstractItemView::NoEditTriggers); _table->setSelectionBehavior(QAbstractItemView::SelectRows);
	for (int i = 0; i < 256; ++i) {
		QString name = i < 32 ? ctl[i] : i == 32 ? "SP" : i == 127 ? "DEL" : i < 160 && i >= 128 ? "" : QString();
		QString ch = i < 32 || (i >= 127 && i < 160) ? QString() : QString(QChar(i));
		_table->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
		_table->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(i, 2, 16, QChar('0')).toUpper()));
		_table->setItem(i, 2, new QTableWidgetItem(ch));
		_table->setItem(i, 3, new QTableWidgetItem(name));
		_table->item(i, 0)->setData(Qt::UserRole, i);
	}
	_table->resizeColumnsToContents(); _table->horizontalHeader()->setStretchLastSection(true);
	setWidget(_table);
	connect(_table, &QTableWidget::cellDoubleClicked, this, [this](int r, int) { emit insertChar(QString(QChar(r))); });
}

// ================================================================================================== Document Map
DocMapPanel::DocMapPanel(QWidget* parent) : QDockWidget(tr("Document Map"), parent) {
	setObjectName("docMapDock");
	_map = new ScintillaEdit;
	_map->send(SCI_SETZOOM, -7); _map->send(SCI_SETREADONLY, 1);
	for (int m = 0; m < 5; ++m) _map->send(SCI_SETMARGINWIDTHN, m, 0);
	_map->send(SCI_SETCARETWIDTH, 0); _map->send(SCI_SETCARETLINEVISIBLE, 0);
	_map->send(SCI_SETVSCROLLBAR, 0); _map->send(SCI_SETHSCROLLBAR, 0);
	setWidget(_map);
	_band = new QWidget(_map->viewport());                      // the visible part of the document: an overlay, so the shared text is never touched
	_band->setAttribute(Qt::WA_TransparentForMouseEvents); _band->setStyleSheet("background: rgba(128,128,128,70);"); _band->hide();
	_map->viewport()->installEventFilter(this);
}

void DocMapPanel::follow(ScintillaEdit* main) {
	if (!main || isHidden()) return;
	_main = main;
	sptr_t doc = main->send(SCI_GETDOCPOINTER);
	if (_map->send(SCI_GETDOCPOINTER) != doc) _map->send(SCI_SETDOCPOINTER, 0, doc);
	_map->send(SCI_STYLESETFORE, STYLE_DEFAULT, main->send(SCI_STYLEGETFORE, STYLE_DEFAULT));
	_map->send(SCI_STYLESETBACK, STYLE_DEFAULT, main->send(SCI_STYLEGETBACK, STYLE_DEFAULT));
	_map->send(SCI_STYLECLEARALL);
	long first = main->send(SCI_GETFIRSTVISIBLELINE), shown = main->send(SCI_LINESONSCREEN);
	long lines = main->send(SCI_GETLINECOUNT);
	long mapShown = _map->send(SCI_LINESONSCREEN);                 // keep the visible block inside the map
	long target = first + shown / 2 - mapShown / 2;
	_map->send(SCI_SETFIRSTVISIBLELINE, target < 0 ? 0 : target);
	long a = main->send(SCI_DOCLINEFROMVISIBLE, first), z = main->send(SCI_DOCLINEFROMVISIBLE, first + shown - 1);
	if (z >= lines) z = lines - 1;
	auto y = [this](long line) { return (int)_map->send(SCI_POINTYFROMPOSITION, 0, _map->send(SCI_POSITIONFROMLINE, line)); };
	int top = y(a), bottom = y(z) + (int)_map->send(SCI_TEXTHEIGHT, z);
	_band->setGeometry(0, top, _map->viewport()->width(), bottom - top); _band->show(); _band->raise();
}

bool DocMapPanel::eventFilter(QObject* o, QEvent* e) {
	if ((e->type() == QEvent::MouseButtonPress || (e->type() == QEvent::MouseMove && static_cast<QMouseEvent*>(e)->buttons() & Qt::LeftButton)) && o == _map->viewport()) {
		QPoint p = static_cast<QMouseEvent*>(e)->position().toPoint();
		long pos = _map->send(SCI_POSITIONFROMPOINT, p.x(), p.y());
		emit scrollToLine((int)_map->send(SCI_LINEFROMPOSITION, pos));
		return true;
	}
	return QDockWidget::eventFilter(o, e);
}

// ================================================================================================== Incremental search
static const int kIncIndicator = 9;      // Notepad++ uses SCE_UNIVERSAL_FOUND_STYLE_INC (31 - 5 = 26); any free indicator will do

IncSearchBar::IncSearchBar(std::function<ScintillaEdit*()> ed, QWidget* parent) : QWidget(parent), _editor(std::move(ed)) {
	auto* l = new QHBoxLayout(this); l->setContentsMargins(6, 3, 6, 3);
	_edit = new QLineEdit; _edit->setPlaceholderText(tr("Incremental search")); _edit->setClearButtonEnabled(true);
	_case = new QCheckBox(tr("Match case")); _all = new QCheckBox(tr("Highlight all"));
	auto* pv = new QPushButton("▲"), * nx = new QPushButton("▼"), * cl = new QPushButton("✕");
	for (QPushButton* b : { pv, nx, cl }) { b->setFlat(true); b->setFixedWidth(28); b->setFocusPolicy(Qt::NoFocus); }
	_msg = new QLabel;
	for (QWidget* w : std::initializer_list<QWidget*>{ new QLabel(tr("Find:")), _edit, pv, nx, _case, _all, _msg }) l->addWidget(w);
	l->setStretch(1, 1); l->addStretch(); l->addWidget(cl);
	connect(_edit, &QLineEdit::textChanged, this, [this] { search(true, true); highlight(); });
	connect(_edit, &QLineEdit::returnPressed, this, [this] { Qt::KeyboardModifiers m = QApplication::keyboardModifiers(); search(!(m & Qt::ShiftModifier), false); });
	connect(pv, &QPushButton::clicked, this, &IncSearchBar::prev); connect(nx, &QPushButton::clicked, this, &IncSearchBar::next);
	connect(cl, &QPushButton::clicked, this, &IncSearchBar::closeBar);
	connect(_case, &QCheckBox::toggled, this, [this] { search(true, true); highlight(); });
	connect(_all, &QCheckBox::toggled, this, [this] { highlight(); });
	new QShortcut(Qt::Key_Escape, this, [this] { closeBar(); });
	hide();
}

void IncSearchBar::open(const QString& seed) {
	show();
	if (!seed.isEmpty() && !seed.contains('\n')) _edit->setText(seed);
	_edit->setFocus(); _edit->selectAll();
}
void IncSearchBar::setText(const QString& t) { show(); _edit->setText(t); }
QString IncSearchBar::state() const { return _msg->text(); }

void IncSearchBar::search(bool fwd, bool fromStart) {
	ScintillaEdit* ed = _editor(); if (!ed) return;
	QByteArray pat = _edit->text().toUtf8();
	_edit->setStyleSheet(QString());
	_msg->clear();
	if (pat.isEmpty()) return;
	ed->send(SCI_SETSEARCHFLAGS, _case->isChecked() ? SCFIND_MATCHCASE : 0);
	long len = ed->send(SCI_GETLENGTH), s = ed->send(SCI_GETSELECTIONSTART), e = ed->send(SCI_GETSELECTIONEND);
	long from = fwd ? (fromStart ? s : e) : s, to = fwd ? len : 0;
	auto run = [&](long a, long b) { ed->send(SCI_SETTARGETRANGE, a, b); return (long)ed->send(SCI_SEARCHINTARGET, pat.size(), (sptr_t)pat.constData()); };
	long hit = run(from, to);
	if (hit < 0) hit = run(fwd ? 0 : len, fwd ? from : from);     // wrap around
	if (hit < 0) { _edit->setStyleSheet("QLineEdit{background:#8b2e2e;color:white}"); _msg->setText(tr("not found")); return; }
	ed->send(SCI_SETSEL, hit, hit + pat.size()); ed->send(SCI_SCROLLCARET);
	_msg->setText(tr("found"));
}

void IncSearchBar::highlight() {
	ScintillaEdit* ed = _editor(); if (!ed) return;
	ed->send(SCI_SETINDICATORCURRENT, kIncIndicator);
	ed->send(SCI_INDICSETSTYLE, kIncIndicator, INDIC_ROUNDBOX); ed->send(SCI_INDICSETFORE, kIncIndicator, 0x00C0FF); ed->send(SCI_INDICSETALPHA, kIncIndicator, 120);
	long len = ed->send(SCI_GETLENGTH);
	ed->send(SCI_INDICATORCLEARRANGE, 0, len);
	QByteArray pat = _edit->text().toUtf8();
	if (!isVisible() || !_all->isChecked() || pat.isEmpty()) return;
	ed->send(SCI_SETSEARCHFLAGS, _case->isChecked() ? SCFIND_MATCHCASE : 0);
	for (long pos = 0, n = 0; n < 5000; ++n) {
		ed->send(SCI_SETTARGETRANGE, pos, len);
		long hit = ed->send(SCI_SEARCHINTARGET, pat.size(), (sptr_t)pat.constData());
		if (hit < 0) break;
		ed->send(SCI_INDICATORFILLRANGE, hit, pat.size()); pos = hit + pat.size();
	}
}

void IncSearchBar::closeBar() {
	if (ScintillaEdit* ed = _editor()) { ed->send(SCI_SETINDICATORCURRENT, kIncIndicator); ed->send(SCI_INDICATORCLEARRANGE, 0, ed->send(SCI_GETLENGTH)); static_cast<QWidget*>(ed)->setFocus(); }
	hide();
}
