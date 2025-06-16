
TEXT xwrite+0(SB), 7, $0        // NOSPLIT | DUPOK | NOPROF

    MOVQ    $1, AX             // syscall number for write
    MOVQ    $1, DI             // fd = 1 (stdout)
    MOVQ    buf+0(FP), SI      // 1st argument: buf
    MOVQ    len+8(FP), DX      // 2nd argument: len
    SYSCALL
    RET

// -------------------------------------------
// to shup up the linker
// -------------------------------------------

TEXT runtime·morestack00(SB), 7, $0
    RET

TEXT runtime·morestack10(SB), 7, $0
    RET

TEXT runtime·morestack01(SB), 7, $0
    RET

TEXT runtime·morestack11(SB), 7, $0
    RET

TEXT runtime·morestack8(SB), 7, $0
    RET

TEXT runtime·morestack16(SB), 7, $0
    RET

TEXT runtime·morestack24(SB), 7, $0
    RET

TEXT runtime·morestack32(SB), 7, $0
    RET

TEXT runtime·morestack40(SB), 7, $0
    RET

TEXT runtime·morestack48(SB), 7, $0
    RET
