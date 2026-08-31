// Process startup glue for linux/m68k, built once into libc.a instead
// of pasted per test (compare tests/c/mini/start_m68k.s's own, much
// simpler _start block, and this arch's own docs/claude_notes/
// notes_arch_m68k.txt for the A6/a6base background this file leans
// on). Sets up the SB base register (A6), reads argc/argv straight
// off the raw kernel-provided stack, stashes argv for port/
// mainargs.c, calls user main(argc, argv), and falls back to
// exit(0)/an infinite loop if main() returns without calling exit()
// itself.
//
// Unlike sparc's own rt0.s (docs/claude_notes/notes_arch_sparc.txt),
// this arch has NO leaf-vs-non-leaf constraint to work around: 2c's
// own calling convention already uses the real hardware A7/%sp
// directly (no fake REGSP the kernel never initializes), and 2l has
// no auto-prologue-insertion pass at all (unlike kl/ql/vl's noop.c --
// confirmed by grepping linkers/2l/ for one: none exists). So a
// genuine BSR-then-return call to main() is safe from the very first
// instruction, same shape as arch/power/rt0.s/arch/mips/rt0.s.
//
// argc/argv bridge: at the true process entry point (before this
// TEXT's own first instruction runs), A7 points directly at the
// kernel's raw argc word with NO shift to account for -- no
// auto-prologue has run yet to push anything first (confirmed via
// 2c -S on a hand-written 2-arg function call site: the caller pushes
// arguments right-to-left with plain "MOVL x,TOS", the pseudo-
// destination for a predecrement push, matching TOS's own use below
// to set up main(argc, argv)'s own two stack arguments).
TEXT _main(SB), $0
	MOVL	$a6base(SB), A6

	MOVL	0(A7), R1		// argc
	MOVL	A7, R2
	ADDL	$4, R2			// argv = &rawstack[1]
	MOVL	R2, _mainargv+0(SB)	// see port/mainargs.c
	MOVL	R1, _mainargc+0(SB)	// see port/mainargs.c's own _mainargc comment

	MOVL	R2, TOS			// push argv (2nd arg, pushed first)
	MOVL	R1, TOS			// push argc (1st arg, pushed second)
	BSR	main(SB)
	MOVL	$0, TOS
	BSR	exit(SB)
loop:
	BRA	loop
