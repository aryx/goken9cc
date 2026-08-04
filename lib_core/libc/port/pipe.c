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

/* pipe() (include/os/ipc.h) -- Plan9's pipe(int fd[2]) and POSIX's
 * pipe(int fd[2]) agree exactly (unlike dup()'s two-in-one shape), so
 * this is a thin -errno-to--1 normalization over _syspipe(), the same
 * reasoning as port/fork.c's own comment -- not a shape bridge.
 *
 * _syspipe() itself is per-arch (syscall/os/linux/): a direct SYS_pipe
 * wrapper on 386/amd64/arm, and a pipe2(fd,0)-based shim on arm64/
 * riscv64/mips, whose raw pipe syscalls are either absent (arm64/
 * riscv64's "generic" ABI dropped it) or unusable through the generic
 * trampoline (mips's sysm_pipe returns fds via $v0/$v1, not the
 * pointer argument -- see numbers_mips.h's own comment).
 */

extern int _syspipe(int *fd);

int
pipe(int *fd)
{
	if(_syspipe(fd) < 0)
		return -1;
	return 0;
}
