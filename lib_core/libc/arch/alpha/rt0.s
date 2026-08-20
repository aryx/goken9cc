// Process startup glue for linux/alpha, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_alpha.s's _main
// block, and tests/s/mini/hello_linux_alpha.s's own setSB dance).
// Sets up the SB base register (R29 = gp), reads argc/argv straight
// off the raw kernel-provided stack, stashes argv for port/mainargs.c
// (see that file's own comment), calls user main(argc, argv), and
// falls back to exit(0)/an infinite loop if main() returns without
// calling exit() itself.
//
// -E _main makes this the literal kernel entry point (no intervening
// frame), so R30 (the real hardware sp, z.out.h's REGSP) holds exactly
// what the kernel set: argc at 0(R30), argv[0] at 8(R30) -- confirmed
// via qemu-alpha execution of this exact file (not assumed from
// another arch: riscv64's own rt0.s needed a +8 correction nobody
// could explain from first principles, so this was checked directly
// rather than copied).
//
// TEXT _main(SB), $0 gets NO automatic stack frame (true Plan 9 leaf
// convention) -- the SUBQ below is a real, manual sp adjustment for
// main(argc, argv)'s own outgoing call, whose stack-passed second
// argument (argv, a pointer) needs a real 8-byte home slot at 16(R30)
// after that adjustment. Confirmed via a `zc -S` probe on an
// equivalent 2-arg call (docs/claude_notes/notes_arch_alpha.txt):
// zc's own caller-side codegen for "f(int, char**)" stores the second
// argument at exactly this offset within a $24 frame.
TEXT _main(SB), $0
	MOVQ	$setSB(SB), R29
	MOVQ	0(R30), R1		// argc (scratch; R0/REGARG is set just before the call below)
	MOVQ	$8(R30), R2		// argv = &rawstack[1] (scratch)
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
