TEXT _start+0(SB), $0

    // RISC-V64 Linux: same syscall ABI as riscv32 (a0=R10, a7=R17, exit=93);
    // this is assembled/linked with ja/jl (thechar='j'), not ia/il, which
    // produces a real 64-bit ELF (see mkfiles/riscv64/mkfile). MOVW is fine
    // here even in 64-bit mode: these are plain small-immediate register
    // loads (ADDI from x0), not memory-width-sensitive ops.
    MOVW $42, R10     // a0 = exit code
    MOVW $93, R17     // a7 = syscall number = exit (93)
    ECALL             // trap to kernel
    RET               // not reached
