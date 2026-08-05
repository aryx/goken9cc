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

/* wait() for windows -- the counterpart os/windows/spawn.c needs
 * (docs/claude_notes/notes_libc_api_design.txt's "spawn(): a portable
 * process-spawn primitive" section). Unlike POSIX's wait4(-1,...) or
 * Plan9's own kernel-tracked "any child" wait, Win32 has NO "wait for
 * any child" primitive at all -- WaitForSingleObject takes exactly one
 * handle, and WaitForMultipleObjects takes an explicit list you have to
 * already know, capped at MAXIMUM_WAIT_OBJECTS=64.
 *
 * So this keeps a small table of in-flight child HANDLEs (populated by
 * spawn()'s own addwaitproc() below every time it successfully starts
 * one, drained here as each is reaped) and passes the CURRENT live set
 * to WaitForMultipleObjects every call -- the same "keep a real table
 * instead of assuming a truncated int is enough" idea todo.org's own
 * windows dup() gap already flagged as the eventual right fix for
 * fd-as-HANDLE truncation, just scoped to processes instead of
 * general fds.
 *
 * A fixed 64-entry table, not a growable one: MAXIMUM_WAIT_OBJECTS is
 * a hard Win32 ceiling on WaitForMultipleObjects's own handle-count
 * argument, so a 65th concurrently in-flight child could never be
 * waited on this way regardless of how big this table was -- sizing it
 * any larger would just move where the real limit bites.
 */

#define MAXPROC			64
#define WAIT_FAILED		0xffffffffUL
#define WAIT_TIMEOUT		258UL
#define INFINITE_MS		0xffffffffUL

extern ulong _winwaitmultiple(ulong count, void *handles, ulong waitall, ulong ms);
extern int _wingetexitcode(void *handle, void *code);
extern int _winclose(void *handle);

static void *proctab[MAXPROC];
static int   pidtab[MAXPROC];
static int   nproc;

/* claude: called by os/windows/spawn.c right after a successful
 * CreateProcessA -- silently drops the registration if the table is
 * already full (MAXPROC in-flight children at once), matching this
 * file's own comment on why growing the table wouldn't help past 64
 * anyway; a caller that hits this finds out the honest way, via
 * wait() never reaping that particular child.
 */
void
addwaitproc(void *handle, int pid)
{
	if(nproc >= MAXPROC)
		return;
	proctab[nproc] = handle;
	pidtab[nproc] = pid;
	nproc++;
}

Waitmsg*
wait(void)
{
	Waitmsg *w;
	void *handles[MAXPROC];
	ulong code;
	ulong r;
	int i, idx;

	if(nproc == 0)
		return nil;

	for(i = 0; i < nproc; i++)
		handles[i] = proctab[i];
	r = _winwaitmultiple(nproc, handles, 0, INFINITE_MS);
	if(r == WAIT_FAILED || r == WAIT_TIMEOUT)
		return nil;
	idx = (int)r; /* WAIT_OBJECT_0 is 0, so a successful index needs no offsetting */
	if(idx < 0 || idx >= nproc)
		return nil;

	code = 0;
	_wingetexitcode(proctab[idx], &code);

	w = mallocz(sizeof(Waitmsg) + 32, 1);
	if(w == nil)
		return nil;
	w->msg = (char*)&w[1];
	w->pid = pidtab[idx];
	if(code != 0)
		sprint(w->msg, "%lud", code);

	_winclose(proctab[idx]);
	/* remove idx from the table by swapping in the last live entry */
	nproc--;
	proctab[idx] = proctab[nproc];
	pidtab[idx] = pidtab[nproc];

	return w;
}
