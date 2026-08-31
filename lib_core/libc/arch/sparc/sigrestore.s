// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/mips/sigrestore.s's own header comment for the general story
// this file follows: the kernel invokes a signal handler with sig in
// the real hardware ABI's first-argument register (%o0, R8 in this
// port's flat numbering), but kc's own convention for a plain C
// function's first argument is R7 (REGARG) -- so move sig from R8 to
// R7, then call signotify normally.
//
// Unlike rt0.s's _main, this TEXT does NOT need any leaf trick: by the
// time a signal is delivered, R1/REGSP already holds a valid stack
// pointer (the process is fully running), so kl's own auto-inserted
// prologue (SUB $8,R1 / MOVW R15,0(R1)) is safe here, and the trailing
// RETURN correctly bounces back through whatever the kernel set R15
// (%o7) to before jumping here -- same "no manual restorer needed"
// assumption arch/mips/sigrestore.s's own comment makes, carried over
// by analogy rather than independently confirmed: notify.c/signals are
// not exercised by hello.c and remain UNVERIFIED on this arch.
TEXT sigentry(SB), $0
	MOVW	R8, R7
	JMPL	signotify(SB)
	RETURN
