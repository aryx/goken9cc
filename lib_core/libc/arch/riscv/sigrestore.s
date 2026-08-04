// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/amd64/sigrestore.s's own header comment for the general story.
// riscv needs a real bridge: the kernel invokes a signal handler with
// sig in the real ABI's first-argument register, a0 (this compiler's
// own R10 -- confirmed against syscall/os/linux/svc_riscv.s's own
// `a1+4(FP), R10 // a0` comment), but THIS compiler's own convention
// for a plain C function's first argument uses R8 instead (confirmed
// against arch/riscv/rt0.s's own `MOVW 8(R2), R8 // argc` before
// `JAL R1, main(SB)` -- argc reaches main() via R8, the same register
// _syscall6's own raw-stub convention already uses for its `num`
// argument). So: move sig from R10 to R8, then call signotify.
//
// UNLIKE mips/arm/arm64, this arch's assembler does NOT auto-save the
// link register (R1) across a JAL in a non-leaf block (established
// fact, docs/claude_notes -- Tier 7's own setjmp work already found
// this the hard way for riscv) -- so R1 (which the kernel set to
// sa_restorer/its own return point before jumping here) has to be
// saved and restored explicitly around the JAL below, or it would be
// clobbered by JAL's own link-register write.
//
// No SA_RESTORER needed (empirically confirmed: omitting it and
// letting the kernel supply its own default worked in testing, the
// same story arm64 already has -- numbers_riscv.h's own SA_RESTORER_VAL
// is unused by this file).
TEXT sigentry(SB), $0
	SUB	$8, R2
	MOVW	R1, 0(R2)
	MOVW	R10, R8
	JAL	R1, signotify(SB)
	MOVW	0(R2), R1
	ADD	$8, R2
	RET
