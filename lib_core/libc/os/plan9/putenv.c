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

/* putenv() for plan9. Ported as-is from principia's
 * lib_core/libc/9sys/putenv.c (the real, working Plan9 libc code).
 *
 * See os/plan9/getenv.c's own header comment for the general /env
 * story (a directory, one file per variable, no in-memory array).
 * create(path, OWRITE, perm) here does double duty as both "add" and
 * "replace": Plan9's create(2) truncates an already-existing file
 * (include/os/dir.h) exactly the way OTRUNC would, so there is no
 * separate branch needed for the two cases the POSIX-side
 * implementation (port/putenv.c) has to distinguish explicitly.
 *
 * val is written byte-for-byte, not nul-terminated on disk (/env
 * files hold exactly the bytes written, matching getenv()'s own
 * read-back convention of treating internal nuls as separators, not
 * padding). A caller passing a plain space-separated C string gets
 * plain spaces stored; nul-separating a multi-word value (rc's own
 * convention for a list-valued variable) is the caller's choice to
 * make before calling, same as principia's own reference.
 */

int
putenv(char *name, char *val)
{
	fdt f;
	char ename[100];
	long s;

	if(strchr(name, '/') != nil)
		return -1;
	snprint(ename, sizeof ename, "/env/%s", name);
	/* snprint truncated the name: refuse rather than write a prefix */
	if(strcmp(ename+5, name) != 0)
		return -1;
	f = create(ename, OWRITE, 0664);
	if(f < 0)
		return -1;
	s = strlen(val);
	if(write(f, val, s) != s){
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
