// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QCoreApplication>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include "chrome.h"

ChromePalette chromePalette(bool dark) {
	if (dark) return { "#262626", "#3a3a3a", "#454545", "#e0e0e0", "#9a9a9a", "#4a4a4a", "#7fd3b0" };   // accent: mint bar on the active tab
	return { "#ececec", "#ffffff", "#e0e0e0", "#1e1e1e", "#6e6e6e", "#c8c8c8", "#f5a33a" };             // light: orange bar
}

QString chromeStyleSheet(const ChromePalette& p) {
	return QString(
		"QWidget#tabStrip { background: %1; }"
		"QTabBar { background: %1; qproperty-expanding: false; }"
		"QTabBar QToolButton { background: %1; border: none; }"
		"QTabBar::tab { background: %1; color: %5; border: none; border-top: 3px solid transparent; border-right: 1px solid %6; padding: 5px 6px 5px 12px; min-width: 70px; }"
		"QTabBar::tab:selected { background: %2; color: %4; border-top: 3px solid %7; }"
		"QTabBar::tab:hover:!selected { background: %3; color: %4; }"
		"QStatusBar { background: %1; color: %4; }"
		"QStatusBar::item { border: none; }"
		"QStatusBar QLabel { color: %4; padding: 2px 10px; }"
		"QStatusBar QLabel[sep=\"true\"] { border-left: 1px solid %6; }"
		"QToolBar { background: %1; border: none; spacing: 3px; padding: 2px 4px; }"
		"QToolBar QToolButton { background: transparent; border: 1px solid transparent; border-radius: 3px; padding: 2px; }"
		"QToolBar QToolButton:hover { background: %3; }"
		"QToolBar QToolButton:checked { background: %2; border-color: %6; }"
		"QToolBar::separator { background: %6; width: 1px; margin: 4px 5px; }")
		.arg(p.bg.name(), p.soft.name(), p.hot.name(), p.text.name(), p.dim.name(), p.edge.name(), p.accent.name());
}

static QPixmap canvas() { QPixmap pm(32, 32); pm.setDevicePixelRatio(2); pm.fill(Qt::transparent); return pm; }   // 16x16 logical, crisp on Retina

QIcon pencilIcon(const QColor& c) {
	QPixmap pm = canvas(); QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen); p.setBrush(c);
	p.translate(8, 8); p.rotate(45);
	p.drawRoundedRect(QRectF(-2, -7, 4, 10), 1, 1);                       // body
	p.drawPolygon(QPolygonF({ QPointF(-2, 4), QPointF(2, 4), QPointF(0, 8) })); // tip
	return QIcon(pm);
}

QIcon closeIcon(const QColor& c) {
	QPixmap pm = canvas(); QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(QPen(c, 1.2)); p.setBrush(Qt::NoBrush);
	p.drawRoundedRect(QRectF(2, 2, 12, 12), 1.5, 1.5);                    // boxed x, like Notepad++'s tab close button
	p.drawLine(QPointF(5.2, 5.2), QPointF(10.8, 10.8)); p.drawLine(QPointF(10.8, 5.2), QPointF(5.2, 10.8));
	return QIcon(pm);
}

bool isDarkColor(long ref) {
	int r = ref & 0xFF, g = (ref >> 8) & 0xFF, b = (ref >> 16) & 0xFF;
	return 0.299 * r + 0.587 * g + 0.114 * b < 128;
}

QString resourcesDir() {
	QString dir = QCoreApplication::applicationDirPath();
	return dir.endsWith("/Contents/MacOS") ? dir.chopped(5) + "Resources" : dir;   // ".../Contents/MacOS" -> ".../Contents/Resources"
}

QIcon toolbarIcon(const QString& dir, bool dark, const QString& name) {
	QString base = QString("%1/icons/%2/toolbar/regular/%3").arg(dir, dark ? "dark" : "light", name);
	QIcon ic;
	if (QFileInfo::exists(base + "_off.ico")) ic.addFile(base + "_off.ico", QSize(), QIcon::Normal);
	if (QFileInfo::exists(base + "_dis.ico")) ic.addFile(base + "_dis.ico", QSize(), QIcon::Disabled);
	return ic;
}
