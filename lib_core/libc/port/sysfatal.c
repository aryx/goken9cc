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

/* Ported as-is from ~/principia/lib_core/libc/9sys/sysfatal.c (the
 * "coupling: principia" reference this tree's syscall layer already
 * follows -- see lib_core/libc/syscall/os/plan9/sys.h). Unlike
 * BOOT/lib9/sysfatal.c's own version of this same hook (marked there
 * as "seems dead"), principia's _sysfatal is live: it is what
 * _sysfatalimpl below is assigned to by default, and any caller wanting
 * custom fatal-error reporting overrides the function pointer, not the
 * function.
 */

static void
_sysfatalimpl(char *fmt, va_list arg)
{
	char buf[1024];

	vseprint(buf, buf+sizeof(buf), fmt, arg);
	if(argv0)
		fprint(2, "%s: %s\n", argv0, buf);
	else
		fprint(2, "%s\n", buf);
	exits(buf);
}

void (*_sysfatal)(char *fmt, va_list arg) = _sysfatalimpl;

void
sysfatal(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	(*_sysfatal)(fmt, arg);
	va_end(arg);
}
