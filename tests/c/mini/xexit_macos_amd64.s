TEXT    xexit+0(SB), $0
        // syscall: exit(0)
        MOVQ    $0, DI             // status = 0
        MOVQ    $(0x2000000+1), AX // syscall number: exit (BSD numbering)
        SYSCALL
