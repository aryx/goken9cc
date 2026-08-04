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

/* rerrstr() (include/os/err.h), plain copy of principia's
 * lib_core/libc/9sys/rerrstr.c: a non-destructive read built on two
 * errstr() swaps (put an empty string in, get the real value out; put
 * the real value straight back). Portable as-is, same reasoning as
 * port/werrstr.c.
 */
void
rerrstr(char *buf, uint nbuf)
{
	char tmp[ERRMAX];

	tmp[0] = '\0';
	errstr(tmp, sizeof tmp);

	utfecpy(buf, buf+nbuf, tmp);
	errstr(tmp, sizeof tmp);
}
