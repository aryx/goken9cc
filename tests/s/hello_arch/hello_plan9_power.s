// same idea as hello_plan9_arm.s/hello_plan9_mips.s, but for PowerPC.
// can be assembled/linked by qa/ql (-H2) and run via qi.
//
// Plan9 syscall convention here (see machines/qi/syscall.c's
// REGSP=1/REGRET=3): args go on the stack at 4(R1), 8(R1), ..., the
// syscall number goes in R3 (not R0 -- that's the *Linux* convention
// used by tests/s/mini/hello_linux_power.s's SYSCALL), then SYSCALL
// (the "sc" instruction) traps.

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
	MOVW	$11 /*PWRITE*/, R3
	SYSCALL

	/* prepare the system call EXITS(0) */
	MOVW	$0, R4
	MOVW	R4, 4(R1)
	MOVW	$3 /*EXITS*/, R3
	/* system call */
	SYSCALL
	RETURN /* not reached */

GLOBL	hello(SB), $13
DATA	hello+0(SB)/8, $"Hello, w"
DATA	hello+8(SB)/5, $"orld\n"
