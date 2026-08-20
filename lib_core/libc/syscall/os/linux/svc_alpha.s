// The only raw syscall entry point for linux/alpha: loads up to 6 args
// into the registers Alpha's callsys trap expects (v0/R0=number,
// a0-a5=R16-R21) and traps via CALL_PAL $0x83. Every OS/arch's syscall
// wrappers (see syscall/os/$OS/) are generated thin C functions
// calling this, so this is the one place that ever needs hand-written
// assembly for this arch.
//
// zc passes only the *first* named parameter (num) in a register (R0,
// REGARG) -- like every other arch here, a compiler-generated function
// only spills it to its home slot if its address is actually taken, so
// this hand-written function (no compiler prologue) reads it straight
// from R0, never via FP. num therefore needs no explicit move at all:
// it already sits in R0/v0, exactly where CALL_PAL expects the syscall
// number.
//
// a1..a6 are declared `vlong` (8 bytes), not `long` (4 bytes) -- see
// syscall_linux_alpha.h's own comment for why: this compiler's `long`
// stays 4 bytes even on this 64-bit arch, so a syscall argument that's
// actually a pointer (e.g. write()'s buf) would get silently
// truncated passing through a `long`-typed a1. Confirmed via zc -S
// that each vlong argument after num lands 8 bytes apart starting at
// FP+8 (a1+8(FP), a2+16(FP), ... a6+48(FP)) -- the same uniform 8-byte
// packing this arch already uses whenever a pointer-or-wider argument
// precedes, see docs/claude_notes/notes_arch_alpha.txt.
//
// return value: like every arch here, the caller expects this
// function's own `long` return in R0 (REGRET) -- which, unlike every
// OTHER arch here, is the exact same register CALL_PAL already leaves
// the kernel's result in (v0/R0 doubles as both the syscall-number-in
// and result-out register on this ABI). So unlike riscv64/arm64/amd64's
// svc_$cputype.s, _syscall6 needs no explicit return-value copy at
// all, and _syscall6v (needed by lseek/brk for a real 64-bit result,
// see syscall_linux_alpha.h's own comment) is not just similar to
// _syscall6 but byte-for-byte IDENTICAL: whether the caller treats the
// `long` or `vlong`-typed return as significant, the full 64 bits
// CALL_PAL leaves in R0 are already there either way, nothing to
// truncate or widen.
TEXT _syscall6+0(SB), $0
	MOVQ	a1+8(FP), R16
	MOVQ	a2+16(FP), R17
	MOVQ	a3+24(FP), R18
	MOVQ	a4+32(FP), R19
	MOVQ	a5+40(FP), R20
	MOVQ	a6+48(FP), R21
	CALL_PAL $0x83
	RET

TEXT _syscall6v+0(SB), $0
	MOVQ	a1+8(FP), R16
	MOVQ	a2+16(FP), R17
	MOVQ	a3+24(FP), R18
	MOVQ	a4+32(FP), R19
	MOVQ	a5+40(FP), R20
	MOVQ	a6+48(FP), R21
	CALL_PAL $0x83
	RET
