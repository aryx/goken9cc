// vlong operations (vlop) for arm -- the 64-bit multiply helper the
// compiler frontends emit calls to.
//
// compilers/cc/com64.c (and compilers/cck/com64.c, and upstream Plan9's
// own cc) turn "vlong * vlong" on a 32-bit arch into a call to _mulv,
// exactly as they turn + and - into _addv/_subv. The difference is that
// _addv/_subv can be written in portable C and live in port/vlrt.c,
// while a 64x64 multiply needs the machine's widening-multiply
// instruction (or a 16-bit-limb dance in C), so upstream Plan9 keeps it
// in per-arch assembly -- which is why this file exists at all and why
// there is no port/ equivalent.
//
// Adapted from ~/principia/lib_core/libc/arm/vlop.s.
// Only _mulv is taken: that file also carries _addv/_subv (and, on 386,
// _mul64by32/_div64by32), but port/vlrt.c already defines _addv/_subv in
// C here, so copying the whole thing would be a duplicate-symbol link
// error. The two 386-only helpers have no caller in this tree's vlrt.c.
//
// Nothing linked against the self-hosted libc had ever multiplied two
// vlongs before, so this was missing without anyone noticing until
// os/linux/time.c's nsec() did (sec * 1000000000LL); 5l reported
// "undefined: _mulv" followed by a cascade of bogus encoding errors on
// the unresolved BL.

TEXT	_mulv(SB), 1, $0
	MOVW	4(FP),R8	/* l0 */
	MOVW	8(FP),R11	/* h0 */
	MOVW	12(FP),R4	/* l1 */
	MOVW	16(FP),R5	/* h1 */
	MULLU	R8,R4,(R6, R7)	/* l0*l1 */
	MUL	R8,R5,R5	/* l0*h1 */
	MUL	R11,R4,R4	/* h0*l1 */
	ADD	R4,R6
	ADD	R5,R6
	MOVW	R6,4(R0)
	MOVW	R7,0(R0)
	RET
