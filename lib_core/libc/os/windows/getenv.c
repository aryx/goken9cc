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

/* getenv() for windows, over GetEnvironmentVariableA (winio_amd64.s).
 *
 * Windows does keep a per-process environment BLOCK much like the SysV
 * one port/getenv.c walks, but it is not handed to the program on the
 * stack -- there is no argv-then-envp layout to walk past, since the
 * entry point receives nothing. kernel32 is the documented way to reach
 * it, so this is a call rather than a pointer walk.
 *
 * Like os/plan9/getenv.c and unlike port/getenv.c, the result is
 * malloc'd: GetEnvironmentVariableA copies into a caller buffer rather
 * than handing back a pointer into the block.
 *
 * The return value is overloaded exactly like GetCurrentDirectoryA's
 * (see os/windows/getwd.c):
 *   0            not found (or failure)
 *   < size       success; characters written, NOT counting the nul
 *   >= size      buffer too small; the REQUIRED size, counting the nul
 * so the first call sizes the buffer and the second fills it.
 */

extern ulong _wingetenv(char *name, char *buf, ulong size);

char*
getenv(char *name)
{
	char *ans;
	ulong n;

	if(name == nil || *name == '\0')
		return nil;

	/* size probe: a nil buffer with size 0 makes Windows report the
	 * required length (including the nul) rather than writing anything
	 */
	n = _wingetenv(name, nil, 0);
	if(n == 0)
		return nil;

	ans = malloc(n);
	if(ans == nil)
		return nil;
	if(_wingetenv(name, ans, n) >= n) {
		/* grew between the two calls, or vanished */
		free(ans);
		return nil;
	}
	return ans;
}
