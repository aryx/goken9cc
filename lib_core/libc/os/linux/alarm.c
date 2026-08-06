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

/* alarm() for linux -- the "os/$GOOS/" glue over the raw setitimer in
 * syscall/os/linux/, same shape as os/linux/time.c's glue over
 * clock_gettime/clock_nanosleep.
 *
 * setitimer(ITIMER_REAL), not the alarm(2) syscall, and not because
 * alarm(2) is worse: it does not EXIST on half the arches this GOOS
 * targets. The generic syscall ABI (arm64, riscv, riscv64) dropped the
 * whole legacy group -- alarm, pause, time -- in favour of the
 * setitimer/ppoll/clock_gettime family, exactly as it dropped fork()
 * and pipe() in favour of clone() and pipe2() (see this arch's own
 * .decl for that story). setitimer is present on all seven. It is also
 * the finer-grained call: alarm(2) takes whole SECONDS, while Plan9's
 * alarm() promises milliseconds, so alarm(2) could not implement this
 * signature honestly anywhere even where it exists.
 *
 * ITIMER_REAL delivers SIGALRM, which os/linux/notify.c's sigtab
 * already maps to the note string "alarm" -- so the receiving half of
 * this was in place before the arming half was, and a program that
 * has called notify() gets a real Plan9 "alarm" note. A program that
 * has NOT gets SIGALRM's default action, process death, which is also
 * what Plan9 does with an unhandled alarm note. Neither behaviour is
 * invented here.
 *
 * The itimerval fields are the KERNEL's `long`, so this file uses
 * intptr and stays arch-INDEPENDENT -- the thing os/linux/time.c had
 * to pick its syscalls carefully to achieve. It works here because
 * sizeof(long) == sizeof(void*) holds on every Linux ABI this tree
 * targets (ILP32 on 386/arm/mips/riscv, LP64 on amd64/arm64/riscv64),
 * so include/arch/$cputype/u.h's intptr is the kernel's long on all
 * seven. Note this is a fact about the ABIs, not about C: the
 * compiler's own `long` is 4 bytes on every arch here including the
 * 64-bit ones (SZ_LONG in compilers/*c/gc.h), which is precisely why
 * `long sec` would be wrong on four of them. Unlike the timespec in
 * time.c there is no *_time64 variant to escape to -- the time64
 * series (clock_gettime64 and friends, kernel 5.6) deliberately left
 * setitimer alone, since an interval is a duration and has no 2038
 * problem.
 */

typedef struct Timeval Timeval;
struct Timeval {
	intptr	sec;
	intptr	usec;
};

typedef struct Itimerval Itimerval;
struct Itimerval {
	Timeval	interval;	/* reload value; 0 = one-shot */
	Timeval	value;		/* time until next expiry; 0 = disarm */
};

#define ITIMER_REAL	0

enum
{
	Usec	= 1000000	/* microseconds per second */
};

extern int _syssetitimer(int which, void *new, void *old);

/* alarm(ms): arm a one-shot timer ms milliseconds from now, and return
 * the milliseconds that were left on the PREVIOUS one (0 if none was
 * pending). alarm(0) cancels without arming a new one -- an all-zero
 * it_value is setitimer's own disarm, and it is Plan9's meaning for
 * alarm(0) too, so the two agree with no special case here.
 *
 * interval is left zero so the timer does not reload: Plan9's alarm is
 * one-shot, and a caller wanting a periodic timer re-arms from its own
 * note handler (which is portable to plan9, where the kernel offers
 * nothing else). See include/os/time.h for why no general
 * setitimer()-shaped call is exposed on top of this.
 *
 * The `long` return truncates a pending alarm longer than about 24.8
 * days (2^31 ms), inherited from the declared signature rather than
 * introduced here -- the same shape of limit time()'s own `long` has,
 * and the same one BOOT/lib9/sleep.c's p9alarm() has always had.
 */
long
alarm(ulong milli)
{
	Itimerval new, old;

	new.interval.sec = 0;
	new.interval.usec = 0;
	new.value.sec = (intptr)(milli / 1000);
	new.value.usec = (intptr)((milli % 1000) * 1000);
	if(_syssetitimer(ITIMER_REAL, &new, &old) < 0)
		return -1;
	return (long)(old.value.sec * 1000 + old.value.usec / 1000);
}
