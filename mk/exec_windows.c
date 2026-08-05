/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* exec_windows.c -- execsh()/pipecmd() for windows, built on the new
 * portable spawn() (include/os/proc.h, docs/claude_notes/
 * notes_libc_api_design.txt's "spawn(): a portable process-spawn
 * primitive" section) instead of mk/exec_fork.c's fork()+dup()+exec().
 *
 * Much simpler than exec_fork.c's own double-fork structure: that
 * shape exists there ONLY because a Unix fork() is needed to get a
 * second, independent process that can feed shinput into the shell's
 * stdin without blocking the top-level caller -- spawn() doesn't need
 * that trick at all, since CreateProcessA (what it's built on) is
 * already non-blocking. One spawn() call, then this same top-level
 * call writes shinput and drains stdout itself, sequentially.
 *
 * That sequential write-then-read IS a real, accepted limitation
 * spawn()'s own POSIX-side double-fork avoids: if shinput is large
 * enough to fill the pipe buffer before the child starts reading it
 * (e.g. because the child is itself blocked writing a large amount of
 * stdout this process hasn't started draining yet), this can
 * deadlock. Not fixed here -- doing so honestly would need a
 * background thread to feed stdin concurrently with draining stdout,
 * and this tree has no threading primitives yet. Accepted because mk
 * recipes are realistically small (this is recipe TEXT, not command
 * output), the same "confidently correct parts only, documented gap"
 * standard this project's other windows/darwin work already holds
 * itself to, and because there is no Windows host here to verify a
 * more elaborate fix against anyway.
 */
#include	"mk.h"

int
execsh(char *shargs, char *shinput, Bufblock *buf, ShellEnvVar *e)
{
	fdt in[2], out[2];
	char *argv[4];
	int i, pid;
	char *endshinput;
	long n, tot;

	if(pipe(in) < 0){
		perror("pipe");
		Exit();
	}
	if(buf && pipe(out) < 0){
		perror("pipe");
		Exit();
	}

	if(e)
		exportenv(e);

	i = 0;
	argv[i++] = shell->shellname;
	if(shflags)
		argv[i++] = shflags;
	argv[i++] = shargs;
	argv[i] = nil;

	pid = spawn(shell->shell, argv, in[0], buf ? out[1] : -1, -1);
	close(in[0]);
	if(buf)
		close(out[1]);
	if(pid < 0){
		perror(shell->shell);
		Exit();
	}

	endshinput = shinput + strlen(shinput);
	while(shinput < endshinput){
		n = write(in[1], shinput, endshinput - shinput);
		if(n < 0)
			break;
		shinput += n;
	}
	close(in[1]);

	if(buf){
		tot = 0;
		for(;;){
			if(buf->current >= buf->end)
				growbuf(buf);
			n = read(out[0], buf->current, buf->end-buf->current);
			if(n <= 0)
				break;
			buf->current += n;
			tot += n;
		}
		if(tot && buf->current[-1] == '\n')
			buf->current--;
		close(out[0]);
	}
	return pid;
}

int
pipecmd(char *cmd, ShellEnvVar *e, int *fd)
{
	fdt pfd[2];
	char *argv[5];
	int i, pid;

	if(DEBUG(D_EXEC))
		fprint(STDOUT, "pipecmd='%s'\n", cmd);

	if(fd && pipe(pfd) < 0){
		perror("pipe");
		Exit();
	}

	if(e)
		exportenv(e);

	i = 0;
	argv[i++] = shell->shellname;
	if(shflags)
		argv[i++] = shflags;
	argv[i++] = "-c";
	argv[i++] = cmd;
	argv[i] = nil;

	pid = spawn(shell->shell, argv, -1, fd ? pfd[1] : -1, -1);
	if(fd)
		close(pfd[1]);
	if(pid < 0){
		perror(shell->shell);
		Exit();
	}
	if(fd)
		*fd = pfd[0];
	return pid;
}
