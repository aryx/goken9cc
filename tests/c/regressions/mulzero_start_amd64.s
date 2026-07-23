// Minimal amd64 Linux runtime stub for mulzero.c: just _main -> main()
// and exit(). See tests/c/mini2/linux_amd64.s for a fuller version.

TEXT	_main(SB), $0
	CALL	main(SB)

//extern void exit(int);
TEXT	exit(SB), $0
	MOVL	8(SP), DI
	MOVL	$60, AX
	SYSCALL
	RET // never reached
