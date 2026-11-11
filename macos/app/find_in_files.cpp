// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCheckBox>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringConverter>
#include <QVBoxLayout>
#include "find_in_files.h"

static QString decode(const QByteArray& d) {
	if (d.startsWith("\xFF\xFE")) return QStringDecoder(QStringConverter::Utf16LE)(d.mid(2));
	if (d.startsWith("\xFE\xFF")) return QStringDecoder(QStringConverter::Utf16BE)(d.mid(2));
	QStringDecoder dec(QStringConverter::Utf8);
	QString t = dec(d.startsWith("\xEF\xBB\xBF") ? d.mid(3) : d);
	return dec.hasError() ? QString::fromLatin1(d) : t;  // not valid UTF-8: treat as a single-byte (ANSI) file
}

QVector<FifFile> findInFiles(const FifOptions& o, const std::function<bool(const QString&)>& progress, QString* error) {
	QVector<FifFile> out;
	if (o.what.isEmpty()) return out;
	QString pat = o.regex ? o.what : QRegularExpression::escape(o.what);
	if (o.wholeWord) pat = "\\b(?:" + pat + ")\\b";
	QRegularExpression re(pat, o.matchCase ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
	if (!re.isValid()) { if (error) *error = re.errorString(); return out; }

	QStringList filters = o.filters.split(QRegularExpression("[;\\s]+"), Qt::SkipEmptyParts);
	if (filters.isEmpty()) filters << "*";
	QDir::Filters df = QDir::Files | QDir::NoDotAndDotDot | (o.hidden ? QDir::Hidden : QDir::Filters());
	QDirIterator it(o.dir, filters, df, o.recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
		QString path = it.next();
		if (!o.hidden && path.mid(o.dir.size()).contains("/.")) continue;          // files under hidden folders
		if (progress && !progress(path)) break;
		QFile f(path);
		if (f.size() > 64 * 1024 * 1024 || !f.open(QIODevice::ReadOnly)) continue;
		QByteArray data = f.readAll();
		if (data.left(8000).contains('\0') && !data.startsWith("\xFF\xFE") && !data.startsWith("\xFE\xFF")) continue; // binary
		const QStringList lines = decode(data).split('\n');
		FifFile ff; ff.path = path;
		for (int i = 0; i < lines.size(); ++i) {
			QString line = lines[i]; if (line.endsWith('\r')) line.chop(1);
			auto m = re.globalMatch(line);
			while (m.hasNext()) {
				auto mm = m.next();
				if (mm.capturedLength() > 0) ff.hits.push_back({ i + 1, line, (int)mm.capturedStart(), (int)mm.capturedLength() });
			}
		}
		if (!ff.hits.isEmpty()) out.push_back(std::move(ff));
	}
	return out;
}

FindInFilesDialog::FindInFilesDialog(QWidget* parent) : QDialog(parent) {
	setWindowTitle(tr("Find in Files"));
	setWindowFlag(Qt::Tool);
	_what = new QLineEdit; _dir = new QLineEdit; _filters = new QLineEdit("*.*");
	_case = new QCheckBox(tr("Match case")); _word = new QCheckBox(tr("Whole word")); _regex = new QCheckBox(tr("Regular expression"));
	_recursive = new QCheckBox(tr("In all sub-folders")); _recursive->setChecked(true); _hidden = new QCheckBox(tr("In hidden folders and files"));
	auto* browse = new QPushButton(tr("..."));
	auto* go = new QPushButton(tr("Find All")); go->setDefault(true);
	auto* grid = new QGridLayout;
	grid->addWidget(new QLabel(tr("Find what:")), 0, 0); grid->addWidget(_what, 0, 1, 1, 2);
	grid->addWidget(new QLabel(tr("Filters:")), 1, 0); grid->addWidget(_filters, 1, 1, 1, 2);
	grid->addWidget(new QLabel(tr("Directory:")), 2, 0); grid->addWidget(_dir, 2, 1); grid->addWidget(browse, 2, 2);
	auto* opts = new QHBoxLayout; for (auto* c : {_case, _word, _regex}) opts->addWidget(c);
	auto* opts2 = new QHBoxLayout; opts2->addWidget(_recursive); opts2->addWidget(_hidden);
	auto* lay = new QVBoxLayout(this);
	lay->addLayout(grid); lay->addLayout(opts); lay->addLayout(opts2); lay->addWidget(go, 0, Qt::AlignRight);
	setMinimumWidth(560);
	connect(browse, &QPushButton::clicked, this, [this] { QString d = QFileDialog::getExistingDirectory(this, tr("Directory"), _dir->text()); if (!d.isEmpty()) _dir->setText(d); });
	auto fire = [this] {
		FifOptions o; o.what = _what->text(); o.dir = _dir->text(); o.filters = _filters->text();
		o.matchCase = _case->isChecked(); o.wholeWord = _word->isChecked(); o.regex = _regex->isChecked();
		o.recursive = _recursive->isChecked(); o.hidden = _hidden->isChecked();
		emit searchRequested(o);
	};
	connect(go, &QPushButton::clicked, this, fire);
	connect(_what, &QLineEdit::returnPressed, this, fire);
}

void FindInFilesDialog::showDialog(const QString& seed, const QString& dir) {
	if (!seed.isEmpty() && !seed.contains('\n')) _what->setText(seed);
	if (_dir->text().isEmpty()) _dir->setText(dir);
	show(); raise(); activateWindow(); _what->setFocus(); _what->selectAll();
}
