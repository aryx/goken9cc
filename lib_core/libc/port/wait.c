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

/* wait() (include/os/proc.h) -- Plan9's Waitmsg-returning wait(), built
 * on the raw POSIX wait4() every Linux arch's syscall/ layer now
 * exposes as _syswait4() (int pid, int *status, int options, void
 * *rusage). rusage is passed as nil throughout: Waitmsg's own time[]
 * field is left zeroed rather than filled from it, since no caller in
 * this tree reads it yet (compilers/pcc/pcc.c's doexec()/dopipe() only
 * ever look at w->pid and w->msg) -- a real gap if a caller ever does,
 * not silently wrong, just unfilled.
 *
 * Loosely modeled on BOOT/lib9/await.c's own _wait(), which this
 * project is meant to stop depending on, but considerably smaller:
 * that version also builds a full signal-name table (_p9sigstr) for
 * BOOT's own gcc-hosted <signal.h> SIGxxx constants, which this tree
 * has no equivalent header for at all (Plan9 has no POSIX signals as a
 * first-class libc concept -- see os/plan9/'s own /proc/n/note-based
 * story). WTERMSIG's raw signal number is used directly here instead.
 *
 * The WIFEXITED/WEXITSTATUS/WIFSIGNALED/WTERMSIG bit-tests below are
 * the standard Linux/glibc <sys/wait.h> definitions, inlined rather
 * than pulled from a header this tree doesn't have -- they operate on
 * the raw kernel `int status` wait4() already handed back, not on
 * anything syscall-numbered or arch-specific, so one copy suffices for
 * every Linux arch this file is built on.
 */

#define WIFEXITED(status)	(((status) & 0x7f) == 0)
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#define WIFSIGNALED(status)	(((status) & 0x7f) != 0 && ((status) & 0x7f) != 0x7f)
#define WTERMSIG(status)	((status) & 0x7f)

extern int _syswait4(int pid, void *status, int options, void *rusage);

static Waitmsg*
_wait1(int pid, int opt)
{
	Waitmsg *w;
	int status, r;

	w = mallocz(sizeof(Waitmsg) + 64, 1);
	if(w == nil)
		return nil;
	w->msg = (char*)&w[1];

	status = 0;
	r = _syswait4(pid, &status, opt, nil);
	if(r < 0){
		free(w);
		return nil;
	}
	w->pid = r;
	if(WIFEXITED(status)){
		if(WEXITSTATUS(status))
			sprint(w->msg, "%d", WEXITSTATUS(status));
	}else if(WIFSIGNALED(status))
		sprint(w->msg, "signal %d", WTERMSIG(status));
	return w;
}

Waitmsg*
wait(void)
{
	return _wait1(-1, 0);
}
