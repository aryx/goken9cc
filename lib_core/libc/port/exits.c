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
 * UPDATE: that split is now real -- exits() runs port/atexit.c's
 * registered callbacks (atexitrun()) before exit(), _exits() skips
 * straight to it, matching principia's own lib_core/libc/port/
 * atexit.c (which defines exits() itself, calling its onex[] table
 * directly; kept as a separate seam here instead since this project's
 * exits() already existed as its own per-GOOS-collapsed file, see
 * atexit.c's own header comment for why the two weren't merged).
 * Otherwise both still collapse to the same exit(int) -- this libc's
 * own exit(int) is a bare syscall/ExitProcess trampoline (syscall/os/
 * $GOOS/zsyscall_*.c, os/windows/winio_amd64.s) with nothing else to
 * skip, unlike BOOT/lib9/{exits,_exits}.c (host exit()/_exit(), which
 * also differ in stdio flushing).
 *
 * Not built for plan9: its exits(char*) is already the raw EXITS
 * syscall itself (syscall/os/plan9/svc_$cputype.s), which needs no
 * status-collapsing at all since the kernel takes the string directly
 * -- see lib_core/libc/mkfile's EXITSOFILES for the selection. Real
 * Plan9 processes still run atexit handlers before EXITS, but nothing
 * in this tree has hit the need for that on the plan9 GOOS specifically
 * yet -- tracked in todo.org alongside plan9's other emulator gaps if
 * it ever does.
 */

extern void atexitrun(void);

void
exits(char *s)
{
	atexitrun();
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
