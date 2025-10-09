
TEXT xwrite+0(SB), $0
	//the first arg is passed via R1

        /* write(1, msg, len) */
        MOVW    $1, R4          // fd = 1 (stdout)
        MOVW    R1, R5
        MOVW    count+4(FP), R6 // count (second arg)
        MOVW    $4004, R2          // syscall number 4 = sys_write
        SYSCALL
	RET

