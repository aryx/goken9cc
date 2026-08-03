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

/* sbrk() for windows. Same situation as darwin's (os/darwin/sbrk.c) and
 * for the same reason: there is no program break here, so port/sbrk.c's
 * bump-over-brk() has nothing to bump and is not built -- see
 * lib_core/libc/mkfile's SBRKOFILES, which chooses between the three
 * implementations rather than #ifdef'ing inside any of them.
 *
 * This is why sbrk rather than brk is the primitive the toolchain codes
 * against (see port/sbrk.c's fuller comment): sbrk promises n contiguous
 * bytes, never that the block abuts the previous one, so unrelated
 * VirtualAlloc regions satisfy it exactly. A brk-shaped API could not be
 * honored here at all without reserving an address range up front.
 *
 * Unlike dup() (deliberately absent from os/windows/open.c -- see
 * todo.org), nothing about sbrk is blocked by this port's fd-as-
 * truncated-HANDLE simplification: no descriptors are involved.
 *
 * NOT YET RUN ON REAL WINDOWS: written on a Linux host. The Win64 ABI
 * details in winio_amd64.s's _winalloc are the same shadow-space and
 * alignment pattern every other stub in that file already uses and which
 * were verified by a real native build+run, but this particular call has
 * not executed. tests/c/hello_libc's test_windows target builds mem.exe
 * and will exercise it.
 */

extern void *_winalloc(ulong n);

/* end of the most recent block, so sbrk(0) can answer "where are we
 * now". Only meaningful per-block here, since successive regions are
 * unrelated addresses -- but no in-tree caller uses sbrk(0) for anything
 * else, and answering is cheaper than failing confusingly. Mirrors
 * os/darwin/sbrk.c.
 */
static char *bloc = nil;

void*
sbrk(ulong n)
{
	void *p;

	if(n == 0)
		return bloc;

	p = _winalloc(n);

	/* VirtualAlloc reports failure with NULL, not with the (void*)-1
	 * that sbrk's own callers test for -- so this is a real conversion,
	 * not a pass-through. Getting it wrong would hand every caller a
	 * null pointer that passes their `== (char*)-1` check.
	 */
	if(p == nil)
		return (void*)-1;

	bloc = (char*)p + n;
	return p;
}
