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

/* dup() for windows -- rc self-hosting's own doredir() (rc/simple.c,
 * a shared file, `>file`/`2>&1`-style redirections before running a
 * command) needs a real dup(old, new), which os/windows/open.c's own
 * header comment explicitly says was left unimplemented: "it cannot
 * be implemented honestly until fd numbers stop being truncated
 * HANDLEs... dup(old, newfd) has to place a descriptor at a
 * CALLER-CHOSEN number, which a HANDLE-as-fd model cannot express at
 * all."
 *
 * That is still true in general -- an arbitrary caller-chosen `new`
 * has no Win32 equivalent without the real fd-table todo.org's own
 * windows dup() gap already flags as the eventual right fix. But
 * doredir()'s OWN real call sites only ever target new ∈ {0,1,2}
 * (stdin/stdout/stderr redirection -- the only fds a real shell
 * script's `>`/`<`/`2>&1` ever redirect), and THAT specific case has
 * an honest Win32 answer: DuplicateHandle the source, then
 * SetStdHandle to make the copy this process's new std handle for
 * that slot. So this implements exactly that slice -- new ∈ {0,1,2}
 * works for real, anything else returns -1 rather than silently doing
 * the wrong thing, same "confidently correct parts only" bar this
 * project already holds itself to elsewhere (darwin's notify()/
 * noted() gap, mk's own windows execsh() limitations, ...).
 */
#define STD_INPUT_HANDLE	(-10)
#define STD_OUTPUT_HANDLE	(-11)
#define STD_ERROR_HANDLE	(-12)

extern void *_wingetstdhandle(int std);
extern int _winduphandle(void *src, void *dst);
extern int _winsetstdhandle(int std, void *handle);

static void*
fdhandle(fdt fd)
{
	switch(fd){
	case 0:
		return _wingetstdhandle(STD_INPUT_HANDLE);
	case 1:
		return _wingetstdhandle(STD_OUTPUT_HANDLE);
	case 2:
		return _wingetstdhandle(STD_ERROR_HANDLE);
	default:
		return (void*)(vlong)fd;
	}
}

int
dup(fdt old, fdt new)
{
	void *src, *copy;
	int std;

	switch(new){
	case 0:
		std = STD_INPUT_HANDLE;
		break;
	case 1:
		std = STD_OUTPUT_HANDLE;
		break;
	case 2:
		std = STD_ERROR_HANDLE;
		break;
	default:
		/* no fd table yet -- see this file's own header comment */
		return -1;
	}

	src = fdhandle(old);
	copy = nil;
	if(!_winduphandle(src, &copy))
		return -1;
	if(!_winsetstdhandle(std, copy))
		return -1;
	return new;
}
