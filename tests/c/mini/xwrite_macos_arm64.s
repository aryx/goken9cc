TEXT xwrite+0(SB), $0
    // first arg in R0
    // MOVW not MOV: count is an int and the caller (7c) stores only 32
    // bits on the stack; a 64-bit load picks up garbage in the upper
    // half (left there by dyld) and write() then fails with EINVAL
    MOVW count+8(FP), R2
    MOV R0, R1 // buf
    MOV $1, R0          // fd = 1
    // macOS (XNU): BSD syscall number in R16 (not R8), trap is SVC $0x80
    // (see also tests/s/mini/hello_macos_arm64.s)
    MOV $4, R16         // syscall number: write
    SVC $0x80
    RETURN
