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

/* atof() (include/str/ascii.h) -- declared for years, never
 * implemented (same "declared but nothing wired it up" gap port/
 * strstr.c, port/mkstemp.c and others hit before it). Same thin-
 * wrapper shape as port/atol.c's own atoi(), just over strtod()
 * instead of atol()'s own hand-rolled parse loop. Found self-hosting
 * assemblers/7a via objtype=arm64: yylex() calls it directly to parse
 * float-literal tokens.
 */
double
atof(char *s)
{
	return strtod(s, nil);
}
