TEXT xwrite+0(SB), $0
    // first arg in R0
    //old: was MOV, but count is an int and the caller (7c) stores only
    // 32 bits on the stack; the 64-bit load worked by luck on Linux
    // (fresh stack happens to be zero) but not on macOS where dyld
    // leaves garbage above the stored word (write fails with EINVAL)
    MOVW count+8(FP), R2
    MOV R0, R1 // buf
    MOV $1, R0          // fd = 1
    MOV $64, R8         // syscall number: write
    SVC $0
    RETURN
