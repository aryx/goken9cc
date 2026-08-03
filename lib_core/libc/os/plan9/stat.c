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
#include "stat9p.h"

/* dirfstat()/dirfwstat() (include/os/stat.h) for plan9 -- the one GOOS
 * where the machine-independent stat wire format is real and native,
 * so unlike linux/darwin/windows, this file genuinely needs the raw
 * FSTAT/FWSTAT syscalls (svc_$cputype.s) plus convM2D/convD2M
 * (this directory) to unpack/pack them. One file for both arm and
 * mips (unlike os/linux/'s per-$cputype split): the raw syscalls
 * differ only in their asm stub (already arch-split in svc_$cputype.s),
 * not in this C-level shape -- see lib_core/libc/mkfile's STATOFILES
 * comment.
 *
 * DIRSIZE is a guess at "big enough for the fixed part plus a few
 * reasonable-length strings", matching principia's lib_core/libc/9sys/
 * dirstat.c/dirfstat.c exactly: try once at that size, and if the
 * kernel reports (via the buffer's own leading count) that more space
 * was needed, retry once at the reported size. Two tries, not a loop,
 * because a well-behaved server's second answer is authoritative.
 */
extern int fstat(int fd, uchar *buf, int nbuf);
extern int fwstat(int fd, uchar *buf, int nbuf);

enum {
	DIRSIZE = STATFIXLEN + 16*4
};

Dir*
dirfstat(fdt fd)
{
	Dir *d;
	uchar *buf;
	int n, nd, i;

	nd = DIRSIZE;
	for (i = 0; i < 2; i++) {
		d = malloc(sizeof(Dir) + BIT16SZ + nd);
		if (d == nil)
			return nil;
		buf = (uchar*)&d[1];
		n = fstat(fd, buf, BIT16SZ+nd);
		if (n < BIT16SZ) {
			free(d);
			return nil;
		}
		nd = GBIT16(buf);
		if (nd <= n) {
			convM2D(buf, n, d, (char*)&d[1]);
			return d;
		}
		free(d);
	}
	return nil;
}

/* claude: mode/mtime/length via convD2M's "~field == 0 means
 * unchanged" sentinel convention (nulldir() in port/nulldir.c) --
 * convD2M packs whatever the caller set, including any fields left at
 * their nulldir() default, so unlike the other GOOSes' dirfwstat()
 * there is no per-field ~x!=0 check needed here at all: the wire
 * format's own semantics already are "all-ones bits = leave alone" on
 * the SERVER side (principia's kernel wstat handler), not something
 * this libc has to implement itself. Renaming via Dir.name DOES work
 * here, unlike port/dirwstat.c's documented gap for the other GOOSes:
 * the wire format carries a name string natively, and the Plan9 kernel
 * genuinely treats a changed name as a rename.
 */
int
dirfwstat(fdt fd, Dir *d)
{
	uchar *buf;
	int r;

	r = sizeD2M(d);
	buf = malloc(r);
	if (buf == nil)
		return -1;
	convD2M(d, buf, r);
	r = fwstat(fd, buf, r);
	free(buf);
	return r;
}
