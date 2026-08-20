// Software integer divide/modulo for alpha: real Alpha hardware has no
// integer divide instruction at all, so zc's own ADIVQ/AMODQ/ADIVL/
// AMODL (etc.) pseudo-ops don't compile to a real instruction --
// linkers/zl/noop.c's own divsubr()/noops() resolve them into a JSR to
// one of the eight symbols this file provides (_divq/_divqu/_modq/
// _modqu/_divl/_divlu/_modl/_modlu, looked up by exactly those names),
// the same way ARM's own lack-of-hardware-divide is bridged by
// arch/arm/div.s (mkfiles/arm/mkfile's ARCHEXTRAOFILES).
//
// Ported verbatim (only NOPROF's own #define substituted to its literal
// 1, and the two source files -- divq.s/divl.s -- merged into one,
// matching arm's own single div.s) from ~/xxx/plan9/SRC/libc/alpha/
// {divq,divl}.s, the ORIGINAL Plan 9 4th edition Alpha libc -- not
// derived or rewritten, since this is pure arithmetic bit-twiddling
// with no arch-ABI dependency beyond register numbers, and those
// (R10-R12/R23-R24 scratch, R26 REGLINK, R27 REGTMP, R30 REGSP, R31
// REGZERO) already match z.out.h's own convention exactly -- za
// assembles this file's mnemonics (MOVQ/MOVL/SLLQ/SLLL/SRLQ/SRLL/
// CMPUGE/BNE/BGE/BLT/BEQ/SUBQ/SUBL/ADDQ/OR/XOR/JSR/JMP/RET, the 3-arg
// TEXT form with a middle flags word) unchanged, confirmed via a
// direct `za -c` probe on this exact content before committing to the
// full port.
//
// Calling convention (compiler/linker-internal, NOT the normal C ABI
// zc uses for ordinary function calls): the caller (zl's own noops()
// expansion of e.g. `DIVQ a,b,c`) is expected to have already pushed
// num/den onto the stack at fixed offsets before JSR-ing here; see
// each TEXT block's own header comment below for the exact protocol.
// Nothing outside this file, and nothing this project's own C code
// writes, ever calls these eight symbols directly.

/*
 *	uvlong
 *	_udivmodq(uvlong num, uvlong den)
 *	{
 *		int i;
 *		uvlong quo;
 *
 *		if(den == 0)
 *			*(ulong*)-1 = 0;
 *		quo = num;
 *		if(quo > 1<<(64-1))
 *			quo = 1<<(64-1);
 *		for(i=0; den<quo; i++)
 *			den <<= 1;
 *		quo = 0;
 *		for(; i>=0; i--) {
 *			quo <<= 1;
 *			if(num >= den) {
 *				num -= den;
 *				quo |= 1;
 *			}
 *			den >>= 1;
 *		}
 *		return quo::num;
 *	}
 *
 * calling sequence:
 *	num: 8(R30)
 *	den: 16(R30)
 * returns
 *	quo: 8(R30)
 *	rem: 16(R30)
 */
TEXT	_udivmodq(SB), 1, $-8

	MOVQ	$1, R11
	SLLQ	$63, R11
	MOVQ	8(R30), R23	/* numerator */
	MOVQ	16(R30), R10	/* denominator */
	BNE	R10, udm20
	MOVQ	R31, -1(R31)	/* fault -- divide by zero */
udm20:
	MOVQ	R23, R12
	BGE	R12, udm34
	MOVQ	R11, R12
udm34:
	MOVQ	R31, R11
udm38:
	CMPUGE	R10, R12, R24
	BNE	R24, udm54
	SLLQ	$1, R10
	ADDQ	$1, R11
	JMP	udm38
udm54:
	MOVQ	R31, R12
udm58:
	BLT	R11, udm8c
	SLLQ	$1, R12
	CMPUGE	R23, R10, R24
	BEQ	R24, udm7c
	SUBQ	R10, R23
	OR	$1, R12
udm7c:
	SRLQ	$1, R10
	SUBQ	$1, R11
	JMP	udm58
udm8c:
	MOVQ	R12, 8(R30)	/* quotient */
	MOVQ	R23, 16(R30)	/* remainder */
	RET

/*
 * save working registers
 * and bring in num/den parameters
 */
TEXT	_unsargq(SB), 1, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVQ	R27, 8(R30)
	MOVQ	72(R30), R27
	MOVQ	R27, 16(R30)

	RET

/*
 * save working registers
 * and bring in absolute value
 * of num/den parameters
 */
TEXT	_absargq(SB), 1, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVQ	R27, 64(R30)
	BGE	R27, ab1
	SUBQ	R27, R31, R27
ab1:
	MOVQ	R27, 8(R30)	/* numerator */

	MOVQ	72(R30), R27
	BGE	R27, ab2
	SUBQ	R27, R31, R27
ab2:
	MOVQ	R27, 16(R30)	/* denominator */
	RET

/*
 * restore registers and
 * return to original caller
 * answer is in R27
 */
TEXT	_retargq(SB), 1, $-8
	MOVQ	24(R30), R10
	MOVQ	32(R30), R11
	MOVQ	40(R30), R12
	MOVQ	48(R30), R23
	MOVQ	56(R30), R24
	MOVQ	0(R30), R26

	ADDQ	$64, R30
	RET			/* back to main sequence */

