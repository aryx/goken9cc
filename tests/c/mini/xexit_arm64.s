TEXT    xexit+0(SB), 7, $0
        // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0

