// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
#include <QPainter>
#include <QPrinter>
#include "ScintillaEdit.h"
#include "chrome.h"
#include "print_doc.h"

int printDocument(ScintillaEdit* ed, QPrinter* pr) {
	const long len = (long)ed->send(SCI_GETLENGTH);
	if (len == 0) return 0;
	QPainter painter;
	if (!painter.begin(pr)) return 0;
	ed->send(SCI_SETPRINTWRAPMODE, SC_WRAP_WORD);               // long lines wrap on paper
	// dark themes use light text colours that vanish on white paper: print those black on white; light themes print as they look
	ed->send(SCI_SETPRINTCOLOURMODE, isDarkColor((long)ed->send(SCI_STYLEGETBACK, STYLE_DEFAULT)) ? SC_PRINT_BLACKONWHITE : SC_PRINT_NORMAL);
	const QRect page(0, 0, pr->width(), pr->height());          // painter coordinates: the printable area in device pixels
	long pos = 0; int pages = 0;
	while (pos < len) {
		if (pages) pr->newPage();
		long next = ed->format_range(true, pr, pr, page, page, pos, len);   // draws one page, returns where the next one starts
		++pages;
		if (next <= pos) break;                                  // no progress: never loop forever
		pos = next;
	}
	ed->send(SCI_FORMATRANGEFULL, 0, 0);                         // release Scintilla's printing cache
	painter.end();
	return pages;
}
