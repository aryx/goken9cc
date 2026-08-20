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
TEXT sigentry(SB), $0
	SUBQ	$16, R30
	MOVQ	R26, 0(R30)
	MOVQ	R16, R0
	JSR	,signotify(SB)
	MOVQ	0(R30), R26
	ADDQ	$16, R30
	RET
