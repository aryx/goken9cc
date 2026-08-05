/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* exec_fork.c -- execsh()/pipecmd() for linux/darwin/plan9 (fork()-
 * capable GOOSes). Split out of goken.c into its own GOOS-selected
 * file for the same reason notify_full.c/notify_none.c already are:
 * windows has no fork() at all, so a single shared goken.c can't
 * define these the same way everywhere -- see mk/exec_windows.c and
 * mk/mkfile's own EXECVARIANTOFILES, and docs/claude_notes/
 * notes_libc_api_design.txt's "spawn(): a portable process-spawn
 * primitive" section for the fuller design story.
 *
 * Bodies unchanged from goken.c's own original versions: rfork(RFPROC|
 * RFFDG|RFENVG) in Plan9.c's own execsh()/pipecmd() -> plain fork()
 * here, since Linux's/darwin's/plan9's own fork() already gives the
 * child an independent (copy-on-write) copy of the parent's
 * environment memory, the same thing RFENVG asks Plan9's rfork() for
 * explicitly.
 */
#include	"mk.h"

int
execsh(char *shargs, char *shinput, Bufblock *buf, ShellEnvVar *e)
{
	int pid1, pid2;
	fdt in[2];
	int err;
	char *endshinput;
	fdt out[2];
	int tot, n;

	if(buf && pipe(out) < 0){
		perror("pipe");
		Exit();
	}

	pid1 = fork();
	if(pid1 < 0){
		perror("mk fork");
		Exit();
	}
	if(pid1 == 0){
		if(buf)
			close(out[0]);
		err = pipe(in);
		if(err < 0){
			perror("pipe");
			Exit();
		}
		pid2 = fork();
		if(pid2 < 0){
			perror("mk fork");
			Exit();
		}
		if(pid2 != 0){
			dup(in[0], STDIN);
			if(buf){
				dup(out[1], STDOUT);
				close(out[1]);
			}
			close(in[0]);
			close(in[1]);
			if(e)
				exportenv(e);
			if(shflags)
				execl(shell->shell, shell->shellname, shflags, shargs, nil);
			else
				execl(shell->shell, shell->shellname, shargs, nil);
			perror(shell->shell);
			_exits("exec");
		}
		if(buf)
			close(out[1]);
		close(in[0]);
		endshinput = shinput + strlen(shinput);
		while(shinput < endshinput){
			n = write(in[1], shinput, endshinput - shinput);
			if(n < 0)
				break;
			shinput += n;
		}
		close(in[1]);
		_exits(nil);
	}
	if(buf){
		close(out[1]);
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
	return pid1;
}

int
pipecmd(char *cmd, ShellEnvVar *e, int *fd)
{
	int pid;
	fdt pfd[2];

	if(DEBUG(D_EXEC))
		fprint(STDOUT, "pipecmd='%s'\n", cmd);

	if(fd && pipe(pfd) < 0){
		perror("pipe");
		Exit();
	}
	pid = fork();
	if(pid < 0){
		perror("mk fork");
		Exit();
	}
	if(pid == 0){
		if(fd){
			close(pfd[0]);
			dup(pfd[1], 1);
			close(pfd[1]);
		}
		if(e)
			exportenv(e);
		if(shflags)
			execl(shell->shell, shell->shellname, shflags, "-c", cmd, nil);
		else
			execl(shell->shell, shell->shellname, "-c", cmd, nil);
		perror(shell->shell);
		_exits("exec");
	}
	if(fd){
		close(pfd[1]);
		*fd = pfd[0];
	}
	return pid;
}
