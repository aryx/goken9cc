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

/* getwd() for linux -- the "os/$GOOS/" glue bridging POSIX getcwd(2)
 * onto Plan9's getwd() (include/os/dir.h).
 *
 * Two shape differences, hence the _sys prefix on the raw call:
 *   - POSIX returns the LENGTH written (including the NUL) on success,
 *     or a negative errno; Plan9 returns the BUFFER, or nil.
 *   - POSIX's getcwd fails with ERANGE if the path does not fit; Plan9's
 *     getwd simply returns nil. Both collapse to nil here, which loses
 *     the distinction between "too small" and "no cwd" -- acceptable
 *     because Plan9's own getwd(2) makes no such distinction either.
 *
 * Compare os/darwin/getwd.c and os/plan9/getwd.c, which reach the same
 * answer through an entirely different mechanism: neither has a getcwd
 * syscall, and both instead open "." and ask the kernel for that
 * descriptor's path (fcntl(F_GETPATH) and fd2path respectively).
 */

extern long _sysgetcwd(char *buf, ulong size);

char*
getwd(char *buf, int nbuf)
{
	if(nbuf <= 0)
		return nil;
	if(_sysgetcwd(buf, (ulong)nbuf) < 0)
		return nil;
	return buf;
}
