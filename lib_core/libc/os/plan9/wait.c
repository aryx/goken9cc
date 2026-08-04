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

/* wait() (include/os/proc.h) -- Plan9 has no wait4()-shaped syscall at
 * all (contrast Linux/Darwin, which get a Waitmsg by decoding a plain
 * int status word -- see lib_core/libc/port/wait.c). Instead await(2)
 * (svc_$cputype.s's raw stub, already exactly include/os/proc.h's
 * shape) blocks for a dead child and hands back a TEXT message:
 * "pid utime stime rtime msg", five space-separated fields, the last
 * one (msg) being the exact string that child's own exits(char*) call
 * passed. tokenize() (port/tokenize.c) splits it.
 *
 * Ported as-is from principia's lib_core/libc/9sys/wait.c -- the real
 * text format a genuine Plan9 kernel produces, not reverse-engineered.
 * Notably richer than the Linux/Darwin Waitmsg: time[] is really
 * filled in here (from the kernel's own account), and msg carries the
 * child's ACTUAL exits() string verbatim -- neither is possible on
 * Unix, where a child's string is irrecoverably collapsed to a plain
 * exit code before wait4() ever sees it (see port/wait.c's own
 * comment on that loss).
 */

Waitmsg*
wait(void)
{
	int n, l;
	char buf[512];
	char *fld[5];
	Waitmsg *w;

	n = await(buf, sizeof buf-1);
	if(n < 0)
		return nil;
	buf[n] = '\0';
	if(tokenize(buf, fld, nelem(fld)) != nelem(fld)){
		werrstr("couldn't parse wait message");
		return nil;
	}
	l = strlen(fld[4])+1;
	w = malloc(sizeof(Waitmsg)+l);
	if(w == nil)
		return nil;
	w->pid = atoi(fld[0]);
	w->time[0] = atoi(fld[1]);
	w->time[1] = atoi(fld[2]);
	w->time[2] = atoi(fld[3]);
	w->msg = (char*)&w[1];
	memmove(w->msg, fld[4], l);
	return w;
}
