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

/* cputime() (include/os/time.h) -- declared, never implemented.
 * Found self-hosting linkers/7l via objtype=arm64: every caller
 * (asm.c/noop.c/obj.c/pobj.c, all this tree's linkers likely share the
 * same calls) is a `debug['v']` progress print ("%5.2f asm\n",
 * cputime()) -- human-readable elapsed-time diagnostics, not anything
 * correctness-relevant. Real CPU time (excluding I/O wait etc.) would
 * need a per-GOOS syscall this libc doesn't have yet (times()/
 * clock_gettime(CLOCK_PROCESS_CPUTIME_ID,...)); wall-clock elapsed
 * time since the first call, built on nsec() (already implemented for
 * every GOOS this tree targets), is an honest, good-enough
 * approximation for what this is actually used for.
 */

static vlong start;

double
cputime(void)
{
	if(start == 0)
		start = nsec();
	return (double)(nsec() - start) / 1e9;
}
