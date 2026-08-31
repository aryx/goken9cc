// Process startup glue for plan9/sparc.
//
// Unlike linux/sparc's own rt0.s, there's no register-window save-area
// to skip and no separate hardware %sp(R14)->R1 copy needed: Plan9's
// own process-entry convention (machines/ki/ki.c's initstk()) already
// hands the process a valid R1 (REGSP) from the very first
// instruction, with argc at 0(R1) and argv starting at 4(R1) -- the
// same simple shape arm/mips/power's own single-block rt0.s already
// reads directly (see e.g. arch/power/rt0.s).
//
// Still needs the SAME two-block split linux/sparc/rt0.s uses, for an
// unrelated, GOOS-independent reason: kl's own noop.c auto-inserts a
// "SUB $frame,R1 / MOVW R15,0(R1)" prologue immediately after ATEXT
// for ANY TEXT containing a JMPL, regardless of where in the block it
// appears -- so reading argc/argv at 0(R1)/4(R1) in the SAME block
// that later JMPLs to main() would read through that auto-shifted R1,
// not the true raw process-entry one. _main here stays leaf (only a
// plain JMP, which kl does not auto-prologue) so its own 0(R1)/4(R1)
// reads see the real, unshifted R1; _realmain is a separate non-leaf
// block that does the actual JMPL-to-main()-then-exit(0) fallback,
// same role as arch/power/rt0.s's own single BL+fallthrough (which
// needs no such split, since REGSP there isn't threatened by an
// R14-copy timing issue in the first place -- this split is purely
// about avoiding kl's own auto-prologue, same as linux/sparc/rt0.s's
// identical split for its own, different reason).
//
// Selected via RT0OFILE=arch/sparc/rt0_plan9.$O on the command line,
// same override mechanism arch/amd64/rt0_darwin.s and
// arch/arm64/rt0_darwin.s already use (see lib_core/libc/mkfile's own
// RT0OFILE comment).
TEXT _main(SB), $0
	MOVW	$setSB(SB), R2

	MOVW	0(R1), R7		// argc
	ADD	$4, R1, R9		// argv
	MOVW	R9, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R7, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment

	MOVW	$_realmain(SB), R8
	JMP	(R8)

TEXT _realmain(SB), $0
	SUB	$8, R1
	MOVW	R9, 8(R1)		// outgoing argv slot for main(argc, argv)
	JMPL	main(SB)

	MOVW	$0, R7
	JMPL	exit(SB)
loop:
	JMP	loop
