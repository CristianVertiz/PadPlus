// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <map>
#include "panels.h"

// ================================================================================================== Function List
FunctionListPanel::FunctionListPanel(QWidget* parent) : QDockWidget(tr("Function List"), parent) {
	setObjectName("functionListDock");
	_filter = new QLineEdit; _filter->setPlaceholderText(tr("Search")); _filter->setClearButtonEnabled(true);
	_sort = new QToolButton; _sort->setText("A→Z"); _sort->setCheckable(true); _sort->setToolTip(tr("Sort functions alphabetically"));
	_tree = new QTreeWidget; _tree->setHeaderHidden(true); _tree->setUniformRowHeights(true);
	auto* top = new QHBoxLayout; top->addWidget(_filter, 1); top->addWidget(_sort);
	auto* w = new QWidget; auto* lay = new QVBoxLayout(w); lay->setContentsMargins(4, 4, 4, 4);
	lay->addLayout(top); lay->addWidget(_tree, 1);
	setWidget(w);
	connect(_filter, &QLineEdit::textChanged, this, [this] { rebuild(); });
	connect(_sort, &QToolButton::toggled, this, [this](bool on) { _sorted = on; rebuild(); });
	connect(_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* it) { QVariant v = it->data(0, Qt::UserRole); if (v.isValid()) emit jump(v.toLongLong()); });
}

void FunctionListPanel::setSorted(bool s) { _sort->setChecked(s); }

void FunctionListPanel::setEntries(const QString& file, const std::vector<NppHost::FuncEntry>& e) { _file = file; _entries = e; rebuild(); }

void FunctionListPanel::rebuild() {
	_tree->clear();
	if (_file.isEmpty()) return;
	auto* root = new QTreeWidgetItem(_tree, QStringList(_file));
	QString f = _filter->text();
	std::vector<NppHost::FuncEntry> list = _entries;
	if (_sorted) std::stable_sort(list.begin(), list.end(), [](const NppHost::FuncEntry& a, const NppHost::FuncEntry& b) { return QString::fromStdWString(a.name).compare(QString::fromStdWString(b.name), Qt::CaseInsensitive) < 0; });
	std::map<QString, QTreeWidgetItem*> parents;     // class/struct name -> its node
	for (const auto& e : list) {
		QString name = QString::fromStdWString(e.name), parent = QString::fromStdWString(e.parent);
		if (!f.isEmpty() && !name.contains(f, Qt::CaseInsensitive) && !parent.contains(f, Qt::CaseInsensitive)) continue;
		QTreeWidgetItem* host = root;
		if (!parent.isEmpty()) {
			auto it = parents.find(parent);
			if (it == parents.end()) it = parents.emplace(parent, new QTreeWidgetItem(root, QStringList(parent))).first;
			host = it->second;
		}
		auto* item = new QTreeWidgetItem(host, QStringList(name));
		item->setData(0, Qt::UserRole, static_cast<qlonglong>(e.pos));
	}
	_tree->expandAll();
}

// ================================================================================================== Folder as Workspace
static QIcon iconFor(const QFileInfo& fi) { static QFileIconProvider p; return p.icon(fi); }

WorkspacePanel::WorkspacePanel(QWidget* parent) : QDockWidget(tr("Folder as Workspace"), parent) {
	setObjectName("workspaceDock");
	_tree = new QTreeWidget; _tree->setHeaderHidden(true); _tree->setContextMenuPolicy(Qt::CustomContextMenu);
	setWidget(_tree);
	connect(_tree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem* it) { populate(it); });
	connect(_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* it) { if (!it->data(0, Qt::UserRole + 1).toBool()) emit openFile(it->data(0, Qt::UserRole).toString()); });
	connect(_tree, &QTreeWidget::customContextMenuRequested, this, &WorkspacePanel::contextMenu);
}

QStringList WorkspacePanel::roots() const {
	QStringList r;
	for (int i = 0; i < _tree->topLevelItemCount(); ++i) r << _tree->topLevelItem(i)->data(0, Qt::UserRole).toString();
	return r;
}

void WorkspacePanel::addRoot(const QString& dirIn) {
	QString dir = QDir(dirIn).absolutePath();
	if (roots().contains(dir)) return;
	auto* it = new QTreeWidgetItem(_tree, QStringList(QFileInfo(dir).fileName().isEmpty() ? dir : QFileInfo(dir).fileName()));
	it->setData(0, Qt::UserRole, dir); it->setData(0, Qt::UserRole + 1, true); it->setData(0, Qt::UserRole + 2, true); // 2 = is a root
	it->setIcon(0, iconFor(QFileInfo(dir))); it->setToolTip(0, dir);
	populate(it);
	it->setExpanded(true);
}

