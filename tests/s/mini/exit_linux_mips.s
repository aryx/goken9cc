TEXT _start+0(SB),7,$0

    MOVW $42, R4    // a0 = exit code
    MOVW $4001, R2  // v0 = syscall number (exit)
    SYSCALL
    RET              // not reached
