/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* Intentionally empty. Every other os/$GOOS/open.c translates Plan9's
 * open() mode bits (OREAD/OWRITE/OTRUNC/...) into that OS's own real
 * flag encoding before calling a differently-named raw syscall
 * (_sysopen()) -- but Plan9's own real open(2) syscall already speaks
 * exactly this dialect (same 2-arg shape, same OREAD/OWRITE/... bits,
 * see include/os/file.h), so there's nothing to translate: the raw
 * syscall (lib_core/libc/syscall/os/plan9/svc_$cputype.s) is directly
 * named "open" and needs no glue on top at all. This file exists only
 * so lib_core/libc/mkfile's existing, GOOS-generic OSFILES rule
 * (os/$GOOS/open.$O) has something to compile without needing a
 * plan9-specific carve-out there too.
 */
