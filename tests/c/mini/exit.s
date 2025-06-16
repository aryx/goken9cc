TEXT    xexit+0(SB), 7, $0
        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL
