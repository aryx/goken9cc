// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/amd64/sigrestore.s's own header comment for the general story.
// mips needs a real bridge: the kernel invokes a signal handler with
// sig in the real o32 ABI's first-argument register, $a0 (this
// compiler's own R4 -- a direct hardware-register-number
// correspondence, same as R29=$sp/R31=$ra elsewhere in this tree),
// but THIS compiler's own convention for a plain C function's first
// argument uses R1 instead (confirmed against arch/mips/rt0.s's own
// `MOVW 4(R29), R1 // argc` before `JAL main` -- argc reaches main()
// via R1, not R4, the same register _syscall6's own raw-stub
// convention already uses for its `num` argument). So: move sig from
// R4 to R1, then call signotify normally.
//
// No manual R31 (return address) save/restore needed around the JAL:
// `va` auto-reserves and manages a save slot for R31 on any non-leaf
// TEXT block (arch/mips/rt0.s's own comment on this same fact) -- so
// the final RET below correctly returns to whatever the KERNEL set R31
// to before jumping here (empirically confirmed: no explicit
// SA_RESTORER is needed on this arch at all -- omitting it and letting
// the kernel supply its own default worked in testing, and mips's own
// struct has no restorer field to set one with regardless, see
// numbers_mips.h's own comment on SA_RESTORER being removed from this
// arch's kernel entirely).
TEXT sigentry(SB), $0
	MOVW	R4, R1
	JAL	signotify(SB)
	RET
