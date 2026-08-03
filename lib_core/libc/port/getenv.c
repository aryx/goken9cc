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

/* getenv() for the POSIX-shaped GOOSes (linux and darwin), which reach
 * the environment identically: it is NOT a syscall at all, it is memory
 * the kernel already laid on the initial stack. Immediately past argv's
 * terminating nil sits the environment, a nil-terminated array of
 * "NAME=value" strings. So this whole file needs no syscall layer and
 * lives in port/ rather than os/$GOOS/ -- the one call in this group
 * where linux and darwin genuinely agree.
 *
 * They agree because it is the SysV ABI's process-startup contract, not
 * a Linux invention. plan9 and windows have neither that contract nor
 * that memory layout, and get their own os/$GOOS/getenv.c.
 *
 * _mainargv is stashed by arch/$cputype/rt0.s -- see port/mainargs.c
 * for why argv rather than an envp computed there.
 */

extern char **_mainargv;

/* walk past argv's own nil to reach the environment. Recomputed per
 * call rather than cached: getenv is not hot (23 call sites across the
 * toolchain, all of them one-shot configuration lookups like $objtype
 * and $GOROOT), and a cache would need invalidating if putenv ever
 * grows a real implementation.
 */
static char**
environ(void)
{
	char **p;

	p = _mainargv;
	if(p == nil)
		return nil;
	while(*p != nil)
		p++;
	return p + 1;
}

/* Returns a pointer INTO the environment block, not a copy -- same as
 * POSIX getenv(3), and deliberately unlike os/plan9/getenv.c, which has
 * to malloc because /env/NAME's contents only exist once read.
 * Callers must not free or modify it. Plan9's own getenv(2) does
 * malloc, so a caller written against the Plan9 API and freeing the
 * result would be wrong here -- but nothing in this tree does, and
 * matching POSIX costs no allocation on the path every toolchain
 * startup takes.
 */
char*
getenv(char *name)
{
	char **e, *p, *q;

	if(name == nil || *name == '\0')
		return nil;
	e = environ();
	if(e == nil)
		return nil;
	for(; *e != nil; e++) {
		/* compare inline rather than with strncmp: this libc does not
		 * implement it (include/base/str.h has it commented out), and
		 * the prefix test alone would be wrong anyway -- the match must
		 * end exactly at '=', or "PATH" would also match "PATHEXT=...".
		 */
		p = *e;
		q = name;
		while(*q != '\0' && *p == *q) {
			p++;
			q++;
		}
		if(*q == '\0' && *p == '=')
			return p + 1;
	}
	return nil;
}
