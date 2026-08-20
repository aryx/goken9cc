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
// svc_$cputype.s, _syscall6 needs no explicit register COPY at all --
// but it does need the same error-convention FIXUP syscall/os/linux/
// svc_mips.s already documents for o32 mips, and for the same reason:
// alpha is one of the legacy BSD-shaped Linux syscall ABIs (like o32
// mips), not the "negative errno in the result register" convention
// every OTHER arch here uses. On error, CALL_PAL leaves a POSITIVE
// errno in v0/R0 *and* signals failure out-of-band in a3/R19 (0 on
// success, nonzero on error) -- the exact same a3-as-flag shape as
// mips's $a3/R7, just under Alpha's own register name for the 4th
// argument slot (a0-a3 = R16-R19, so R19 doubles as both this
// function's own incoming `a4` parameter *and* the kernel's
// post-syscall error flag, same dual-purpose-register shape as mips's
// own a5/a3 collision). Without this, access() on a missing file
// returned +2 (ENOENT) -- >= 0, i.e. success -- the same class of bug
// svc_mips.s's own comment describes finding via tests/c/hello_libc/
// dir.c, and found here the same way (dir.c/fd.c/proc.c all check a
// *failing* syscall's return value, which hello.c/io.c never do).
// R31 is this arch's hardwired zero register (z.out.h's REGZERO), and
// this assembler's SUBQ is "dst = src1 - src2" written
// `SUBQ src2, src1, dst` (confirmed against arch/alpha/div.s's own
// `SUBQ R27, R31, R27`), so `SUBQ R0, R31, R0` negates v0 into itself
// (0 - errno = -errno) when the error flag is set.
TEXT _syscall6+0(SB), $0
	MOVQ	a1+8(FP), R16
	MOVQ	a2+16(FP), R17
	MOVQ	a3+24(FP), R18
	MOVQ	a4+32(FP), R19
	MOVQ	a5+40(FP), R20
	MOVQ	a6+48(FP), R21
	CALL_PAL $0x83
	BEQ	R19, syscallok
	SUBQ	R0, R31, R0
syscallok:
	RET

TEXT _syscall6v+0(SB), $0
	MOVQ	a1+8(FP), R16
	MOVQ	a2+16(FP), R17
	MOVQ	a3+24(FP), R18
	MOVQ	a4+32(FP), R19
	MOVQ	a5+40(FP), R20
	MOVQ	a6+48(FP), R21
	CALL_PAL $0x83
	BEQ	R19, syscallokv
	SUBQ	R0, R31, R0
syscallokv:
	RET
