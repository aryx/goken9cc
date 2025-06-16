// 7 below is NOPROF | DUPOK | NOSPLIT
// and NOSPLIT is what matters and tells 6l to not do go-specific stack stuff

// -------------------------------------------
// main procedure
// -------------------------------------------

// we use _start and not main to avoid triggering Go-specific stuff in 6l
TEXT    _start+0(SB), 7, $0

        // Allocate space for return address (CALL pushes return addr)
        SUBQ    $16, SP         // make space: 8 bytes each for buf, len

        LEAQ    msg(SB), AX     // get pointer to message
        MOVQ    AX, 0(SP)       // push buf (arg 1)
        MOVQ    $14, 8(SP)      // push len (arg 2)

        CALL    write(SB)

        ADDQ    $16, SP         // clean up stack

        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL

// -------------------------------------------
// write(buf *byte, len int)
// -------------------------------------------
TEXT    write+0(SB), 7, $0

        MOVQ    $1, AX          // syscall: write
        MOVQ    $1, DI          // fd = 1 (stdout)

        MOVQ    buf+0(FP), SI   // arg 1: buf
        MOVQ    len+8(FP), DX   // arg 2: len

        SYSCALL
        RET


// -------------------------------------------
// to shup up the linker
// -------------------------------------------

TEXT runtime·morestack00(SB), 7, $0
    RET

TEXT runtime·morestack10(SB), 7, $0
    RET

TEXT runtime·morestack01(SB), 7, $0
    RET

TEXT runtime·morestack11(SB), 7, $0
    RET

TEXT runtime·morestack8(SB), 7, $0
    RET

TEXT runtime·morestack16(SB), 7, $0
    RET

TEXT runtime·morestack24(SB), 7, $0
    RET

TEXT runtime·morestack32(SB), 7, $0
    RET

TEXT runtime·morestack40(SB), 7, $0
    RET

TEXT runtime·morestack48(SB), 7, $0
    RET

// -------------------------------------------
// msg: must split into 8-byte chunks
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/6, $"orld\n"
GLOBL   msg(SB), $14
