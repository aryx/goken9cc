// SPARC32 Linux hello world. Same syscall convention as
// tests/s/exit/exit_linux_sparc.s (see that file's comment), but this
// one exercises the .data segment and SB-relative symbol addressing via
// R2 (REGSB, this port's "static pointer"), neither of which the
// exit(42) test touches.

TEXT _start+0(SB), $0

    MOVW $setSB(SB), R2   // static base, needed for $msg(SB)

    // write(int fd=1, buf=&msg, count=13)
    MOVW $1, R8      // o0 = fd = 1 (stdout)
    MOVW $msg(SB), R9 // o1 = buf = &msg
    MOVW $13, R10    // o2 = count = 13
    MOVW $4, R1      // g1 = syscall number = write (4)
    MOVW $0x10, R11  // scratch reg holding the trap number
    TA R11

    // exit(int status=0)
    MOVW $0, R8      // o0 = status = 0
    MOVW $1, R1      // g1 = syscall number = exit (1)
    MOVW $0x10, R11  // scratch reg holding the trap number
    TA R11
    RETURN            // not reached

// -------------------------------------------
// data section
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
