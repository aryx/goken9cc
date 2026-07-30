// getcallerpc(v): returns the return address of the function that
// called getcallerpc's own caller -- v must be the address of that
// caller's first argument (e.g. "getcallerpc(&firstarg)"), which this
// arch's calling convention places immediately above the caller's own
// return address on the stack (confirmed by port/strdup.c's only call
// site, "getcallerpc(&s)", s being strdup's own first parameter -- 386
// has no register-passed arguments at all, see arch/386/rt0.s's
// identical comment, so &s reliably points at a real stack slot).
// Adapted verbatim from ~/principia-softwarica/lib_core/libc/386/
// getcallerpc.s (a real, working reference for this exact arch).
//
// Lives under arch/$cputype/, not syscall/os/$OS/: this is purely a
// calling-convention fact (how this project's own 8c lays out a
// function's stack-passed arguments relative to its return address),
// not an OS syscall-ABI one, so it should be GOOS-independent the same
// way e.g. arm64's register-vs-stack argument facts already proven
// OS-independent elsewhere in this tree are. Not actually verified
// across GOOS values here, though (only ever built/run under
// GOOS=linux so far) -- arch/$cputype/rt0.s once made the identical
// "surely OS-independent" assumption and turned out wrong for
// darwin/amd64 specifically (see docs/claude_notes/
// notes_libc_selfhost.txt), so treat this the same way: plausible, not
// proven, until it's actually exercised under GOOS=darwin. If it does
// turn out to differ, this needs the same per-GOOS override mechanism
// lib_core/libc/mkfile's RT0OFILE already provides for rt0.s, not a
// silent overwrite of this file.

TEXT getcallerpc+0(SB), $0
	MOVL	v+0(FP), AX
	MOVL	-4(AX), AX
	RET
