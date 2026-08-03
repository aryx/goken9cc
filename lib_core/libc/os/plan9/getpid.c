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

/* getpid() for plan9. Adapted from ~/principia/lib_core/libc/9sys/getpid.c.
 *
 * There is no getpid syscall here -- the process id is a FILE, like
 * almost everything else on this system, so this needs nothing beyond
 * the open/read/close already implemented. That is exactly what
 * docs/claude_notes/plan_syscalls.txt predicted for this GOOS: getwd,
 * getenv, time, getpid and sysname are all file operations on Plan9, so
 * its column of the syscall tables collapses to far fewer rows than the
 * Unix ones.
 *
 * "#c/pid" rather than "/dev/pid": the "#c" prefix names the cons
 * device directly rather than going through the namespace, so this
 * still works in a process that has rebound or discarded /dev.
 */

int
getpid(void)
{
	char b[20];
	fdt f;

	memset(b, 0, sizeof(b));
	f = open("#c/pid", OREAD);
	if(f >= 0) {
		read(f, b, sizeof(b)-1);
		close(f);
	}
	return atol(b);
}
