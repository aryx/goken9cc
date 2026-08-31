// Linux SPARC32 syscall ABI: g1 = syscall number, o0-o5 = args, "ta
// 0x10" traps into the kernel. Errors are signaled via the carry (icc.C)
// condition bit rather than a negative return value -- irrelevant here
// since exit() never returns.
//
// This backend's registers are a flat R0-R31 numbering straight onto
// the real SPARC register file (0-7=%g0-%g7, 8-15=%o0-%o7, 16-23=%l0-
// %l7, 24-31=%i0-%i7, see linkers/kl/asm.c's OP_IRR/OP_RRR macros,
// which pass p->from.reg/p->to.reg through unmodified into the rs1/rs2/
// rd instruction fields) -- so R1 is really %g1 and R8 is really %o0,
// regardless of this port's own REGSP/REGARG naming for compiler-
// generated code (irrelevant here, this is hand-written leaf code with
// no frame). kl's TA only assembles the register form (trap# = value
// of the given register, see kl/asm.c's "case 33: trap r"), so the
// 0x10 trap number has to be loaded into a scratch register first
// rather than written as a literal operand.

TEXT _start+0(SB), $0

    MOVW $42, R8    // o0 = exit code (arg0)
    MOVW $1, R1     // g1 = syscall number = exit (1)
    MOVW $0x10, R9  // scratch reg holding the trap number
    TA R9
    RETURN           // not reached
