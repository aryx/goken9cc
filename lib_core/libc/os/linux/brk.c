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

/* The "os/$GOOS/" glue for brk(), exactly parallel to open.c next to it:
 * syscall/os/linux/'s _sysbrk() is the faithful raw adapter, and this is
 * where a different API *shape* gets bridged on top of it.
 *
 * The shape difference is unusually sharp here. Linux's brk(2) does not
 * use the negative-errno convention every other syscall in this tree
 * does -- it returns the NEW break on success, and reports failure by
 * returning the UNCHANGED old one. There is no error code at all: the
 * only way to know it failed is to notice you didn't get what you asked
 * for. include/os/mem.h's brk() is Plan9-shaped instead (0 or -1), which
 * is what port/sbrk.c is written against, so the two have to be
 * reconciled somewhere.
 *
 * Somewhere is here, once, rather than in each of the seven
 * syscall/os/linux/syscall_linux_$cputype.h shims -- this bridge is
 * identical on every arch (only the syscall NUMBER differs, and that
 * already lives in numbers_$cputype.h), so it has no business being
 * per-arch. Compare os/linux/open.c, which is arch-independent for the
 * same reason while _sysopen() itself is not.
 *
 * `<` and not `!=`: the kernel rounds the break up to a page boundary,
 * so a successful call can legitimately return MORE than requested.
 * Treating that as failure would break every non-page-aligned sbrk() --
 * which, since port/sbrk.c only ever rounds to 8, is most of them.
 */

extern uintptr _sysbrk(void *addr);

int
brk(void *addr)
{
	if(_sysbrk(addr) < (uintptr)addr)
		return -1;
	return 0;
}
