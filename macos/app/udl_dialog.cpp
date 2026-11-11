// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <map>
#include "pugixml/pugixml.hpp"
#include "npp_host.h"
#include "udl_dialog.h"

static const char* kStyleNames[] = { "DEFAULT", "COMMENTS", "LINE COMMENTS", "NUMBERS", "KEYWORDS1", "KEYWORDS2", "KEYWORDS3", "KEYWORDS4", "KEYWORDS5", "KEYWORDS6",
	"KEYWORDS7", "KEYWORDS8", "OPERATORS", "FOLDER IN CODE1", "FOLDER IN CODE2", "FOLDER IN COMMENT", "DELIMITERS1", "DELIMITERS2", "DELIMITERS3", "DELIMITERS4",
	"DELIMITERS5", "DELIMITERS6", "DELIMITERS7", "DELIMITERS8" };
static const int kStyleCount = sizeof kStyleNames / sizeof *kStyleNames;
static const unsigned kParse = pugi::parse_default | pugi::parse_comments | pugi::parse_declaration;
enum { ColFg = 1, ColBg, ColBold, ColItalic, ColUnderline };

static int slotCount(const QString& list) { return list == "Comments" ? 5 : list == "Delimiters" ? 24 : 0; }   // lists whose words carry a 2-digit slot prefix

// "00// 00# 01 02 03/* 04*/"  <->  slot -> "// #"
static QMap<int, QString> decode(const QString& raw) {
	QMap<int, QStringList> m;
	for (const QString& t : raw.split(' ', Qt::SkipEmptyParts)) if (t.size() > 2) m[t.left(2).toInt()] << t.mid(2);
	QMap<int, QString> out;
	for (auto it = m.begin(); it != m.end(); ++it) out[it.key()] = it.value().join(' ');
	return out;
}
static QString encode(const QMap<int, QString>& m, int n) {
	QStringList out;
	for (int i = 0; i < n; ++i) {
		const QString code = QString("%1").arg(i, 2, 10, QChar('0'));
		const QStringList words = m.value(i).split(' ', Qt::SkipEmptyParts);
		if (words.isEmpty()) out << code;
		for (const QString& w : words) out << code + w;
	}
	return out.join(' ');
}

static QStringList udlFiles(NppHost& h) {
	QStringList f;
	const QString shared = QString::fromStdWString(h.profileDir()) + "/userDefineLang.xml";
	if (QFile::exists(shared)) f << shared;
	QDir d(QString::fromStdWString(h.udlFolder()));
	for (const QString& n : d.entryList({ "*.xml" }, QDir::Files, QDir::Name)) f << d.filePath(n);
	return f;
}
static pugi::xml_node findLang(pugi::xml_document& doc, const QString& name) {
	return doc.child("NotepadPlus").find_child_by_attribute("UserLang", "name", name.toUtf8().constData());
}

