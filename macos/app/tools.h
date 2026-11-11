// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Tools menu helpers: MD5 / SHA-1 / SHA-256 / SHA-512.
#pragma once
#include <QCryptographicHash>
#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVector>

QString hashHex(QCryptographicHash::Algorithm algo, const QByteArray& data);
struct HashResult { QString file, hash; };                        // hash is empty if the file could not be read
QVector<HashResult> hashFiles(QCryptographicHash::Algorithm algo, const QStringList& paths);

class HashDialog : public QDialog {                               // read-only text with a Copy button
	Q_OBJECT
public:
	HashDialog(const QString& title, const QString& text, QWidget* parent);
};
