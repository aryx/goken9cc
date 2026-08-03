// Process startup glue for linux/mips, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_mips.s's _main
// block, and principia's lib_core/libc/arm/main9.s for the same role
// -- via setR12 there -- on Plan9). Sets up the SB base register (R30
// here, matching this arch's own convention), calls user main(), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself.
//
// claude: argc/argv bridge, same shape and same debugging technique as
// arch/arm/rt0.s's and arch/arm64/rt0.s's (see arch/arm64/rt0.s's
// comment for why a live debug helper stood in for a disassembler, and
// docs/claude_notes/notes_debug_techniques.txt for the general
// method). va auto-reserves 4 bytes at entry to save the return
// address (R31) for any non-leaf TEXT block, so argc lands at 4(R29),
// not 0(R29); and empirically (same "callee's own prologue reservation
// eats one more slot than the naive FP+4 expectation" pattern already
// seen on arm/arm64), the outgoing argv slot for main's own call needs
// to be at (new R29)+8, not +4.
TEXT _main(SB), $0
	MOVW	$setR30(SB), R30
	MOVW	4(R29), R1	// argc
	MOVW	$8(R29), R2	// argv
	MOVW	R2, _mainargv+0(SB)	// see port/mainargs.c
	SUB	$12, R29
	MOVW	R2, 8(R29)
	JAL	main(SB)
	MOVW	$0, R1
	JAL	exit(SB)
loop:
	JMP	loop
