// Process startup glue for linux/power, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_power.s's own _main
// block). Sets up the SB base register (R2 = gp), reads argc/argv
// straight off the raw kernel-provided stack, stashes argv for
// port/mainargs.c, calls user main(argc, argv), and falls back to
// exit(0)/an infinite loop if main() returns without calling exit()
// itself.
//
// claude: argc/argv bridge, same technique as arch/alpha/rt0.s and
// arch/riscv64/rt0.s (see their own comments for the general
// approach and how it was found). ql's own noop.c inserts an
// auto-prologue for any non-leaf TEXT block regardless of its
// declared frame size -- confirmed via 'ql -a' on this exact file:
// even a bare "TEXT _main(SB), $0" that calls main(SB) gets
// `MOVW LR,R31` + `MOVWU R31,-8(R1)` prepended, shifting the kernel's
// own argc/argv[0] from 0(R1)/4(R1) to 8(R1)/12(R1) by the time this
// code's own first instruction runs. Verified against a real qemu-ppc
// -d in_asm trace of the linked binary, not assumed by analogy with
// alpha/riscv64's own (different-offset) findings.
//
// The ADD $-12,R1,R1 below is a second, separate adjustment (same
// shape as alpha's own SUBQ, and arm's own identically-named +4
// correction -- see arch/arm/rt0.s's own header comment for how that
// one was found and confirmed): main(argc, argv)'s own outgoing call
// needs a real 4-byte home slot for argv (its stack-passed second
// argument), but that slot lands at 8(R1) after THIS adjustment, not
// the naively-expected 4(R1) -- ql's D_PARAM offset formula
// (linkers/ql/span.c) adds one extra phantom word on top of the
// declared frame, the same "as if a return address were pushed to the
// stack" assumption baked in for every arch here (power, like arm,
// has no such pushed return address -- BL uses LR -- so rt0.s has to
// manufacture the matching phantom slot itself). Confirmed via a real
// qemu-ppc trace: main's own auto-prologue-adjusted "MOVW
// argv+4(FP),R5" read from R1+24, not R1+20, once every prologue
// adjustment along the call chain (_main's own -8, main's own -16) is
// added up -- see notes_arch_power.txt for the full byte-by-byte
// trace. Originally landed as "ADD $-8,R1,R1" / "MOVW R4,4(R1)" (the
// naive one-word version), which built and ran fine for every 0-arg
// main() in tests/c/hello_libc but segfaulted the instant args.c's
// main(argc,argv) actually read argv[i].
TEXT _main(SB), $0
	MOVW	$setSB(SB), R2
	MOVW	8(R1), R3		// argc
	ADD	$12, R1, R4		// argv = &rawstack[2], past the auto-prologue's own +8 shift
	MOVW	R4, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R3, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment
	ADD	$-12, R1, R1
	MOVW	R4, 8(R1)		// outgoing argv slot for main(argc, argv)
	BL	main(SB)
	MOVW	$0, R3
	BL	exit(SB)
loop:
	BR	loop
