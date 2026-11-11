// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// "Define Your Language": edits user defined languages straight in their XML files (userDefineLangs/*.xml, userDefineLang.xml),
// then has the core re-import the file, so loading and lexing stay Notepad++'s own code.
#pragma once
#include <QDialog>
#include <QVector>
class QComboBox; class QLineEdit; class QCheckBox; class QTableWidget; class QColor; class NppHost;

class UdlDialog : public QDialog {
	Q_OBJECT
public:
	UdlDialog(NppHost& host, QWidget* parent);
	// also used by tests
	void newLanguage(const QString& name);
	bool select(const QString& name);
	void setExtensions(const QString& ext);
	void setField(const QString& list, int code, const QString& text);   // list = the XML list name; code -1 for plain lists, else the 2-digit slot
	QString field(const QString& list, int code) const;
	void setStyle(const QString& style, const QColor& fg, bool bold);
	bool save();
	bool removeCurrent();
signals:
	void changed(const QString& name);          // saved or removed: open documents using it need restyling
private:
	struct Field { QString tab, label, list; int code; QLineEdit* edit = nullptr; };
	void rescan(const QString& selectName);
	void load();
	void clearForm();
	NppHost& _h;
	QComboBox* _lang; QLineEdit *_name, *_ext; QCheckBox *_ignoreCase, *_prefix[8]; QTableWidget* _styles;
	QVector<Field> _fields;
	QString _file, _origName;                   // where the shown language lives; _origName empty = not saved yet
};
