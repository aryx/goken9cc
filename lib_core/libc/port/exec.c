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

/* exec() (include/os/proc.h) -- Plan9's exec(char*, char*[]) has no
 * envp argument (there is no environ concept in the Plan9 API at all,
 * see port/getenv.c's own header comment), so this bridges it onto the
 * raw 3-arg POSIX execve() the running process already has an
 * environment block for -- port/mainargs.c's _mainargv, stashed by
 * arch/$cputype/rt0.s at startup, the same source port/getenv.c reads.
 *
 * _environ() below is a deliberate near-duplicate of port/getenv.c's
 * own file-static environ(), not a shared call: that one is static to
 * its own translation unit (matching Plan9's usual one-helper-per-file
 * style elsewhere in this tree), and the walk is four lines, not worth
 * a new cross-file API surface for.
 *
 * execve() only ever returns on failure, so there is exactly one
 * `return` here (unlike port/dup.c's two-branch shape) -- but the raw
 * _sysexecve() still needs the same -errno-to-Plan9's-exact--1
 * normalization port/fork.c's own comment explains, since a future
 * caller may come to depend on the exact contract even though
 * compilers/pcc/pcc.c's own doexec() currently just treats any return
 * from exec() at all as failure, not `== -1` specifically.
 */

extern int _sysexecve(void *path, void *argv, void *envp);
extern char **_mainargv;

static char**
_environ(void)
{
	char **p;

	p = _mainargv;
	if(p == nil)
		return nil;
	while(*p != nil)
		p++;
	return p + 1;
}

int
exec(char *prog, char *argv[])
{
	_sysexecve(prog, argv, _environ());
	return -1;
}
