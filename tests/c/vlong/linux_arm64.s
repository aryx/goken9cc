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
    // claude: fd is the 1st arg, passed in R0 by 7c (register, not stack),
    // so no load is needed. buf/count are at 8(FP)/16(FP): on arm64 there is
    // no return address pushed on the stack (it lives in LR), so FP=SP+8 and
    // the args are 8 lower than the amd64 (8/16/24) convention. See the
    // working macos_arm64.s stub which uses these same offsets.
    //MOV fd+0(FP), R0
    MOV buf+8(FP), R1
    // claude: MOVW not MOV: count is an int and the caller stores only 32 bits
    MOVW count+16(FP), R2
    MOV $64, R8         // syscall number: write
    SVC $0
    RETURN

TEXT    panic(SB), $0
        // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    RETURN
