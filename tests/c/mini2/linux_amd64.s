TEXT    _main(SB), $0
	CALL main(SB)

TEXT    panic(SB), $0
        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL


// was called exit1 in runtime/linux/amd64/sys.s
TEXT	exit(SB), $0-8
	MOVL	8(SP), DI
	MOVL	$60, AX	// exit - exit the current os thread
	SYSCALL
	RET

//alt?
//TEXT    exit0(SB), 7, $0
//        // syscall: exit(0)
//        MOVQ    $60, AX
//        XORQ    DI, DI
//        SYSCALL

TEXT	write(SB), $0-24
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	$1, AX			// syscall entry
	SYSCALL
	RET

//alt?
//TEXT	write0(SB), $0
//	MOVQ	$1, AX          // syscall number for write
//	MOVQ	fd+0(FP), DI     // fd (arg 1)
//	MOVQ	buf+8(FP), SI    // buf (arg 2)
//	MOVQ	n+16(FP), DX     // n (arg 3)
//	SYSCALL
//	RET
