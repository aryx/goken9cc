// Process startup glue for linux/riscv (rv32), built once into libc.a
// instead of pasted per test (compare tests/c/mini2/linux_riscv.s's
// _main block). Sets up the SB base register (R3 = gp, this arch's own
// convention), calls user main(), and falls back to exit(0)/an
// infinite loop if main() returns without calling exit() itself.
//
// claude: argc/argv bridge, same shape and technique as arch/arm/rt0.s
// and arch/mips/rt0.s (see arch/arm64/rt0.s's comment for why a live
// debug helper stood in for a disassembler here too -- ia additionally
// crashes on very short standalone probe files for unrelated reasons,
// so these offsets were found by editing this real file directly and
// rebuilding via mk, not via minimal throwaway probes). Empirically,
// argc lands at 8(R2) here -- not 4(R2) as mips's single-word auto-push
// would suggest, so ia's own prologue reservation for this arch is
// wider than mips's/arm's (plausibly a full 8-byte slot despite rv32's
// 4-byte registers, unconfirmed why); the outgoing argv slot for
// main's own call needs (new R2)+8, matching the same "one more slot
// than the naive FP+4 expectation" pattern already seen on every other
// arch here.
TEXT _main(SB), $0
	MOVW	$setSB(SB), R3
	MOVW	8(R2), R8	// argc
	MOVW	$12(R2), R9	// argv
	MOVW	R9, _mainargv+0(SB)	// see port/mainargs.c
	SUB	$12, R2
	MOVW	R9, 8(R2)
	JAL	R1, main(SB)
	MOVW	$0, R8
	JAL	R1, exit(SB)
loop:
	JMP	loop
