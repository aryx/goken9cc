// Minimal mips Linux runtime stub for float_const_oob.c: just _main ->
// main() and exit(). See tests/c/mini2/linux_mips.s for a fuller version.

TEXT _main(SB), $0
	MOVW	$setR30(SB), R30
	JAL	main(SB)

//extern void exit(uint32);
TEXT	exit+0(SB), $0
	MOVW	R1, R4              /* exit code */
	MOVW	$4001, R2           /* syscall = exit */
	SYSCALL
	RET // never reached
