// vlong operations (vlop) for power -- the 64-bit multiply helper the
// compiler frontends emit calls to. See arch/arm/vlop.s's own header
// comment for the general story (why this needs per-arch assembly
// while _addv/_subv stay portable C in port/vlrt.c).
//
// Classic 32-bit PowerPC has no 64x64 multiply, only 32x32->64 (a
// mullw/mulhwu pair for the low/high halves of one 32x32 product), so
// the 64x64->64 (truncated) product is built the standard way:
// result = lo(a.lo*b.lo) in the low word, and
// hi(a.lo*b.lo) + lo(a.lo*b.hi) + lo(a.hi*b.lo) in the high word
// (every other cross term only affects bits >= 64, discarded).
//
// dest (the result pointer) arrives in R3 (REGARG); a.lo/a.hi/b.lo/
// b.hi are the next four 4-byte stack args, at +4/+8/+12/+16(FP) --
// same "first arg's unused home slot pushes everything else out by 4
// bytes" convention as every other multi-arg stub in this tree (see
// tests/c/vlong/linux_power.s's own write() stub). Result stored
// hi-then-lo (offset 0/4): power's own struct Vlong is {hi;lo;}, not
// {lo;hi;} like arm's -- see port/vlrt.c's own big-endian comment,
// the same order tests/c/vlong/vlrt.c's own #ifdef power branch uses.
TEXT	_mulv(SB), 1, $0
	MOVW	l0+4(FP), R4	// a.lo
	MOVW	h0+8(FP), R5	// a.hi
	MOVW	l1+12(FP), R6	// b.lo
	MOVW	h1+16(FP), R7	// b.hi
	MULLW	R4, R6, R9	// result.lo = lo(a.lo * b.lo)
	MULHWU	R4, R6, R8	// hi(a.lo * b.lo), the high-word accumulator
	MULLW	R4, R7, R10	// lo(a.lo * b.hi)
	ADD	R10, R8
	MULLW	R5, R6, R10	// lo(a.hi * b.lo)
	ADD	R10, R8
	MOVW	R8, 0(R3)	// result.hi
	MOVW	R9, 4(R3)	// result.lo
	RETURN
