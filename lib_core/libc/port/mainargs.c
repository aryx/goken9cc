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

/* _mainargv: the argv the process actually started with, stashed by
 * arch/$cputype/rt0.s on the way to main().
 *
 * It exists for exactly one caller, port/getenv.c, and only on the
 * POSIX-shaped GOOSes. There the environment is not obtained by any
 * syscall at all: the kernel lays it on the initial stack immediately
 * after argv's terminating nil, so "environ" is just argv walked past
 * its own end. rt0.s already has the argv pointer in a register on
 * every arch (it has to, to pass it to main), so capturing it costs one
 * store -- and then port/getenv.c can find the environment in portable
 * C, with no per-arch pointer arithmetic and no second syscall layer.
 *
 * Only argv is saved, not argc: getenv.c scans forward to argv's nil
 * rather than indexing past argc, which needs one global instead of two
 * and one store per rt0.s instead of two.
 *
 * Defined here in PORTOFILES, i.e. compiled for EVERY GOOS, even though
 * only linux and darwin have a use for it. arch/$cputype/rt0.s is shared
 * between linux and plan9 (only darwin gets its own rt0_darwin.s), so
 * the store in there is compiled for plan9 too and the symbol has to
 * resolve. Harmless: on plan9 nothing reads it (os/plan9/getenv.c goes
 * through /env instead), and on windows rt0.s never stores to it.
 *
 * Kept apart from port/argv0.c, whose argv0 is a different thing
 * entirely -- a program name the PROGRAM assigns to itself (or that
 * include/flag/cli.h's ARGBEGIN sets), not something startup fills in.
 */

char **_mainargv;
