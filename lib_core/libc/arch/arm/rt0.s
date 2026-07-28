// Process startup glue for linux/arm, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_arm.s's _main
// block, and principia's lib_core/libc/arm/main9.s for the same role
// on Plan9). Sets up the SB base register (R12, this arch's own
// convention), calls user main(), and falls back to exit(0)/an
// infinite loop if main() returns without calling exit() itself.
TEXT _main+0(SB), $0
	MOVW	$setR12(SB), R12
	BL	main+0(SB)
	MOVW	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
