/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */
#include <u.h>
#include <libc.h>

/* argv0: the program name, set by main() itself (see cat.c: "argv0 =
 * "cat";") or by include/flag/cli.h's ARGBEGIN, and read by sysfatal()
 * (port/sysfatal.c) to prefix its error message.
 *
 * Just a plain global, unlike ~/principia/lib_core/libc/{386,arm}/
 * argv0.s, which bundles it with _tos/_privates/_nprivates -- those are
 * libthread-scheduler globals this libc has no use for yet, and
 * argv0 itself needs no arch-specific asm to declare (goken's own
 * compilers turn a plain C global into the same BSS symbol on every
 * arch/GOOS, including plan9, which has no special startup
 * initialization for it either -- real Plan9 programs assign it
 * themselves too).
 */
char *argv0;
