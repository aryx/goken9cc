TEXT _start+0(SB), $0

    // RISC-V Linux: a0 = arg0, a7 = syscall number.
    // claude: the ia assembler maps register names directly (Rn -> xn), so
    // a0 is R10 (x10) and a7 is R17 (x17). The old code used R0/R7: R0 is the
    // hardwired zero register (the 42 was discarded) and R7 is x7/t2, not a7,
    // so ECALL ran with a garbage syscall number instead of exit(93). It fell
    // through to RET, which jumped to an unset ra -> segfault.
    MOVW $42, R10     // a0 = exit code
    MOVW $93, R17     // a7 = syscall number = exit (93)
    ECALL             // trap to kernel
    RET               // not reached
