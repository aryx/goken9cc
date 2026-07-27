// Process startup glue for linux/arm64, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_arm64.s's _main
// block, and principia's lib_core/libc/{arm,386}/main9.s for the same
// role on Plan9). Sets up the SB base register, calls user main(), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself (mirrors principia's main9.s safety net).
TEXT _main+0(SB), $0
	MOV	$setSB(SB), R28
	BL	main+0(SB)
	MOV	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
