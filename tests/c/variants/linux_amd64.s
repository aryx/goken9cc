// Minimal Linux/amd64 syscall stubs for the regarg_amd64/fconst64_amd64
// regression tests below: just enough to enter main, exit, and write(2).
// Trimmed from ../float/linux_amd64.s (same syscall numbers/ABI).

TEXT	_main(SB), $0
	CALL	main(SB)

TEXT	exit(SB), $0
	MOVL	8(SP), DI
	MOVL	$60, AX	// exit - exit the current os thread
	SYSCALL
	RET

TEXT	write(SB), $0
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	$1, AX			// syscall entry
	SYSCALL
	RET
