// xwrite_arm.s — Linux ARM32 write(fd=1, buf, count)

TEXT xwrite+0(SB), 7, $0
        MOVW    $1, R0          // fd = 1 (stdout)
        MOVW    buf+0(FP), R1   // buf pointer (first arg)
        MOVW    count+4(FP), R2 // count (second arg)
        MOVW    $4, R7          // syscall number 4 = sys_write
        SWI     $0              // trap into kernel
	RET

