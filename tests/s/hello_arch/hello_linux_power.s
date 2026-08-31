// PowerPC Linux hello world. Same syscall convention as
// tests/s/exit/exit_linux_power.s (see that file's comment), but this
// one exercises the .data segment and SB-relative symbol addressing,
// neither of which the exit(42) test touches.

TEXT _start+0(SB), $0

    MOVW    $setSB(SB), R2      // static base, needed for $msg(SB)

    // write(int fd=1, buf=&msg, count=13)
    MOVW    $1, R3              // r3 = fd = 1 (stdout)
    MOVW    $msg(SB), R4        // r4 = buf = &msg
    MOVW    $13, R5             // r5 = count = 13
    MOVW    $4, R0              // r0 = syscall number = write (4)
    SYSCALL

    // exit(int status=0)
    MOVW    $0, R3              // r3 = status = 0
    MOVW    $1, R0              // r0 = syscall number = exit (1)
    SYSCALL
    RETURN                       // not reached

// -------------------------------------------
// data section
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
