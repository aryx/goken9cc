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

/* time(), nsec() and sleep() for darwin.
 *
 * Both differ from linux's for the same underlying reason: this system
 * has no clock_gettime and no nanosleep syscall (see
 * numbers_darwin_amd64.h). So time comes from gettimeofday, and sleep
 * from select() with no descriptors and only a timeout -- the classic
 * BSD idiom, and a real syscall here where nanosleep is not.
 *
 * Timeval's field types are Darwin's, taken from bsd/sys/_types/
 * _timeval.h: { __darwin_time_t tv_sec; __darwin_suseconds_t tv_usec }
 * = { long; int32 }. That `long` is the KERNEL's, 8 bytes on both arches
 * this GOOS targets -- not this compiler's `long`, which is 4 bytes
 * everywhere (SZ_LONG). Hence vlong below. Getting this wrong would
 * misread tv_usec as the top half of tv_sec and put the clock ~136
 * years out. There is no 32-bit Darwin target here, so unlike linux's
 * timespec this needs no width story beyond that.
 */

typedef struct Timeval Timeval;
struct Timeval {
	vlong	sec;
	s32	usec;
};

enum
{
	Usec	= 1000000LL
};

extern int _sysgettimeofday(void *tv, void *tz, void *mach);
extern int _sysselect(int nd, void *in, void *ou, void *ex, void *tv);

vlong
nsec(void)
{
	Timeval tv;

	if(_sysgettimeofday(&tv, nil, nil) < 0)
		return 0;
	return tv.sec * 1000000000LL + (vlong)tv.usec * 1000LL;
}

long
time(long *tp)
{
	Timeval tv;
	long t;

	if(_sysgettimeofday(&tv, nil, nil) < 0)
		return -1;
	t = (long)tv.sec;
	if(tp != nil)
		*tp = t;
	return t;
}

/* sleep(ms) -- MILLISECONDS, per include/os/time.h, not POSIX sleep(3)'s
 * seconds. select with nd = 0 and all three fd sets nil waits purely on
 * the timeout.
 */
int
sleep(long ms)
{
	Timeval tv;

	tv.sec = (vlong)ms / 1000;
	tv.usec = (s32)((ms % 1000) * 1000);
	return _sysselect(0, nil, nil, nil, &tv);
}
