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

/* spawn() (include/os/proc.h) -- see docs/claude_notes/
 * notes_libc_api_design.txt's "spawn(): a portable process-spawn
 * primitive" section for the full design story and cross-language
 * precedent (POSIX's own posix_spawn(), libuv's uv_spawn(), Rust's
 * Command::spawn(), ...). This file is the linux/darwin/plan9 side --
 * one portable implementation, no GOOS branching, since all three
 * already have a plain, flag-free fork() (port/fork.c's raw syscall
 * wrapper on linux/darwin, os/plan9/fork.c's rfork(RFPROC|RFFDG|RFREND)
 * on plan9) and a plain dup(oldfd, newfd)/exec(path, argv). The
 * windows side (os/windows/spawn.c) is genuinely different: no fork()
 * there at all, only the atomic CreateProcess.
 *
 * fdin/fdout/fderr: -1 means "leave this GOOS's own stdin/stdout/
 * stderr untouched"; any other value is dup()'d onto 0/1/2 in the
 * child, then closed if it wasn't already at that fd number -- the
 * exact fork()+dup()+exec() sequence mk/goken.c's own execsh()/
 * pipecmd() (and rc/processes.c's Xasync()) already hand-roll, factored
 * into one reusable call instead of duplicated per-caller code.
 *
 * Returns immediately after fork() in the parent -- does not wait.
 * Same contract windows' CreateProcess-based version has to have
 * anyway (CreateProcess itself is non-blocking), so callers on every
 * GOOS write/read whichever fd they handed in and close it, then
 * wait() themselves, exactly like today's fork()-based callers already
 * do.
 */
int
spawn(char *path, char **argv, fdt fdin, fdt fdout, fdt fderr)
{
	int pid;

	pid = fork();
	if(pid != 0)
		return pid;

	if(fdin >= 0){
		dup(fdin, 0);
		if(fdin != 0)
			close(fdin);
	}
	if(fdout >= 0){
		dup(fdout, 1);
		if(fdout != 1)
			close(fdout);
	}
	if(fderr >= 0){
		dup(fderr, 2);
		if(fderr != 2)
			close(fderr);
	}
	exec(path, argv);
	_exits("exec");
	return -1; /* unreachable */
}
