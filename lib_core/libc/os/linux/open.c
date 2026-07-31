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

/* This file is the "os/$GOOS/" glue layer, deliberately kept apart
 * from lib_core/libc/syscall/: syscall/ (see _sysopen() there) is just
 * a faithful raw-syscall adapter, one per (OS, arch); this file is
 * where a *different* API shape gets bridged on top of it -- Plan9's
 * fdt open(char*, int) with OREAD/OWRITE/ORDWR/OEXEC/OTRUNC/OCEXEC/
 * ORCLOSE/OEXCL bits (include/os/file.h) instead of POSIX open(2)'s
 * O_RDONLY/O_WRONLY/O_RDWR/O_CREAT/O_TRUNC/... flags. Genuinely
 * OS-specific (the O_* numeric values below differ from Darwin's --
 * see os/darwin/open.c), so it belongs here, not in port/.
 *
 * Linux's fcntl.h O_* flag bits, needed for that translation. These are
 * the "generic" Linux ABI values (asm-generic/fcntl.h), shared by every
 * arch this project targets except mips, which inherited a different,
 * older bit layout for the flags used here -- confirmed against
 * GO/pkg/syscall/zerrors_linux_{386,amd64,arm}.go's O_TRUNC/O_CLOEXEC
 * (identical on all three, and arm64/riscv/riscv64 are new-enough archs
 * to use the same asm-generic header those three do). The mips-specific
 * O_EXCL value is from memory of arch/mips/include/uapi/asm/fcntl.h,
 * NOT yet empirically verified on real mips hardware -- Plan9's OEXCL
 * isn't exercised by tests/c/hello_libc's io.c, so this path hasn't
 * actually been run. Verify before trusting it.
 */
#define O_TRUNC		0x200
#define O_CLOEXEC	0x80000
#ifdef mips
#define O_CREAT		0x100
#define O_EXCL		0x400
#else
#define O_CREAT		0x40
#define O_EXCL		0x80
#endif

extern long _sysopen(void *path, int flags, int mode);

/* claude: the Plan9-mode-bits -> O_* translation, split out of open()
 * when create() arrived rather than copied into it: both Plan9 calls
 * take the same `mode` argument with the same meaning (include/os/file.h),
 * and this switch plus the three or'ed bits above it are exactly the
 * part that's OS-specific -- so having one copy per OS, not per
 * function per OS, is the whole point of this file existing.
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

/* claude: Plan9's create() (include/os/dir.h) is open()-with-O_CREAT,
 * not a syscall of its own on any Unix -- which is why no numbers_*.h
 * in this tree grew a SYS_create. This tree's Unix targets all do have
 * a real creat(2) (386/arm/mips: 8, amd64: 85), but it's useless here:
 * creat() is hardwired to write-only, while Plan9's create() takes a
 * full mode argument and is routinely called with OREAD or ORDWR.
 *
 * Semantics being matched (Plan9's create(2)): the file is created if
 * absent, TRUNCATED IF PRESENT -- unconditionally, not only when the
 * caller passes OTRUNC -- and left open with the access mode given by
 * `mode`. That unconditional O_TRUNC is the one part that doesn't fall
 * out of openflags() above, so it's or'ed in here.
 *
 * `perm` is Plan9's permission word: the low 9 bits are rwxrwxrwx,
 * laid out exactly like POSIX's, so they pass straight through. The
 * high DM* bits (include/os/dir.h) have no open(2) equivalent and are
 * masked off -- except DMDIR, which in Plan9 means "make a directory
 * instead of a file". That needs mkdir(2)/mkdirat(2), a syscall number
 * this tree doesn't carry yet, so it's rejected outright rather than
 * silently creating a plain file of that name. BOOT/lib9/open.c's
 * p9create() (the gcc-built reference implementation) is the model
 * here, including its own `if(perm&DMDIR)` branch -- that's where to
 * look when wiring mkdir up.
 */
int
create(char *path, int mode, ulong perm)
{
	if (perm & DMDIR)
		return -1;
	return (int)_sysopen(path, openflags(mode)|O_CREAT|O_TRUNC,
		(int)(perm & 0777));
}
