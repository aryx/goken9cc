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

/* execl() (include/os/proc.h) -- the variadic-argv convenience over
 * exec(), same two-pass "count then fill" shape as BOOT/lib9/execl.c
 * (this project's own gcc-built reference for the call): one pass to
 * find how many arguments were given (a nil-terminated va_arg walk),
 * then a malloc'd argv array and a second pass to fill it, since
 * va_list can only be walked once per va_start/va_end pair.
 */

int
execl(char *prog, ...)
{
	int i;
	va_list arg;
	char **argv;

	va_start(arg, prog);
	for(i = 0; va_arg(arg, char*) != nil; i++)
		;
	va_end(arg);

	argv = malloc((i+1)*sizeof(char*));
	if(argv == nil)
		return -1;

	va_start(arg, prog);
	for(i = 0; (argv[i] = va_arg(arg, char*)) != nil; i++)
		;
	va_end(arg);

	exec(prog, argv);
	free(argv);
	return -1;
}
