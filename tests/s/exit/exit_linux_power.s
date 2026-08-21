// PowerPC Linux syscall ABI: r0 = syscall number, r3-r8 = args,
// "sc" (trap) instruction. Return value in r3; unlike most other Linux
// syscall ABIs, errors are signaled via the CR0 SO (summary overflow)
// bit rather than a negative r3 -- irrelevant here since exit() never
// returns.

TEXT _start+0(SB), $0

    MOVW $42, R3    // r3 = exit code (arg0)
    MOVW $1, R0     // r0 = syscall number = exit (1)
    SYSCALL
    RETURN           // not reached
