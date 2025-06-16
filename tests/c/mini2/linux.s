TEXT	write(SB), 7, $0
	MOVQ	$1, AX          // syscall number for write
	MOVQ	fd+0(FP), DI     // fd (arg 1)
	MOVQ	buf+8(FP), SI    // buf (arg 2)
	MOVQ	n+16(FP), DX     // n (arg 3)
	SYSCALL
	RET

TEXT    exit+0(SB), 7, $0
        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL

TEXT    panic+0(SB), 7, $0
        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL
