TEXT    xexit+0(SB), $0

	MOVW    $0, R4              /* exit code                */
        MOVW    $4001, R2           /* syscall = exit           */
        SYSCALL
	RET // never reached
