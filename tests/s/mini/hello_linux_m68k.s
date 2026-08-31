// m68k Linux hello world. Same syscall convention as
// tests/s/exit/exit_linux_m68k.s (see that file's comment), but this
// one exercises the .data segment and A6-relative symbol addressing,
// neither of which the exit(42) test touches.
//
// A6 plays the same "static base" role kl's REGSB (R2) plays for
// sparc: assemblers/2a/l.s's own "start" routine bootstraps it the
// same way (`MOVL $a6base(SB), A6`) before any bare `sym(SB)` memory
// reference can be used -- a6base is defined by linkers/2l/span.c as
// INITDAT+A6OFFSET, and asmea()'s D_STATIC/D_EXTERN case encodes a
// bare `sym(SB)` reference as a signed 16-bit displacement off A6
// (sym's value + offset - A6OFFSET), so A6 must hold that bias before
// the displacement can resolve to the real address.

TEXT _start+0(SB), $0

    MOVL $a6base(SB), A6   // static base, needed for bare sym(SB) refs

    LEA  msg(SB), A1       // A1 = &msg (LEA never dereferences)

    // write(int fd=1, buf=&msg, count=13)
    MOVL $1, R1      // d1 = fd = 1 (stdout)
    MOVL A1, R2      // d2 = buf = &msg
    MOVL $13, R3     // d3 = count = 13
    MOVL $4, R0      // d0 = syscall number = write (4)
    TRAP $0

    // exit(int status=0)
    MOVL $0, R1      // d1 = status = 0
    MOVL $1, R0      // d0 = syscall number = exit (1)
    TRAP $0
    RTS               // not reached

// -------------------------------------------
// data section
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