UdlDialog::UdlDialog(NppHost& host, QWidget* parent) : QDialog(parent), _h(host) {
	setWindowTitle(tr("User Defined Language"));
	_lang = new QComboBox; _name = new QLineEdit; _ext = new QLineEdit; _ext->setPlaceholderText(tr("extensions without dots, separated by spaces"));
	_ignoreCase = new QCheckBox(tr("Ignore case"));
	auto* top = new QHBoxLayout;
	top->addWidget(new QLabel(tr("Language:"))); top->addWidget(_lang, 1);
	auto* bNew = new QPushButton(tr("New...")); auto* bDel = new QPushButton(tr("Remove"));
	top->addWidget(bNew); top->addWidget(bDel);
	auto* id = new QHBoxLayout;
	id->addWidget(new QLabel(tr("Name:"))); id->addWidget(_name, 1); id->addWidget(new QLabel(tr("Ext.:"))); id->addWidget(_ext, 1); id->addWidget(_ignoreCase);

	for (int i = 1; i <= 8; ++i) _fields.push_back({ tr("Keywords"), tr("Keywords %1").arg(i), QString("Keywords%1").arg(i), -1 });
	_fields.push_back({ tr("Keywords"), tr("Operators (no separator needed)"), "Operators1", -1 });
	_fields.push_back({ tr("Keywords"), tr("Operators (separator needed)"), "Operators2", -1 });
	const char* com[] = { "Line comment: start", "Line comment: continue character", "Line comment: end", "Block comment: start", "Block comment: end" };
	for (int i = 0; i < 5; ++i) _fields.push_back({ tr("Comments && Numbers"), tr(com[i]), "Comments", i });
	const char* num[][2] = { { "Number prefix 1", "Numbers, prefix1" }, { "Number prefix 2", "Numbers, prefix2" }, { "Number extras 1", "Numbers, extras1" },
		{ "Number extras 2", "Numbers, extras2" }, { "Number suffix 1", "Numbers, suffix1" }, { "Number suffix 2", "Numbers, suffix2" }, { "Number range", "Numbers, range" } };
	for (auto& n : num) _fields.push_back({ tr("Comments && Numbers"), tr(n[0]), n[1], -1 });
	const char* fold[][2] = { { "Folding in code, style 1", "Folders in code1" }, { "Folding in code, style 2", "Folders in code2" }, { "Folding in comment", "Folders in comment" } };
	for (auto& f : fold) for (const char* part : { "open", "middle", "close" }) _fields.push_back({ tr("Folding"), tr("%1: %2").arg(tr(f[0]), tr(part)), QString("%1, %2").arg(f[1], part), -1 });
	for (int i = 0; i < 8; ++i) { int k = 0; for (const char* part : { "open", "escape", "close" }) _fields.push_back({ tr("Delimiters"), tr("Delimiter %1: %2").arg(i + 1).arg(tr(part)), "Delimiters", 3 * i + k++ }); }

	auto* tabs = new QTabWidget;
	std::map<QString, QFormLayout*> forms;
	for (Field& f : _fields) {
		QFormLayout*& form = forms[f.tab];
		if (!form) { auto* page = new QWidget; form = new QFormLayout(page); form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);   /* macOS forms do not stretch their fields by default */ auto* sc = new QScrollArea; sc->setWidgetResizable(true); sc->setFrameShape(QFrame::NoFrame); sc->setWidget(page); tabs->addTab(sc, f.tab); }
		f.edit = new QLineEdit;
		if (f.list.startsWith("Keywords")) {                      // keyword groups can match as prefixes
			int g = f.list.mid(8).toInt() - 1;
			auto* row = new QHBoxLayout; row->addWidget(f.edit, 1); row->addWidget(_prefix[g] = new QCheckBox(tr("Prefix mode")));
			form->addRow(f.label, row);
		} else form->addRow(f.label, f.edit);
	}
	_styles = new QTableWidget(kStyleCount, 6);
	_styles->setHorizontalHeaderLabels({ tr("Style"), tr("Foreground"), tr("Background"), tr("Bold"), tr("Italic"), tr("Underline") });
	_styles->verticalHeader()->hide(); _styles->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	_styles->setSelectionMode(QAbstractItemView::NoSelection);
	for (int r = 0; r < kStyleCount; ++r) {
		auto* n = new QTableWidgetItem(kStyleNames[r]); n->setFlags(Qt::ItemIsEnabled); _styles->setItem(r, 0, n);
		for (int c : { ColFg, ColBg }) { auto* it = new QTableWidgetItem; it->setFlags(Qt::ItemIsEnabled); _styles->setItem(r, c, it); }
		for (int c : { ColBold, ColItalic, ColUnderline }) { auto* it = new QTableWidgetItem; it->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable); it->setCheckState(Qt::Unchecked); _styles->setItem(r, c, it); }
	}
	connect(_styles, &QTableWidget::cellDoubleClicked, this, [this](int r, int c) {
		if (c != ColFg && c != ColBg) return;
		QColor col = QColorDialog::getColor(_styles->item(r, c)->background().color(), this);
		if (col.isValid()) _styles->item(r, c)->setBackground(col); });
	auto* stylePage = new QWidget; auto* sl = new QVBoxLayout(stylePage);
	sl->addWidget(new QLabel(tr("Double-click a colour to change it."))); sl->addWidget(_styles);
	tabs->addTab(stylePage, tr("Styles"));

	auto* box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close);
	auto* lay = new QVBoxLayout(this);
	lay->addLayout(top); lay->addLayout(id); lay->addWidget(tabs, 1); lay->addWidget(box);
	connect(box->button(QDialogButtonBox::Save), &QPushButton::clicked, this, [this] { save(); });
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(bNew, &QPushButton::clicked, this, [this] {
		bool ok = false; QString n = QInputDialog::getText(this, tr("New Language"), tr("Name:"), QLineEdit::Normal, QString(), &ok).trimmed();
		if (ok && !n.isEmpty()) newLanguage(n); });
	connect(bDel, &QPushButton::clicked, this, [this] {
		if (!_name->text().isEmpty() && QMessageBox::question(this, tr("Remove Language"), tr("Remove the language \"%1\"?").arg(_name->text())) == QMessageBox::Yes) removeCurrent(); });
	connect(_lang, &QComboBox::activated, this, [this] { load(); });
	resize(720, 560);
	rescan(QString());
}

