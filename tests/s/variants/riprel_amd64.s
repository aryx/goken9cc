// Regression test for 6l -H6 (macOS Mach-O)'s RIP-relative PIE
// addressing (see linkers/6l/span.c's riprelfix/curppc comment).
// Exercises two distinct disp32-correction cases that a first
// implementation attempt got wrong by conflating the length-delta and
// byte-position corrections:
//  - LEAQ msg(SB),AX: a REX.W prefix is inserted after the disp32 is
//    written, but no other bytes follow it in the instruction.
//  - MOVL $42, counter(SB): no REX prefix, but a trailing 4-byte
//    immediate follows the disp32.
// Golden-diffing the assembled object and linked Mach-O catches a
// regression in either case (a wrong disp32 silently points at the
// wrong address at runtime instead of failing to assemble/link).

TEXT _start(SB), $0
	SUBQ	$16, SP
	LEAQ	msg(SB), AX
	MOVQ	AX, 0(SP)
	MOVQ	$13, 8(SP)
	MOVL	$42, counter(SB)
	CALL	write(SB)
	ADDQ	$16, SP
	MOVQ	$0x2000001, AX
	XORQ	DI, DI
	SYSCALL

// -------------------------------------------
// write(buf *byte, len int)
// -------------------------------------------
TEXT write(SB), $0
	MOVL	$1, DI
	MOVQ	8(SP), SI
	MOVL	16(SP), DX
	MOVQ	$0x2000004, AX
	SYSCALL
	RET

DATA	msg+0(SB)/8, $"Hello, w"
DATA	msg+8(SB)/5, $"orld\n"
GLOBL	msg(SB), $13

DATA	counter(SB)/4, $0
GLOBL	counter(SB), $4
