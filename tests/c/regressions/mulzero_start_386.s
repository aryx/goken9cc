// Minimal 386 Linux runtime stub for mulzero.c: just _main -> main() and
// exit(). See tests/c/mini2/linux_386.s for a fuller version.

TEXT _main(SB), $0
	CALL	main(SB)
	CALL	exit(SB) // should not be reached

TEXT exit+0(SB), $0
	MOVL	status+0(FP), BX
	MOVL	$1, AX
	INT	$0x80
	RET // never reached
