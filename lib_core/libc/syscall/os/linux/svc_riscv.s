// The only raw syscall entry point for linux/riscv (rv32): loads up to
// 6 args into the registers the kernel's ECALL handler expects
// (a7=R17=number, a0-a5=R10-R15=args) and traps. Every OS/arch's
// syscall wrappers (see syscall/os/$OS/) are generated thin C
// functions calling this, so this is the one place that ever needs
// hand-written assembly for this arch.
//
// note on the FP offsets below: like arm64/mips, only the *first*
// named parameter (num) arrives in a register (R8) -- confirmed
// against ic -S output, a compiler-generated callee only spills it to
// its home slot (arg+0(FP)) if its address is actually taken, which
// this hand-written function never does, so num must come from R8
// directly. Every *other* argument packs at FP+4, FP+8, ... (4-byte
// natural slots, no padding needed since every arg here is a plain
// `long`) -- see include/arch/riscv/u.h's own comment for the general
// packing rule (natural, except 8-byte-or-wider types need 8-byte
// alignment, not relevant to this all-`long` signature).
//
// return value: like every arch here, the caller expects this
// function's own `long` return in R8, not wherever ECALL happens to
// leave it (R10/a0) -- an explicit copy is needed.
TEXT _syscall6+0(SB), $0
	MOVW	R8, R17          // a7 = syscall number (num arrives in R8)
	MOVW	a1+4(FP), R10    // a0
	MOVW	a2+8(FP), R11    // a1
	MOVW	a3+12(FP), R12   // a2
	MOVW	a4+16(FP), R13   // a3
	MOVW	a5+20(FP), R14   // a4
	MOVW	a6+24(FP), R15   // a5
	ECALL
	MOVW	R10, R8          // return value: a0 -> R8
	RET
