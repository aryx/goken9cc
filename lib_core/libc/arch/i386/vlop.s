// vlong operations (vlop) for 386 -- the 64-bit multiply helper the
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
// Adapted from ~/principia/lib_core/libc/386/vlop.s.
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
	MOVL	r+0(FP), CX
	MOVL	a+4(FP), AX
	MULL	b+12(FP)
	MOVL	AX, 0(CX)
	MOVL	DX, BX
	MOVL	a+4(FP), AX
	MULL	b+16(FP)
	ADDL	AX, BX
	MOVL	a+8(FP), AX
	MULL	b+12(FP)
	ADDL	AX, BX
	MOVL	BX, 4(CX)
	RET
