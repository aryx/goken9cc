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

/* Plan9's include/os/file.h calls this seek(), not lseek() -- purely a
 * naming difference, not a behavioral one: SEEK__START/SEEK__CUR/
 * SEEK__END (0/1/2) already match POSIX's SEEK_SET/SEEK_CUR/SEEK_END
 * numerically on every OS this project targets, so this is a genuinely
 * portable, OS/arch-independent renaming shim -- unlike open()'s
 * translation (os/$GOOS/open.c), which needs real per-OS flag-bit
 * knowledge and so lives under os/ instead.
 *
 * The cast up to vlong recovers no precision lost inside lseek() itself:
 * the raw syscall wrapper (lib_core/libc/syscall/os/$OS/zsyscall_$OS_
 * $cputype.c) returns this project's 4-byte `long` even on 64-bit archs
 * (see that file's own comment on _syscall6's return type), so a seek
 * position past 4GiB already truncated before it got here. Same
 * accepted, already-documented limitation as write()'s return value --
 * not fixed in this pass.
 */
extern long lseek(int fd, vlong offset, int whence);

vlong
seek(fdt fd, vlong offset, int whence)
{
	return (vlong)lseek(fd, offset, whence);
}
