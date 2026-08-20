// Process startup glue for linux/alpha, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_alpha.s's _main
// block, and tests/s/mini/hello_linux_alpha.s's own setSB dance).
// Sets up the SB base register (R29 = gp), reads argc/argv straight
// off the raw kernel-provided stack, stashes argv for port/mainargs.c
// (see that file's own comment), calls user main(argc, argv), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself.
//
// -E _main makes this the literal kernel entry point, but TEXT ...,$0
// does NOT mean "no prologue at all": zl's own noop.c (the ATEXT case
// in its prologue-insertion pass) only skips the SUBQ+RA-save preamble
// for functions its own LEAF-detection marks as leaves, and _main is
// not one (it calls main/exit via JSR) -- so zl unconditionally
// prepends `SUBQ $8,R30` + a RA save before a single instruction of
// this file ever runs. That shifts the kernel's own argc/argv[0] from
// 0(R30)/8(R30) to 8(R30)/16(R30) by the time this code executes.
// Found via qemu-alpha -d in_asm on this exact binary: the very first
// two instructions at the ELF entry point are that auto-inserted
// `lda sp,-8(sp); stl ra,0(sp)`, not this file's own MOVQ. The original
// "argc at 0(R30)" claim was never actually verified against a real
// argc value (tests/c/hello_libc/hello.c takes no arguments) -- only
// that main() got called and didn't crash, which doesn't distinguish
// "correct offset" from "off by 8 but nothing read past argv[0]" until
// args.c (a later addition) started asserting the real argc value and
// caught it (printed argc=0 instead of the expected argc=4). Same
// shape and same root cause as arch/riscv64/rt0.s's own "argc at
// 8(R2), not 0(R2)" comment -- that file's own linker (ja) reserves an
// analogous one-slot auto-push; see its comment for the exact wording.
//
// The SUBQ below is a second, separate adjustment: main(argc, argv)'s
// own outgoing call needs a real 8-byte home slot for argv (its
// stack-passed second argument) at 16(R30) after THIS adjustment.
// Confirmed via a `zc -S` probe on an equivalent 2-arg call
// (docs/claude_notes/notes_arch_alpha.txt): zc's own caller-side
// codegen for "f(int, char**)" stores the second argument at exactly
// this offset within a $24 frame -- unrelated to, and unaffected by,
// the auto-prologue's own +8 shift fixed above.
TEXT _main(SB), $0
	MOVQ	$setSB(SB), R29
	MOVQ	8(R30), R1		// argc (scratch; R0/REGARG is set just before the call below)
	MOVQ	$16(R30), R2		// argv = &rawstack[2], past the auto-prologue's own +8 shift (scratch)
	MOVQ	R2, _mainargv+0(SB)	// see port/mainargs.c
	MOVQ	R1, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment
	SUBQ	$24, R30
	MOVQ	R2, 16(R30)		// outgoing argv slot for main(argc, argv)
	MOVQ	R1, R0			// argc -> R0 (REGARG), main()'s first argument
	JSR	,main+0(SB)
	MOVQ	$0, R0
	JSR	,exit+0(SB)
loop:
	JMP	loop