void WorkspacePanel::populate(QTreeWidgetItem* dirItem) {
	qDeleteAll(dirItem->takeChildren());
	QDir d(dirItem->data(0, Qt::UserRole).toString());
	for (const QFileInfo& fi : d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::IgnoreCase | QDir::Name)) {
		if (fi.fileName() == ".DS_Store") continue;
		auto* c = new QTreeWidgetItem(dirItem, QStringList(fi.fileName()));
		c->setData(0, Qt::UserRole, fi.absoluteFilePath()); c->setData(0, Qt::UserRole + 1, fi.isDir()); c->setIcon(0, iconFor(fi));
		if (fi.isDir()) new QTreeWidgetItem(c);   // placeholder so the expand arrow shows; filled on expand
	}
}

void WorkspacePanel::refresh(QTreeWidgetItem* item) {
	QTreeWidgetItem* dir = item->data(0, Qt::UserRole + 1).toBool() ? item : item->parent();
	if (dir) populate(dir);
	if (dir) dir->setExpanded(true);
}

void WorkspacePanel::contextMenu(const QPoint& pos) {
	QTreeWidgetItem* it = _tree->itemAt(pos);
	if (!it) return;
	QString path = it->data(0, Qt::UserRole).toString();
	bool isDir = it->data(0, Qt::UserRole + 1).toBool(), isRoot = it->data(0, Qt::UserRole + 2).toBool();
	QString dir = isDir ? path : QFileInfo(path).absolutePath();
	QMenu m;
	if (!isDir) m.addAction(tr("Open"), this, [this, path] { emit openFile(path); });
	m.addAction(tr("Reveal in Finder"), this, [path] { QProcess::startDetached("open", { "-R", path }); });
	m.addAction(tr("Copy Path"), this, [path] { QApplication::clipboard()->setText(path); });
	m.addSeparator();
	m.addAction(tr("New File..."), this, [this, it, dir] {
		QString n = QInputDialog::getText(this, tr("New File"), tr("Name:")); if (n.isEmpty()) return;
		QFile f(QDir(dir).filePath(n)); if (f.open(QIODevice::WriteOnly)) { f.close(); refresh(it); emit openFile(f.fileName()); } });
	m.addAction(tr("New Folder..."), this, [this, it, dir] {
		QString n = QInputDialog::getText(this, tr("New Folder"), tr("Name:")); if (n.isEmpty()) return;
		QDir(dir).mkpath(n); refresh(it); });
	if (!isRoot) {
		m.addAction(tr("Rename..."), this, [this, it, path] {
			QString n = QInputDialog::getText(this, tr("Rename"), tr("New name:"), QLineEdit::Normal, QFileInfo(path).fileName()); if (n.isEmpty()) return;
			QFile::rename(path, QFileInfo(path).absoluteDir().filePath(n)); refresh(it); });
		m.addAction(tr("Move to Trash"), this, [this, it, path] {
			if (QMessageBox::question(this, tr("Move to Trash"), tr("Move \"%1\" to the Trash?").arg(QFileInfo(path).fileName())) == QMessageBox::Yes) {
				QFile::moveToTrash(path); refresh(it); } });
	}
	m.addSeparator();
	m.addAction(tr("Refresh"), this, [this, it] { refresh(it); });
	if (isRoot) m.addAction(tr("Remove from Workspace"), this, [this, it] { delete it; });
	m.exec(_tree->viewport()->mapToGlobal(pos));
}

// ================================================================================================== Document List
DocListPanel::DocListPanel(QWidget* parent) : QDockWidget(tr("Document List"), parent) {
	setObjectName("docListDock");
	_tree = new QTreeWidget; _tree->setColumnCount(2); _tree->setHeaderLabels({ tr("Name"), tr("Path") });
	_tree->setRootIsDecorated(false); _tree->setUniformRowHeights(true);
	setWidget(_tree);
	connect(_tree, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* it) { emit activated(it->data(0, Qt::UserRole).toInt(), it->data(0, Qt::UserRole + 1).toInt()); });
}

void DocListPanel::setDocs(const std::vector<Doc>& docs, int av, int ai) {
	_tree->blockSignals(true);
	_tree->clear();
	for (const Doc& d : docs) {
		auto* it = new QTreeWidgetItem(_tree, QStringList{ d.name + (d.dirty ? " *" : ""), d.path });
		it->setData(0, Qt::UserRole, d.view); it->setData(0, Qt::UserRole + 1, d.index);
		it->setToolTip(0, d.path); it->setIcon(0, iconFor(QFileInfo(d.path)));
		if (d.view == av && d.index == ai) _tree->setCurrentItem(it);
	}
	_tree->header()->resizeSections(QHeaderView::ResizeToContents);
	_tree->blockSignals(false);
}
