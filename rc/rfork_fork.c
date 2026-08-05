/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* rfork_fork.c -- rfork() for linux/darwin/plan9 (fork()-capable
 * GOOSes). Split out of rc/goken.c into its own GOOS-selected file for
 * the same reason mk's own exec_fork.c/exec_windows.c already are:
 * windows has no fork() at all, so a single shared goken.c can't
 * define this the same way everywhere -- see rc/rfork_windows.c and
 * rc/mkfile's own EXECVARIANTOFILES.
 *
 * rc/processes.c's own Xasync() calls rfork(RFFDG|RFPROC|RFNOTEG)
 * directly (a shared file, not goken.c-specific) -- rather than
 * editing that shared call site (which the host boot-gcc build still
 * wants pointed at BOOT/lib9's own real p9rfork(), a pipe-dance
 * emulation over real POSIX fork()+pipe()+SIGCHLD), this defines
 * rfork() itself as a thin fork() wrapper, the same substitution
 * mk/exec_fork.c already made for RFPROC|RFFDG|RFENVG: fork() already
 * gives the child its own independent fd table (RFFDG) and, since
 * POSIX fork() always gives a child its own independently-modifiable
 * copy of the parent's signal disposition table, its own independent
 * note group too (RFNOTEG) -- both already true of fork() alone,
 * nothing left for the flags to add.
 */
#include	"rc.h"
#include	"fns.h"
#include	"io.h"
#include	"exec.h"
#include	"getflags.h"

int
rfork(int flags)
{
	USED(flags);
	return fork();
}
