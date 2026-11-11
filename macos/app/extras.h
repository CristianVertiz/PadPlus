// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Extra panels and bars: Clipboard History, ANSI Chars, Document Map, and the incremental search bar.
#pragma once
#include <QDockWidget>
#include <QWidget>
#include <functional>
class QListWidget; class QTableWidget; class QLineEdit; class QCheckBox; class QLabel; class ScintillaEdit;

class ClipboardPanel : public QDockWidget {                 // text copied while PadPlus runs; double-click pastes it
	Q_OBJECT
public:
	explicit ClipboardPanel(QWidget* parent);
	QListWidget* list() const { return _list; }
	void record(const QString& text);                       // also called by tests
signals:
	void paste(const QString& text);
private:
	QListWidget* _list;
};

class AnsiPanel : public QDockWidget {                      // the 256 Latin-1 characters; double-click inserts one
	Q_OBJECT
public:
	explicit AnsiPanel(QWidget* parent);
	QTableWidget* table() const { return _table; }
signals:
	void insertChar(const QString& ch);
private:
	QTableWidget* _table;
};

class DocMapPanel : public QDockWidget {                    // a miniature of the document sharing its text; click to scroll
	Q_OBJECT
public:
	explicit DocMapPanel(QWidget* parent);
	void follow(ScintillaEdit* main);                       // show main's document, mark the visible lines
	ScintillaEdit* map() const { return _map; }
	QWidget* band() const { return _band; }
signals:
	void scrollToLine(int line);
protected:
	bool eventFilter(QObject*, QEvent*) override;
private:
	ScintillaEdit *_map, *_main = nullptr;
	QWidget* _band;
};

class IncSearchBar : public QWidget {                       // Search > Incremental Search
	Q_OBJECT
public:
	IncSearchBar(std::function<ScintillaEdit*()> editor, QWidget* parent);
	void open(const QString& seed);
	void setText(const QString& t);                         // tests
	QString state() const;                                  // "found" / "not found" / ""
public slots:
	void next() { search(true, false); }
	void prev() { search(false, false); }
	void closeBar();
private:
	void search(bool forward, bool fromStart);
	void highlight();
	std::function<ScintillaEdit*()> _editor;
	QLineEdit* _edit; QCheckBox *_case, *_all; QLabel* _msg;
};
