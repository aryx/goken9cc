TEXT xwrite+0(SB), $0
    MOVQ    buf+0(FP), SI      // 1st argument: buf
    MOVQ    len+8(FP), DX      // 2nd argument: len
    MOVQ    $1, DI             // fd = 1 (stdout)
    // macOS (XNU): BSD syscall number gets the Unix class bit 0x2000000
    // OR'd in (see also tests/s/mini/hello_macos_amd64.s)
    MOVQ    $(0x2000000+4), AX // syscall number: write
    SYSCALL
    RET
