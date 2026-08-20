TEXT xexit+0(SB), $0
	// xexit() -> exit(0). a0=R16 status, v0=R0 syscall number, exit=1.
	MOVQ	$0, R16             // a0 = status = 0
	MOVQ	$1, R0              // v0 = syscall number: exit (1)
	CALL_PAL $0x83              // callsys
