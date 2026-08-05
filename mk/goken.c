/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* goken.c -- a third variant, between Plan9.c and Posix.c, for
 * self-hosting mk with goken's own toolchain+libc on a POSIX-shaped
 * GOOS (linux/darwin/windows). See docs/claude_notes/
 * notes_libc_api_design.txt's "Environment (getenv/putenv/environ)"
 * entry for the design story.
 *
 * mk/mkfile's own OFILES picks exactly ONE of Posix.$O/Plan9.$O/
 * goken.$O per build (never two at once), so unlike a header this
 * file cannot share symbols with Plan9.c by simply not redefining
 * them -- every function mk's other files call that isn't defined
 * elsewhere in mk/ has to exist here too. Most of Plan9.c's own code
 * is ALREADY written against the portable Plan9-shaped libc surface
 * goken's own libc implements on every GOOS (dirstat/dirwstat,
 * wait()/waitpid()/exits()), so chgtime()/mkmtime()/xwaitfor()/
 * Exit()/rcopy() below are copied over UNCHANGED, not redesigned --
 * they would compile and behave identically here. Plan9.c's
 * dirtime()/bulkmtime() are dropped, not copied (dead code on this
 * branch -- see the comment at their old call site below). And
 * notifyf()/catchnotes()/expunge() (Tier 6's atnotify()/postnote()/
 * notify(), NOT implemented identically on every GOOS this file
 * targets) live in their own GOOS-selected variant files instead --
 * see the comment at their old call site below, and mk/mkfile's
 * NOTIFYVARIANTOFILES.
 *
 * What genuinely differs, and is the whole reason this file exists:
 *
 * - readenv()/exportenv(): Plan9.c's own versions read/write /env
 *   directly. goken's own environ()/getenv()/putenv() (include/os/
 *   env.h) are the portable equivalent -- see below.
 * - execsh()/pipecmd()'s rfork(RFPROC|RFFDG|RFENVG): goken has no
 *   rfork() implementation on Linux at all (only fork(), built
 *   internally on rfork(RFPROC|RFFDG|RFREND) on plan9 specifically --
 *   see os/plan9/fork.c). Plain fork() is the substitute: Linux's own
 *   fork() already gives the child an independent (copy-on-write)
 *   copy of the parent's environment memory, the same thing RFENVG
 *   asks Plan9's rfork() for explicitly.
 * - maketmp(): mktemp() (include/os/tmp.h) is declared but has zero
 *   implementation anywhere in this tree. Rather than add one just
 *   for this one call site, this builds a pid-based temp path
 *   directly -- mk only needs a name unlikely to collide with another
 *   concurrent mk, not cryptographic uniqueness, and getpid() already
 *   gives that for free.
 */
#include	"mk.h"

/* claude: mk.h's own Shell struct (a shell path + shell name pair,
 * pointed to by a single "extern Shell *shell" so mk can swap shells
 * at runtime via pushshell()/popshell()) postdates Plan9.c's own
 * separate "char *shell"/"char *shellname" globals -- Posix.c (the
 * variant actually compiled today) already matches mk.h's current
 * shape, so this follows Posix.c's pattern, not Plan9.c's stale one.
 * Positional (not designated) initializer: this compiler's frontend
 * doesn't support C99 ".field = value" initializers. */
Shell rcshell = { "/bin/rc", "rc" };
Shell *shell = &rcshell;

char*
maketmp(void)
{
	static char temp[64];

	sprint(temp, "/tmp/mkarg%d", getpid());
	return temp;
}

/* claude: unlike Plan9.c's own readenv() (which walks a /env
 * directory listing, one open()+read() per variable) or Posix.c's
 * (which mutates each "NAME=value" string from the real environ
 * array in place, splitting it at '='), this copies the NAME
 * substring into a fresh buffer instead -- environ()'s own contract
 * (include/os/env.h) says the array it returns must not be modified,
 * unlike a real POSIX environ this compiler could get away with
 * mutating.
 *
 * Each variable becomes exactly ONE Word (its whole value string),
 * not split on nulls the way Plan9.c's encodenulls() splits /env's
 * own nul-separated-list convention -- there is no such convention on
 * a POSIX environment, matching Posix.c's own simpler
 * newword(s+1)-only approach.
 */
void
readenv(void)
{
	char **e, *eq, *name;
	int len;
	Word *w;

	for(e = environ(); e != nil && *e != nil; e++){
		eq = strchr(*e, '=');
		if(eq == nil)
			continue;
		len = eq - *e;
		name = malloc(len+1);
		if(name == nil)
			continue;
		memmove(name, *e, len);
		name[len] = '\0';
		/* don't import funny names or internal mk variables */
		if(*shname(name) != '\0' || symlook(name, S_INTERNAL, nil)){
			free(name);
			continue;
		}
		w = newword(eq+1);
		setvar(name, (void*)w);
	}
}

