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

/* dirwstat() (include/os/stat.h) for linux -- overrides port/
 * dirwstat.c's portable open+dirfwstat+close for this GOOS specifically,
 * to add real rename support. Arch-independent (one file for all 6
 * Linux arches, like os/plan9/stat.c is for arm+mips): the raw
 * _sysrenameat2() name is the same everywhere regardless of which
 * per-arch SYS_renameat2 number backs it (syscall/os/linux/
 * syscall_linux_$cputype.decl), the same pattern os/linux/stat_
 * $cputype.c's own dirread()/dirreadall() already established for
 * openat()/_sysgetdents64().
 *
 * Found genuinely needed, not preemptively: utilities/files/mv.c calls
 * dirwstat(fromname, &null) with only null.name set (nulldir()'s other
 * fields left at their all-ones "unchanged" sentinel) to rename within
 * the same directory -- port/dirwstat.c's own header comment already
 * flagged this as a known, unimplemented gap ("no caller in this tree
 * hits it yet"); mv.c now does, and without this fix it silently
 * "succeeds" (dirfwstat() returns 0 having touched neither mode nor
 * length, both left unchanged, and simply ignores Dir.name -- no
 * error, just a no-op rename), confirmed by an actual qemu run before
 * writing this fix, not assumed from reading dirfwstat().
 *
 * dirfwstat() itself can't do this: it only has an already-open fd,
 * with no path of its own to rename FROM or of the containing
 * directory to construct a new path AT (fd-based fchmod/ftruncate
 * have no fd-based rename equivalent on Linux). dirwstat() has the
 * original path string though, so the rename happens here, before
 * falling through to the normal open+dirfwstat+close for any other
 * fields (mode/length) the caller also set.
 */

#define AT_FDCWD (-100)

extern long _sysrenameat2(int olddirfd, void *oldpath, int newdirfd, void *newpath, uint flags);

int
dirwstat(char *name, Dir *d)
{
	fdt fd;
	int r;
	char newpath[1024], *dir, *slash;

	if (d->name != nil && d->name[0] != '\0') {
		strncpy(newpath, name, sizeof newpath);
		newpath[sizeof(newpath)-1] = '\0';
		slash = strrchr(newpath, '/');
		if (slash != nil)
			dir = slash+1;
		else
			dir = newpath;
		if (dir - newpath + strlen(d->name) + 1 > sizeof newpath)
			return -1;
		strcpy(dir, d->name);

		if (_sysrenameat2(AT_FDCWD, name, AT_FDCWD, newpath, 0) < 0)
			return -1;
		name = newpath;
	}

	fd = open(name, ORDWR);
	if (fd < 0)
		return -1;
	r = dirfwstat(fd, d);
	close(fd);
	return r;
}
