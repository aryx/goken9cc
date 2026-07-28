// The only raw syscall entry point for linux/arm: loads up to 6 args
// into the registers the kernel's SWI handler expects (r7=number,
// r0-r5=args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so
// this is the one place that ever needs hand-written assembly for
// this arch.
//
// note on the FP offsets below: only the *first* named parameter
// (num) arrives in a register (R0) -- confirmed against 5c -S output,
// a compiler-generated callee only spills it to its home slot
// (arg+0(FP)) if its address is actually taken, which this
// hand-written function never does, so num must come from R0
// directly. Every *other* argument packs at FP+4, FP+8, ... (4-byte
// natural slots, no padding needed since every arg here is a plain
// `long`).
//
// no explicit return-value copy needed: R0 is both this arch's C ABI
// return register and where SWI leaves the syscall's result (same
// register the first argument arrived in).
TEXT _syscall6+0(SB), $0
	MOVW	R0, R7           // r7 = syscall number (num arrives in R0)
	MOVW	a1+4(FP), R0     // r0 (overwrites num, no longer needed)
	MOVW	a2+8(FP), R1     // r1
	MOVW	a3+12(FP), R2    // r2
	MOVW	a4+16(FP), R3    // r3
	MOVW	a5+20(FP), R4    // r4
	MOVW	a6+24(FP), R5    // r5
	SWI	$0
	RET
