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

/* fork() for windows -- a real, documented -1 always, not a stub that
 * happens to be unreachable. There is no fork() on Win32 at all (see
 * os/windows/exec.c's own header comment on why a real emulation --
 * Cygwin's whole-address-space ReadProcessMemory/WriteProcessMemory
 * trick -- is out of scope here), so this exists purely so callers
 * that reference fork() directly (rc/processes.c's own Xpipe(), for
 * `cmd1 | cmd2` pipelines within a running rc script -- duplicating
 * the INTERPRETER itself, not starting a new program, so spawn()
 * cannot stand in for it the way it does for execsh()/pipecmd(); see
 * rc/rfork_windows.c's own header comment for the identical story
 * with rfork()) get a real link-time symbol and a clean runtime
 * failure instead of an "undefined: fork" link error.
 *
 * Safe to add at the libc level rather than caller-local: nothing in
 * this tree's own windows/ files calls fork() internally (port/spawn.c,
 * the only fork()-based caller in the portable layer, is not built for
 * windows -- see os/windows/spawn.c instead), so this cannot change
 * any existing behavior, only provide one where "undefined" used to be.
 */
int
fork(void)
{
	return -1;
}
