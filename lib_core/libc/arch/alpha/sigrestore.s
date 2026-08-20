// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/riscv64/sigrestore.s's own header comment for the general
// story: the kernel invokes a signal handler with sig in this arch's
// native first-argument register (a0/R16, matching svc_alpha.s's own
// syscall convention), but this compiler wants its first argument in
// R0 (REGARG) -- so this trampoline bridges the two before calling
// signotify(), saving/restoring R26 (REGLINK) manually around the JSR
// since a real Alpha JSR overwrites it and this file gets no compiler
// prologue to do that automatically.
//
// No SA_RESTORER field/flag exists on this arch at all (see
// numbers_alpha.h's own comment -- confirmed absent from the real
// uapi/asm/signal.h installed on this host, same as mips), so unlike
// x86/arm this file provides no separate rt_sigreturn trampoline
// either -- the kernel supplies its own default return mechanism.
//
// claude: "1, $-8" (not plain "$0") -- linkers/zl/noop.c's own ATEXT
// case auto-inserts an 8-byte SUBQ+RA-save prologue for any TEXT
// function containing a JSR (its own LEAF auto-detection only tracks
// JSRs, it does not know this is a signal handler), which would
// silently shift every raw R30 offset below by 8 the same way it did
// for arch/alpha/rt0.s's own _main (see that file's own comment for
// the full story -- found the exact same way, via qemu-alpha -d
// in_asm on notify.exe/alarm.exe, both of which segfaulted before this
// fix). Unlike rt0.s, this file cannot just shift its own offsets to
// compensate: R26 at kernel-entry is a real return address (the
// kernel's own default sigreturn trampoline, since there is no
// SA_RESTORER -- see the comment above), and the auto-prologue's own
// epilogue (rewritten RET) would try to restore RA from ITS OWN saved
// copy, stepping on this file's own manual R26 save/restore around the
// JSR to signotify() and returning through garbage instead of back to
// the kernel. "1, $-8" is the exact same escape hatch
// arch/alpha/div.s's _divq (etc, all of which also JSR to helper
// symbols) already uses, ported unchanged from the real Plan 9 4th
// edition source: a $-8 frame makes noop.c's autosize computation
// (p->to.offset + 8) come out to exactly 0, which forces this
// function's LEAF mark on regardless of its own JSRs and skips the
// auto-prologue/epilogue entirely -- leaving R30/R26 exactly as this
// file's own instructions expect.
TEXT sigentry(SB), 1, $-8
	SUBQ	$16, R30
	MOVQ	R26, 0(R30)
	MOVQ	R16, R0
	JSR	,signotify(SB)
	MOVQ	0(R30), R26
	ADDQ	$16, R30
	RET
