// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Find / Replace dialog (Qt). Uses Scintilla's own search (regex goes through the Boost.Regex search the core links in).
#pragma once
#include <QDialog>
#include <functional>
class QLineEdit; class QCheckBox; class QLabel; class ScintillaEdit;

class FindDialog : public QDialog {
	Q_OBJECT
public:
	FindDialog(std::function<ScintillaEdit*()> activeEditor, QWidget* parent);
	void showFind(const QString& seed);
	void setBookmarkLine(bool on);
	void setQuery(const QString& what, const QString& with, bool matchCase, bool wholeWord, bool regex); // scripting / tests
	QString status() const;
public slots:
	void findNext() { find(true); }
	void findPrev() { find(false); }
	void replaceOne();
	void replaceAll();
	void count();
	void markAll();          // the Mark tab of Notepad++: highlight every match (indicator 31), optionally bookmark its line
	void clearMarks();
private:
	bool find(bool forward);
	long long search(ScintillaEdit* ed, long long start, long long end, const QByteArray& pat);
	int flags() const;
	std::function<ScintillaEdit*()> _editor;
	QLineEdit *_what, *_with;
	QCheckBox *_case, *_word, *_regex, *_wrap, *_bookmark;
	QLabel* _status;
};

// Search > Find characters in range: jump to the next byte whose value lies in a range (non-ASCII, ASCII or a custom one).
class QRadioButton; class QSpinBox;
class FindCharsDialog : public QDialog {
	Q_OBJECT
public:
	FindCharsDialog(std::function<ScintillaEdit*()> activeEditor, QWidget* parent);
	static bool findInRange(ScintillaEdit* ed, int lo, int hi, bool down, bool wrap);   // selects the character; false if there is none
private:
	std::function<ScintillaEdit*()> _editor;
	QRadioButton *_nonAscii, *_ascii, *_custom, *_up; QSpinBox *_from, *_to; QCheckBox* _wrap; QLabel* _status;
};
