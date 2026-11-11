// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Look of the window around the editor: tab bar, status bar and the dark/light colour scheme (the editor itself is styled by the theme).
#pragma once
#include <QColor>
#include <QIcon>
#include <QString>

struct ChromePalette { QColor bg, soft, hot, text, dim, edge, accent; };

ChromePalette chromePalette(bool dark);
QString chromeStyleSheet(const ChromePalette& p);   // QTabBar (Notepad++ tabs) and QStatusBar (separated fields)
QIcon pencilIcon(const QColor& c);                  // "modified" marker shown on a tab
QIcon closeIcon(const QColor& c);                   // per-tab close button
bool isDarkColor(long bgrColorRef);                 // Scintilla COLORREF (0xBBGGRR): is it dark?

QString resourcesDir();                            // <app>.app/Contents/Resources inside a bundle, else the executable's folder

// Notepad++'s own toolbar icons (<resources>/icons/<dark|light>/toolbar/regular/<name>_off.ico and _dis.ico). Null icon if the file is missing.
QIcon toolbarIcon(const QString& resourcesDir, bool dark, const QString& name);
