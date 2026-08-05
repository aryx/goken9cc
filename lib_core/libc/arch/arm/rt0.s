// Process startup glue for linux/arm, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_arm.s's _main
// block, and principia's lib_core/libc/arm/main9.s for the same role
// on Plan9). Sets up the SB base register (R12, this arch's own
// convention), calls user main(), and falls back to exit(0)/an
// infinite loop if main() returns without calling exit() itself.
//
// claude: argc/argv bridge. At _main, R13 (SP) is exactly what the
// kernel handed the process at execve(): 0(R13) holds argc, 4(R13)
// onward holds argv[0], argv[1], ..., matching what
// GO/pkg/runtime/arm/asm.s's _rt0_arm reads (minus its g0/m0/istack
// scheduler setup, which this libc has no equivalent of). Empirically
// (5c -S on a 2-arg probe function), main(argc, argv) wants argc in R0
// directly -- unlike amd64/386, this arch DOES pass a first argument
// in a register -- and argv at 4(FP), i.e. 4(R13) right before BL.
//
// claude: +4 correction, found via qemu-arm's gdbstub (see
// docs/claude_notes/notes_debug_techniques.txt): this TEXT block
// calls BL (main, exit), and 5a's assembler auto-emits "push {lr}" as
// the very first instruction for any non-leaf function regardless of
// the declared frame size -- confirmed by single-stepping and dumping
// [R13]/[R13+4] right after entry. So by the time this code's own
// first instruction runs, R13 is already 4 less than the kernel's raw
// value: the true argc lives at 4(R13), not 0(R13), and argv[0]'s
// address is R13+8, not R13+4. Using R13 explicitly rather than the SP
// pseudo-register, mirroring Go's own "use R13 instead of SP to avoid
// linker rewriting the offsets" comment for this exact spot.
// Opens fresh scratch space below the raw argc/argv/envp block before
// writing argv there, so as not to self-clobber argv[0]'s own stored
// pointer (see amd64 rt0.s's fuller version of this same reasoning).
//
// claude: the outgoing argv slot itself lands at 8(R13), not 4(R13) as
// naively expected from "argv is main's 2nd parameter, so FP+4" --
// found by single-stepping past this SUB into main() itself (gdb) and
// comparing where main's own prologue-adjusted "ldr r3,[sp,#20]" (its
// read of argv+4(FP)) actually points against where this code had
// written the value: a consistent +4 gap versus the naive
// call-site-SP-relative expectation, on top of the earlier +4 from the
// auto push{lr}. Not fully explained by the auto-push alone (that
// already priced in above, for argc); empirically confirmed and used
// as-is rather than re-derived, per
// docs/claude_notes/notes_debug_techniques.txt's "verify via a live
// debugger, don't just re-derive the ABI" note. Confirmed correct end
// to end (argc and every argv[i] printed correctly under qemu-arm).
TEXT _main+0(SB), $0
	MOVW	$setR12(SB), R12
	MOVW	4(R13), R0	// argc
	ADD	$8, R13, R1	// argv
	MOVW	R1, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R0, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment
	SUB	$12, R13
	MOVW	R1, 8(R13)
	BL	main+0(SB)
	MOVW	$0, R0
	BL	exit+0(SB)
loop:
	B	loop