void UdlDialog::rescan(const QString& selectName) {
	_lang->clear();
	for (const QString& f : udlFiles(_h)) {
		pugi::xml_document doc;
		if (!doc.load_file(f.toUtf8().constData(), kParse)) continue;
		for (pugi::xml_node ul : doc.child("NotepadPlus").children("UserLang")) _lang->addItem(QString::fromUtf8(ul.attribute("name").value()), f);
	}
	int i = _lang->findText(selectName);
	_lang->setCurrentIndex(i >= 0 ? i : _lang->count() ? 0 : -1);
	load();
}

void UdlDialog::clearForm() {
	_name->clear(); _ext->clear(); _ignoreCase->setChecked(false);
	for (Field& f : _fields) f.edit->clear();
	for (QCheckBox* p : _prefix) p->setChecked(false);
	for (int r = 0; r < kStyleCount; ++r) {
		_styles->item(r, ColFg)->setBackground(Qt::black); _styles->item(r, ColBg)->setBackground(Qt::white);
		for (int c : { ColBold, ColItalic, ColUnderline }) _styles->item(r, c)->setCheckState(Qt::Unchecked);
	}
}

void UdlDialog::load() {
	clearForm();
	_file = _lang->currentData().toString(); _origName = _lang->currentText();
	pugi::xml_document doc;
	if (_file.isEmpty() || !doc.load_file(_file.toUtf8().constData(), kParse)) { _origName.clear(); return; }
	pugi::xml_node ul = findLang(doc, _origName);
	_name->setText(_origName); _ext->setText(QString::fromUtf8(ul.attribute("ext").value()));
	_ignoreCase->setChecked(QByteArray(ul.child("Settings").child("Global").attribute("caseIgnored").value()) == "yes");
	for (int g = 0; g < 8; ++g) _prefix[g]->setChecked(QByteArray(ul.child("Settings").child("Prefix").attribute(QString("Keywords%1").arg(g + 1).toUtf8().constData()).value()) == "yes");
	for (pugi::xml_node kw : ul.child("KeywordLists").children("Keywords")) {
		const QString list = QString::fromUtf8(kw.attribute("name").value()), raw = QString::fromUtf8(kw.text().get());
		const QMap<int, QString> coded = slotCount(list) ? decode(raw) : QMap<int, QString>();
		for (Field& f : _fields) if (f.list == list) f.edit->setText(f.code < 0 ? raw : coded.value(f.code));
	}
	for (pugi::xml_node st : ul.child("Styles").children("WordsStyle"))
		for (int r = 0; r < kStyleCount; ++r) if (QByteArray(st.attribute("name").value()) == kStyleNames[r]) {
			_styles->item(r, ColFg)->setBackground(QColor("#" + QString(st.attribute("fgColor").as_string("000000"))));
			_styles->item(r, ColBg)->setBackground(QColor("#" + QString(st.attribute("bgColor").as_string("FFFFFF"))));
			const int fs = st.attribute("fontStyle").as_int();
			int bit = 1; for (int c : { ColBold, ColItalic, ColUnderline }) { _styles->item(r, c)->setCheckState(fs & bit ? Qt::Checked : Qt::Unchecked); bit <<= 1; }
		}
}

void UdlDialog::newLanguage(const QString& name) {
	clearForm();
	QString base = name; base.replace(QRegularExpression("[^A-Za-z0-9_.-]"), "_");
	_file = QDir(QString::fromStdWString(_h.udlFolder())).filePath(base + ".xml"); _origName.clear();
	_lang->addItem(name, _file); _lang->setCurrentIndex(_lang->count() - 1);
	_name->setText(name);
}

bool UdlDialog::select(const QString& name) { int i = _lang->findText(name); if (i < 0) return false; _lang->setCurrentIndex(i); load(); return true; }
void UdlDialog::setExtensions(const QString& ext) { _ext->setText(ext); }
void UdlDialog::setField(const QString& list, int code, const QString& text) { for (Field& f : _fields) if (f.list == list && f.code == code) f.edit->setText(text); }
QString UdlDialog::field(const QString& list, int code) const { for (const Field& f : _fields) if (f.list == list && f.code == code) return f.edit->text(); return QString(); }
void UdlDialog::setStyle(const QString& style, const QColor& fg, bool bold) {
	for (int r = 0; r < kStyleCount; ++r) if (style == kStyleNames[r]) { _styles->item(r, ColFg)->setBackground(fg); _styles->item(r, ColBold)->setCheckState(bold ? Qt::Checked : Qt::Unchecked); }
}

