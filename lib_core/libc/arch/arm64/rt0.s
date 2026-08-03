// Process startup glue for linux/arm64, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_arm64.s's _main
// block, and principia's lib_core/libc/{arm,386}/main9.s for the same
// role on Plan9). Sets up the SB base register, calls user main(), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself (mirrors principia's main9.s safety net).
//
// claude: argc/argv bridge, same shape as arch/arm/rt0.s's (see its
// fuller comment for the general approach and the debugging technique
// -- docs/claude_notes/notes_debug_techniques.txt -- used to pin down
// the exact offsets empirically rather than trust a first derivation).
// Both offsets below are wider than the naive ABI-only expectation and
// were found by scanning candidate offsets with a tiny native debug
// helper (7a has no BFD-readable output for this linker's binaries, so
// objdump/gdb-on-the-file don't work here -- calling a real C
// function with known values and reading its printed output stood in
// for a disassembler): 7a auto-generates a 16-byte-aligned prologue
// reserving space for the saved link register in any non-leaf TEXT
// block (arm64's mandatory 16-byte SP alignment, unlike arm's plain
// 4-byte "push {lr}"), so argc is at RSP+16 here, not RSP+0; and the
// outgoing argv slot for main's own call lands at (new RSP)+16, not
// +8, mirroring arm's own "off by one more slot than expected" finding
// for the same underlying reason (its callee's own prologue reservation
// again).
TEXT _main+0(SB), $0
	MOV	$setSB(SB), R28
	MOV	16(RSP), R0	// argc
	ADD	$24, RSP, R1	// argv
	SUB	$32, RSP, RSP
	MOV	R1, 16(RSP)
	BL	main+0(SB)
	MOV	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
