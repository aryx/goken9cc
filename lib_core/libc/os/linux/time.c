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

/* time(), nsec() and sleep() for linux -- the "os/$GOOS/" glue over the
 * raw clock_gettime/clock_nanosleep in syscall/os/linux/.
 *
 * This file is arch-INDEPENDENT, which is the whole reason
 * numbers_$cputype.h picks the clock_* family over the obvious
 * gettimeofday/nanosleep. Those pass a struct whose fields are the
 * kernel's `long`: 4 bytes on 32-bit, 8 on 64-bit, so Timespec below
 * would need a per-arch layout and this file would need an #ifdef
 * ladder. Using the *_time64 forms on the 32-bit arches and the plain
 * ones on the 64-bit arches makes the struct 8+8 everywhere instead.
 *
 * Note vlong, not long, for the fields: `long` is 4 bytes in EVERY
 * compiler in this tree (SZ_LONG in compilers/*c/gc.h), including on
 * the 64-bit arches, so `long tv_sec` would be wrong on all seven --
 * the same trap that made uintptr wrong on amd64/arm64 (see
 * tests/c/regressions/uintptr_width.c).
 */

typedef struct Timespec Timespec;
struct Timespec {
	vlong	sec;
	vlong	nsec;
};

#define CLOCK_REALTIME	0

enum
{
	Nsec	= 1000000000LL,
	Msec	= 1000000LL
};

extern int _sysclockgettime(int clockid, void *ts);
extern int _sysclocknanosleep(int clockid, int flags, void *req, void *rem);

/* nsec(): nanoseconds since the epoch. Plan9's own nsec() reads
 * /dev/bintime; here it is the clock read directly.
 */
vlong
nsec(void)
{
	Timespec ts;

	if(_sysclockgettime(CLOCK_REALTIME, &ts) < 0)
		return 0;
	return ts.sec * Nsec + ts.nsec;
}

/* time(): seconds since the epoch, optionally also stored through tp.
 *
 * The return type is `long` (include/os/time.h), i.e. 4 bytes here, so
 * this truncates in 2038 -- a real limit, but one inherited from the
 * declared API rather than introduced by the syscall underneath, which
 * now genuinely carries 64 bits on every arch. Widening it would mean
 * changing time()'s signature and every caller, which is not this
 * change's business.
 */
long
time(long *tp)
{
	Timespec ts;
	long t;

	if(_sysclockgettime(CLOCK_REALTIME, &ts) < 0)
		return -1;
	t = (long)ts.sec;
	if(tp != nil)
		*tp = t;
	return t;
}

/* sleep(ms): Plan9's sleep takes MILLISECONDS (include/os/time.h), not
 * seconds like POSIX sleep(3) -- getting that wrong would be off by a
 * factor of 1000 in the quiet direction. sleep(0) means "yield" on
 * Plan9; clock_nanosleep with an all-zero timespec is the closest
 * equivalent and returns immediately.
 *
 * flags = 0 means the timespec is a RELATIVE duration; passing
 * TIMER_ABSTIME (1) instead would treat it as a wall-clock deadline.
 * rem is nil: we do not resume a sleep cut short by a signal, matching
 * what Plan9's own sleep(2) does (it returns -1 with "interrupted").
 */
int
sleep(long ms)
{
	Timespec req;

	req.sec = (vlong)ms / 1000;
	req.nsec = ((vlong)ms % 1000) * Msec;
	return _sysclocknanosleep(CLOCK_REALTIME, 0, &req, nil);
}
