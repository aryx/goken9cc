// sigentry() -- installed as Ksigaction.handler (os/linux/notify.c),
// Tier 6 notification (docs/claude_notes/plan_syscalls.txt). See
// arch/riscv/sigrestore.s's own header comment for the full story
// (identical here -- kernel passes sig in a0/R10, this compiler wants
// it in R8, R1 needs manual save/restore around the JAL since this
// arch's assembler doesn't auto-save it). $16, not rv32's $8, matching
// arch/riscv64/rt0.s's own wider/8-byte-aligned stack slot choice for
// this arch's 8-byte registers.
//
// No SA_RESTORER needed (empirically confirmed: omitting it and
// letting the kernel supply its own default worked in testing --
// numbers_riscv64.h's own SA_RESTORER_VAL is unused by this file).
TEXT sigentry(SB), $0
	SUB	$16, R2
	MOV	R1, 0(R2)
	MOVW	R10, R8
	JAL	R1, signotify(SB)
	MOV	0(R2), R1
	ADD	$16, R2
	RET
