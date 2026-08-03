// vlong operations (vlop) for riscv (rv32) -- the 64-bit multiply helper the
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
// Adapted from ~/xxx/miller-riscv/ROOT/sys/src/libc/riscv/vlop.s
// (Richard Miller's riscv Plan9 port -- the same source
// include/arch/riscv64/u.h's typedefs are credited to). XLEN there is a
// macro for the register width; expanded to 4 here, since this is the
// 32-bit target and lib_core/libc/arch/ has no such macro.
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

/* _mulv(&result, x.lo, x.hi, y.lo, y.hi) */
TEXT	_mulv(SB), $0
	MOVW	4(FP), R9	// x.lo
	MOVW	8(FP), R10	// x.hi
	MOVW	12(FP), R11	// y.lo
	MOVW	16(FP), R12	// y.hi
	MULHU	R11, R9, R14	// (x.lo*y.lo).hi
	MUL	R11, R9, R13	// (x.lo*y.lo).lo
	MUL	R10, R11, R15	// (x.hi*y.lo).lo
	ADD	R15, R14
	MUL	R9, R12, R15	// (x.lo*y.hi).lo
	ADD	R15, R14
	MOVW	R13, 0(R8)
	MOVW	R14, 4(R8)
	RET
