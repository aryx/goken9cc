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
#define O_TRUNC		0x400
#define O_EXCL		0x800
#define O_CLOEXEC	0x1000000

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
