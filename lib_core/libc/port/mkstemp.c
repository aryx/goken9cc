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

/* mkstemp() (include/os/tmp.h) -- POSIX's atomic "create a unique temp
 * file from a template ending in a run of X's" contract, layered on
 * this libc's own create() (os/$GOOS/open.c, Plan9's O_CREAT-equivalent
 * primitive, already generated/linkable for every GOOS this tree
 * targets) with OEXCL so a name collision makes create() fail outright
 * instead of silently truncating someone else's file -- a real create()
 * call, not a stub.
 *
 * Not cryptographically unique: pid xored against an in-process
 * counter, retried on collision. Matches the same "good enough, not a
 * real CSPRNG" standard mk/goken.c's own maketmp() comment already
 * established for this project's other temp-name needs -- a single
 * process making a handful of temp files (yacc's own two call sites,
 * ttempname/tactname) has no real uniqueness requirement beyond "don't
 * collide with another concurrent run of the same tool".
 */

static int counter;

fdt
mkstemp(char *template)
{
	int len, tries;
	fdt fd;
	long pid, n;

	len = strlen(template);
	if(len < 6 || strncmp(template+len-6, "XXXXXX", 6) != 0)
		return -1;

	pid = getpid();
	for(tries = 0; tries < 1000; tries++){
		n = (pid ^ (counter++ * 2654435761UL)) % 1000000;
		if(n < 0)
			n = -n;
		snprint(template+len-6, 7, "%06ld", n);
		fd = create(template, OEXCL|ORDWR, 0600);
		if(fd >= 0)
			return fd;
	}
	return -1;
}
