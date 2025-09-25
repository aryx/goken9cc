TEXT xwrite+0(SB), 7, $0        // NOSPLIT | DUPOK | NOPROF
    MOV count+8(FP), R2
    MOV R0, R1 // buf
    MOV $1, R0          // fd = 1
    MOV $64, R8         // syscall number: write
    SVC $0
    //TODO? no RET?
    //RET
