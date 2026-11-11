// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Dockable side panels: Function List, Folder as Workspace, Document List.
#pragma once
#include <QDockWidget>
#include <QStringList>
#include <vector>
#include "npp_host.h"
class QLineEdit; class QToolButton; class QTreeWidget; class QTreeWidgetItem;

// ---- Function List: functions/classes of the current document, parsed with Notepad++'s own regex rules (functionList/*.xml) ----
class FunctionListPanel : public QDockWidget {
	Q_OBJECT
public:
	explicit FunctionListPanel(QWidget* parent);
	void setEntries(const QString& fileName, const std::vector<NppHost::FuncEntry>& entries);
	QTreeWidget* tree() const { return _tree; }
	void setSorted(bool s);
signals:
	void jump(long long pos);
private:
	void rebuild();
	QLineEdit* _filter; QToolButton* _sort; QTreeWidget* _tree;
	QString _file; std::vector<NppHost::FuncEntry> _entries; bool _sorted = false;
};

// ---- Folder as Workspace: one or more root folders, lazily listed ----
class WorkspacePanel : public QDockWidget {
	Q_OBJECT
public:
	explicit WorkspacePanel(QWidget* parent);
	void addRoot(const QString& dir);
	QStringList roots() const;
	QTreeWidget* tree() const { return _tree; }
	void refresh(QTreeWidgetItem* item);
signals:
	void openFile(const QString& path);
private:
	void populate(QTreeWidgetItem* dirItem);
	void contextMenu(const QPoint& pos);
	QTreeWidget* _tree;
};

// ---- Document List: every open document of both views ----
class DocListPanel : public QDockWidget {
	Q_OBJECT
public:
	struct Doc { int view, index; QString name, path; bool dirty; };
	explicit DocListPanel(QWidget* parent);
	void setDocs(const std::vector<Doc>& docs, int activeView, int activeIndex);
	QTreeWidget* tree() const { return _tree; }
signals:
	void activated(int view, int index);
private:
	QTreeWidget* _tree;
};
