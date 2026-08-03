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

/* sbrk(): grow the heap by n bytes and return a pointer to the new
 * bytes. Adapted from ~/principia/lib_core/libc/9sys/sbrk.c, which is
 * the same ten lines.
 *
 * This is the portable half of the pair, and the one the rest of the
 * toolchain actually calls (utilities/text/grep/sub.c,
 * utilities/byte/dd.c, linkers/8lk/compat.c's mysbrk, and eventually
 * pool.c in place of port/minimal_malloc.c). brk() underneath it is the
 * per-OS primitive: a real syscall on plan9 (syscall/os/plan9/
 * svc_$cputype.s, already 0/-1 shaped), and on linux the same syscall
 * behind os/linux/brk.c's return-convention bridge. Not built at all for
 * darwin or windows, which have no break to move and get their own
 * mmap-/VirtualAlloc-backed sbrk instead -- see lib_core/libc/mkfile,
 * which selects between them rather than #ifdef'ing in here.
 *
 * Why sbrk and not brk is the portable primitive, since it looks
 * backwards (brk is the syscall, sbrk the library function): sbrk's
 * contract is "here are n contiguous bytes", and says nothing about the
 * block abutting the previous one. That is exactly what lets it be
 * backed by mmap or VirtualAlloc on the systems with no break at all.
 * brk's contract -- one monotonically-moving boundary in a single
 * contiguous segment -- cannot be honored that way. All three in-tree
 * callers above do treat each hunk independently, so nothing depends on
 * the stronger guarantee.
 *
 * `bloc` is userspace bookkeeping the kernel knows nothing about: brk
 * tracks only the boundary, so remembering where the last handout ended
 * is what lets sbrk return the START of what it just added. It begins at
 * `end`, the linker-provided end-of-bss symbol (declared in
 * include/os/mem.h, defined by every linker here -- e.g.
 * linkers/5l/layout.c's and linkers/8l/pass.c's xdefine("end", SBSS...)).
 */

extern char end[];

static char *bloc = { end };

enum
{
	Round	= 7
};

void*
sbrk(ulong n)
{
	uintptr bl;

	/* ~(uintptr)Round, not principia's plain ~Round: there it relies on
	 * the int -8 sign-extending to all-ones when converted to uintptr
	 * for the &. True here too, but only because uintptr is unsigned --
	 * and uintptr's width on the 64-bit arches was wrong until very
	 * recently (see tests/c/regressions/uintptr_width.c), so this says
	 * what it means instead of leaning on a conversion rule.
	 */
	bl = ((uintptr)bloc + Round) & ~(uintptr)Round;

	/* claude: overflow guard, not in principia's version. Without it an
	 * absurd n (0xFFFFFFFF, say) wraps bl+n around to a LOW address, and
	 * brk() to a low address is a legal shrink that reports success --
	 * so sbrk would hand back a pointer to memory it never obtained.
	 *
	 * This cannot be left to the OS to reject, which was the assumption
	 * worth checking: under qemu-user, brk is emulated by qemu itself
	 * rather than passed to the host kernel, and it accepts requests the
	 * real kernel would refuse -- an _sysbrk((void*)-1) on 386 returned
	 * 0xFFFFFFFF as though it had worked. Guarding here makes sbrk's
	 * contract hold regardless of what is underneath it. Only reachable
	 * on the 32-bit arches, since n is a ulong (4 bytes on every arch
	 * here) and so cannot overflow a 64-bit uintptr.
	 */
	if(bl + n < bl)
		return (void*)-1;
	if(brk((void*)(bl+n)) < 0)
		return (void*)-1;
	bloc = (char*)bl + n;
	return (void*)bl;
}
