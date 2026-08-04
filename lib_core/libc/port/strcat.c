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

/* strcat() (include/base/str.h), plain copy of principia's
 * lib_core/libc/port/strcat.c. Found blocking utilities/byte/split.c.
 */
char*
strcat(char *s1, char *s2)
{
	strcpy(strchr(s1, '\0'), s2);
	return s1;
}
