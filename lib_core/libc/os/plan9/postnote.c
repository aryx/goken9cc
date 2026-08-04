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

/* postnote() (include/os/plan9/note.h) -- Tier 6 notification
 * (docs/claude_notes/plan_syscalls.txt). NOT a raw syscall: on real
 * Plan9 there is no dedicated kernel call for "send a note", only a
 * plain write() to a control file under /proc -- /proc/PID/note for a
 * single process (PNPROC), /proc/PID/notepg for its whole process
 * group (PNGROUP). Ported as-is from principia's lib_core/libc/9sys/
 * postnote.c (the real, working convention).
 */

int
postnote(int group, int pid, char *note)
{
	char file[128];
	fdt f;
	int r;

	switch(group){
	case PNPROC:
		sprint(file, "/proc/%d/note", pid);
		break;
	case PNGROUP:
		sprint(file, "/proc/%d/notepg", pid);
		break;
	default:
		return -1;
	}

	f = open(file, OWRITE);
	if(f < 0)
		return -1;

	r = strlen(note);
	if(write(f, note, r) != r){
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
