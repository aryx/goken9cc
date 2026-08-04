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

/* postnote() (include/os/plan9/note.h) for darwin -- Tier 6
 * notification (docs/claude_notes/plan_syscalls.txt), postnote() ONLY.
 *
 * notify()/noted() (installing a real signal HANDLER, i.e. sigaction
 * itself) are deliberately NOT implemented here, unlike os/linux/
 * notify.c's equivalent. XNU's raw sigaction(2) needs a real userspace
 * sa_tramp trampoline (struct __sigaction: a handler union, an
 * sa_tramp function pointer XNU calls with 5 arguments, sa_mask,
 * sa_flags -- not the plain handler-shaped struct every other GOOS in
 * this tree gets away with), and public XNU source leaves genuine
 * ambiguity about whether that trampoline is still mandatory on
 * modern XNU (a newer SA_USERTRAMP flag suggests the kernel can
 * supply its own instead) or version-dependent. Unlike every other
 * gap in this tree, this is not "unverified without hardware" (every
 * other darwin round in this project ships that way) -- it's "the
 * design itself isn't confidently resolved from the public sources
 * available here", and there is no qemu-user emulation for Darwin/XNU
 * to fall back on the way every Linux arch had for this same tier.
 * Left as a real, documented gap (port/atnotify.c still compiles
 * clean for darwin, unresolved-until-linked, same treatment
 * lib_core/libc/mkfile's own NOTIFYOFILES comment already gives it)
 * rather than shipped as unverifiable guessed trampoline assembly.
 *
 * postnote() itself needs none of that -- it is just kill(2), and
 * XNU's kill(2) is confidently sourced (numbers_amd64.h's own
 * comment): a real 3-argument BSD syscall, `posix` always 1 (matching
 * this project's own vendored 2010 Go snapshot's real runtime
 * behavior). The note<->signal table is the same curated 6-signal set
 * os/linux/notify.c uses, with the same numbers (confirmed identical
 * across Linux/darwin for this set via that same Go snapshot's
 * zerrors_darwin_amd64.go).
 */

extern int _syskill(int pid, int sig, int posix);

#define SIGHUP	1
#define SIGINT	2
#define SIGQUIT	3
#define SIGALRM	14
#define SIGTERM	15
#define SIGPIPE	13

static struct {
	int	sig;
	char	*str;
} sigtab[] = {
	SIGHUP,		"hangup",
	SIGINT,		"interrupt",
	SIGQUIT,	"quit",
	SIGALRM,	"alarm",
	SIGTERM,	"kill",
	SIGPIPE,	"sys: write on closed pipe",
};

static int
str2sig(char *s)
{
	int i;

	for(i = 0; i < nelem(sigtab); i++)
		if(strcmp(s, sigtab[i].str) == 0)
			return sigtab[i].sig;
	return 0;
}

int
postnote(int group, int pid, char *note)
{
	int sig;

	sig = str2sig(note);
	if(sig == 0)
		return -1;
	switch(group){
	case PNPROC:
		return _syskill(pid, sig, 1) < 0 ? -1 : 0;
	case PNGROUP:
		return _syskill(-pid, sig, 1) < 0 ? -1 : 0;
	}
	return -1;
}
