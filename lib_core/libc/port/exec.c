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

/* exec() (include/os/proc.h) -- Plan9's exec(char*, char*[]) has no
 * envp argument (there is no environ concept in the Plan9 API at all,
 * see port/getenv.c's own header comment), so this bridges it onto the
 * raw 3-arg POSIX execve() by handing it whatever port/getenv.c's
 * environ() currently reports.
 *
 * claude: environ(), NOT a local re-walk of port/mainargs.c's
 * _mainargv, which is what this file used to do. Two separate bugs in
 * one four-line helper. First, it read the kernel-provided block
 * directly and so was blind to port/putenv.c, which cannot grow that
 * block in place and instead switches getenv.c's _environp to a fresh
 * array -- meaning every putenv() a process made was silently dropped
 * the moment it exec()ed, exactly the "set a variable, then run a
 * child that reads it" pattern putenv() exists for. Second, it found
 * envp by scanning _mainargv for its terminating nil, the very trick
 * _mainargc was introduced to replace (see port/mainargs.c's own
 * comment: any getopt-style parser that compacts argv in place moves
 * that nil earlier and the scan then reads shifted argv strings as
 * environment entries). getenv.c's environ() handles both correctly
 * and has been the public API in include/os/env.h since it was added,
 * so there was never a "it's file-static, not worth exporting"
 * tradeoff to make -- the old comment here claiming otherwise was
 * simply out of date.
 *
 * Found self-hosting `mk bootstrap`'s stage 2 on linux/arm64: a
 * goken-built mk ran every recipe with $stem/$target/$prereq unset,
 * because mk/goken.c's exportenv() putenv()s them in the forked child
 * and execsh() then exec()s the shell. Symptom was one "rc: null list
 * in concatenation" per compile job (mkfiles/mkone's own `$stem.c`).
 *
 * execve() only ever returns on failure, so there is exactly one
 * `return` here (unlike port/dup.c's two-branch shape) -- but the raw
 * _sysexecve() still needs the same -errno-to-Plan9's-exact--1
 * normalization port/fork.c's own comment explains, since a future
 * caller may come to depend on the exact contract even though
 * compilers/pcc/pcc.c's own doexec() currently just treats any return
 * from exec() at all as failure, not `== -1` specifically.
 */

extern int _sysexecve(void *path, void *argv, void *envp);

int
exec(char *prog, char *argv[])
{
	_sysexecve(prog, argv, environ());
	return -1;
}
