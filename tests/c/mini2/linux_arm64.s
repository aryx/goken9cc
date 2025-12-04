TEXT _main+0(SB), $0
	MOV    $setSB(SB), R28
	BL main+0(SB)

TEXT    exit+0(SB), $0
    // syscall: exit(0)
    //MOV    0(FP), R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    RETURN // never reached

TEXT write+0(SB), $0        // NOSPLIT | DUPOK | NOPROF
    //MOV fd+0(FP), R0
    MOV buf+8(FP), R1
    MOV count+16(FP), R2
    MOV $64, R8         // syscall number: write
    SVC $0
    RETURN

TEXT panic(SB), $0
    // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    RETURN
