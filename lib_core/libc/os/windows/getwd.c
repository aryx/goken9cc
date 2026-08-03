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

/* getwd() for windows, over GetCurrentDirectoryA (see winio_amd64.s).
 *
 * Of the four GOOSes this is the only one whose native call is already
 * "write the cwd into this buffer" -- linux's getcwd is the same idea
 * but reports length differently, while darwin and plan9 have no such
 * call at all and both go via a descriptor (fcntl(F_GETPATH), fd2path).
 *
 * GetCurrentDirectoryA's return value is doubly overloaded, which is the
 * one thing to get right here:
 *   0            failure
 *   < nBufferLength   success; chars written, NOT counting the NUL
 *   >= nBufferLength  buffer too small; the REQUIRED size, counting the
 *                     NUL, and nothing was written
 * So a return of exactly nbuf means "too small", not "exactly filled" --
 * treating >= as success would hand back an unterminated buffer.
 */

extern ulong _wingetcwd(ulong nbuf, char *buf);

char*
getwd(char *buf, int nbuf)
{
	ulong n;

	if(nbuf <= 0)
		return nil;
	n = _wingetcwd((ulong)nbuf, buf);
	if(n == 0 || n >= (ulong)nbuf)
		return nil;
	return buf;
}
