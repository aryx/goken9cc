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

/* strncmp() (include/base/str.h), plain copy of principia's
 * lib_core/libc/port/strncmp.c. Found blocking lib_strings/libstring/
 * s_rdinstack.c and several utilities/ while self-hosting utilities/
 * with goken's own compiler+libc instead of the host bootstrap gcc+lib9.
 */

int
strncmp(char *s1, char *s2, long n)
{
	unsigned c1, c2;

	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}
