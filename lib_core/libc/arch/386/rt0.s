// Process startup glue for linux/386, built once into libc.a instead
// of pasted per test (compare tests/c/mini2/linux_386.s's _main
// block). Like amd64, 386 needs no explicit SB/global-pointer register
// setup here (confirmed against 8c -S output: 8c-generated code never
// relies on one) -- just call user main(), and fall back to exit(0)
// if main() returns without calling exit() itself.
//
// note: like amd64, 386 has no register-passed argument at all
// (confirmed against 8c -S: every argument, including a callee's
// first, is written to the stack by the *caller* before the call) --
// so exit's code argument goes at 0(SP), not into some register.
//
// claude: argc/argv bridge, same shape as arch/amd64/rt0.s's (see its
// own longer comment for the why): at _main, 0(SP) is the kernel's
// argc and 4(SP) onward is argv[0].. (4-byte slots here, not 8 --
// ptrsize on this arch), same layout Go's own GO/pkg/runtime/386/
// asm.s _rt0_386 reads (minus the TLS/scheduler setup around it, which
// this libc has no equivalent of). Read argc/argv into registers
// before opening fresh stack space for them, so the SUBL doesn't
// self-clobber the very values being copied.
//
// claude: same "not strictly load-bearing" note as arch/amd64/rt0.s's:
// CALL's own return-address push doesn't disturb 0(SP)/4(SP), so a
// bare "CALL main" would already have worked here too. Added for the
// same explicit, no-coincidences consistency with every other arch.
TEXT _main+0(SB), $0
	MOVL	0(SP), AX	// argc
	LEAL	4(SP), BX	// argv
	MOVL	BX, _mainargv+0(SB)	// see port/mainargs.c
	SUBL	$8, SP
	MOVL	AX, 0(SP)
	MOVL	BX, 4(SP)
	CALL	main+0(SB)
	MOVL	$0, (SP)
	CALL	exit+0(SB)
loop:
	JMP	loop
