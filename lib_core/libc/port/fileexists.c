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

/* fileexists() (include/os/dir.h) -- declared, never implemented
 * (BOOT/lib9/fileexists.c has a real reference version, built on a
 * raw host <sys/stat.h> stat(2) -- not portable to this libc as-is).
 * Built on the already-portable dirstat() instead (os/$GOOS-agnostic,
 * see its own header comment) -- exists iff dirstat() can produce a
 * Dir for it. Found self-hosting linkers/7l via objtype=arm64:
 * findlib() calls it directly to probe candidate library paths.
 */
bool
fileexists(char *file)
{
	Dir *d;

	d = dirstat(file);
	if(d == nil)
		return false;
	free(d);
	return true;
}
