/* Claude Code
 *
 * Copyright (C) 2026 Yoann Padioleau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

/* rfork_windows.c -- rfork() for windows: a real, documented -1
 * always, NOT a spawn()-based imitation.
 *
 * rc/processes.c's own Xasync() (the `cmd &` background-job builtin)
 * uses rfork(RFFDG|RFPROC|RFNOTEG) to duplicate the RUNNING
 * INTERPRETER into two processes that both keep executing the SAME rc
 * script from that point on -- the parent continues past the `&`,
 * the child keeps running whatever code came after it in the SAME
 * function-call sense real fork() gives, not a new program image.
 * That is fundamentally different from execsh()/pipecmd()'s own
 * windows story (exec_windows.c): those start a brand NEW program,
 * which spawn()'s CreateProcessA-based contract expresses just fine.
 * Duplicating an ALREADY-RUNNING process's memory and continuing it
 * in two places at once has no CreateProcess equivalent at all --
 * this is exactly the "fundamentally different, much larger
 * engineering project" os/windows/exec.c's own header comment already
 * flagged fork() itself as being out of scope for (Cygwin's own real
 * fork() emulation works by forcing every DLL to load at identical
 * fixed addresses and manually copying the whole address space via
 * ReadProcessMemory/WriteProcessMemory -- not attempted here).
 *
 * Returning -1 unconditionally is not a crash-prone stub: it is
 * EXACTLY the failure rc/processes.c's own Xasync() already handles
 * (`case -1: close(null); Xerror("try again"); break;`), the same
 * code path a real Unix rfork() failure (fork bomb, out of memory,
 * ...) already takes there. So `cmd &` fails cleanly with a real rc
 * error message on windows instead of silently doing the wrong thing
 * -- a real, accepted, documented gap (this GOOS's rc has no working
 * background-job support), not an oversight.
 */
#include	"rc.h"
#include	"fns.h"
#include	"io.h"
#include	"exec.h"
#include	"getflags.h"

int
rfork(int flags)
{
	USED(flags);
	return -1;
}
