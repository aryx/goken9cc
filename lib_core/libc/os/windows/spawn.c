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

#include "startupinfo.h"

/* spawn() for windows -- see include/os/proc.h's own declaration and
 * docs/claude_notes/notes_libc_api_design.txt's "spawn(): a portable
 * process-spawn primitive" section for the full design story. Unlike
 * port/spawn.c (linux/darwin/plan9's shared fork()+dup()+exec()
 * version), there is no fork() to build this on here at all -- Win32's
 * CreateProcessA is the atomic equivalent of fork()+dup()+exec() in
 * one call, redirecting the child's stdio via STARTUPINFOA.hStdInput/
 * hStdOutput/hStdError instead of dup()ing onto fds 0/1/2.
 *
 * fdin/fdout/fderr follow the same truncated-HANDLE-as-fdt convention
 * os/windows/open.c's own fdhandle() already established (see that
 * file's header comment): -1 means "leave mine as-is" (this process's
 * own corresponding std handle, via _wingetstdhandle), 0/1/2 mean this
 * process's own std handles explicitly, anything else is a raw
 * pointer-width cast back to the real HANDLE a caller's own pipe()/
 * open() call produced.
 *
 * CreatePipe's own handles are created NON-inheritable by default
 * (winio_amd64.s's _wincreatepipe comment) -- bInheritHandles=TRUE on
 * CreateProcessA (hardcoded in _wincreateprocess) only propagates
 * handles that are ALREADY marked inheritable, so any real (non-std)
 * fd passed in here needs SetHandleInformation first or the child
 * would get an unusable handle value it can't actually use.
 *
 * Does not wait -- CreateProcessA is already non-blocking, so unlike
 * exec()'s own design (which has to wait internally to keep its "never
 * returns on success" contract honest) spawn() can return the moment
 * the child exists, exactly like the fork()-based version. The new
 * child's handle is registered with os/windows/wait.c's own table so a
 * later wait()/waitpid() call can reap it -- Win32 has no "wait for
 * any child" primitive of its own, see that file's header comment.
 */

#define STD_INPUT_HANDLE	(-10)
#define STD_OUTPUT_HANDLE	(-11)
#define STD_ERROR_HANDLE	(-12)
#define HANDLE_FLAG_INHERIT	1

extern void *_wingetstdhandle(int std);
extern int _winsethandleinfo(void *handle, ulong mask, ulong flags);
extern int _wincreateprocess(char *app, char *cmdline, void *startupinfo, void *procinfo);
extern int _winclose(void *handle);
extern int buildcmdline(char *buf, long bufsize, char *argv[]);
extern void addwaitproc(void *handle, int pid);

static void*
stdfdhandle(fdt fd, int std)
{
	switch(fd){
	case -1:
		return _wingetstdhandle(std);
	case 0:
		return _wingetstdhandle(STD_INPUT_HANDLE);
	case 1:
		return _wingetstdhandle(STD_OUTPUT_HANDLE);
	case 2:
		return _wingetstdhandle(STD_ERROR_HANDLE);
	default:
		_winsethandleinfo((void*)(vlong)fd, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		return (void*)(vlong)fd;
	}
}

int
spawn(char *path, char **argv, fdt fdin, fdt fdout, fdt fderr)
{
	char cmdline[4096];
	byte startupinfo[STARTUPINFOA_SIZE];
	byte procinfo[PROCESS_INFORMATION_SIZE];
	void *hprocess;
	int pid;

	if(buildcmdline(cmdline, sizeof cmdline, argv) < 0)
		return -1;

	memset(startupinfo, 0, sizeof startupinfo);
	*(long*)startupinfo = STARTUPINFOA_CB;
	*(long*)(startupinfo+STARTUPINFOA_DWFLAGS_OFF) = STARTF_USESTDHANDLES;
	*(void**)(startupinfo+STARTUPINFOA_HSTDINPUT_OFF) = stdfdhandle(fdin, STD_INPUT_HANDLE);
	*(void**)(startupinfo+STARTUPINFOA_HSTDOUTPUT_OFF) = stdfdhandle(fdout, STD_OUTPUT_HANDLE);
	*(void**)(startupinfo+STARTUPINFOA_HSTDERROR_OFF) = stdfdhandle(fderr, STD_ERROR_HANDLE);
	memset(procinfo, 0, sizeof procinfo);

	if(!_wincreateprocess(path, cmdline, startupinfo, procinfo))
		return -1;

	hprocess = *(void**)(procinfo+PROCESS_INFORMATION_HPROCESS_OFF);
	/* hThread (offset 8): this process has no use for it, close it
	 * immediately -- unlike hProcess, nothing here ever waits on or
	 * queries the child's main thread. */
	_winclose(*(void**)(procinfo+8));

	pid = (int)(vlong)hprocess;
	addwaitproc(hprocess, pid);
	return pid;
}
