// uintptr_width: not diff-derived -- a goken-original bug in the base
// typedefs themselves (include/arch/$cputype/u.h), found while starting
// port/sbrk.c, whose break arithmetic goes through uintptr.
//
// amd64's and arm64's u.h both had `typedef unsigned long uintptr`,
// which reads as obviously right and is obviously wrong here: every
// Plan9 C compiler in this tree defines `long` as 4 bytes (SZ_LONG in
// compilers/*c/gc.h) regardless of arch, including 6c and 7c, while
// pointers on those two are 8 (SZ_IND). So uintptr was 4 bytes on a
// 64-bit arch and silently truncated the top 32 bits of any pointer
// cast through it. riscv64's u.h had already hit this and used
// `unsigned long long`; its comment asserted the other two were fine
// "because those two *do* have an 8-byte long", which is what kept
// anyone from checking them.
//
// Why nothing caught it: the two places that actually round-trip a
// pointer through uintptr (lib_core/libc/port/getcallerpc.c's return
// value, lib_core/libc/fmt/fmtfd.c's (void*)(uintptr)fd) both survive
// truncation in a statically linked binary, whose text and data
// addresses all fit in 32 bits anyway. The stack does not -- see below.
//
// Verified to actually catch it, by putting the old typedef back and
// re-running: amd64 exits 1 (check 1) where arm64 and riscv64 still
// pass. Worth knowing that 6c ALSO reports it directly on check 2 --
// "warning: conversion of pointer to shorter integer" -- so this was
// diagnosable all along; it predates warnings being visible by default
// (see warn_default_visible.check in this directory's mkfile).

#include <u.h>

extern void exit(int);

void
main(void)
{
	char local;
	void *p;
	uintptr u;

	// 1: the invariant itself. Deterministic (compile-time), and the
	// direct statement of what was broken.
	if(sizeof(uintptr) != sizeof(void*))
		exit(1);
	if(sizeof(intptr) != sizeof(void*))
		exit(2);

	// 2: the runtime consequence. &local, deliberately, NOT a static or
	// a string literal: with these linkers' load addresses those all fit
	// in 32 bits and round-trip fine even through a broken 4-byte
	// uintptr, so they would assert nothing. The stack is the one region
	// that reliably sits above 4GB on 64-bit Linux, which is what makes
	// this bite. Kept alongside check 1 rather than replacing it because
	// the two fail independently: this one is what a real caller hits,
	// but it depends on where the kernel happens to put the stack.
	p = &local;
	u = (uintptr)p;
	if((void*)u != p)
		exit(3);

	// 3: small values must survive too, in both directions -- this is
	// fmt/fmtfd.c's own idiom (an fd stashed as (void*)(uintptr)fd and
	// read back). A uintptr that was too WIDE, or wrongly signed, would
	// pass check 1 and fail here.
	if((int)(uintptr)(void*)(uintptr)7 != 7)
		exit(4);

	exit(0);
}
