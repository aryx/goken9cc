// Process startup glue for darwin/amd64 -- unlike arch/amd64/rt0.s
// (linux/amd64, and every other GOOS/arch this project builds), this
// one is genuinely GOOS-specific, not shared, because dyld's LC_MAIN
// calling convention for amd64 is fundamentally different from a raw
// kernel ELF entry: dyld CALLs the entry point with real C-ABI
// register arguments (confirmed empirically with lldb, breaking
// exactly at this function's own first instruction rather than at
// dyld's own entry, which a plain `process launch --stop-at-entry`
// stops at instead and gives misleading register values for --
// RDI=argc, RSI=argv, RDX=envp, RCX=apple), matching Apple's
// documented "looks like main()" contract for LC_MAIN. Linux's raw
// ELF _start, by contrast, gets no such register setup at all --
// argc/argv/envp are already sitting on the stack at kernel-entry SP,
// which is why linux/amd64's rt0.s needs no bridging (see its own
// comment: 6c-compiled code, including this project's own main(),
// always expects arguments passed on the stack, by the immediate
// caller, never in registers -- and Linux's raw kernel stack layout
// already happens to satisfy that for argc at FP+0, which is the only
// thing any currently-wired-up test program actually reads).
//
// This bridges dyld's RDI(argc)/RSI(argv) into the stack slots a
// 6c-compiled main(int argc, char **argv) expects to find at its own
// FP+0/FP+8, so fib.c's `if (argc >= 2) n = atoi(argv[1])` works
// instead of reading dyld-internal stack contents as if they were
// argc/argv (which crashed with a real SIGSEGV before this fix --
// see docs/claude_notes/notes_libc_selfhost.txt for the full
// debugging story). envp/apple (RDX/RCX) are not forwarded: nothing
// built against this libc.a reads them yet, and Plan9 C's traditional
// main() signature doesn't have a third parameter for them anyway.
TEXT _main+0(SB), $0
	MOVQ	DI, AX
	MOVQ	SI, BX
	MOVQ	BX, _mainargv+0(SB)	// see port/mainargs.c
	SUBQ	$16, SP
	MOVQ	AX, 0(SP)
	MOVQ	BX, 8(SP)
	CALL	main+0(SB)
	MOVL	$0, (SP)
	CALL	exit+0(SB)
loop:
	JMP	loop
