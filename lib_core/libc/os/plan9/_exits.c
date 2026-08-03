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

/* Plan9's exits(char*) IS the raw EXITS syscall (syscall/os/plan9/
 * svc_$cputype.s's own TEXT exits(SB)) -- there is no separate abrupt
 * form the kernel offers, so _exits() here is just an alias for it,
 * the mirror image of port/exits.c's collapsed pair for every other
 * GOOS (see that file's comment).
 */
void
_exits(char *s)
{
	exits(s);
}
