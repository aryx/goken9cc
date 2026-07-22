// Regression test for assemblers/6a/lex.c's zaddr() using `long l`
// instead of `int32 l`: on our 64-bit hosts `long` is 64 bits, so
// `l = a->offset; if((vlong)l != a->offset)` never differs and the
// T_64 object-file flag was never set, silently truncating any
// hand-assembled 64-bit immediate down to its low 32 bits -- e.g.
// "MOVQ $0x7FF0000000000001,CX" assembled to the 32-bit-immediate
// 0xc7 opcode form encoding just $1, instead of the 64-bit-immediate
// 0xb8+reg form encoding the full value. Same bug, same fix already
// applied to compilers/6c/swt.c's zaddr() (the compiler-side writer);
// this is the assembler-side writer, a separate code path. Go-era's
// own 6a (src/cmd/6a/lex.c zaddr()) already uses int32 here.

TEXT	_main(SB), $0
	CALL	main(SB)

TEXT	main(SB), $0
	MOVQ	$0x7FF0000000000001, CX
	MOVQ	CX, buf+0(SB)
	LEAQ	buf+0(SB), SI
	MOVQ	$1, DI
	MOVQ	$8, DX
	MOVQ	$1, AX		// write
	SYSCALL
	MOVQ	$60, AX		// exit
	XORQ	DI, DI
	SYSCALL

GLOBL	buf+0(SB), $8
