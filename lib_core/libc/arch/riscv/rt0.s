// Process startup glue for linux/riscv (rv32), built once into libc.a
// instead of pasted per test (compare tests/c/mini2/linux_riscv.s's
// _main block). Sets up the SB base register (R3 = gp, this arch's own
// convention), calls user main(), and falls back to exit(0)/an
// infinite loop if main() returns without calling exit() itself.
TEXT _main(SB), $0
	MOVW	$setSB(SB), R3
	JAL	R1, main(SB)
	MOVW	$0, R8
	JAL	R1, exit(SB)
loop:
	JMP	loop
