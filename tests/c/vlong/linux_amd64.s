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

TEXT	write(SB), $0-24
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	$1, AX			// syscall entry
	SYSCALL
	RET