TEXT	_divq(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	,_absargq(SB)
	JSR	,_udivmodq(SB)
	MOVQ	8(R30), R27

	MOVQ	64(R30), R10	/* clean up the sign */
	MOVQ	72(R30), R11
	XOR	R11, R10
	BGE	R10, divq1
	SUBQ	R27, R31, R27
divq1:

	JSR	,_retargq(SB)
	RET			/* not executed */

TEXT	_divqu(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	,_unsargq(SB)
	JSR	,_udivmodq(SB)
	MOVQ	8(R30), R27

	JSR	,_retargq(SB)
	RET			/* not executed */

TEXT	_modq(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	,_absargq(SB)
	JSR	,_udivmodq(SB)
	MOVQ	16(R30), R27

	MOVQ	64(R30), R10	/* clean up the sign */
	BGE	R10, modq2
	SUBQ	R27, R31, R27
modq2:

	JSR	,_retargq(SB)
	RET			/* not executed */

TEXT	_modqu(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVQ	R26, 0(R30)

	JSR	,_unsargq(SB)
	JSR	,_udivmodq(SB)
	MOVQ	16(R30), R27

	JSR	,_retargq(SB)
	RET			/* not executed */

/*
 *	ulong
 *	_udivmodl(ulong num, ulong den)
 *	{ ... same algorithm as _udivmodq above, 32-bit width ... }
 *
 * calling sequence:
 *	num: 8(R30)
 *	den: 12(R30)
 * returns
 *	quo: 8(R30)
 *	rem: 12(R30)
 */
TEXT	_udivmodl(SB), 1, $-8

	MOVQ	$-1, R11
	SLLQ	$31, R11		/* (1<<31) in canonical form */
	MOVL	8(R30), R23	/* numerator */
	MOVL	12(R30), R10	/* denominator */
	BNE	R10, udl20
	MOVQ	R31, -1(R31)	/* fault -- divide by zero */
udl20:
	MOVQ	R23, R12
	BGE	R12, udl34
	MOVQ	R11, R12
udl34:
	MOVQ	R31, R11
udl38:
	CMPUGE	R10, R12, R24
	BNE	R24, udl54
	SLLL	$1, R10
	ADDQ	$1, R11
	JMP	udl38
udl54:
	MOVQ	R31, R12
udl58:
	BLT	R11, udl8c
	SLLL	$1, R12
	CMPUGE	R23, R10, R24
	BEQ	R24, udl7c
	SUBL	R10, R23
	OR	$1, R12
udl7c:
	SRLL	$1, R10
	SUBQ	$1, R11
	JMP	udl58
udl8c:
	MOVL	R12, 8(R30)	/* quotient */
	MOVL	R23, 12(R30)	/* remainder */
	RET

TEXT	_unsargl(SB), 1, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVL	R27, 8(R30)
	MOVL	72(R30), R27
	MOVL	R27, 12(R30)

	RET

TEXT	_absargl(SB), 1, $-8
	MOVQ	R10, 24(R30)
	MOVQ	R11, 32(R30)
	MOVQ	R12, 40(R30)
	MOVQ	R23, 48(R30)
	MOVQ	R24, 56(R30)

	MOVL	R27, 64(R30)
	BGE	R27, abl1
	SUBL	R27, R31, R27
abl1:
	MOVL	R27, 8(R30)	/* numerator */

	MOVL	72(R30), R27
	BGE	R27, abl2
	SUBL	R27, R31, R27
abl2:
	MOVL	R27, 12(R30)	/* denominator */
	RET

TEXT	_retargl(SB), 1, $-8
	MOVQ	24(R30), R10
	MOVQ	32(R30), R11
	MOVQ	40(R30), R12
	MOVQ	48(R30), R23
	MOVQ	56(R30), R24
	MOVL	0(R30), R26

	ADDQ	$64, R30
	RET			/* back to main sequence */

TEXT	_divl(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	,_absargl(SB)
	JSR	,_udivmodl(SB)
	MOVL	8(R30), R27

	MOVL	64(R30), R10	/* clean up the sign */
	MOVL	72(R30), R11
	XOR	R11, R10
	BGE	R10, divl1
	SUBL	R27, R31, R27
divl1:

	JSR	,_retargl(SB)
	RET			/* not executed */

TEXT	_divlu(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	,_unsargl(SB)
	JSR	,_udivmodl(SB)
	MOVL	8(R30), R27

	JSR	,_retargl(SB)
	RET			/* not executed */

TEXT	_modl(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	,_absargl(SB)
	JSR	,_udivmodl(SB)
	MOVL	12(R30), R27

	MOVL	64(R30), R10	/* clean up the sign */
	BGE	R10, modl2
	SUBL	R27, R31, R27
modl2:

	JSR	,_retargl(SB)
	RET			/* not executed */

TEXT	_modlu(SB), 1, $-8
	SUBQ	$64, R30	/* 5 reg save, 2 parameters, link */
	MOVL	R26, 0(R30)

	JSR	,_unsargl(SB)
	JSR	,_udivmodl(SB)
	MOVL	12(R30), R27

	JSR	,_retargl(SB)
	RET			/* not executed */
