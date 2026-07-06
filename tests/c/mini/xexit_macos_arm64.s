TEXT    xexit+0(SB), $0
        // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $1, R16         // syscall number: exit (BSD numbering)
    SVC     $0x80
