// setjmp()/longjmp() for linux/386. This compiler family preserves no
// registers across a call (every Xc backend reloads from the stack
// after any call instead), so the only state a "return a second time"
// trick needs is the stack pointer and the return address -- see
// include/core/exn.h's own header comment for the fuller story and
// why jmp_buf is sized for exactly these two uintptr-wide fields.
//
// SP here means the REAL hardware stack pointer, not a local-variable
// offset: 8a's own lexer marks bare "SP" as an LSP/D_AUTO pseudo-token
// (assemblers/8a/lex.c), the same FP-relative-locals mechanism mips/
// riscv's own "SP" tokens use (see those arches' own setjmp.s
// comments) -- but for a TEXT block declared with frame size $0, that
// pseudo-SP and the real hardware register are numerically identical
// (no local-variable space to offset by), which is exactly the same
// property os/windows/winio_amd64.s's own stubs already rely on
// (`MOVQ SP, DI` there, real amd64 hardware RSP, verified via a real
// Windows run) -- inferred to hold identically here for 386's sibling
// assembler (8a), not independently re-verified against real 386
// hardware.
//
// Ported in spirit from principia's lib_core/libc/386/setjmp.s (same
// two-field save, same stack-top return-address trick a plain CALL/RET
// pair needs on x86, since there is no dedicated link register here
// unlike arm/arm64/mips/riscv) -- not a literal copy, since jmp_buf's
// own layout differs (a real 2-field struct here, not principia's bare
// 2-word buffer).
TEXT setjmp(SB), $0
	MOVL	j+0(FP), AX
	MOVL	SP, 0(AX)	// save real sp
	MOVL	0(SP), BX	// return address currently on the stack top
	MOVL	BX, 4(AX)
	MOVL	$0, AX		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVL	v+4(FP), AX
	CMPL	AX, $0
	JNE	ok
	MOVL	$1, AX		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
ok:
	MOVL	j+0(FP), BX
	MOVL	0(BX), SP	// restore real sp
	MOVL	4(BX), BX	// saved return address
	MOVL	BX, 0(SP)	// write it back onto the (now-restored) stack top
	RET			// RET pops that slot and jumps there, with AX=v
