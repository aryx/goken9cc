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

/* getwd() for darwin. There is no getcwd syscall on this system at all
 * (see numbers_darwin_amd64.h), so this opens "." and asks the kernel
 * what path that descriptor refers to -- structurally identical to
 * os/plan9/getwd.c's fd2path, and quite unlike os/linux/getwd.c.
 *
 * The bounce buffer is not optional. F_GETPATH is documented to require
 * a buffer of at least MAXPATHLEN (PATH_MAX, 1024 -- bsd/sys/param.h)
 * bytes and writes into it unconditionally; handing it the caller's
 * smaller buffer would be a straightforward overflow rather than a
 * truncation. So the path always lands in `tmp` first and is copied out
 * only if it fits, with getwd's usual nil-on-failure otherwise.
 */

#define F_GETPATH	50
#define MAXPATHLEN	1024

extern int _sysfcntl(int fd, int cmd, void *arg);

char*
getwd(char *buf, int nbuf)
{
	char tmp[MAXPATHLEN];
	fdt fd;
	int n;

	if(nbuf <= 0)
		return nil;
	fd = open(".", OREAD);
	if(fd < 0)
		return nil;
	n = _sysfcntl(fd, F_GETPATH, tmp);
	close(fd);
	if(n < 0)
		return nil;
	if(strlen(tmp) >= (uint)nbuf)
		return nil;
	strcpy(buf, tmp);
	return buf;
}
