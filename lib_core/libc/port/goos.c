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

/* getgoos() (include/os/goos.h) -- BOOT/lib9/goos.c's own version
 * falls back to a compile-time-baked-in default (-DGOOS="..." on the
 * host bootstrap build's own command line) when $GOOS isn't set in
 * the environment; this libc's own build has no equivalent compile-
 * time flag plumbed through yet, so this is honestly narrower: reads
 * $GOOS only, nil if unset. Every caller in this tree already
 * null-checks the result before using it (e.g. linkers/7l/obj.c's own
 * -H auto-detection), and every mkfile invocation that matters here
 * already passes GOOS=... as a real mk variable, which mk's own
 * environment-export behavior (mk/env.c's initenv()) turns into a
 * real $GOOS for this to read -- same mechanism the mkfile's own
 * bootstrap target had to explicitly account for (BOOTLIBS leaking
 * the same way). Found self-hosting linkers/7l via objtype=arm64:
 * obj.c calls this directly for its own -H default when no explicit
 * -H flag is given.
 */
char*
getgoos(void)
{
	return getenv("GOOS");
}
