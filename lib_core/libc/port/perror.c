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

/* perror() (include/os/err.h), plain copy of principia's
 * lib_core/libc/port/perror.c -- Plan9's own perror() (errstr()-based,
 * not the APE/stdio.h Unix-compat macro, `#define perror(msg)
 * print(msg)`, which is a different, narrower shim only reached when
 * <APE/stdio.h> itself is #include'd). Found blocking
 * utilities/files/wc.c, which calls this one.
 */
void
perror(char *s)
{
	char buf[ERRMAX];

	buf[0] = '\0';
	errstr(buf, sizeof buf);
	if(s && *s)
		fprint(2, "%s: %s\n", s, buf);
	else
		fprint(2, "%s\n", buf);
}
