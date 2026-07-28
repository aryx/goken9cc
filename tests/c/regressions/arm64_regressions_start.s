// Minimal linux/arm64 runtime stub, shared by this directory's arm64
// regressions that need neither write() nor a full libc: just
// _main -> main() and exit(). Modeled on tests/c/mini2/linux_arm64.s
// (same file, minus write()) -- note exit's own body never needs to
// read its "code" argument explicitly: arm64's calling convention
// delivers a function's first argument in R0, which is also exactly
// where the SVC exit syscall itself expects the exit status, and
// nothing in between touches R0, so it passes straight through.

TEXT _main+0(SB), $0
	MOV	$setSB(SB), R28
	BL	main+0(SB)

TEXT exit+0(SB), $0
	MOV	$93, R8		// syscall number: exit
	SVC	$0
	RETURN			// never reached
