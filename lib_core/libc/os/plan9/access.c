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

/* access() is the one call in this group that runs the OTHER way round
 * from everything else under syscall/os/plan9/: on Linux, Darwin and
 * Windows it is a real kernel call needing no glue, while Plan9 has no
 * access(2) syscall at all -- it is an ordinary libc function built out
 * of open(). Hence this file, rather than another stub in
 * svc_$cputype.s.
 *
 * Ported from principia's lib_core/libc/9sys/access.c, which is the
 * reference implementation (see lib_core/libc/syscall/os/plan9/sys.h's
 * "coupling: principia" note), including its omode[] table and its own
 * "only approximate" remarks: Plan9's open() has no way to ask for
 * exec-and-write, so the two combinations involving both collapse onto
 * something close enough.
 *
 * ONE DELIBERATE DIVERGENCE, and it is a real gap. principia special-
 * cases AEXIST (mode 0) by calling dirstat() instead of open(), because
 * "does this file exist" and "can I open it for reading" are not the
 * same question -- a file that exists but denies read permission, or a
 * directory in some cases, answers differently. dirstat() needs the
 * stat(2) syscall, which this libc does not have yet (see
 * docs/claude_notes/plan_syscalls.txt's Tier 3), so AEXIST falls
 * through to the table below like every other mode -- and omode[0] is
 * 0, i.e. OREAD, so access(f, AEXIST) currently means "can I open it
 * for reading". Restore principia's dirstat() branch as soon as stat
 * lands; that is the whole fix, no other change needed here.
 */
int
access(char *name, int mode)
{
	fdt fd;
	static char omode[] = {
		0,		/* AEXIST -- see the gap noted above */
		OEXEC,		/* AEXEC */
		OWRITE,		/* AWRITE */
		ORDWR,		/* AWRITE|AEXEC */
		OREAD,		/* AREAD */
		OEXEC,		/* AREAD|AEXEC -- only approximate */
		ORDWR,		/* AREAD|AWRITE */
		ORDWR		/* AREAD|AWRITE|AEXEC -- only approximate */
	};

	fd = open(name, omode[mode&7]);
	if (fd >= 0) {
		close(fd);
		return 0;
	}
	return -1;
}
