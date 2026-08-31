// same idea as hello_plan9_arm.s/hello_plan9_mips.s, but for SPARC.
// can be assembled/linked by ka/kl (-H2) and run via ki.
//
// Plan9 syscall convention here (see machines/ki/syscall.c's
// REGSP=1/REGRET=7): args go on the stack at 4(R1), 8(R1), ..., the
// syscall number goes in R7, then TA (the SPARC trap-always
// instruction; unlike other arches there's no bare SYSCALL mnemonic --
// see tests/s/mini/hello_linux_sparc.s) traps. ki decodes any TA as a
// syscall regardless of the trap vector operand, so R11/0x10 here is
// just following the same convention hello_linux_sparc.s uses.

TEXT _main(SB), $20
	MOVW	$setSB(SB), R2	// static base, needed for $hello(SB)

	/* prepare the system call PWRITE(1,&hello,13) */
	MOVW	$1, R4
	MOVW	R4, 4(R1)
	MOVW	$hello(SB), R4
	MOVW	R4, 8(R1)
	MOVW	$13, R4
	MOVW	R4, 12(R1)

	/* system call */
	MOVW	$11 /*PWRITE*/, R7
	MOVW	$0x10, R11
	TA	R11

	/* prepare the system call EXITS(0) */
	MOVW	$0, R4
	MOVW	R4, 4(R1)
	MOVW	$3 /*EXITS*/, R7
	/* system call */
	MOVW	$0x10, R11
	TA	R11
	RETURN /* not reached */

GLOBL	hello(SB), $13
DATA	hello+0(SB)/8, $"Hello, w"
DATA	hello+8(SB)/5, $"orld\n"
