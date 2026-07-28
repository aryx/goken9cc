// Process startup glue for linux/mips, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_mips.s's _main
// block, and principia's lib_core/libc/arm/main9.s for the same role
// -- via setR12 there -- on Plan9). Sets up the SB base register (R30
// here, matching this arch's own convention), calls user main(), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself.
TEXT _main(SB), $0
	MOVW	$setR30(SB), R30
	JAL	main(SB)
	MOVW	$0, R1
	JAL	exit(SB)
loop:
	JMP	loop
