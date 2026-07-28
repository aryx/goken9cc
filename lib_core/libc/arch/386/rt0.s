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
TEXT _main+0(SB), $0
	CALL	main+0(SB)
	MOVL	$0, (SP)
	CALL	exit+0(SB)
loop:
	JMP	loop
