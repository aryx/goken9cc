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

/* dirwstat() by path, built portably on top of dirfwstat() by fd --
 * the write-side mirror of port/dirstat.c, same rationale.
 *
 * Known, accepted gap, same scope as BOOT/lib9/dirwstat.c (the
 * gcc-built reference this tree is meant to stop depending on, whose
 * own comment admits as much -- "BUG handle more"): renaming via
 * Dir.name is not implemented, only mode/mtime/length (see
 * os/$GOOS/stat.c's dirfwstat()). And opening ORDWR to reach a
 * per-fd chmod/truncate/utime means a currently-unwritable file (say
 * mode 0444) can't have its mode CHANGED via this path -- chmod(1)-
 * style "make a read-only file writable" needs a real path-based
 * chmod(2), which this does not call. No caller in this tree hits
 * either gap yet.
 */
int
dirwstat(char *name, Dir *d)
{
	fdt fd;
	int r;

	fd = open(name, ORDWR);
	if (fd < 0)
		return -1;
	r = dirfwstat(fd, d);
	close(fd);
	return r;
}
