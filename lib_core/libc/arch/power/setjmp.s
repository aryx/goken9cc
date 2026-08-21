// setjmp()/longjmp() for linux/power. See include/core/exn.h's own
// header comment for the general approach shared by every arch here:
// jmp_buf only ever needs the two things a "return a second time"
// trick actually requires -- the stack pointer to rewind to (offset
// 0) and the return address to jump back to (offset 4, both uintptr-
// width per struct Jmpbuf) -- since this compiler's own codegen
// reloads everything else from memory after any call, unlike gcc/
// clang which would need real callee-saved-register preservation.
//
// qc passes the first argument (j, a pointer) in R3 (REGARG=REGRET=3)
// and the second (v, an int, in longjmp only) on the stack -- at
// v+4(FP), matching every other pointer-then-int 2-arg case already
// confirmed via qc -S elsewhere (tests/c/vlong/linux_power.s's own
// write() stub). REGSP=R1 (q.out.h) is this arch's real hardware sp;
// LR is a special-purpose register (not a GPR), read/written via
// "MOVW LR,Rn"/"MOVW Rn,LR" (see tests/c/exit/exit_linux_power.s-
// style _main prologues, which already save/restore LR this way).
//
// Genuine LEAF function (no BL inside either body), so ql's own
// auto-LR-save prologue for non-leaf TEXT blocks does not apply --
// LR at entry is exactly the caller's own return address.
TEXT setjmp(SB), $0
	MOVW	R1, 0(R3)	// save real sp (j arrives in R3)
	MOVW	LR, R4
	MOVW	R4, 4(R3)	// save return address into j->pc
	MOVW	$0, R3		// setjmp() returns 0 on the direct call
	RETURN

TEXT longjmp(SB), $0
	MOVW	v+4(FP), R4
	CMP	R4, $0
	BNE	notzero
	MOVW	$1, R4		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOVW	0(R3), R5	// saved sp (R3 still holds j here)
	MOVW	4(R3), R6	// saved return address
	MOVW	R6, LR		// restore LR directly
	MOVW	R4, R3		// return value into R3 (frees R3, j no longer needed)
	MOVW	R5, R1		// restore sp last
	RETURN			// branches via the now-restored LR, with R3=v
