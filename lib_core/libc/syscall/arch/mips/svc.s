// The only raw syscall entry point for linux/mips (o32): loads args
// into the registers the kernel's SYSCALL handler expects (R2=number,
// R4-R7=first 4 args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so this
// is the one place that ever needs hand-written assembly for this arch.
//
// note: real o32 Linux syscalls with a 5th/6th argument need those
// passed on the caller's stack (not a plain register), a wrinkle
// unique to mips among the archs handled so far. Not implemented here
// since neither syscall declared in syscall_linux_mips.decl (write,
// exit) needs more than 3 args -- if a future syscall does, this is
// the place to add it.
//
// note on the FP offsets below: like arm64, only the *first* named
// parameter (num) arrives in a register (R1) -- confirmed against
// vc -S output, a compiler-generated callee only spills it to its home
// slot (arg+0(FP)) if its address is actually taken, which this
// hand-written function never does, so num must come from R1 directly.
// Unlike arm64 (uniform 8-byte slots) though, every *other* argument
// here is packed at its natural 4-byte width with no padding, and
// critically the first argument's (unused) home slot still reserves
// FP+0 -- so a1 starts at FP+4, not FP+0 (confirmed against
// tests/c/mini2/linux_mips.s's write() stub, which already relies on
// this same offset for its own buf+4(FP)).
TEXT _syscall6+0(SB), $0
	MOVW	R1, R2
	MOVW	a1+4(FP), R4
	MOVW	a2+8(FP), R5
	MOVW	a3+12(FP), R6
	MOVW	a4+16(FP), R7
	SYSCALL
	RET
