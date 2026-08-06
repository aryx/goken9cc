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

/* putenv() for the POSIX-shaped GOOSes (linux and darwin) -- see
 * port/getenv.c's own header comment for why they share this file.
 * First real implementation anywhere in this tree (include/os/env.h
 * declared it from the start, but nothing backed it until now).
 *
 * The kernel-provided environment block (what environ() returns
 * before the first putenv() call) cannot be grown or shrunk in
 * place -- it sits on the initial stack, packed directly against
 * argv and the auxv that follows it, with no slack. So this always
 * allocates a fresh array (existing entries copied by pointer, only
 * the new/changed one freshly built with smprint()) and repoints
 * port/getenv.c's own _environp at it -- the same "can't mutate
 * in place, must switch to a new block entirely" shape a real
 * POSIX putenv()/setenv() has internally, just visible here instead
 * of hidden in glibc.
 *
 * Old arrays are deliberately not freed: a later putenv() might still
 * have entries aliased into an earlier array's strings... no, actually
 * each entry is copied by pointer into the new array too, so the old
 * array's SPINE (the char** itself) is safe to leak but not free --
 * something else could in principle still be mid-iteration over
 * environ()'s previous return value (the "invalidated by the next
 * putenv()" contract environ(3) already has to document even on a
 * real POSIX system). Not hot enough to be worth solving for real:
 * nothing in this tree calls putenv() in a loop.
 */

extern char **_environp;	/* port/getenv.c */

int
putenv(char *name, char *value)
{
	char **old, **new;
	char *entry, *p, *q;
	int i, n, idx;

	if(name == nil || *name == '\0')
		return -1;

	entry = smprint("%s=%s", name, value);
	if(entry == nil)
		return -1;

	old = environ();
	n = 0;
	if(old != nil)
		while(old[n] != nil)
			n++;

	idx = -1;
	for(i = 0; i < n; i++){
		p = old[i];
		q = name;
		while(*q != '\0' && *p == *q){
			p++;
			q++;
		}
		if(*q == '\0' && *p == '='){
			idx = i;
			break;
		}
	}

	if(idx >= 0){
		new = malloc((n+1) * sizeof(char*));
		if(new == nil)
			return -1;
		memmove(new, old, n * sizeof(char*));
		/* claude: the nil terminator has to be written explicitly --
		 * `old` holds n entries PLUS a terminator (n+1 slots), and the
		 * memmove above deliberately copies only the n entries, so
		 * without this the last slot of the fresh array is whatever
		 * malloc happened to leave there and every environ() walk runs
		 * off the end. Latent until port/exec.c stopped bypassing
		 * environ() (see its own comment): before that, no putenv()
		 * result was ever handed to execve(), so the missing
		 * terminator only had to survive in-process getenv() calls,
		 * which usually found their variable before reaching it. The
		 * else-branch below always got this right; only the
		 * replace-an-existing-variable path was wrong. */
		new[n] = nil;
		new[idx] = entry;
	}else{
		new = malloc((n+2) * sizeof(char*));
		if(new == nil)
			return -1;
		memmove(new, old, n * sizeof(char*));
		new[n] = entry;
		new[n+1] = nil;
	}
	_environp = new;
	return 0;
}
