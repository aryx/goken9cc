// macOS (XNU) arm64 syscall stubs: the BSD syscall number goes in R16
// (not R8 as on Linux), the trap is SVC $0x80, and the numbers follow
// the BSD numbering (write=4, exit=1). See docs/notes_macos.txt.
//old: the linux_arm64.s stub here reads buf+16(FP)/count+24(FP) but
// those offsets look stale (arm64 was never working); the verified
// convention (see ../mini2/macos_arm64.s) is buf+8(FP)/count+16(FP)
// with the first argument in R0.
TEXT _main+0(SB), $0
	MOV    $setSB(SB), R28
	BL main+0(SB)

TEXT    exit+0(SB), $0
    // syscall: exit(0)
    //MOV    0(FP), R0          // status = 0
    MOV    $1, R16         // syscall number: exit
    SVC     $0x80
    RETURN // never reached

TEXT write+0(SB), $0        // NOSPLIT | DUPOK | NOPROF
    //MOV fd+0(FP), R0
    MOV buf+8(FP), R1
    // MOVW not MOV: count is an int and the caller stores only 32 bits
    // (see ../mini/xwrite_arm64.s for the whole story)
    MOVW count+16(FP), R2
    MOV $4, R16         // syscall number: write
    SVC $0x80
    RETURN

TEXT panic(SB), $0
    // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $1, R16         // syscall number: exit
    SVC     $0x80
    RETURN
