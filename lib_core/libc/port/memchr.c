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

/* memchr() (include/core/mem.h), plain copy of principia's
 * lib_core/libc/port/memchr.c. Found blocking utilities/byte/split.c
 * and utilities/pipe/p.c.
 */
void*
memchr(void *ap, int c, ulong n)
{
	uchar *sp;

	sp = ap;
	c &= 0xFF;
	while(n > 0) {
		if(*sp++ == c)
			return sp-1;
		n--;
	}
	return nil;
}
