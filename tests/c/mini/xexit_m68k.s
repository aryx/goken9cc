// xexit_m68k.s -- Linux m68k exit(status=0)

TEXT xexit+0(SB), $0
	MOVL	$0, R1     // d1 = status = 0
	MOVL	$1, R0     // d0 = syscall number = exit (1)
	TRAP	$0
	RTS               // not reached
