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

/* dirstat() by path, built portably on top of dirfstat() by fd
 * (os/$GOOS/stat.c) instead of every GOOS needing its own by-path
 * syscall/translation too. include/os/stat.h's own comment has the
 * full rationale; the short version is that every real caller in this
 * tree only ever needs read access to stat something, so open(OREAD)
 * plus dirfstat() is enough -- no GOOS here has a stat(2) whose result
 * can differ from open+fstat for a file the caller can already read.
 *
 * Known, accepted gap: on POSIX, plain stat(2) only needs directory
 * search permission, not read permission on the file itself, so
 * something statable-but-unreadable (0300 owned-by-someone-else, or a
 * network share with odd ACLs) would fail here where real stat(2)
 * would succeed. Not a case any caller in this tree hits.
 */
Dir*
dirstat(char *name)
{
	fdt fd;
	Dir *d;

	fd = open(name, OREAD);
	if (fd < 0)
		return nil;
	d = dirfstat(fd);
	close(fd);
	return d;
}
