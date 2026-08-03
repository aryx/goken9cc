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

/* exits()/_exits(): Plan9's process-termination pair, string-shaped
 * instead of POSIX's int status (include/os/proc.h: "exits() is the
 * libc exit that performs some cleanup (and handle atexit) while
 * _exits() is the syscall that is more abrupt").
 *
 * Both collapse to the same thing here, unlike BOOT/lib9/{exits,_exits}.c
 * (which call the host's real exit()/_exit(), genuinely different --
 * one runs atexit handlers and flushes stdio, the other doesn't). This
 * libc's own exit(int) is already a bare syscall/ExitProcess trampoline
 * (syscall/os/$GOOS/zsyscall_*.c, os/windows/winio_amd64.s) with no
 * cleanup phase to skip, so there is nothing for exits() to do that
 * _exits() doesn't. If an atexit-driven cleanup layer is ever added to
 * exit(), it belongs there (exit(int)'s own body), not duplicated here
 * -- exits()/_exits() only need to keep agreeing on the msg->status
 * mapping.
 *
 * Not built for plan9: its exits(char*) is already the raw EXITS
 * syscall itself (syscall/os/plan9/svc_$cputype.s), which needs no
 * status-collapsing at all since the kernel takes the string directly
 * -- see lib_core/libc/mkfile's EXITSOFILES for the selection.
 */

void
exits(char *s)
{
	if(s == nil || s[0] == '\0')
		exit(0);
	exit(1);
}

void
_exits(char *s)
{
	if(s == nil || s[0] == '\0')
		exit(0);
	exit(1);
}
