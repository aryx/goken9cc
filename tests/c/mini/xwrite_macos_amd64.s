TEXT xwrite+0(SB), $0
    MOVQ    buf+0(FP), SI      // 1st argument: buf
    // MOVL not MOVQ: len is an int and the caller (6c) stores only 32
    // bits on the stack (MOVL AX, 8(SP)); a 64-bit load picks up stale
    // garbage in the upper half and write() then fails with EINVAL,
    // writing nothing (the return value is unchecked, so the program
    // still exits 0). That garbage happened to be zero on older macOS,
    // so this latent bug only surfaced on macOS 26 (Tahoe). MOVL
    // zero-extends into the full RDX, matching the hand-written stub in
    // tests/s/mini/hello_macos_amd64.s and the arm64 MOVW in
    // xwrite_macos_arm64.s.
    MOVL    len+8(FP), DX      // 2nd argument: len (32-bit, zero-extended)
    MOVQ    $1, DI             // fd = 1 (stdout)
    // macOS (XNU): BSD syscall number gets the Unix class bit 0x2000000
    // OR'd in (see also tests/s/mini/hello_macos_amd64.s)
    MOVQ    $(0x2000000+4), AX // syscall number: write
    SYSCALL
    RET
