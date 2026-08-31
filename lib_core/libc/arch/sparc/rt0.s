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
// argc/argv bridge: the kernel's raw argc/argv block is NOT at 0(R1)/
// 4(R1) the way it is on every other arch here -- SPARC32 Linux's
// process-entry stack reserves a 64-byte "minimum stack frame" (a
// full register window's worth of save-area slots, %l0-%l7/%i0-%i7,
// 16 words) BELOW argc, exactly as if the kernel had already executed
// a plain function prologue's own SAVE before jumping to _main. argc
// is really at 64(R1), argv starts at 68(R1). Found the hard way: an
// earlier version of this file assumed the naive 0(R1)/4(R1) offsets
// (matching this TEXT's own genuinely-unshifted leaf status -- no
// auto-prologue phantom slot to correct for, unlike mips/power/arm's
// own rt0.s) and every test reading argc/argv (args.c, env.c via
// envp = _mainargv+_mainargc+1) silently saw argc=0 -- confirmed via
// qemu's own gdbstub (`set endian big` FIRST, see notes_debug_
// techniques.txt) disassembling _main's own first instructions and
// dumping memory from $sp: all zero for the first 64 bytes, then the
// real argc(4)/argv[]/nil/envp[] block starting exactly at $sp+0x40.
// Every OTHER hello_libc test passed without this fix simply because
// none of them read argv/envp at all.
//
// The SUB $8,R1 below carves out a safe outgoing-arg slot for main()'s
// own call (kc's convention: a 2nd/stack argument is written by the
// caller at (R1 at call time)+8, confirmed empirically with 'kc -S' on
// a hand-written 2-arg function, same as xwrite_sparc.s's own count
// arg) -- landing well inside the 64-byte reserved window-save area
// below the real argc/argv/envp block, not on anything the kernel
// still needs.
TEXT _main(SB), $0
	MOVW	R14, R1
	MOVW	$setSB(SB), R2

	MOVW	64(R1), R7		// argc
	ADD	$68, R1, R9		// argv = &rawstack[17] (past the 64-byte window save area + argc word)
	MOVW	R9, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R7, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment

	SUB	$8, R1
	MOVW	R9, 8(R1)		// outgoing argv slot for main(argc, argv)

	MOVW	$main(SB), R8
	JMP	(R8)
