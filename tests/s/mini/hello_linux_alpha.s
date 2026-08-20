// Alpha Linux hello world. Same CALL_PAL/callsys syscall convention as
// tests/s/exit/exit_linux_alpha.s (see that file's comment), but this one
// exercises the .data segment and SB-relative symbol addressing, neither
// of which the exit(42) test touches -- see docs/claude_notes/
// notes_arch_alpha.txt's "what's actually verified" section.

TEXT _start+0(SB), $0

    MOVQ    $setSB(SB), R29     // static base (gp), needed for $msg(SB)

    // write(int fd=1, buf=&msg, count=13)
    MOVQ    $1, R16             // a0 = fd = 1 (stdout)
    MOVQ    $msg(SB), R17       // a1 = buf = &msg
    MOVQ    $13, R18            // a2 = count = 13
    MOVQ    $4, R0              // v0 = syscall number = write (4)
    CALL_PAL $0x83              // callsys

    // exit(int status=0)
    MOVQ    $0, R16             // a0 = status = 0
    MOVQ    $1, R0              // v0 = syscall number = exit (1)
    CALL_PAL $0x83              // callsys

// -------------------------------------------
// data section
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/5, $"orld\n"
GLOBL   msg(SB), $13
