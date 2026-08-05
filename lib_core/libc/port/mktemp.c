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

/* mktemp() (include/os/tmp.h) -- the deprecated, TOCTOU-racy half of
 * the mktemp()/mkstemp() pair (see that header's own comment): fills
 * in a template's trailing "XXXXXX" run with a plausible-unique name
 * and returns it, WITHOUT creating or checking for the file -- the
 * caller is responsible for that separately (and races on it, which is
 * exactly why mkstemp() exists as the safe alternative). Found self-
 * hosting linkers/ar/ar.c, which uses this exact mktemp()-then-
 * create() pattern for its own scratch/paging files (ORCLOSE, not a
 * persistent temp file).
 *
 * Same "good enough, not a real CSPRNG" uniqueness as port/mkstemp.c
 * (pid xored against an in-process counter) -- a separate counter, not
 * shared with mkstemp()'s, since these are independent name-generators
 * with no reason to coordinate.
 */

static int counter;

char*
mktemp(char *template)
{
	int len;
	long pid, n;

	len = strlen(template);
	if(len < 6 || strncmp(template+len-6, "XXXXXX", 6) != 0)
		return template;

	pid = getpid();
	n = (pid ^ (counter++ * 2654435761UL)) % 1000000;
	if(n < 0)
		n = -n;
	snprint(template+len-6, 7, "%06ld", n);
	return template;
}
