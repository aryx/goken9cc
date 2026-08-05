/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* goken.c -- rc's own counterpart to mk/goken.c: a third variant, for
 * self-hosting rc with goken's own toolchain+libc on a POSIX-shaped
 * GOOS (linux/darwin/windows), between rc/plan9.c (the real Plan9
 * host, notify()/noted()/dirread()-based) and rc/unix.c (a real-glibc
 * host: <errno.h>, NOPLAN9DEFINES, a real assignable `environ` global,
 * signal()/kill()/wait(int*)/opendir()/readdir() -- none of which
 * exist in this libc). rc/mkfile's own OFILES picks exactly ONE of
 * unix.$O/plan9.$O/goken.$O per build (unix.$O is rc's own current
 * unconditional default -- unlike mk/mkfile, rc/mkfile never had a
 * Posix.$O/Plan9.$O split at all), so goken.c independently defines
 * every symbol rc's other files call that isn't defined elsewhere in
 * rc/, not just the pieces that differ from one variant or the other.
 * See docs/claude_notes/notes_libc_selfhost.txt's "rc self-hosting
 * survey" entry for the fuller design story.
 *
 * Most functions below are copied UNCHANGED from whichever of
 * unix.c/plan9.c already happens to be portable against this libc as
 * it stands (noted per function), not redesigned:
 *
 * - Rcmain/Fdprefix, enval()/bigpath()/pathinit() (PATH<->path list
 *   conversion), cmpenv()/mkenv() (pure malloc+string logic building a
 *   fresh "NAME=value"-shaped block from rc's own variable table), and
 *   Exit()/Globsize()/Malloc() all come from unix.c verbatim -- they
 *   were already written against portable primitives (exit(), fprint,
 *   malloc, plain string ops), the <errno.h>/environ-global stuff
 *   lives in the handful of OTHER unix.c functions this file does NOT
 *   reuse.
 * - signame[] (trap.c's own extern dependency, so this needs defining
 *   regardless of notify() support), Eintr()/Noerror()/`interrupted`
 *   (a note-driven flag, not errno-driven) come from plan9.c verbatim.
 * - Opendir()/Readdir()/Closedir() come from plan9.c verbatim
 *   (dirread()/dirfstat()-based, already portable on every GOOS this
 *   libc implements them for).
 *
 * What genuinely differs, and is the whole reason this file exists:
 *
 * - Vinit()/Updenv(): unix.c's own versions read/write the real
 *   assignable `environ` global directly, which conflicts with this
 *   tree's environ() being a FUNCTION (include/os/env.h). Rewritten
 *   below to call environ()/putenv() instead -- mkenv()'s own output
 *   shape (a "NAME=value"-joined char** block) is reused unchanged,
 *   only how it gets INSTALLED differs.
 * - Waitfor(): unix.c's own version decodes a raw POSIX wstat int from
 *   a real wait(int*); this libc's own wait() (include/os/proc.h) is
 *   Plan9-shaped instead (Waitmsg* with a ready-made message STRING,
 *   not a wstat to decode) -- arguably closer to what a genuine
 *   Plan9-hosted rc would see than unix.c's own POSIX-decoding version
 *   is. The `persist` parameter (retry through EINTR) has no clean
 *   equivalent: this libc's own wait() (port/wait.c) returns nil for
 *   ANY failure, interrupted-by-a-note included, with no way to tell
 *   that case apart from "no more children" -- an accepted, pre-
 *   existing limitation of wait() itself, not something this file
 *   works around.
 * - Isatty(): plan9.c's own version is fd2path()-based (a real Plan9
 *   syscall with zero POSIX implementation in this tree). Routed
 *   instead through a genuinely NEW libc primitive, os/linux/isatty.c
 *   (a real ioctl(TCGETS) probe) -- not a port of an existing
 *   principia/lib9 file, unlike almost everything else Tier 6-onward
 *   added this session.
 * - Xrdfn(): referenced by name as a real function pointer in pfnc.c's
 *   builtin dispatch table (compiled in regardless), but its only real
 *   trigger, execfinit(), is ALREADY a no-op on this GOOS (builtins.c's
 *   own "TODO: commented for goken for now because Linux does not have
 *   a /env" -- predates this file). A minimal stub that just calls
 *   Xreturn() satisfies the linker without pretending the /env-based
 *   function-reading feature works here.
 *
 * notifyf()/Trapinit() (rc's own notify()/noted()-based signal
 * handling, plan9.c's shape) live in their own GOOS-selected variant
 * files instead of here -- same reason and same split shape as
 * mk/goken.c's notify_full.c/notify_partial.c/notify_none.c: notify()/
 * noted() aren't implemented on every GOOS this file targets. See
 * rc/mkfile's own NOTIFYVARIANTOFILES.
 */
#include	"rc.h"
#include	"fns.h"
#include	"io.h"
#include	"exec.h"
#include	"getflags.h"

// system-specific globals defined here but used in other files
//goken: was /usr/lib/rcmain
char *Rcmain = "#9/etc/rcmain.unix";
char *Fdprefix = "/dev/fd/";

//******************************************************************************
// Environment
//******************************************************************************

#define	SEP	'\1'

word*
enval(char *s)
{
	char *t, c;
	word *v;
	for(t = s; *t && *t!=SEP; t++);
	c=*t;
	*t='\0';
	v = newword(s, c=='\0'?(word *)0:enval(t+1));
	*t = c;
	return v;
}

// from plan9port/sys/.../rc/var.c
void
bigpath(var *v)
{
	/* convert $PATH to $path */
	char *p, *q;
	word **l, *w;

	if(v->val == nil){
		setvar("path", nil);
		return;
	}
	p = v->val->word;
	w = nil;
	l = &w;
	/*
	 * Doesn't handle escaped colon nonsense.
	 */
	if(p[0] == 0)
		p = nil;
	while(p){
		q = strchr(p, ':');
		if(q)
			*q = 0;
		*l = newword(p[0] ? p : ".", nil);
		l = &(*l)->next;
		if(q){
			*q = ':';
			p = q+1;
		}else
			p = nil;
	}
	setvar("path", w);
}

/* claude: unix.c's own pathinit() also calls unsharp(Rcmain) here,
 * translating the "#9/etc/rcmain.unix" sharp-path into a real
 * filesystem path (a plan9port compatibility shim -- include/os/
 * path.h's own unsharp() declaration is commented out, no
 * implementation exists anywhere in this tree). Rcmain is only ever
 * used to `. $Rcmain` a bootstrap init script (main.c), which already
 * fails gracefully like any other missing sourced file if it doesn't
 * exist -- true here regardless, since no rcmain.unix install path has
 * been set up for a goken-native build yet -- so this just skips the
 * translation rather than inventing an unsharp() shim for one string. */
void
pathinit(void)
{
	var *v;

	v = gvlook("PATH");
	bigpath(v);
}

/* claude: environ()-based, unlike unix.c's own `extern char **environ;
 * char **env = environ;` -- see this file's own header comment. */
void
Vinit(void)
{
	char **env, *s;
	word *wd;

	for(env = environ(); env != nil && *env != nil; env++){
		for(s = *env; *s && *s!='(' && *s!='='; s++);
		switch(*s){
		case '\0':
			pfmt(err, "environment %q?\n", *env);
			break;
		case '=':
			*s = '\0';
			wd = enval(s+1);
			setvar(*env, wd);
			if(strcmp(*env, "RCMAIN") == 0)
				Rcmain = strdup(wd->word);
			*s = '=';
			break;
		case '(':	/* ignore functions for now */
			break;
		}
	}
	pathinit();
}

/* claude: Xrdfn()'s only real trigger, execfinit() (builtins.c), is
 * already a no-op on this GOOS -- see this file's own header comment.
 * pfnc.c's builtin dispatch table still references Xrdfn by name
 * (compiled in regardless of whether execfinit() ever runs it), so a
 * stub is needed to satisfy the linker; it just behaves as if there
 * were never any more function definitions to read back. */
void
Xrdfn(void)
{
	Xreturn();
}

/* claude: cmpenv()/mkenv() are unix.c's own, copied verbatim -- pure
 * malloc+string logic building a fresh "NAME=value"-shaped char**
 * block from rc's variable table, nothing POSIX-specific in either.
 * See unix.c's own header comment on mkenv() for the SEP/'\1' and
 * "#()fn name body" encoding this format uses. */
static int
cmpenv(const void *aa, const void *ab)
{
	char **a = (char **)aa, **b = (char **)ab;

	return strcmp(*a, *b);
}

static char **
mkenv(void)
{
	char **env, **ep, *p, *q;
	var **h, *v;
	word *a;
	int nvar = 0, nchr = 0, sep;

	for(h = gvar-1; h != &gvar[NVAR]; h++)
	for(v = h >= gvar? *h: (runq? runq->local : nil); v ;v = v->next){
		if((v==vlook(v->name)) && v->val){
			nvar++;
			nchr+=strlen(v->name)+1;
			for(a = v->val;a;a = a->next)
				nchr+=strlen(a->word)+1;
		}
		if(v->fn){
			nvar++;
			nchr+=strlen(v->name)+strlen(v->fn[v->pc-1].s)+8;
		}
	}
	env = (char **)emalloc((nvar+1)*sizeof(char *)+nchr);
	ep = env;
	p = (char *)&env[nvar+1];
	for(h = gvar-1; h != &gvar[NVAR]; h++)
	for(v = h >= gvar? *h: (runq? runq->local : nil);v;v = v->next){
		if((v==vlook(v->name)) && v->val){
			*ep++=p;
			q = v->name;
			while(*q) *p++=*q++;
			sep='=';
			for(a = v->val;a;a = a->next){
				*p++=sep;
				sep = SEP;
				q = a->word;
				while(*q) *p++=*q++;
			}
			*p++='\0';
		}
		if(v->fn){
			*ep++=p;
			*p++='#'; *p++='('; *p++=')';	/* to fool Bourne */
			*p++='f'; *p++='n'; *p++=' ';
			q = v->name;
			while(*q) *p++=*q++;
			*p++=' ';
			q = v->fn[v->pc-1].s;
			while(*q) *p++=*q++;
			*p++='\0';
		}
	}
	*ep = 0;
	qsort((void *)env, nvar, sizeof ep[0], cmpenv);
	return env;
}

/* claude: putenv()-based, unlike unix.c's own `environ = env;` -- see
 * this file's own header comment. mkenv()'s block is still rebuilt
 * from scratch every call (same as unix.c: rc's own var table has no
 * per-variable "changed" tracking on this side, unlike plan9.c's own
 * v->changed-gated addenv()), but installing it now means one
 * putenv() per "NAME=value" entry instead of one pointer swap -- no
 * unsetenv() equivalent exists in this libc yet, so a variable rc
 * unset stays inherited from the parent's real environment rather
 * than being removed, the same accepted limitation mk/goken.c's own
 * exportenv() documents. */
void
Updenv(void)
{
	char **env, *eq, *name;
	int len;

	env = mkenv();
	for(; *env != nil; env++){
		eq = strchr(*env, '=');
		if(eq == nil)
			continue;
		len = eq - *env;
		name = malloc(len+1);
		if(name == nil)
			continue;
		memmove(name, *env, len);
		name[len] = '\0';
		putenv(name, eq+1);
		free(name);
	}
	efree((char *)env);
}

//******************************************************************************
// Signals/notes and Waitfor()
//******************************************************************************

/* claude: from plan9.c verbatim -- trap.c's own `extern char
 * *signame[]` needs this regardless of whether notify()/noted() are
 * implemented on this GOOS, so it lives here rather than in the
 * notify_full.c/notify_none.c split. */
char *signame[] = {
	"sigexit",
	"sighup",
	"sigint",
	"sigquit",
	"sigalrm",
	"sigkill",
	"sigfpe",
	"sigterm",
	0
};

/* claude: from plan9.c verbatim -- set by notifyf() (notify_full.c),
 * read by Eintr()/Noerror() below regardless of whether notify()
 * itself is available on this GOOS (on a GOOS without it, this simply
 * never becomes true, matching notify_none.c's own no-op Trapinit()). */
bool interrupted = false;

bool
Eintr(void)
{
	return interrupted;
}

void
Noerror(void)
{
	interrupted = false;
}

/* claude: this libc's own wait() (include/os/proc.h, port/wait.c) is
 * Plan9-shaped -- a Waitmsg* with a ready-made message string, not a
 * raw POSIX wstat int to decode -- so this is NOT unix.c's own
 * Waitfor() with the decoding stripped out, it is closer to what a
 * genuine Plan9-hosted rc would see. `persist` (retry through EINTR)
 * has no clean equivalent: port/wait.c's own wait() returns nil for
 * ANY failure, an interrupted-by-a-note wait4() included, with no way
 * to distinguish that from "no more children" -- see this file's own
 * header comment. */
int
Waitfor(int pid, bool persist)
{
	Waitmsg *w;
	thread *p;

	USED(persist);
	for(;;){
		w = wait();
		if(w == nil)
			return -1;
		if(w->pid == pid){
			setstatus(w->msg);
			free(w);
			return 0;
		}
		for(p = runq->ret; p; p = p->ret)
			if(p->pid == w->pid){
				p->pid = -1;
				strecpy(p->status, p->status+NSTATUS-1, w->msg);
				break;
			}
		free(w);
	}
}

//******************************************************************************
// Directories
//******************************************************************************

/* claude: Opendir()/Readdir()/Closedir() from plan9.c verbatim --
 * dirread()/dirfstat()-based, already portable on every GOOS this libc
 * implements them for (currently linux; see lib_core/libc/mkfile's
 * OSGOOSOFILES). */
#define	NFD	50

struct DirEntryWrapper {
	Dir	*dbuf;
	int	i;
	int	n;
};
struct DirEntryWrapper dir[NFD];

static int
trimdirs(Dir *d, int nd)
{
	int r, w;

	for(r=w=0; r<nd; r++)
		if(d[r].mode&DMDIR)
			d[w++] = d[r];
	return w;
}

int
Readdir(int f, void *p, int onlydirs)
{
	int n;

	if(f<0 || f>=NFD)
		return 0;
Again:
	if(dir[f].i==dir[f].n){	/* read */
		free(dir[f].dbuf);
		dir[f].dbuf = 0;
		n = dirread(f, &dir[f].dbuf);
		if(n>0){
			if(onlydirs){
				n = trimdirs(dir[f].dbuf, n);
				if(n == 0)
					goto Again;
			}
			dir[f].n = n;
		}else
			dir[f].n = 0;
		dir[f].i = 0;
	}
	if(dir[f].i == dir[f].n)
		return 0;
	strcpy(p, dir[f].dbuf[dir[f].i].name);
	dir[f].i++;
	return 1;
}

int
Opendir(char *name)
{
	Dir *db;
	int f;
	f = open(name, 0);
	if(f==-1)
		return f;
	db = dirfstat(f);
	if(db!=nil && (db->mode&DMDIR)){
		if(f<NFD){
			dir[f].i = 0;
			dir[f].n = 0;
		}
		free(db);
		return f;
	}
	free(db);
	close(f);
	return -1;
}

void
Closedir(int f)
{
	if(f>=0 && f<NFD){
		free(dir[f].dbuf);
		dir[f].i = 0;
		dir[f].n = 0;
		dir[f].dbuf = 0;
	}
	close(f);
}

//******************************************************************************
// Misc
//******************************************************************************

/* claude: from unix.c verbatim -- NDIR=256 (bugfixed from V7 Unix's
 * original 14-char-filename-era value, see unix.c's own comment). */
#define	NDIR	256
int
Globsize(char *p)
{
	int isglob = 0, globlen = NDIR+1;
	for(;*p;p++){
		if(*p==GLOB){
			p++;
			if(*p!=GLOB)
				isglob++;
			globlen+=*p=='*'?NDIR:1;
		}
		else
			globlen++;
	}
	return isglob?globlen:0;
}

void*
Malloc(unsigned long n)
{
	return (void *)malloc(n);
}

/* claude: from unix.c verbatim -- already portable (fprint/exit()). */
void
Exit(char *stat, char *loc)
{
	int n = 0;
	if(flag['s'])
		fprint(STDERR, "Exit from %s: %s\n", loc, stat);

	while(*stat){
		if(*stat!='|'){
			if(*stat<'0' || '9'<*stat)
				exit(1);
			else n = n*10+*stat-'0';
		}
		stat++;
	}
	exit(n);
}

/* claude: routed through os/linux/isatty.c's real ioctl(TCGETS) probe
 * -- a genuinely new libc primitive, not fd2path() (plan9.c's own
 * version) or real POSIX isatty(3) (unix.c calls the host's directly,
 * which happens to share this same name/signature -- see this file's
 * own header comment). */
bool
Isatty(fdt fd)
{
	return isatty(fd);
}

/* claude: from unix.c verbatim -- needed by fmt.c's own pwrd() (word
 * quoting for %q), a plain string-of-special-characters check with
 * nothing POSIX-specific in it. plan9.c doesn't define this either
 * (same gap there, out of scope for this round). */
int
needsrcquote(int c)
{
	if(c <= ' ')
		return 1;
	if(strchr("`^#*[]=|\\?${}()'<>&;", c))
		return 1;
	return 0;
}

/* claude: rfork() moved to its own GOOS-selected files (rc/rfork_fork.c
 * for linux/darwin/plan9, rc/rfork_windows.c for windows) -- see
 * rc/rfork_windows.c's own header comment for why windows genuinely
 * cannot support this the way a thin fork() wrapper does everywhere
 * else (rc/processes.c's own Xasync() needs REAL fork() semantics --
 * duplicate the running interpreter into two processes -- which is not
 * expressible via spawn()'s "start a new program image" contract at
 * all, unlike execsh()/pipecmd()'s windows story). Wired via
 * rc/mkfile's own EXECVARIANTOFILES. */
