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

/* errstr() (include/os/err.h) -- the per-process error string swap
 * Plan9's own errstr(2) documents: copies the current error string
 * into buf, and replaces it with buf's OLD contents. On real Plan9
 * this is a raw kernel syscall (principia's lib_core/libc/9syscall/
 * errstr.s), so unlike ctime()/atexit()/etc there is no portable C
 * reference to copy -- this is a from-scratch implementation of the
 * documented contract, backed by one static buffer (single-threaded,
 * same assumption port/lock.c's own no-op locks already make).
 *
 * NOT bridged from POSIX errno/strerror(), unlike fmt/errfmt.c's %r:
 * checked first, and confirmed that nothing in this tree's syscall
 * layer (os/$GOOS/*.c, syscall/os/$GOOS/*) ever actually sets the
 * global `errno` on a failed call -- it is set in exactly one place
 * (fmt/strtod.c's ERANGE), so bridging from it here would silently
 * return an empty string on every real syscall failure anyway. Kept
 * honest instead: the per-process error string is exactly whatever
 * werrstr() last set it to (empty until then) -- a real, separate gap
 * (no caller in this tree currently reports failures via werrstr()),
 * tracked in todo.org rather than papered over with a fake bridge.
 *
 * Built for every GOOS uniformly (unlike PORTPOSIXOFILES's plan9/
 * windows exclusions elsewhere): this doesn't touch errno or any
 * syscall at all, so there is nothing GOOS-specific to get wrong --
 * including on plan9, whose own raw ERRSTR syscall (already reserved
 * in syscall/os/plan9/sys.h, unused) this does NOT call, matching the
 * "syscall layer doesn't populate errors yet" gap being uniform there
 * too, not better or worse than linux/darwin/windows today.
 */

static char errbuf[ERRMAX];

int
errstr(char *buf, uint n)
{
	char old[ERRMAX];

	strncpy(old, errbuf, ERRMAX);
	old[ERRMAX-1] = '\0';

	strncpy(errbuf, buf, ERRMAX);
	errbuf[ERRMAX-1] = '\0';

	if (n > 0) {
		strncpy(buf, old, n);
		buf[n-1] = '\0';
	}
	return 0;
}
