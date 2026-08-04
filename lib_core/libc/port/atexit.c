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

/* atexit() (include/os/proc.h), ported near-verbatim from principia's
 * lib_core/libc/port/atexit.c: a fixed-size table of (callback, pid)
 * pairs, pid-tagged so a forked child doesn't re-run its parent's
 * handlers (matters once Tier 4's fork() lands -- today getpid() is
 * constant for the life of a process either way).
 *
 * Unlike principia's own atexit.c, this file does NOT also define
 * exits() -- that already exists here as port/exits.c, per-GOOS-
 * collapsed onto exit(int) rather than Plan9's raw EXITS syscall
 * (see that file's own header comment). atexitrun() below is the seam
 * between the two: port/exits.c's exits() calls it before exit(),
 * _exits() does not, matching the exact exits()-runs-atexit-handlers/
 * _exits()-doesn't split include/os/proc.h already promises.
 */

#define NEXIT 33

typedef struct Onex Onex;
struct Onex {
	void (*f)(void);
	int pid;
};

static Lock onexlock;
Onex onex[NEXIT];

int
atexit(void (*f)(void))
{
	int i;

	lock(&onexlock);
	for (i = 0; i < NEXIT; i++)
		if (onex[i].f == nil) {
			onex[i].pid = getpid();
			onex[i].f = f;
			unlock(&onexlock);
			return 1;
		}
	unlock(&onexlock);
	return 0;
}

void
atexitrun(void)
{
	int i, pid;
	void (*f)(void);

	pid = getpid();
	for (i = NEXIT-1; i >= 0; i--)
		if ((f = onex[i].f) && pid == onex[i].pid) {
			onex[i].f = nil;
			(*f)();
		}
}
