// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "style_dialog.h"

static QColor fromRef(long c) { return QColor(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF); }              // COLORREF is 0x00BBGGRR
static long toRef(const QColor& c) { return c.red() | (c.green() << 8) | (c.blue() << 16); }
static void paint(QPushButton* b, long ref) {
	b->setText(ref < 0 ? QObject::tr("(inherit)") : QString("#%1").arg(fromRef(ref).rgb() & 0xFFFFFF, 6, 16, QChar('0')).toUpper());
	b->setStyleSheet(ref < 0 ? QString() : QString("background:%1; color:%2;").arg(fromRef(ref).name(), fromRef(ref).lightness() > 128 ? "black" : "white"));
}

StyleDialog::StyleDialog(NppHost& host, QWidget* parent) : QDialog(parent), _h(host) {
	setWindowTitle(tr("Style Configurator"));
	resize(760, 480);
	_theme = new QComboBox;
	for (const std::wstring& t : _h.themes()) _theme->addItem(QString::fromStdWString(t));
	_theme->setCurrentIndex(_h.currentTheme());
	_groups = new QListWidget; _styles = new QListWidget;
	_fg = new QPushButton; _bg = new QPushButton;
	_font = new QFontComboBox; _size = new QSpinBox; _size->setRange(0, 72); _size->setSpecialValueText(tr("(inherit)"));
	_bold = new QCheckBox(tr("Bold")); _italic = new QCheckBox(tr("Italic")); _underline = new QCheckBox(tr("Underline"));

	auto* form = new QGridLayout;
	form->addWidget(new QLabel(tr("Foreground:")), 0, 0); form->addWidget(_fg, 0, 1);
	form->addWidget(new QLabel(tr("Background:")), 1, 0); form->addWidget(_bg, 1, 1);
	form->addWidget(new QLabel(tr("Font:")), 2, 0); form->addWidget(_font, 2, 1);
	form->addWidget(new QLabel(tr("Size:")), 3, 0); form->addWidget(_size, 3, 1);
	form->addWidget(_bold, 4, 0, 1, 2); form->addWidget(_italic, 5, 0, 1, 2); form->addWidget(_underline, 6, 0, 1, 2);
	form->setRowStretch(7, 1);
	auto* top = new QHBoxLayout; top->addWidget(new QLabel(tr("Theme:"))); top->addWidget(_theme, 1);
	auto* cols = new QHBoxLayout; cols->addWidget(_groups, 1); cols->addWidget(_styles, 1); cols->addLayout(form, 1);
	auto* box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close);
	auto* lay = new QVBoxLayout(this); lay->addLayout(top); lay->addLayout(cols, 1); lay->addWidget(box);

	connect(_theme, &QComboBox::activated, this, [this](int i) { _h.setTheme(i); loadGroups(); });
	connect(_groups, &QListWidget::currentRowChanged, this, [this] { loadStyles(); });
	connect(_styles, &QListWidget::currentRowChanged, this, [this] { loadItem(); });
	connect(_fg, &QPushButton::clicked, this, [this] { pickColor(true); });
	connect(_bg, &QPushButton::clicked, this, [this] { pickColor(false); });
	connect(_font, &QFontComboBox::currentFontChanged, this, [this] { commit(); });
	connect(_size, &QSpinBox::valueChanged, this, [this] { commit(); });
	for (QCheckBox* c : {_bold, _italic, _underline}) connect(c, &QCheckBox::toggled, this, [this] { commit(); });
	connect(box, &QDialogButtonBox::accepted, this, [this] { _h.saveStyles(); accept(); });
	connect(box->button(QDialogButtonBox::Save), &QPushButton::clicked, this, [this] { _h.saveStyles(); accept(); });
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	loadGroups();
}

void StyleDialog::loadGroups() {
	_loading = true; _groups->clear();
	for (const std::wstring& g : _h.styleGroups()) _groups->addItem(QString::fromStdWString(g));
	_loading = false;
	_groups->setCurrentRow(0);
}

void StyleDialog::loadStyles() {
	if (_loading) return;
	_loading = true; _styles->clear();
	for (const auto& s : _h.styleItems(_groups->currentRow())) _styles->addItem(QString::fromStdWString(s.desc));
	_loading = false;
	_styles->setCurrentRow(0);
}

void StyleDialog::loadItem() {
	if (_loading) return;
	auto items = _h.styleItems(_groups->currentRow());
	int i = _styles->currentRow();
	if (i < 0 || i >= (int)items.size()) return;
	_item = items[i];
	_loading = true;
	paint(_fg, _item.fg); paint(_bg, _item.bg);
	_font->setCurrentFont(QFont(QString::fromStdWString(_item.font)));
	_size->setValue(_item.size < 0 ? 0 : _item.size);
	_bold->setChecked(_item.bold); _italic->setChecked(_item.italic); _underline->setChecked(_item.underline);
	_loading = false;
}

void StyleDialog::commit() {
	if (_loading || _styles->currentRow() < 0) return;
	_item.font = _font->currentFont().family().toStdWString();
	_item.size = _size->value() == 0 ? -1 : _size->value();
	_item.bold = _bold->isChecked(); _item.italic = _italic->isChecked(); _item.underline = _underline->isChecked();
	_h.setStyleItem(_groups->currentRow(), _styles->currentRow(), _item);
	_h.applyStyles();
}

void StyleDialog::pickColor(bool fg) {
	long& ref = fg ? _item.fg : _item.bg;
	QColor c = QColorDialog::getColor(ref < 0 ? QColor(Qt::white) : fromRef(ref), this, fg ? tr("Foreground") : tr("Background"));
	if (!c.isValid()) return;
	ref = toRef(c);
	paint(fg ? _fg : _bg, ref);
	commit();
}
