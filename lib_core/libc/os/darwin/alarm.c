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

/* alarm() for darwin -- setitimer(ITIMER_REAL), same call linux uses
 * (os/linux/alarm.c), but with this system's own timeval layout rather
 * than a shared one.
 *
 * The layout is the reason this is a separate file and not a shared
 * port/ one. Darwin's `struct timeval` is { __darwin_time_t tv_sec;
 * __darwin_suseconds_t tv_usec } = { long; int32 } (bsd/sys/_types/
 * _timeval.h) -- a MIXED-width pair, unlike Linux's two kernel longs,
 * so it needs the explicit {vlong, s32} spelling os/darwin/time.c
 * already worked out for gettimeofday. That struct is 16 bytes here
 * (8 + 4 + 4 tail padding, align 8), so the itimerval below is 32,
 * which is what the kernel expects on both arches this GOOS targets --
 * both are LP64, so unlike linux there is no width story beyond that.
 *
 * Getting the pair wrong would misread tv_usec as the top half of
 * tv_sec, the same trap time.c's own header comment describes -- here
 * it would silently arm a timer years out instead of milliseconds.
 *
 * SIGALRM's Plan9 note name ("alarm") is already in os/darwin/
 * postnote.c's sigtab. Note that darwin's notification support is
 * postnote()-only so far (lib_core/libc/mkfile's NOTIFYOFILES): there
 * is no notify() here yet, so a program cannot install a handler and
 * SIGALRM's default action -- process death -- is what actually
 * happens on expiry. That is still Plan9's own behaviour for an
 * unhandled alarm note, so the semantics are right even though the
 * useful case is not reachable until darwin grows a real notify().
 */

typedef struct Timeval Timeval;
struct Timeval {
	vlong	sec;
	s32	usec;
};

typedef struct Itimerval Itimerval;
struct Itimerval {
	Timeval	interval;	/* reload value; 0 = one-shot */
	Timeval	value;		/* time until next expiry; 0 = disarm */
};

#define ITIMER_REAL	0

extern int _syssetitimer(int which, void *new, void *old);

/* alarm(ms): see os/linux/alarm.c's identical contract -- one-shot,
 * returns the milliseconds left on the previous alarm, alarm(0)
 * cancels.
 */
long
alarm(ulong milli)
{
	Itimerval new, old;

	new.interval.sec = 0;
	new.interval.usec = 0;
	new.value.sec = (vlong)(milli / 1000);
	new.value.usec = (s32)((milli % 1000) * 1000);
	if(_syssetitimer(ITIMER_REAL, &new, &old) < 0)
		return -1;
	return (long)(old.value.sec * 1000 + old.value.usec / 1000);
}
