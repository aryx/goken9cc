// setjmp()/longjmp() for linux/alpha. See include/core/exn.h's own
// header comment for the general approach shared by every arch here:
// jmp_buf only ever needs the two things a "return a second time"
// trick actually requires -- the stack pointer to rewind to (offset
// 0) and the return address to jump back to (offset 8, both uintptr-
// width per struct Jmpbuf) -- since this compiler's own codegen
// reloads everything else from memory after any call, unlike gcc/
// clang which would need real callee-saved-register preservation.
//
// zc passes the first argument (j, a pointer) in R0 (REGARG=REGRET=0)
// and the second (v, an int, in longjmp only) on the stack -- at
// v+8(FP), matching every other pointer-then-int 2-arg case already
// confirmed via zc -S elsewhere (docs/claude_notes/notes_arch_alpha.txt).
// REGSP=R30 and REGLINK=R26 are this arch's real hardware sp/ra
// registers (include/objexec/z.out.h).
TEXT setjmp(SB), $0
	MOVQ	R30, 0(R0)	// save real sp into j->sp (j arrives in R0)
	MOVQ	R26, 8(R0)	// save return address into j->pc
	MOVQ	$0, R0		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVL	v+8(FP), R1
	BNE	R1, notzero
	MOVQ	$1, R1		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOVQ	0(R0), R2	// saved sp (R0 still holds j here)
	MOVQ	8(R0), R26	// saved return address -> restore R26 directly
	MOVQ	R1, R0		// return value into R0 (frees R0, j no longer needed)
	MOVQ	R2, R30		// restore sp last
	RET			// branches via the now-restored R26, with R0=v
