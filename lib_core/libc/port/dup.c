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

/* Plan9's dup(oldfd, newfd) (include/os/file.h) is one call covering
 * what POSIX splits in two: newfd == -1 means "give me the lowest free
 * descriptor", which is POSIX dup(2), and any other newfd means "make
 * exactly this descriptor a copy", which is POSIX dup2(2). Confirmed
 * against principia's kernel/files/sysfile.c sysdup(), whose
 * `if(fd != -1)` branch is precisely this split.
 *
 * Like port/remove.c this is portable given the two raw primitives --
 * no per-OS constants, nothing to branch on -- so which GOOSes it
 * applies to is decided in lib_core/libc/mkfile's PORTPOSIXOFILES
 * rather than by an #ifdef here. syscall/os/$GOOS/ supplies
 * _sysdup()/_sysdup2() per (OS, arch): two generated wrappers on the
 * legacy-numbered archs, and dup plus a dup3(old,new,0) shim on
 * arm64/riscv/riscv64, whose ABI dropped dup2.
 *
 * Note this does NOT reject newfd < -1. Plan9's own kernel treats any
 * negative value other than -1 as an error (growfd() rejects fd < 0),
 * and so does POSIX dup2, so passing it straight through gives the
 * same answer without this file needing to know which errno either
 * one picks.
 */

extern int _sysdup(int fd);
extern int _sysdup2(int oldfd, int newfd);

int
dup(int oldfd, int newfd)
{
	if (newfd == -1)
		return _sysdup(oldfd);
	return _sysdup2(oldfd, newfd);
}
