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

/* nsec() and time() for plan9. Adapted from
 * ~/principia/lib_core/libc/9sys/{nsec,time}.c.
 *
 * Like getpid(), these need no syscall: the clock is a file. Reading 8
 * bytes from /dev/bintime yields the nanoseconds since the epoch as a
 * BIG-ENDIAN integer -- big-endian regardless of the machine, since it
 * is a 9P-served file and 9P is byte-order-explicit. arm and mips are
 * both little-endian as built here, so the swap below is real work, not
 * a no-op that happens to be correct.
 *
 * sleep() is NOT here: it is a genuine syscall on this GOOS
 * (syscall/os/plan9/svc_$cputype.s), already taking milliseconds and
 * already shaped exactly like include/os/time.h's sleep(long).
 *
 * Two deliberate simplifications against principia's version, both
 * safe here and both worth knowing if this is ever revisited:
 *   - no per-pid fd table. principia keeps one because a threaded
 *     program's procs can have separate fd tables after rfork, so a
 *     single cached fd may be invalid in a sibling proc. This tree has
 *     no threads and no rfork yet.
 *   - no _tos->pid fast path, for the same reason.
 * A single cached fd is kept, which is the part that actually matters:
 * nsec() is called in loops (timing, profiling), and reopening the file
 * every call would dominate what it is trying to measure.
 */

static int bintimefd = -1;

/* the 8 bytes arrive most-significant first; assemble them explicitly
 * rather than casting, which would depend on this machine's byte order
 */
static vlong
be2vlong(uchar *b)
{
	vlong v;
	int i;

	v = 0;
	for(i = 0; i < 8; i++)
		v = (v << 8) | b[i];
	return v;
}

vlong
nsec(void)
{
	uchar b[8];

	if(bintimefd < 0) {
		bintimefd = open("/dev/bintime", OREAD|OCEXEC);
		if(bintimefd < 0)
			return 0;
	}
	/* pread, not read: the offset must stay 0 every time, and pread
	 * says so without a separate seek -- which also makes this safe
	 * against another proc sharing the same Chan offset, the race
	 * principia's own time.c works around with a retry loop.
	 */
	if(pread(bintimefd, b, sizeof b, 0) != sizeof b)
		return 0;
	return be2vlong(b);
}

long
time(long *tp)
{
	long t;

	t = (long)(nsec() / 1000000000LL);
	if(tp != nil)
		*tp = t;
	return t;
}
