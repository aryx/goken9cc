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

/* werrstr() (include/os/err.h), plain copy of principia's
 * lib_core/libc/9sys/werrstr.c. Portable as-is: it only formats into a
 * local buffer and calls errstr() (port/errstr.c, the actually
 * GOOS-specific-in-spirit piece), same on every GOOS.
 */
void
werrstr(char *fmt, ...)
{
	va_list arg;
	char buf[ERRMAX];

	va_start(arg, fmt);
	vseprint(buf, buf+ERRMAX, fmt, arg);
	va_end(arg);
	errstr(buf, sizeof buf);
}
