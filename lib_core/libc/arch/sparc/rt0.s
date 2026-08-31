// Process startup glue for linux/sparc, built once into libc.a instead
// of pasted per test (compare tests/c/mini/start_sparc.s's own, much
// simpler _start block, and this arch's own docs/claude_notes/
// notes_arch_sparc.txt for the R14->R1/leaf/REGSP-collision background
// this file leans on throughout).
//
// R1 (REGSP, kc's own logical stack pointer) is never initialized by
// the kernel -- only the real hardware %sp (%o6, R14 in this port's
// flat numbering) is -- so the very first instruction here must copy
// R14 into R1, before anything else (including kl's own auto-inserted
// prologue) touches it. That forces this TEXT to stay LEAF: kl's
// noop.c splices its "SUB $frame,R1 / MOVW R15,0(R1)" prologue in
// immediately after ATEXT for any TEXT containing so much as one
// AJMPL, regardless of where in the file that AJMPL appears -- so a
// real CALL to main() here (expecting a return, to fall back to
// exit(0) the way arch/power/rt0.s's own BL does) would have that
// auto-prologue clobber R1 before this file's own R14->R1 copy ran,
// reproducing the exact segfault notes_arch_sparc.txt's tests/c/mini
// section documents.
//
// claude: TODO -- only tail-calls main() (JMP, not JMPL), so it never
// returns here and there is NO exit(0) fallback if a future test's
// main() just falls off the end without calling exit() itself (unlike
// arch/power/rt0.s/arch/mips/rt0.s, which can safely call+return since
// their own REGSP already IS the real hardware register, no leaf
// constraint). hello.c calls exit(0) itself, so this is fine for now.
// Fixing this properly needs a SECOND, non-leaf TEXT block that this
// one tail-jumps into once R1 is valid -- kl's own LEAF-clearing only
// looks at the CURRENT TEXT, so a separate block can safely contain
// the real JMPL-to-main()-then-exit(0) sequence. Not done: no test
// exercised so far needs it.
//
// argc/argv bridge: since this TEXT stays LEAF (no auto-prologue), the
// kernel's raw argc/argv block is read at its own natural, UNSHIFTED
// offsets (argc at 0(R1), argv at 4(R1)) -- unlike mips/power/arm's
// own rt0.s, which all need a "+4" or "+8" correction to account for
// their own auto-prologue's phantom slot. The SUB $8,R1 below only
// exists to carve out a safe outgoing-arg slot for main()'s own call
// (kc's convention: a 2nd/stack argument is written by the caller at
// (R1 at call time)+8, confirmed empirically with 'kc -S' on a
// hand-written 2-arg function, same as xwrite_sparc.s's own count arg)
// -- writing there BEFORE this adjustment would land on argv[0]/
// argv[1] themselves (4(R1)/8(R1), the kernel's own data), corrupting
// them for any real multi-argument invocation. After the SUB, that
// slot lands exactly on the vacated argc word (already copied out
// above), not on anything still needed.
TEXT _main(SB), $0
	MOVW	R14, R1
	MOVW	$setSB(SB), R2

	MOVW	0(R1), R7		// argc
	ADD	$4, R1, R9		// argv = &rawstack[1]
	MOVW	R9, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R7, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment

	SUB	$8, R1
	MOVW	R9, 8(R1)		// outgoing argv slot for main(argc, argv)

	MOVW	$main(SB), R8
	JMP	(R8)
