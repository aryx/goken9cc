// setjmp()/longjmp() for linux/riscv (rv32). See arch/arm/setjmp.s's
// own header comment for the general approach. j (the first argument,
// a pointer) arrives unspilled in R8 here, not R0 -- confirmed via
// ic -S on a probe function, matching svc_riscv64.s's own established
// "first named parameter arrives in R8" convention (this compiler's
// own choice, not a real-ISA-mandated register). R2/R1 are the real
// hardware sp/ra registers (assemblers/ia/lex.c's own register table;
// "SP" itself is an LSP/D_AUTO pseudo-token here too, same story as
// x86/mips, which is why arch/riscv/rt0.s already uses R2/R1 directly
// rather than "SP"/"RA"). v (longjmp's second argument) lands at
// v+4(FP), confirmed via the same probe.
//
// Genuine LEAF function (no JAL inside either body): unlike arm/arm64/
// mips, this compiler's own JAL does not appear to auto-save R1 for a
// non-leaf TEXT block at all (arch/riscv/rt0.s's own _main calls JAL
// twice with a plain $0 frame and no R1 save of its own, since it
// never needs its own return address preserved) -- but since setjmp/
// longjmp never issue JAL either way, R1/R2 at entry are simply the
// caller's own values regardless of that convention's details.
//
// This arch's own compiler-generated branch shape (confirmed via
// `ic -S` on a real `if(v==0)` probe) is a genuine 2-register
// comparison against R0 (the hardwired zero register), not a separate
// CMP+conditional-jump pair like x86/arm -- `BNE R0, R8, label` -- used
// directly below for the ansi longjmp(j, 0) special case.
TEXT setjmp(SB), $0
	MOVW	R2, 0(R8)	// save real sp
	MOVW	R1, 4(R8)	// save return address (the real ra register)
	MOVW	$0, R8		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVW	v+4(FP), R9
	BNE	R0, R9, notzero
	MOVW	$1, R9		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOVW	0(R8), R10	// saved sp (R8 still holds j here)
	MOVW	4(R8), R1	// saved return address -> restore R1 directly
	MOVW	R9, R8		// return value into R8 (frees R8, j no longer needed)
	MOVW	R10, R2		// restore sp last
	RET			// branches via the now-restored R1, with R8=v
