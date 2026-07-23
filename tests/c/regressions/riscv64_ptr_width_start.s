// Minimal riscv64 (thechar='j') Linux runtime stub for riscv64_ptr_width.c:
// just _main -> main() and exit(). See tests/c/mini2/linux_riscv64.s for a
// fuller version (write() too); this test needs neither write() nor libc.

TEXT _main(SB), $0
	MOVW	$setSB(SB), R3      // static base (gp), needed for $"x"(SB)
	JAL	R1, main(SB)

//extern void exit(int32);
TEXT	exit+0(SB), $0
	MOVW	R8, R10             // a0 = exit code (1st arg arrives in R8)
	MOVW	$93, R17            // a7 = syscall number: exit
	ECALL
	RET // never reached
