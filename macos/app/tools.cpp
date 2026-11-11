// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "tools.h"

QString hashHex(QCryptographicHash::Algorithm a, const QByteArray& d) { return QString::fromLatin1(QCryptographicHash::hash(d, a).toHex()); }

QVector<HashResult> hashFiles(QCryptographicHash::Algorithm a, const QStringList& paths) {
	QVector<HashResult> out;
	for (const QString& p : paths) {
		QFile f(p);
		HashResult r{ p, QString() };
		if (f.open(QIODevice::ReadOnly)) { QCryptographicHash h(a); h.addData(&f); r.hash = QString::fromLatin1(h.result().toHex()); }   // streams: any file size
		out.push_back(r);
	}
	return out;
}

HashDialog::HashDialog(const QString& title, const QString& text, QWidget* parent) : QDialog(parent) {
	setWindowTitle(title);
	resize(700, 260);
	auto* view = new QPlainTextEdit(text); view->setReadOnly(true); view->setLineWrapMode(QPlainTextEdit::NoWrap);
	QFont mono("Menlo"); mono.setStyleHint(QFont::Monospace); view->setFont(mono);
	auto* copy = new QPushButton(tr("Copy to Clipboard")), *close = new QPushButton(tr("Close"));
	auto* row = new QHBoxLayout; row->addStretch(); row->addWidget(copy); row->addWidget(close);
	auto* lay = new QVBoxLayout(this); lay->addWidget(view); lay->addLayout(row);
	connect(copy, &QPushButton::clicked, this, [text] { QGuiApplication::clipboard()->setText(text); });
	connect(close, &QPushButton::clicked, this, &QDialog::accept);
}
