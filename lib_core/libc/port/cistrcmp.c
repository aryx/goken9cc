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

/* cistrcmp() (include/base/str.h), plain copy of principia's
 * lib_core/libc/port/cistrcmp.c. Found blocking utilities/files/du.c
 * (case-insensitive match against a table of scale-unit prefixes).
 */
int
cistrcmp(char *s1, char *s2)
{
	int c1, c2;

	while(*s1){
		c1 = *(uchar*)s1++;
		c2 = *(uchar*)s2++;

		if(c1 == c2)
			continue;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	return -*s2;
}
