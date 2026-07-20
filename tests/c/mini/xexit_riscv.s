TEXT xexit+0(SB), $0
	// xexit() -> exit(0). a0=R10 status, a7=R17 syscall number, exit=93.
	MOVW	$0, R10             // a0 = status = 0
	MOVW	$93, R17            // a7 = syscall number: exit
	ECALL
