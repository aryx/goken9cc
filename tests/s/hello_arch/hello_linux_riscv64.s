// RISC-V64 Linux hello world. Same syscall ABI as riscv32 (see
// hello_linux_riscv.s), but assembled/linked with ja/jl (thechar='j')
// instead of ia/il, producing a real 64-bit ELF (mkfiles/riscv64/mkfile).
//
// This is also the regression test for a real bug found while bringing
// riscv64 up: the MOVW $setSB(SB), R3 static-base load (and any other
// "MOVW $sym(SB), R" load of a symbol's address into a register) used to
// resolve to a bogus near-zero pointer instead of the symbol's real
// address, because it hit a linker case (optab.c's AMOVW/C_LECON, case 9
// in linkers/il/asm.c) that -- unlike AMOV's equivalent case 20 -- never
// added INITDAT and never emitted the auipc-relative form thechar=='j'
// needs. Fixed in linkers/il/obj.c's ldobj() by mirroring the existing
// thechar=='i' AMOVW-with-no-memory-operand rewrite for thechar=='j' too,
// so this now routes through the already-correct AMOV/case 20 path.
// exit_linux_riscv64.s (tests/s/exit) predates this fix and didn't catch
// it because it has no .data segment / no SB-relative symbol load at all.

TEXT _start(SB), $0

    MOVW    $setSB(SB), R3      // static base (gp), needed for $msg(SB)

    // write(int fd=1, buf=&msg, count=13)
    MOVW    $1, R10             // a0 = fd = 1 (stdout)
    MOVW    $msg(SB), R11       // a1 = buf = &msg
    MOVW    $13, R12            // a2 = count = 13
    MOVW    $64, R17            // a7 = syscall number: write
    ECALL

    // exit(int status=0)
    MOVW    $0, R10             // a0 = status = 0
    MOVW    $93, R17            // a7 = syscall number: exit
    ECALL

// -------------------------------------------
// data section
// -------------------------------------------
DATA    msg+0(SB)/8, $"Hello, w"
DATA    msg+8(SB)/6, $"orld\n\z"
GLOBL   msg(SB), $14
