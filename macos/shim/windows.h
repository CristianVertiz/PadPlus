// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Cristian Vertiz. Part of PadPlus, a macOS editor based on the Notepad++ source code.
// Shim so lexilla/lexers/LexUser.cxx (UDL lexer) compiles on macOS without editing vendored code.
#pragma once
#include <cstdio>
#include <cstdlib>
inline char* _itoa(int v, char* buf, int radix) { // radix is always 10 in LexUser
	(void)radix; snprintf(buf, 33, "%d", v); return buf;
}
