
TEXT xwrite+0(SB), 7, $0        // NOSPLIT | DUPOK | NOPROF
    MOVQ    $1, AX             // syscall number for write
    MOVQ    $1, DI             // fd = 1 (stdout)
    MOVQ    buf+0(FP), SI      // 1st argument: buf
    MOVQ    len+8(FP), DX      // 2nd argument: len
    SYSCALL
    RET
