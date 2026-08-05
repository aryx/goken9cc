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

/* buildcmdline() -- CreateProcessA takes ONE command-line string, not
 * an argv[] array (unlike exec()'s own Plan9-shaped signature), so
 * both os/windows/exec.c and os/windows/spawn.c need to join argv back
 * into one before calling it. Shared here rather than duplicated: it's
 * the one piece of real, correctness-sensitive quoting logic in this
 * corner of the tree, and the two callers need to agree on it exactly.
 *
 * Intentionally simple, not a full implementation of Windows' own
 * notoriously intricate argv-quoting rules (backslash-before-quote
 * escaping, etc -- see Microsoft's own "Parsing C++ Command-Line
 * Arguments" documentation): each argv[i] containing a space is
 * wrapped in a plain pair of double quotes, every other argument is
 * copied through unchanged. Correct for the common case (paths, flags
 * with no embedded quote characters -- everything compilers/pcc/pcc.c's
 * own argv construction, and mk/rc's own recipe/command argv building,
 * ever produce), not a general-purpose shell-quoting engine.
 */
int
buildcmdline(char *buf, long bufsize, char *argv[])
{
	int i;
	long n;
	char *s;

	n = 0;
	for(i = 0; argv[i] != nil; i++){
		int quote;

		if(i > 0 && n < bufsize-1)
			buf[n++] = ' ';
		quote = 0;
		for(s = argv[i]; *s != '\0'; s++)
			if(*s == ' ')
				quote = 1;
		if(quote && n < bufsize-1)
			buf[n++] = '"';
		for(s = argv[i]; *s != '\0' && n < bufsize-1; s++)
			buf[n++] = *s;
		if(quote && n < bufsize-1)
			buf[n++] = '"';
	}
	if(n >= bufsize)
		return -1;
	buf[n] = '\0';
	return 0;
}
