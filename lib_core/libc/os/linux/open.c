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
#define O_EXCL		0x400
#else
#define O_EXCL		0x80
#endif

extern long _sysopen(void *path, int flags, int mode);

fdt
open(char *path, int mode)
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

	return (fdt)_sysopen(path, flags, 0);
}
