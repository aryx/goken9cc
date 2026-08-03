// vlong operations (vlop) for mips -- the 64-bit multiply helper the
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
// Adapted from plan9front's sys/src/libc/mips/vlop.s (principia has no mips libc).
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

TEXT	_mulv(SB), $0
	MOVW	8(FP), R2
	MOVW	4(FP), R3
	MOVW	16(FP), R4
	MOVW	12(FP), R5
	MULU	R4, R2
	MOVW	LO, R6
	MOVW	HI, R7
	MULU	R3, R4
	MOVW	LO, R8
	ADDU	R8, R7
	MULU	R2, R5
	MOVW	LO, R8
	ADDU	R8, R7
	MOVW	R6, 4(R1)
	MOVW	R7, 0(R1)
	RET
