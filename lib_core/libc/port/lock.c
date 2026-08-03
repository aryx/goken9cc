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

/* claude: NOT a port of ~/principia/lib_core/libc/port/lock.c, whose
 * lock()/unlock()/canlock() are built on ainc()/adec() (atomic
 * increment/decrement) and semacquire()/semrelease() (Plan9 kernel
 * semaphores) -- none of which this libc implements, since nothing
 * here creates a second thread of execution yet (no rfork-with-shared-
 * memory support, no libthread equivalent). A real contended lock has
 * no meaning without that, so these are no-op stubs instead, same
 * convention as fmt/fmtlock.c's own single-threaded __fmtlock/
 * __fmtunlock: the only caller today is lib_strings/libstring/
 * s_alloc.c's sp->lk, and there is only ever one thread able to
 * contend for it. Revisit (restore principia's real version) if/when
 * this libc grows actual concurrency.
 */

void
lock(Lock *l)
{
}

void
unlock(Lock *l)
{
}

int
canlock(Lock *l)
{
	return 1;
}
