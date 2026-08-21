// The only raw syscall entry point for linux/power: loads up to 6 args
// into the registers PowerPC's `sc` trap expects (r0=number, r3-r8=
// args) and traps. Every OS/arch's syscall wrappers (see
// syscall/os/$OS/) are generated thin C functions calling this, so
// this is the one place that ever needs hand-written assembly for
// this arch.
//
// qc passes only the *first* named parameter (num) in a register (R3,
// REGARG=REGRET=3) -- like every other arch here, a compiler-generated
// function only spills it to its home slot if its address is actually
// taken, so this hand-written function (no compiler prologue) reads it
// straight from R3, never via FP. Every other argument is packed at
// its natural 4-byte width with no padding (power's pointers are 4
// bytes, SZ_IND==SZ_LONG), starting right after num's own (unused)
// home slot -- confirmed against tests/c/vlong/linux_power.s's own
// write() stub, which already relies on this same a1+4(FP)/a2+8(FP)
// offset convention.
//
// return value: like every arch here, the caller expects this
// function's own `long` result in R3 (REGRET) -- which, unlike mips's
// $v0/$v1 split, is the exact same register the kernel already leaves
// its own result in after `sc`. So unlike svc_mips.s, _syscall6 needs
// no explicit register COPY at all for the success path.
//
// error convention: PowerPC is one of the legacy BSD-shaped Linux
// syscall ABIs (like o32 mips and alpha), not the "negative errno in
// the result register" convention every other arch here uses. On
// error, `sc` leaves a POSITIVE errno in r3 *and* sets CR0's SO
// (Summary Overflow) bit; on success CR0.SO is clear. BVC/BVS (this
// assembler's names for "branch if CR0.SO clear/set" -- confirmed
// against linkers/ql/asmout.c's own encoding: BI=3 selects CR0's SO
// bit, matching real PowerPC ABI convention) test exactly that flag,
// and NEG (a real PowerPC instruction, unlike alpha's/mips's
// hardwired-zero-register SUBQ/SUBU trick -- PowerPC has no such
// register) negates r3 into itself when the flag is set.
TEXT _syscall6+0(SB), $0
	MOVW	R3, R0
	MOVW	a1+4(FP), R3
	MOVW	a2+8(FP), R4
	MOVW	a3+12(FP), R5
	MOVW	a4+16(FP), R6
	MOVW	a5+20(FP), R7
	MOVW	a6+24(FP), R8
	SYSCALL
	BVC	syscallok
	NEG	R3, R3
syscallok:
	RETURN