static pugi::xml_node childNamed(pugi::xml_node parent, const char* tag, const char* name = nullptr) {   // find or create
	pugi::xml_node n = name ? parent.find_child_by_attribute(tag, "name", name) : parent.child(tag);
	if (!n) { n = parent.append_child(tag); if (name) n.append_attribute("name") = name; }
	return n;
}
static void setAttr(pugi::xml_node n, const char* a, const QString& v) { if (!n.attribute(a)) n.append_attribute(a); n.attribute(a) = v.toUtf8().constData(); }

bool UdlDialog::save() {
	const QString name = _name->text().trimmed();
	if (name.isEmpty() || _file.isEmpty()) return false;
	if (name != _origName && _lang->findText(name) >= 0 && _lang->findText(name) != _lang->currentIndex()) {
		QMessageBox::warning(this, windowTitle(), tr("A language named \"%1\" already exists.").arg(name)); return false; }
	pugi::xml_document doc;
	doc.load_file(_file.toUtf8().constData(), kParse);                        // a new file starts empty
	pugi::xml_node root = childNamed(doc, "NotepadPlus");
	pugi::xml_node ul = _origName.isEmpty() ? pugi::xml_node() : findLang(doc, _origName);
	if (!ul) ul = root.append_child("UserLang");
	setAttr(ul, "name", name); setAttr(ul, "ext", _ext->text().trimmed());
	if (!ul.attribute("udlVersion")) ul.append_attribute("udlVersion") = "2.1";
	pugi::xml_node settings = childNamed(ul, "Settings"), global = childNamed(settings, "Global"), prefix = childNamed(settings, "Prefix");
	setAttr(global, "caseIgnored", _ignoreCase->isChecked() ? "yes" : "no");
	for (const char* a : { "allowFoldOfComments", "foldCompact" }) if (!global.attribute(a)) global.append_attribute(a) = "no";
	for (const char* a : { "forcePureLC", "decimalSeparator" }) if (!global.attribute(a)) global.append_attribute(a) = "0";
	for (int g = 0; g < 8; ++g) setAttr(prefix, QString("Keywords%1").arg(g + 1).toUtf8().constData(), _prefix[g]->isChecked() ? "yes" : "no");
	pugi::xml_node lists = childNamed(ul, "KeywordLists");
	QMap<QString, QMap<int, QString>> coded;
	for (const Field& f : _fields) {
		if (f.code >= 0) { coded[f.list][f.code] = f.edit->text(); continue; }
		childNamed(lists, "Keywords", f.list.toUtf8().constData()).text() = f.edit->text().toUtf8().constData();
	}
	for (auto it = coded.begin(); it != coded.end(); ++it)
		childNamed(lists, "Keywords", it.key().toUtf8().constData()).text() = encode(it.value(), slotCount(it.key())).toUtf8().constData();
	pugi::xml_node styles = childNamed(ul, "Styles");
	for (int r = 0; r < kStyleCount; ++r) {
		pugi::xml_node st = childNamed(styles, "WordsStyle", kStyleNames[r]);
		setAttr(st, "fgColor", _styles->item(r, ColFg)->background().color().name().mid(1).toUpper());
		setAttr(st, "bgColor", _styles->item(r, ColBg)->background().color().name().mid(1).toUpper());
		int fs = 0, bit = 1; for (int c : { ColBold, ColItalic, ColUnderline }) { if (_styles->item(r, c)->checkState() == Qt::Checked) fs |= bit; bit <<= 1; }
		setAttr(st, "fontStyle", QString::number(fs));
		if (!st.attribute("nesting")) st.append_attribute("nesting") = "0";
	}
	QDir().mkpath(QFileInfo(_file).absolutePath());
	if (!doc.save_file(_file.toUtf8().constData(), "    ")) { QMessageBox::warning(this, windowTitle(), tr("Cannot write %1").arg(_file)); return false; }
	_h.reloadUdl((_origName.isEmpty() ? name : _origName).toStdWString(), _file.toStdWString());
	const QString old = _origName;
	rescan(name);
	if (!old.isEmpty() && old != name) emit changed(old);
	emit changed(name);
	return true;
}

bool UdlDialog::removeCurrent() {
	const QString name = _origName.isEmpty() ? _name->text() : _origName;
	pugi::xml_document doc;
	if (!_origName.isEmpty() && doc.load_file(_file.toUtf8().constData(), kParse)) {
		pugi::xml_node root = doc.child("NotepadPlus");
		root.remove_child(findLang(doc, _origName));
		if (root.child("UserLang")) doc.save_file(_file.toUtf8().constData(), "    "); else QFile::remove(_file);
		_h.removeUdl(_origName.toStdWString());
	}
	rescan(QString());
	emit changed(name);
	return true;
}
