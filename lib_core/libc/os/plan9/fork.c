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

/* fork() (include/os/proc.h) -- Plan9 has no fork(2) syscall of its
 * own, only rfork(2) (include/os/proc.h's RF* flags, and svc_$cputype.s's
 * raw rfork stub, already exactly this shape). fork() is the classic
 * POSIX-compat special case: RFPROC (new process) | RFFDG (private fd
 * table, copied from the parent -- POSIX fork's own fd-inheritance
 * semantics) | RFREND ("rendezvous", needed so the parent's future
 * wait() can actually observe this child -- see os/plan9/wait.c).
 *
 * Ported as-is from principia's lib_core/libc/9sys/fork.c (the
 * authoritative real-Plan9 combination, not guessed) -- see
 * include/os/proc.h's own comment on where the RF* values came from.
 */

int
fork(void)
{
	return rfork(RFPROC|RFFDG|RFREND);
}
