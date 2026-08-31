
TEXT    _start(SB), $0
        SUBQ    $16, SP
        LEAQ    msg(SB), AX
        MOVQ    AX, 0(SP)
        MOVQ    $13, 8(SP)
        CALL    write(SB)
        ADDQ    $16, SP
        CALL    exit(SB)

// Not OK (has nothing to do with tokens :) ) =~ panic
TEXT	notok(SB), $0
	MOVL	$0xf1, BP
	MOVQ	BP, (BP)
	RET

// -------------------------------------------
// write(buf *byte, len int)
// -------------------------------------------
TEXT    write(SB), $0
	MOVL	$1, DI		// arg 1 fd
	MOVQ	8(SP), SI		// arg 2 buf
	MOVL	16(SP), DX		// arg 3 count
	MOVL	$(0x2000000+4), AX	// syscall entry
	SYSCALL
	JCC	2(PC) // will JMP + 2 so to RET if everything is fine?
	CALL	notok(SB)
	RET


// -------------------------------------------
// exit()
// -------------------------------------------
TEXT    exit(SB), $0
	MOVL	$0, DI		// arg 1 exit status
	MOVL	$(0x2000000+1), AX	// syscall entry
	SYSCALL

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
