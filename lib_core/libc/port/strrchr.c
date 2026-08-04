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

/* strrchr() (include/base/str.h), ported verbatim from principia's
 * lib_core/libc/port/strrchr.c: last occurrence of c in s, built on
 * strchr() (already in PORTOFILES) rather than its own scan. Needed by
 * utf/utfrrune.c's non-UTF fast path (c < Runesync).
 */
char*
strrchr(char *s, int c)
{
	char *r;

	if(c == '\0')
		return strchr(s, '\0');

	r = 0;
	while(s = strchr(s, c))
		r = s++;
	return r;
}
