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

/* exec()/execl() (include/os/proc.h) -- Windows has no execve(2) and
 * no fork(2) at all, so this is not a raw-syscall bridge like every
 * other GOOS's port/exec.c, it is a deliberately different design:
 * CreateProcessA (winio_amd64.s's _wincreateprocess) + a blocking wait
 * (_winwaitprocess) + reading the child's real exit code
 * (_wingetexitcode) + this process's own exit(). That is the only way
 * to keep exec()'s contract -- "never returns on success" -- honest
 * without a real fork(): CreateProcessA SPAWNS a new, separate process
 * rather than replacing this one, so the only way this process can
 * behave as if it "became" that command is to wait for it and adopt
 * its exit code as its own, exactly mirroring what MSVCRT's own real
 * _execve() does for the same reason.
 *
 * fork() itself stays UNIMPLEMENTED on this GOOS -- not approximated,
 * not stubbed to "succeed" and do the wrong thing. There is no
 * address-space-duplication primitive on Windows at all; Cygwin's own
 * real fork() works by forcing every DLL in the whole system to load
 * at identical fixed addresses and manually copying the entire address
 * space into a suspended child via ReadProcessMemory/WriteProcessMemory
 * -- a fundamentally different, much larger engineering project than a
 * libc syscall wrapper, out of scope here. compilers/pcc/pcc.c's own
 * `switch(fork()){case 0: exec(...)}` pattern therefore still cannot
 * work end to end on Windows with this round's changes -- see
 * todo.org's own gap entry.
 *
 * wait() ALSO stays unimplemented here, for a related but different
 * reason: it is not merely unported, it has nothing left to do. On
 * every other GOOS, wait() is called by the PARENT after a SEPARATE
 * fork()'d child has run exec() on its own -- two different call
 * sites in two different processes. Windows collapses fork+exec+wait
 * into this ONE call, in this ONE process: by the time _wincreateprocess
 * returns here, the wait has already happened internally (this
 * function does not return to ITS OWN caller until the child is done,
 * and then only via exit(), never via a normal `return`). There is no
 * second call for a standalone wait() to usefully make.
 *
 * The command-line quoting below is intentionally simple, not a full
 * implementation of Windows' own notoriously intricate argv-quoting
 * rules (backslash-before-quote escaping, etc -- see Microsoft's own
 * "Parsing C++ Command-Line Arguments" documentation): each argv[i]
 * containing a space is wrapped in a plain pair of double quotes,
 * every other argument is copied through unchanged. Correct for the
 * common case (paths, flags with no embedded quote characters, which
 * is everything compilers/pcc/pcc.c's own argv construction ever
 * builds), not a general-purpose shell-quoting engine.
 */

extern int _wincreateprocess(char *app, char *cmdline, void *startupinfo, void *procinfo);
extern int _winwaitprocess(void *handle);
extern int _wingetexitcode(void *handle, void *code);

/* claude: STARTUPINFOA is 104 bytes on x64 -- hand-derived field by
 * field (cb DWORD=4, then three 8-byte pointers, seven DWORDs, two
 * WORDs, an 8-byte pointer, and three more 8-byte HANDLEs, with the
 * padding each 8-byte pointer forces before it), not copied from a
 * real <windows.h> (this project has none). Only `cb` (offset 0) is
 * ever written -- every other field just needs to be zero, which is
 * what CreateProcessA expects when STARTF_USESTDHANDLES and friends
 * are not set in dwFlags. Sized generously above the derived 104
 * (not exactly 104) as a margin against that hand-derivation being
 * slightly wrong; a too-large zeroed buffer is harmless, a too-small
 * one is a real stack overflow, so the asymmetry is worth the few
 * wasted bytes. PROCESS_INFORMATION is 24 bytes (HANDLE hProcess at
 * offset 0, HANDLE hThread at offset 8, then two DWORDs) -- hProcess
 * at offset 0 is the only field this file reads. Neither size, nor
 * the offset-0 placement of cb/hProcess, has been checked against a
 * real Windows SDK header -- unverified, like the rest of os/windows/
 * added without a Windows host to run it on (see winio_amd64.s's own
 * "NOT verified on a real Windows host" precedent).
 */
#define STARTUPINFOA_SIZE 128
#define PROCESS_INFORMATION_SIZE 32

static int
buildcmdline(char *buf, long bufsize, char *argv[])
{
	int i;
	long n;
	char *s;

	n = 0;
	for(i = 0; argv[i] != nil; i++){
		int quote;

		if(i > 0 && n < bufsize-1)
			buf[n++] = ' ';
		quote = 0;
		for(s = argv[i]; *s != '\0'; s++)
			if(*s == ' ')
				quote = 1;
		if(quote && n < bufsize-1)
			buf[n++] = '"';
		for(s = argv[i]; *s != '\0' && n < bufsize-1; s++)
			buf[n++] = *s;
		if(quote && n < bufsize-1)
			buf[n++] = '"';
	}
	if(n >= bufsize)
		return -1;
	buf[n] = '\0';
	return 0;
}

int
exec(char *prog, char *argv[])
{
	char cmdline[4096];
	byte startupinfo[STARTUPINFOA_SIZE];
	byte procinfo[PROCESS_INFORMATION_SIZE];
	void *hprocess;
	long code;

	if(buildcmdline(cmdline, sizeof cmdline, argv) < 0)
		return -1;

	memset(startupinfo, 0, sizeof startupinfo);
	*(long*)startupinfo = 104; /* cb: real sizeof(STARTUPINFOA), see comment above */
	memset(procinfo, 0, sizeof procinfo);

	if(!_wincreateprocess(prog, cmdline, startupinfo, procinfo))
		return -1;

	hprocess = *(void**)procinfo; /* PROCESS_INFORMATION.hProcess, offset 0 */
	_winwaitprocess(hprocess);
	code = 0;
	_wingetexitcode(hprocess, &code);
	exit((int)code);
	return -1; /* unreachable */
}
