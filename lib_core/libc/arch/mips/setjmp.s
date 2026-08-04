// setjmp()/longjmp() for linux/mips (o32). See arch/arm/setjmp.s's own
// header comment for the general approach. j (the first argument, a
// pointer) arrives unspilled in R1 here, not R0 -- confirmed via
// vc -S on a probe function, matching svc_mips.s's own established
// "only the first named parameter arrives in a register (R1)"
// convention. R29/R31 are the real hardware sp/return-address
// registers (assemblers/va/lex.c's own register table; "SP" itself is
// an LSP/D_AUTO pseudo-token on this assembler too, same story as x86,
// which is exactly why arch/mips/rt0.s and svc_mips.s both already use
// R29 directly rather than "SP"). v (longjmp's second argument) lands
// at v+4(FP), the natural 4-byte packing convention svc_mips.s's own
// comment already established.
//
// Genuine LEAF function (no JAL inside either body), so va's own
// auto-4-byte-R31-saving prologue for non-leaf TEXT blocks
// (arch/mips/rt0.s's own comment on this) does not apply -- R29/R31 at
// entry are exactly the caller's own values, matching every other
// $0-frame leaf stub already in this tree (svc_mips.s's open/close/
// create/... and its own seek()'s SUBU/BEQ idiom, reused below).
//
// This assembler's BEQ only ever takes a single register operand
// (branch-if-zero -- svc_mips.s's own comment on this, and its seek()
// stub's identical structuring), with no confirmed BNE counterpart
// anywhere else in this tree, so the ansi longjmp(j, 0) special case
// below is built from BEQ plus an unconditional JMP (already proven in
// arch/mips/rt0.s's own "loop: JMP loop") rather than assuming BNE
// works the same way.
TEXT setjmp(SB), $0
	MOVW	R29, 0(R1)	// save real sp
	MOVW	R31, 4(R1)	// save return address (the real ra register)
	MOVW	$0, R1		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVW	v+4(FP), R2
	BEQ	R2, zero
	JMP	notzero
zero:
	MOVW	$1, R2		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOVW	0(R1), R3	// saved sp (R1 still holds j here)
	MOVW	4(R1), R31	// saved return address -> restore R31 directly
	MOVW	R2, R1		// return value into R1 (frees R1, j no longer needed)
	MOVW	R3, R29		// restore sp last
	RET			// branches via the now-restored R31, with R1=v
