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

/* unsharp() (include/os/path.h) -- plan9port's own "#9/..."/"#d/..."
 * sharp-path translation (BOOT/lib9/unsharp.c's own reference version
 * resolves "#9" against a real GOROOT via get9root(), which this libc
 * has no equivalent of yet -- no compiled-in ROOT path, no #9-namespace
 * concept at all). Rather than fake that up, this is an honest no-op:
 * return the path unchanged, same decision rc/goken.c's own pathinit()
 * already made for the identical "#9/etc/rcmain.unix" case ("Rcmain is
 * only ever dot-sourced by main.c's own bootstrap code, which already
 * fails gracefully like any other missing sourced file"). Real callers
 * needing a translated path (generators/yacc/yacc.c's own default
 * YACCPAR lookup, unsharp(PARSER)) should set the equivalent env var
 * explicitly instead -- this project's own env.sh already documents
 * doing exactly that for YACCPAR.
 */
char*
unsharp(char *old)
{
	return old;
}
