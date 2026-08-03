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

/* time(), nsec(), sleep() and getpid() for windows, over kernel32 (see
 * winio_amd64.s for the stubs). This GOOS has no raw-syscall layer at
 * all, so unlike linux/darwin/plan9 there is nothing under this file --
 * these ARE the primitives.
 *
 * The FILETIME epoch shift is the only real content here. Windows
 * counts 100-nanosecond ticks since 1601-01-01, not seconds since
 * 1970-01-01, so every value needs both a scale change and an origin
 * change. The offset is 11644473600 seconds -- 369 years, of which 89
 * are leap years, i.e. (369*365 + 89) * 86400. Written out below rather
 * than as a bare magic number, because a wrong constant here is
 * invisible in testing (the clock is merely off by centuries, and no
 * test asserts an absolute date) yet corrupts every timestamp mk
 * compares.
 *
 * Note this file also carries getpid(), which has nowhere better to
 * live: it is neither file nor time, but os/windows/open.c is
 * explicitly the file-operations glue, and one more file for a
 * three-line function is worse than this.
 */

enum
{
	/* 100ns ticks per second, and per millisecond */
	Tick		= 10000000LL,
	/* seconds between 1601-01-01 and 1970-01-01 */
	Epochdelta	= ((369LL*365LL + 89LL) * 86400LL)
};

extern void _winfiletime(void *ft);
extern void _winsleep(ulong ms);
extern ulong _wingetpid(void);

/* FILETIME is documented as two 32-bit halves rather than a uvlong
 * because it need not be 8-byte aligned; we always pass an aligned
 * local, so reading it as one 64-bit value is safe here and avoids a
 * reassembly step.
 */
static uvlong
filetime(void)
{
	uvlong ft;

	ft = 0;
	_winfiletime(&ft);
	return ft;
}

vlong
nsec(void)
{
	uvlong ft;

	ft = filetime();
	if(ft == 0)
		return 0;
	return (vlong)(ft - (uvlong)Epochdelta * Tick) * 100LL;
}

long
time(long *tp)
{
	uvlong ft;
	long t;

	ft = filetime();
	if(ft == 0)
		return -1;
	t = (long)(ft / Tick - (uvlong)Epochdelta);
	if(tp != nil)
		*tp = t;
	return t;
}

/* sleep(ms): Win32's Sleep already takes milliseconds, matching
 * include/os/time.h exactly -- the only GOOS besides plan9 needing no
 * unit conversion. Sleep returns void, so there is no failure to
 * report; Plan9's sleep returns an int, and 0 is the honest answer.
 */
int
sleep(long ms)
{
	if(ms < 0)
		return -1;
	_winsleep((ulong)ms);
	return 0;
}

int
getpid(void)
{
	return (int)_wingetpid();
}
