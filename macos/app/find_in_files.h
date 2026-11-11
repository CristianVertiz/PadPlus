// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Find in Files: a Qt-only search engine plus its dialog. Results are shown by the main window's "Search results" dock.
#pragma once
#include <QDialog>
#include <QString>
#include <QVector>
#include <functional>
class QLineEdit; class QCheckBox;

struct FifOptions {
	QString what, dir, filters = "*.*";
	bool matchCase = false, wholeWord = false, regex = false, recursive = true, hidden = false;
};
struct FifHit { int line = 0; QString lineText; int column = 0, length = 0; };   // line is 1-based; column/length count QChars
struct FifFile { QString path; QVector<FifHit> hits; };

// progress(file) returns false to cancel. On a bad regex returns empty and fills *error.
QVector<FifFile> findInFiles(const FifOptions& o, const std::function<bool(const QString&)>& progress = {}, QString* error = nullptr);

class FindInFilesDialog : public QDialog {
	Q_OBJECT
public:
	explicit FindInFilesDialog(QWidget* parent);
	void showDialog(const QString& seed, const QString& dir);
signals:
	void searchRequested(const FifOptions&);
private:
	QLineEdit *_what, *_dir, *_filters;
	QCheckBox *_case, *_word, *_regex, *_recursive, *_hidden;
};
