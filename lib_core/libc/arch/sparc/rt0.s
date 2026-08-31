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
// _main itself only tail-calls into _realmain below (JMP, not JMPL),
// once R1 is valid -- kl's own LEAF-clearing only looks at the CURRENT
// TEXT, so a separate, non-leaf block can safely contain the real
// JMPL-to-main()-then-exit(0) fallback that arch/power/rt0.s's/
// arch/mips/rt0.s's own single-block BL+fallthrough already have
// (their own REGSP already IS the real hardware register, no leaf
// constraint to route around in the first place).
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
// _main hands off argc(R7)/argv(R9) to _realmain below unchanged (a
// plain JMP, not a call, so registers just carry over) -- the outgoing
// arg slot for main()'s OWN call can't be set up here: it needs to
// land relative to R1 as _realmain's own auto-inserted prologue will
// leave it, not as _main's leaf-only R1 has it now (see _realmain's
// own comment).
TEXT _main(SB), $0
	MOVW	R14, R1
	MOVW	$setSB(SB), R2

	MOVW	64(R1), R7		// argc
	ADD	$68, R1, R9		// argv = &rawstack[17] (past the 64-byte window save area + argc word)
	MOVW	R9, _mainargv+0(SB)	// see port/mainargs.c
	MOVW	R7, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment

	MOVW	$_realmain(SB), R8
	JMP	(R8)

// Separate, non-leaf TEXT block -- kl's own LEAF-clearing (noop.c) only
// looks at the CURRENT TEXT, so putting the real JMPL-to-main() call
// here, instead of in _main above, keeps _main itself leaf (still
// required: R1 isn't valid yet at _main's own entry) while still
// getting a real call+return here, where R1 already IS valid by the
// time this block starts. kl auto-inserts its usual "SUB $frame,R1 /
// MOVW R15,0(R1)" prologue at the top of this block regardless of the
// $0 declared here (confirmed via 'kl -a', same as every other arch's
// own rt0.s note on this) -- safe this time, since unlike _main's own
// story, R1 is already correct when it runs. The SUB $8,R1 below (kc's
// convention: a 2nd/stack argument is written by the caller at (R1 at
// call time)+8, confirmed empirically with 'kc -S' on a hand-written
// 2-arg function, same as xwrite_sparc.s's own count arg) carves out
// main()'s own outgoing argv slot on top of whatever that auto-prologue
// already reserved -- deliberately NOT done back in _main, where R1
// doesn't yet reflect this block's own auto-prologue shift.
TEXT _realmain(SB), $0
	SUB	$8, R1
	MOVW	R9, 8(R1)		// outgoing argv slot for main(argc, argv)
	JMPL	main(SB)

	MOVW	$0, R7
	JMPL	exit(SB)
loop:
	JMP	loop
