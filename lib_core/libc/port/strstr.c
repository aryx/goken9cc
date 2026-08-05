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

/* strstr() -- rc self-hosting's real gap (docs/claude_notes/
 * notes_libc_selfhost.txt's "rc self-hosting survey"): declared
 * already (include/base/str.h) and needed both directly (rc/processes.c's
 * Execute()) and internally by utf/utfutf.c's own fast path (whenever
 * the needle's first rune represents itself, i.e. every plain ASCII
 * search), but never implemented anywhere in this tree until now.
 * Textbook substring search, nothing arch/GOOS-specific about it. */
char*
strstr(char *s1, char *s2)
{
	char *p1, *p2, *s;

	if(*s2 == '\0')
		return s1;
	for(; *s1 != '\0'; s1++){
		if(*s1 != *s2)
			continue;
		s = s1;
		p1 = s1;
		p2 = s2;
		while(*p1 != '\0' && *p2 != '\0' && *p1 == *p2){
			p1++;
			p2++;
		}
		if(*p2 == '\0')
			return s;
	}
	return nil;
}
