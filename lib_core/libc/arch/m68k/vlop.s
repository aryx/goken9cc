// vlong operations (vlop) for m68k -- the 64-bit multiply helper the
// compiler frontends emit calls to (compilers/cck/com64.c's own
// `nodmulv = fvn("_mulv", TVLONG);`). See arch/power/vlop.s's own
// header comment for the general story (why this needs per-arch
// assembly while _addv/_subv stay portable C in port/vlrt.c).
//
// Ported near-verbatim from ~/xxx/plan9/SRC/libc/68020/vlop.s (the
// real Plan9 68020 port), not re-derived: real 68020 hardware DOES
// have a genuine widening 32x32->64 MULU.L (Dh:Dl destination pair),
// but even Plan9's OWN original assembler couldn't express that form
// through its normal MULUL grammar -- hence the raw WORD-encoded
// instruction below and its own "philw made me do it!" comment,
// carried over unchanged. This project's own 2a has the identical gap
// (linkers/2l/asm.c's case 14 encoder always clears the size bit, so
// a plain "MULUL src,Rn" only ever gives the truncated 32-bit result
// -- confirmed empirically before finding this reference, see
// notes_arch_m68k.txt for that dead-end and why the raw WORD encoding
// below was tried directly instead of re-deriving a software
// widening multiply from 16-bit limbs). Verified against this port's
// own FP-offset conventions (not assumed identical to the reference
// just because the offsets happen to read the same -- confirmed via
// the same hand-written debug-echo probe technique used to verify
// sparc's own vlop.s, then cross-checked end to end against Python
// with two values including the 0xffffffff*0xffffffff maximum-carry
// case) -- both matched exactly with zero changes needed beyond the
// FP-offset label renaming already established for this file (r+0,
// a.hi+4/a.lo+8, b.hi+12/b.lo+16 -- see this file's own history for
// where those were pinned down).
//
// dest (the result pointer) arrives at r+0(FP); a.hi/a.lo/b.hi/b.lo
// are the next four 4-byte stack slots. Result stored hi-then-lo
// (offset 0/4): m68k's own struct Vlong is {hi;lo;} like sparc/
// power's, not {lo;hi;} like arm's -- see port/vlrt.c's own #ifdef
// m68k branch, and the reference's own vlrt.c confirms the same
// layout independently.
TEXT	_mulv(SB), $0
	MOVL	r+0(FP), A0
	MOVL	a2+8(FP), R0		// a.lo

	WORD	$0x4c2f
	WORD	$0x0401
	WORD	$0x0014
	// MULUL	b2+16(FP), R0:R1 -- 68020's real widening 32x32->64
	// unsigned multiply (a.lo * b.lo), hand-encoded because this
	// assembler's MULUL grammar has no way to select the 64-bit
	// result form (see this file's own header comment). R1 ends up
	// with the high 32 bits, R0 the low 32 bits -- confirmed
	// empirically via the store order below and end-to-end testing,
	// not read off the raw encoding (the extension word's Dh/Dl bit
	// assignment is easy to misread by inspection; trust the test).

	MOVL	a1+4(FP), R2		// a.hi
	MULUL	b2+16(FP), R2		// lo(a.hi * b.lo)
	ADDL	R2, R1

	MOVL	a2+8(FP), R2		// a.lo
	MULUL	b1+12(FP), R2		// lo(a.lo * b.hi)
	ADDL	R2, R1

	MOVL	R1, (A0)+		// result.hi
	MOVL	R0, (A0)		// result.lo
	RTS
