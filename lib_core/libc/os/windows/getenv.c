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
extern char *_wingetenvstrings(void);
extern int _winfreeenvstrings(char *p);

/* environ() for windows -- rc/goken.c self-hosting's own Vinit()/
 * Updenv() need this (docs/claude_notes/notes_libc_selfhost.txt's rc
 * self-hosting entries), same as environ() already does on linux/
 * darwin (port/getenv.c), but built on a genuinely different Win32
 * primitive: GetEnvironmentVariableA above only ever answers a single
 * name, there is no "list them all" mode for it. GetEnvironmentStrings
 * is the real Win32 call for that -- it hands back one block of
 * "NAME=value\0NAME2=value2\0...\0\0" strings (double-nul terminated),
 * NOT an argv-style array of pointers, so this walks it once to count
 * entries and total bytes, copies the whole block into a fresh buffer
 * this process owns (the original is Win32-owned memory, released via
 * _winfreeenvstrings() below -- pointers into it would dangle the
 * moment that call returns), and builds a nil-terminated char** over
 * the copy, matching every other GOOS's environ() contract
 * (include/os/env.h).
 *
 * Unlike port/getenv.c's own environ(), there is no _environp caching
 * here and no need for any: os/windows/putenv.c's own SetEnvironmentVariableA
 * call already mutates the real OS-owned environment block directly,
 * so simply asking Win32 fresh every call is already correct and
 * needs no local shadow copy to keep in sync.
 */
char**
environ(void)
{
	char *block, *p, *copy, *cp;
	char **env;
	long nvar, nbyte, len;

	block = _wingetenvstrings();
	if(block == nil)
		return nil;

	nvar = 0;
	nbyte = 0;
	for(p = block; *p != '\0'; ){
		len = strlen(p) + 1;
		nvar++;
		nbyte += len;
		p += len;
	}

	copy = malloc(nbyte);
	if(copy == nil){
		_winfreeenvstrings(block);
		return nil;
	}
	memmove(copy, block, nbyte);
	_winfreeenvstrings(block);

	env = malloc((nvar+1) * sizeof(char*));
	if(env == nil){
		free(copy);
		return nil;
	}
	cp = copy;
	nvar = 0;
	while(*cp != '\0'){
		env[nvar++] = cp;
		cp += strlen(cp) + 1;
	}
	env[nvar] = nil;
	return env;
}

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
