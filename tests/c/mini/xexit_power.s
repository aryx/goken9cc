TEXT	xexit+0(SB), $0
	MOVW	$0, R3          // status = 0
	MOVW	$1, R0          // syscall number = exit (1)
	SYSCALL
	RETURN // never reached
