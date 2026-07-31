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

/* See os/linux/open.c's header comment for the syscall/ vs os/
 * split this belongs to -- same job (translate Plan9's open() mode
 * bits, include/os/file.h, into the raw _sysopen() this OS's open(2)
 * expects), different, genuinely OS-specific O_* numeric values.
 *
 * Darwin's (XNU/BSD) fcntl.h O_* flag bits, confirmed against
 * GO/pkg/syscall/zerrors_darwin_amd64.go's O_TRUNC/O_EXCL (arm64 shares
 * the same BSD-derived values -- see syscall/os/darwin/numbers_amd64.h's
 * own "arch-independent BSD table" comment for the same reasoning
 * applied to syscall numbers). O_CLOEXEC is NOT in that 2010-era Go
 * snapshot (it postdates Mac OS X 10.6, added in 10.7 Lion's fcntl.h)
 * -- 0x1000000 is from memory of the real header, not yet confirmed
 * against this repo's own references or real hardware; this whole file
 * is untested on real macOS (no host available this session) per the
 * user's own "generate what we think should be the code, I'll refine
 * on my machine" framing -- verify before trusting it.
 */
/* claude: O_CREAT added for create() below. 0x200 is the BSD value, and
 * is exactly the kind of divergence that keeps this translation per-OS:
 * Linux's O_CREAT is 0x40 (asm-generic) or 0x100 (mips), and 0x200 is
 * what Linux calls O_EXCL. Same 2010-era
 * GO/pkg/syscall/zerrors_darwin_amd64.go source as O_TRUNC/O_EXCL
 * below, and it sits in the expected place in that header's own
 * doubling sequence (O_NOFOLLOW 0x100, O_CREAT 0x200, O_TRUNC 0x400,
 * O_EXCL 0x800), so unlike O_CLOEXEC it isn't a from-memory value.
 */
#define O_CREAT		0x200
#define O_TRUNC		0x400
#define O_EXCL		0x800
#define O_CLOEXEC	0x1000000

extern long _sysopen(void *path, int flags, int mode);
extern int _sysmkdir(char *path, int mode);

/* claude: shared by open() and create() -- see os/linux/open.c's
 * identical comment for why this got split out.
 */
static int
openflags(int mode)
{
	int flags;

	switch (mode & 3) {
	case OWRITE:
		flags = 1; /* O_WRONLY */
		break;
	case ORDWR:
		flags = 2; /* O_RDWR */
		break;
	case OEXEC: /* no POSIX "exec-only" open mode; O_RDONLY is closest */
	case OREAD:
	default:
		flags = 0; /* O_RDONLY */
		break;
	}
	if (mode & OTRUNC)
		flags |= O_TRUNC;
	if (mode & OCEXEC)
		flags |= O_CLOEXEC;
	if (mode & OEXCL)
		flags |= O_EXCL;
	/* ORCLOSE (remove-on-close) has no POSIX open() equivalent -- would
	 * need a following unlink(), not wired up in this project yet.
	 */
	return flags;
}

fdt
open(char *path, int mode)
{
	return (fdt)_sysopen(path, openflags(mode), 0);
}

/* claude: see os/linux/open.c's create() for the full story (Plan9's
 * create() is open()-with-O_CREAT plus an unconditional truncate, with
 * DMDIR dispatching to mkdir(2) instead). Only the O_* constants
 * differ between the two files -- the mkdir path is identical, since
 * SYS_mkdir is a plain 2-arg mkdir on Darwin exactly as on the
 * legacy-numbered Linux archs. Untested on real macOS, like the rest
 * of this file.
 */
int
create(char *path, int mode, ulong perm)
{
	if (perm & DMDIR) {
		if ((mode & ~OCEXEC) != OREAD)
			return -1;
		if (_sysmkdir(path, (int)(perm & 0777)) < 0)
			return -1;
		return (int)_sysopen(path, openflags(mode), 0);
	}
	return (int)_sysopen(path, openflags(mode)|O_CREAT|O_TRUNC,
		(int)(perm & 0777));
}
