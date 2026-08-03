// Process startup glue for linux/riscv64 (rv64), built once into
// libc.a instead of pasted per test (compare
// tests/c/mini2/linux_riscv64.s's _main block, identical to rv32's
// here since only FP-relative argument *offsets* differ between the
// two widths, not this entry sequence). Sets up the SB base register
// (R3 = gp), calls user main(), and falls back to exit(0)/an infinite
// loop if main() returns without calling exit() itself.
//
// claude: argc/argv bridge, same shape and technique as arch/riscv/
// rt0.s (rv32) -- see its comment for the debugging approach. Found by
// direct measurement, not by doubling rv32's offsets for the wider
// pointer: argc is still at 8(R2) here (not 16, despite 8-byte
// registers), so ja's own auto-push reserves one 8-byte-aligned slot
// on rv32 *and* rv64 alike -- rv32 was seemingly already over-aligned
// relative to its own 4-byte registers. The outgoing argv slot for
// main's own call needs (new R2)+16, matching rv64's own 8-byte
// pointer width for the "one more slot than naive FP+8" pattern.
TEXT _main(SB), $0
	MOVW	$setSB(SB), R3
	MOV	8(R2), R8	// argc
	MOV	$16(R2), R9	// argv
	SUB	$24, R2
	MOV	R9, 16(R2)
	JAL	R1, main(SB)
	MOVW	$0, R8
	JAL	R1, exit(SB)
loop:
	JMP	loop
