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

/* fork() (include/os/proc.h) -- Plan9 and POSIX agree on this call's
 * shape exactly (no arguments, 0 back in the child, the child's pid
 * back in the parent, -1 on failure), so unlike dup()/remove() there is
 * no shape translation to do here. The one thing this file still has to
 * do is normalize the raw syscall layer's failure convention: every
 * arch's _sysfork() (syscall/os/linux/, either a direct generated
 * SYS_fork wrapper or, on arm64/riscv64, a clone(SIGCHLD,...) shim --
 * see syscall_linux_arm64.h) returns -errno on failure, not exactly -1.
 *
 * That distinction matters for a real caller here, not just tidiness:
 * compilers/pcc/pcc.c's doexec() does `switch(fork()){ case -1: ...
 * case 0: ... }`, an exact-match switch. A raw -errno (e.g. -12 for
 * ENOMEM) would miss the `case -1` arm entirely and fall through to the
 * default (parent) case with a bogus negative "pid" -- silently taking
 * the wrong branch instead of reporting the failure. See todo.org's
 * "most Plan9-shaped syscall wrappers return the raw negative errno on
 * failure, not -1" gap for the same class of bug already found
 * elsewhere in this tree; this file deliberately does not repeat it.
 */

extern int _sysfork(void);

int
fork(void)
{
	int pid;

	pid = _sysfork();
	if(pid < 0)
		return -1;
	return pid;
}