/* claude: done on the child side of fork() (same convention Plan9.c's
 * own exportenv() documents), so mutating the real environment here
 * never affects the parent mk.
 *
 * Multi-word values are joined with a plain space, not Plan9.c's own
 * nul-separator convention (encodenulls()'s counterpart on the way
 * back out) -- there is no reason for a POSIX child process to see
 * embedded nuls in its environment, and a space-joined value is what
 * every other POSIX program already expects there.
 *
 * No unsetenv() equivalent exists in this libc yet (include/os/env.h
 * only has getenv()/putenv()/environ()), so a variable mk wants
 * REMOVED (present in the real symbol table with no value) simply
 * stays inherited from the parent's real environment instead of being
 * deleted -- a real, accepted limitation, not an oversight. Matches
 * this project's "diff-and-putenv is fine" decision: putenv() alone
 * handles the add/update case every real caller in this tree actually
 * needs, and removal was explicitly not asked for.
 */
void
exportenv(ShellEnvVar *e)
{
	char *values;
	Symtab *sym;
	bool hasvalue;

	for(; e->name; e++){
		hasvalue = !empty_words(e->values);
		sym = symlook(e->name, S_VAR, nil);
		if(sym == nil && !hasvalue)
			continue;
		if(sym != nil && !hasvalue){
			freewords(e->values);
			e->values = nil;
			continue;
		}
		values = wtos(e->values, ' ');
		putenv(e->name, values);
		free(values);
	}
}

/* claude: rfork(RFPROC|RFFDG|RFENVG) in Plan9.c's own execsh() ->
 * plain fork() here -- see this file's own header comment. */
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

/* claude: rfork(RFPROC|RFFDG|RFENVG) in Plan9.c's own pipecmd() ->
 * plain fork() here -- see this file's own header comment. */
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

/* claude: everything below is copied UNCHANGED from mk/Plan9.c -- it
 * is already written against goken's own portable libc surface (see
 * this file's own header comment), nothing GOOS-specific to swap. */

int
chgtime(char *name)
{
	Dir sbuf;

	if(access(name, AEXIST) >= 0) {
		nulldir(&sbuf);
		sbuf.mtime = time((long *)nil);
		return dirwstat(name, &sbuf);
	}
	return close(create(name, OWRITE, 0666));
}

/* claude: Plan9.c's own dirtime()/bulkmtime() (a dirread()-based bulk
 * mtime cache, meant to precede a run of mkmtime() calls under the
 * same directory) are dropped here rather than copied over: nothing
 * in this mk/ tree actually calls them (mk.h declares them but grep
 * across mk/*.c turns up only the "-- is back in Plan9.c" placeholder
 * comment in file.c, no real call site) -- dead code on this branch,
 * not a genuinely-needed function. Worth noting for darwin/windows
 * specifically: dirread() itself has no implementation there yet
 * (lib_core/libc/mkfile's OSGOOSOFILES), so keeping these two would
 * have blocked those GOOSes from linking mk at all over an
 * unused code path. */

double
mkmtime(char *name, bool force)
{
	Dir *d;
	ulong t;
	char buf[4096];

	strecpy(buf, buf + sizeof buf - 1, name);
	cleanname(buf);
	name = buf;
	USED(force);
	d = dirstat(name);
	if(d == nil)
		return 0;
	t = d->mtime;
	free(d);

	return t;
}

pidt
xwaitfor(char *msg)
{
	Waitmsg *w;
	pidt pid;

	/* blocking call, wait for any children */
	w = wait();
	/* no more children */
	if(w == nil)
		return -1;
	strecpy(msg, msg+ERRMAX, w->msg);
	pid = w->pid;
	free(w);
	return pid;
}

void
Exit(void)
{
	while(waitpid() >= 0)
		;
	exits("error");
}

/* claude: notifyf()/catchnotes()/expunge() (main.c's unconditional
 * catchnotes() call, run.c's killchildren()->expunge() path) are NOT
 * defined here -- unlike everything else in this file, they don't
 * work identically on every POSIX-shaped GOOS goken.c targets.
 * catchnotes() needs notify()/noted() (only implemented for GOOS=linux
 * so far -- Tier 6, docs/claude_notes/notes_libc_selfhost.txt);
 * expunge() only needs postnote() (implemented for linux AND darwin,
 * but not windows). Three GOOS-conditional variant files
 * (notify_full.c/notify_partial.c/notify_none.c, selected in
 * mk/mkfile the same way lib_core/libc/mkfile's own NOTIFYOFILES
 * picks per-GOOS notify support) provide these instead of a #ifdef
 * $GOOS branch inside this one -- same file-selection-over-branching
 * habit CLAUDE.md asks for. */

void
rcopy(char **to, Resub *match, int n)
{
	int c;
	char *p;

	*to = match->s.sp;		/* stem0 matches complete target */
	for(to++, match++; --n > 0; to++, match++){
		if(match->s.sp && match->e.ep){
			p = match->e.ep;
			c = *p;
			*p = 0;
			*to = strdup(match->s.sp);
			*p = c;
		}
		else
			*to = 0;
	}
}
