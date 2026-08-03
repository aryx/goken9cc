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

/* getwd() for plan9. Adapted from ~/principia/lib_core/libc/9sys/getwd.c.
 *
 * Plan9 has no getcwd(2). Instead of the kernel maintaining a "current
 * path" string, you open the current directory and ask what path that
 * DESCRIPTOR refers to -- which is strictly better, since it cannot go
 * stale or race with a rename the way a cached string can.
 *
 * Darwin, which also lacks getcwd, independently arrives at the same
 * answer via fcntl(F_GETPATH) -- see os/darwin/getwd.c. Only linux has
 * a real getcwd syscall (os/linux/getwd.c).
 */

char*
getwd(char *buf, int nbuf)
{
	int n;
	fdt fd;

	fd = open(".", OREAD);
	if(fd < 0)
		return nil;
	n = fd2path(fd, buf, nbuf);
	close(fd);
	if(n < 0)
		return nil;
	return buf;
}
