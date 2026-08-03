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

/* sbrk() for darwin, which unlike every other GOOS here cannot use
 * port/sbrk.c: there is no usable brk on modern macOS. SYS_break (17)
 * survives in the BSD table but fails, so there is no break to move and
 * nothing for port/sbrk.c's bump-over-brk() to bump. lib_core/libc/mkfile
 * selects between the two rather than #ifdef'ing inside either.
 *
 * This is also the reason sbrk, not brk, is the primitive the rest of
 * the toolchain codes against (see port/sbrk.c's comment). sbrk's
 * contract is "here are n contiguous bytes" -- it never promised the
 * block abuts the previous one, which is exactly what lets it be backed
 * by unrelated mmap regions here. brk's contract (one monotonically
 * moving boundary in a single segment) could not be honored this way
 * without reserving an address range up front. Verified that all three
 * in-tree callers (utilities/text/grep/sub.c, utilities/byte/dd.c,
 * linkers/8lk/compat.c) treat each hunk independently.
 *
 * MAP_ANON is 0x1000 here, NOT Linux's 0x20 -- a genuinely different
 * value, and the kind of constant that fails silently (an unrecognized
 * flag bit means the mapping is file-backed on fd -1). From
 * GO/pkg/runtime/darwin/amd64/defs.h in this tree, the same snapshot
 * syscall/os/darwin/'s numbers come from.
 *
 * NOT YET RUN ON REAL macOS: this host has none. The layering and the
 * constants are checked against in-tree sources, but no XNU has executed
 * it -- same caveat syscall/os/darwin/svc_arm64.s already carries for
 * its error path. tests/c/hello_libc's test_macos_arm64/test_macos_amd64
 * targets build mem.exe and will exercise it on a real Mac.
 */

#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define MAP_PRIVATE	0x2
#define MAP_ANON	0x1000

extern uintptr _sysmmap(void *addr, ulong len, int prot, int flags, int fd,
	vlong offset);

/* end of the most recent block handed out, so sbrk(0) can answer "where
 * are we now" the way it does on a real break. It is only meaningful
 * per-block here -- successive regions are unrelated addresses -- but no
 * in-tree caller uses sbrk(0) for anything else, and answering it is
 * cheaper than having it fail confusingly.
 */
static char *bloc = nil;

void*
sbrk(ulong n)
{
	uintptr p;

	if(n == 0)
		return bloc;

	p = _sysmmap(nil, n, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
		-1, 0);

	/* svc_$cputype.s already normalizes XNU's carry-flag error
	 * convention into a negated errno, so a failure arrives as a small
	 * negative value rather than as MAP_FAILED. User addresses on macOS
	 * are well below the sign bit, so testing the sign is unambiguous.
	 */
	if((vlong)p < 0)
		return (void*)-1;

	bloc = (char*)p + n;
	return (void*)p;
}
