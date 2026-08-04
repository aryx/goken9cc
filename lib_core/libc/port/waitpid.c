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

/* waitpid() (include/os/proc.h) -- unlike principia's own 9sys/
 * waitpid.c (which reimplements await()'s text-parsing a second time),
 * this is built directly on the portable wait() every GOOS here already
 * provides (port/wait.c for linux/darwin, os/plan9/wait.c for plan9),
 * so one file works unchanged everywhere wait() does -- no GOOS split
 * needed. Found blocking mk/Posix.c's Exit() ("while(waitpid() >= 0)
 * ;") while self-hosting mk with goken's own compiler+libc.
 */

int
waitpid(void)
{
	Waitmsg *w;
	int pid;

	w = wait();
	if(w == nil)
		return -1;
	pid = w->pid;
	free(w);
	return pid;
}
