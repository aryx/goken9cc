// setjmp()/longjmp() for m68k -- needed unconditionally by
// os/linux/notify.c's own noted() (same as every other arch's own
// arch/$cputype/setjmp.s). Ported verbatim (register/offset shape
// unchanged, only comments added) from ~/xxx/plan9/SRC/libc/68020/
// setjmp.s, the real Plan9 68020 port -- m68k's real hardware SP (A7)
// is exactly what kc's own $FP addressing already assumes, no
// REGSP/logical-vs-real-SP bridging needed the way sparc's own
// version needs, so there was nothing arch-specific left to
// re-derive here.
//
// setjmp saves A7 itself (as it stands AT ENTRY, pointing at the
// return address the caller's BSR just pushed) into env[0], then the
// return address value at that location into env[1]. longjmp restores
// A7 from env[0] -- landing back on that SAME return-address slot --
// then overwrites the value THERE (not A7 itself again) with env[1],
// so the trailing RTS pops the right target. Cheaper than a separate
// indirect JMP: no extra register needed to hold the jump target.
TEXT setjmp(SB), $0
	MOVL	env+0(FP), A0
	MOVL	A7, (A0)+		// env[0] = our own entry SP
	MOVL	(A7), (A0)		// env[1] = return address at that SP
	CLRL	R0			// setjmp() returns 0 on the direct call
	RTS

TEXT longjmp(SB), $0
	MOVL	env+0(FP), A0
	MOVL	val+4(FP), R0
	BNE	nonzero			// MOVL above already set N/Z on R0
	MOVL	$1, R0			// ansi: longjmp(env, 0) behaves as longjmp(env, 1)
nonzero:
	MOVL	(A0)+, A7		// restore the saved entry SP
	MOVL	(A0), (A7)		// patch its return-address slot with env[1]
	RTS
