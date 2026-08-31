// vlong operations (vlop) for sparc -- the 64-bit multiply helper the
// compiler frontends emit calls to (compilers/cck/com64.c's own
// `nodmulv = fvn("_mulv", TVLONG);`). See arch/power/vlop.s's own
// header comment for the general story (why this needs per-arch
// assembly while _addv/_subv stay portable C in port/vlrt.c).
//
// SPARC v8 has a real 32x32->64 unsigned multiply (UMUL): the low 32
// bits land in the destination register, the high 32 bits land in the
// %y register (read back via "MOVW Y,Rn", the same D_Y pseudo-
// register k.out.h/kl already expose for kl's own division opcode
// encoding -- see notes_arch_sparc.txt's DIV case-52 decode). kc's own
// AMUL opcode already maps to real UMUL (confirmed empirically: the
// mini2 fact()/fact_iter() bring-up needed mkfiles/sparc/mkfile's own
// -M flag specifically so AMUL/ADIV reach this real hardware encoding
// instead of a nonexistent software _mul/_div call -- see that
// mkfile's own comment). So the 64x64->64 (truncated) product is the
// same standard construction power's own vlop.s uses: result.lo =
// lo(a.lo*b.lo), result.hi = hi(a.lo*b.lo) + lo(a.lo*b.hi) +
// lo(a.hi*b.lo) (every other cross term only affects bits >= 64,
// discarded).
//
// dest (the result pointer) arrives in R7 (REGARG); a and b are each
// passed as ONE composite 8-byte stack argument (not four separate
// scalar longs the way this comment originally assumed, copying
// power's own l0+4/h0+8/l1+12/h1+16 labeling uncritically) -- sparc
// is big-endian, so an 8-byte argument's HIGH word lands at the LOWER
// address, i.e. a.hi is at +4(FP) and a.lo at +8(FP), b.hi at +12(FP)
// and b.lo at +16(FP) (each still a plain 4-byte-per-slot stack step,
// same convention as every other multi-arg stub in this tree -- see
// tests/c/vlong/linux_sparc.s's own write() stub -- just with hi/lo
// SWAPPED from the little-endian-first assumption power's own
// comment, and this file's own first version, made). Confirmed
// empirically with a hand-written debug stub echoing each offset back
// out and a distinguishable test value (0x1111222233334444) rather
// than re-guessed -- see docs/claude_notes/notes_arch_sparc.txt for
// the full account of how the original (little-endian-labeled)
// version silently computed the wrong product (small values like the
// mini2 fact() smoke test never exercised this since the "wrong"
// half of a small a.hi=0/b.hi=0 argument is still zero either way).
// Result stored hi-then-lo (offset 0/4) in the OUTPUT struct too:
// sparc's own struct Vlong is {hi;lo;} like power's, not {lo;hi;}
// like arm's -- see tests/c/vlong/vlrt.c's and lib_core/libc/port/
// vlrt.c's own #ifdef sparc branches.
//
// Genuinely leaf (no calls inside), so R1/REGSP is never touched --
// this backend's own auto-prologue-vs-leaf concerns (notes_arch_sparc.txt)
// don't apply here at all.
TEXT	_mulv(SB), $0
	MOVW	h0+4(FP), R8	// a.hi
	MOVW	l0+8(FP), R9	// a.lo
	MOVW	h1+12(FP), R10	// b.hi
	MOVW	l1+16(FP), R11	// b.lo
	MUL	R9, R11, R12	// result.lo = lo(a.lo * b.lo)
	MOVW	Y, R13		// hi(a.lo * b.lo), the high-word accumulator
	MUL	R9, R10, R6	// lo(a.lo * b.hi)
	ADD	R6, R13
	MUL	R8, R11, R6	// lo(a.hi * b.lo)
	ADD	R6, R13
	MOVW	R13, 0(R7)	// result->hi
	MOVW	R12, 4(R7)	// result->lo
	RETURN
