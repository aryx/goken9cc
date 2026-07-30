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

/* Minimal fopen()/fgets()/fclose() for include/APE/stdio.h -- real
 * file I/O (as opposed to that header's existing putc/fprintf, which
 * only ever write to the fixed fd 1/2), needed once a benchmark
 * actually reads its own input file (benchs/compcert/knucleotide.c,
 * the first and so far only consumer).
 *
 * FILE is a deliberately incomplete/opaque struct -- there is no real
 * FILE struct anywhere, `FILE*` is just an fdt (open()'s own return
 * type) reinterpreted as a pointer, so callers can never accidentally
 * dereference it. This intentionally skips two things a real stdio
 * would have: any internal buffering (fgets() below reads one byte at
 * a time via the raw read() syscall -- correct, just not fast, and
 * nothing wired up against this libc so far is I/O-bound enough for
 * that to matter) and write-mode support (fopen() always opens OREAD,
 * regardless of `mode` -- a real, narrower-than-standard limitation,
 * same spirit as this project's own fprintf()/putc() macros, not a
 * silent bug: extend properly, with an fdopen()-style write path, if a
 * future caller actually needs to write to a real file).
 *
 * `struct FILE` is redeclared locally here rather than shared via a
 * lib_core/libc-reachable header: include/ stays Plan9-native/APE-free
 * (this UNIX-compat FILE* concept has no place in include/os/file.h,
 * which declares Plan9's own real, FILE*-less file syscalls), so
 * include/APE/stdio.h (where application code actually sees this type)
 * has its own, separate `typedef struct FILE FILE;`. Two independent
 * declarations of the same never-defined (incomplete) struct tag are
 * fine in C -- neither side ever needs a real definition, just the
 * opaque pointer type, and nothing here needs them to be literally the
 * same declaration to link correctly.
 */
typedef struct FILE FILE;

FILE*
fopen(char *path, char *mode)
{
	fdt fd;

	USED(mode);
	fd = open(path, OREAD);
	if(fd < 0)
		return nil;
	return (FILE*)fd;
}

int
fclose(FILE *f)
{
	return close((fdt)f);
}

char*
fgets(char *buf, int size, FILE *f)
{
	fdt fd;
	int i;
	char c;

	if(size <= 0)
		return nil;
	fd = (fdt)f;
	i = 0;
	while(i < size - 1){
		if(read(fd, &c, 1) <= 0)
			break;
		buf[i++] = c;
		if(c == '\n')
			break;
	}
	buf[i] = 0;
	if(i == 0)
		return nil;
	return buf;
}
