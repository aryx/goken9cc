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

/* putenv() for windows, over SetEnvironmentVariableA (winio_amd64.s) --
 * the write-side counterpart of os/windows/getenv.c's GetEnvironmentVariableA
 * call. See that file's own comment for why this is a real Win32 call
 * rather than a pointer walk: the per-process environment block exists,
 * but is not handed to the program on the stack the way the SysV ABI
 * hands port/getenv.c's one, so kernel32 is the only way to reach it
 * from either direction.
 *
 * Unverified on a real Windows host -- unlike os/windows/open.c's own
 * stubs (verified natively per that file's own header comment), this
 * arrived with the rest of Tier 6's environ() round and no Windows
 * machine was available to run it. The Win32 ABI mechanics themselves
 * (two register arguments, shadow-space-only) are the exact same shape
 * as several already-verified stubs (_windelete, _winchdir), so the
 * risk is confined to SetEnvironmentVariableA's own documented
 * contract, not the calling convention.
 */

extern long _winsetenv(char *name, char *value);

int
putenv(char *name, char *value)
{
	if(name == nil || *name == '\0')
		return -1;
	return _winsetenv(name, value) ? 0 : -1;
}
