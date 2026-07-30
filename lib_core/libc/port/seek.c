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
 * naming difference on every OTHER OS, not a behavioral one:
 * SEEK__START/SEEK__CUR/SEEK__END (0/1/2) already match POSIX's
 * SEEK_SET/SEEK_CUR/SEEK_END numerically everywhere, so this is a
 * genuinely portable, OS/arch-independent renaming shim there --
 * unlike open()'s translation (os/$GOOS/open.c), which needs real
 * per-OS flag-bit knowledge and so lives under os/ instead.
 *
 * Plan9 itself is the one exception: its own real seek(2) syscall is
 * already named/shaped exactly like the public seek() (see
 * syscall/os/plan9/svc_$cputype.s), so this whole file is `#ifndef
 * plan9`-guarded out for that GOOS -- linking this shim in
 * unconditionally would either fail to resolve `lseek` (which plan9
 * has no such symbol for at all) or silently conflict with the real
 * one. This file is compiled once, for every (GOOS, arch) combination
 * (port/), which is also why the rest of it -- the return-width #ifdef
 * below -- has to match lseek()'s own real prototype exactly on every
 * OTHER os/arch it as compiled for.
 */
#ifndef plan9

/* lseek()'s own return type differs by arch: on the archs with a real
 * 64-bit register to return a `vlong` off_t in (amd64/arm64/riscv64 --
 * riscv64 only on Linux, amd64/arm64 on both Linux and Darwin), its
 * raw syscall wrapper (lib_core/libc/syscall/os/$OS/) routes through
 * _syscall6v and genuinely returns `vlong`; every other (32-bit) arch
 * still returns plain `long` via the default _syscall6 (see
 * scripts/mksyscall.sh's own comment on the two trampolines, and
 * syscall_linux_amd64.h's _syscall6v comment for why a second
 * trampoline was added instead of widening _syscall6 itself).
 *
 * claude: nested #ifdef, not `#if defined(X) || defined(Y)` -- 7c/vc/ic's
 * preprocessor doesn't understand `#if` with an expression at all
 * ("unknown #: if"), only plain #ifdef/#ifndef/#else/#endif (see every
 * other #ifdef in lib_core/libc, e.g. os/linux/open.c's `#ifdef mips`,
 * none of which use `#if`).
 */
#ifdef amd64
extern vlong lseek(int fd, vlong offset, int whence);
#else
#ifdef arm64
extern vlong lseek(int fd, vlong offset, int whence);
#else
#ifdef riscv64
extern vlong lseek(int fd, vlong offset, int whence);
#else
extern long lseek(int fd, vlong offset, int whence);
#endif
#endif
#endif

vlong
seek(fdt fd, vlong offset, int whence)
{
	return (vlong)lseek(fd, offset, whence);
}

#endif /* !plan9 */
