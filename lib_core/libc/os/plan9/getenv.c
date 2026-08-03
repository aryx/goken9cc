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

/* getenv() for plan9. Adapted from ~/principia/lib_core/libc/9sys/getenv.c.
 *
 * The environment is a DIRECTORY here, /env, one file per variable --
 * so like getpid() and time() this needs no syscall of its own, just
 * the open/seek/read/close already implemented. It also means the
 * environment is shared and mutable through the file system rather than
 * copied into each process's memory at exec time, which is why there is
 * no environ array to walk (contrast port/getenv.c, where that array IS
 * the environment).
 *
 * Two consequences the POSIX version does not have:
 *   - the result must be malloc'd, since the bytes only exist once
 *     read. POSIX getenv returns a pointer into memory that is already
 *     there. Callers here own the result; nothing in this tree frees it,
 *     which is what Plan9 programs do too.
 *   - a name containing '/' would escape into another directory, so it
 *     is rejected outright rather than silently reading the wrong file.
 *
 * The nul-to-space rewrite is Plan9's own convention: /env values are
 * nul-SEPARATED lists (that is how rc exports $path as several
 * elements), and a C caller expecting one string needs them joined.
 */

char*
getenv(char *name)
{
	int r;
	fdt f;
	long s;
	char *ans;
	char *p, *ep, ename[100];

	if(strchr(name, '/') != nil)
		return nil;
	snprint(ename, sizeof ename, "/env/%s", name);
	/* snprint truncated the name: refuse rather than read a prefix */
	if(strcmp(ename+5, name) != 0)
		return nil;
	f = open(ename, OREAD);
	if(f < 0)
		return nil;
	s = seek(f, 0, 2);
	ans = malloc(s+1);
	if(ans != nil) {
		seek(f, 0, 0);
		r = read(f, ans, s);
		if(r >= 0) {
			ep = ans + s - 1;
			for(p = ans; p < ep; p++)
				if(*p == '\0')
					*p = ' ';
			ans[s] = '\0';
		}
	}
	close(f);
	return ans;
}
