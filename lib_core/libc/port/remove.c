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

/* Plan9's remove() (include/os/dir.h) deletes a file OR AN EMPTY
 * DIRECTORY -- one call, either kind. POSIX splits that in two:
 * unlink(2) refuses a directory (EISDIR on Linux, EPERM on BSD/Darwin)
 * and rmdir(2) refuses everything else. So remove() needs a real bridge
 * on the POSIX-shaped GOOSes, unlike chdir(), which maps 1:1.
 *
 * The bridge itself is genuinely portable given those two primitives:
 * no per-OS constants, no per-arch widths, nothing to branch on. Which
 * GOOSes it applies to is a build question, answered in
 * lib_core/libc/mkfile's PORTPOSIXOFILES, not an #ifdef here -- see
 * that variable's comment for why plan9 and windows are excluded.
 *
 * syscall/os/$GOOS/ supplies _sysunlink()/_sysrmdir() per (OS, arch):
 * a directly generated wrapper per syscall on the legacy-numbered archs,
 * and a shim over unlinkat()/unlinkat(AT_REMOVEDIR) on arm64/riscv/
 * riscv64, whose ABI has no bare unlink() at all.
 *
 * Deliberately an unconditional fallback rather than testing errno for
 * EISDIR: that value differs between Linux and Darwin for this exact
 * case (21 vs 1), and these raw wrappers return the negative errno
 * straight from the kernel with no errno-decoding layer in between --
 * so testing it would put OS knowledge back into this file. Trying
 * rmdir() whenever unlink() failed is both simpler and more portable,
 * and costs only one extra failed syscall on a genuine error (e.g. a
 * nonexistent path, where both fail with ENOENT and the caller sees
 * rmdir's -- the same value unlink would have returned).
 */

extern int _sysunlink(char *path);
extern int _sysrmdir(char *path);

/* claude: normalizes failure to exactly -1 -- found needed by
 * utilities/files/rm.c/mv.c, both of which check `remove(f) ==
 * ERROR_NEG1`/`!= ERROR_NEG1` (ERROR_NEG1 is literally `(-1)`,
 * include/base/error.h), matching genuine Plan9 syscall convention
 * (every real Plan9 syscall fails with exactly -1, never a raw
 * negative errno -- the failure detail is conveyed separately, via
 * errstr()). _sysrmdir() (like every other raw syscall wrapper in
 * this tree -- see os/linux/open.c's own open(), which has the exact
 * same gap, not yet fixed) returns the raw negative Linux errno
 * instead (e.g. -2 for ENOENT, -13 for EACCES), so `remove(f) !=
 * ERROR_NEG1` silently took the "succeeded" branch on any failure
 * whose errno wasn't literally 1 (EPERM) -- confirmed directly: `rm
 * nonexistent-file` exited 0 with no error printed before this fix.
 * A full fix belongs at the raw-syscall-wrapper layer, GOOS/arch-wide
 * (every os/$GOOS/*.c file, not just this one) -- tracked in
 * todo.org rather than attempted here; this is the minimal fix for
 * remove() specifically, the one call this session's utilities
 * actually depend on for an exact -1 check.
 */
int
remove(char *path)
{
	if (_sysunlink(path) >= 0)
		return 0;
	if (_sysrmdir(path) >= 0)
		return 0;
	return -1;
}
