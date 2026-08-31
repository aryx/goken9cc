// Linux m68k syscall ABI: d0 = syscall number, d1-d5/a0 = args, "trap
// #0" traps into the kernel. Errors are signaled by a return value in
// -4095..-1 rather than a separate flag -- irrelevant here since exit()
// never returns.
//
// This backend names the data registers R0-R7 (not D0-D7, see
// assemblers/2a/lex.c's own keyword table), but they map straight onto
// the real hardware d0-d7 -- no renaming layer like sparc's kl.

TEXT _start+0(SB), $0

    MOVL $42, R1    // d1 = exit code (arg0)
    MOVL $1, R0     // d0 = syscall number = exit (1)
    TRAP $0
    RTS              // not reached
