//---------------------------------
// Entry and exit point
//---------------------------------
TEXT _main(SB), $0
#ifdef arm_
	MOVW $setR12(SB), R12
#endif
	BL main(SB)
	BL exit(SB) // should not be reached


TEXT    exit+0(SB), $0
#ifdef arm_
	//with 5c_, the first arg is already passed via R0
#else
        MOVW    status+0(FP), R0
#endif
        MOVW    $1, R7          // syscall number 1 = sys_exit
        SWI     $0
	RET // never reached

//---------------------------------
// Basic functions
//---------------------------------

TEXT    panic+0(SB), $0
#ifdef arm_
#else
        MOVW    status+0(FP), R0
#endif
        MOVW    $1, R7          // syscall number 1 = sys_exit
        SWI     $0
	RET // never reached


TEXT write+0(SB), $0
#ifdef arm_
#else
        MOVW    fd+0(FP), R0
#endif
        MOVW    buf+4(FP), R1
        MOVW    count+8(FP), R2
        MOVW    $4, R7          // syscall number 4 = sys_write
        SWI     $0
	RET

// pad's own debug function (was used for tracking float/vlong
// functions to implement first, see commented code further below
// using 'BL debug(SB)')
TEXT 	debug+0(SB), $0
	MOVW R0, R1
	//alt: call write
	MOVW $1, R0
	MOVW $4, R2
	MOVW    $4, R7
	SWI     $0
	RET

//TODO: see GO/.../runtime/arm/asm.s and copy the code there
// or in principia/.../libc/arm/

//---------------------------------
// from principia/libc/arm/div.s
//---------------------------------

// no arm instructions for those operations so must be
// provided as "builtins"

//old:
//TEXT 	_div+0(SB), 7, $0
//	RET
//TEXT 	_divu+0(SB), 7, $0
//	RET
//TEXT 	_mod+0(SB), 7, $0
//	RET
//TEXT 	_modu+0(SB), 7, $0
//	RET

Q	= 0
N	= 1
D	= 2
CC	= 3
TMP	= 11

//TODO: need ifdef arm_ and define code for 5l/5a/5c
// the code below is from principia and so assumes 5a_/5c_/5l_
// calling convention (first arg in R0, not in 0(FP))
TEXT	save<>(SB), 1, $0
 MOVW	R(Q), 0(FP)
 MOVW	R(N), 4(FP)
 MOVW	R(D), 8(FP)
 MOVW	R(CC), 12(FP)

 MOVW	R(TMP), R(Q)		/* numerator */
 MOVW	20(FP), R(D)		/* denominator */
 CMP	$0, R(D)
 BNE	s1
 MOVW	-1(R(D)), R(TMP)	/* divide by zero fault */
s1:	RET

TEXT	rest<>(SB), 1, $0
 MOVW	0(FP), R(Q)
 MOVW	4(FP), R(N)
 MOVW	8(FP), R(D)
 MOVW	12(FP), R(CC)
/*
 * return to caller
 * of rest<>
 */
 MOVW	0(R13), R14
 ADD	$20, R13
 B	(R14)

TEXT	div<>(SB), 1, $0
 MOVW	$32, R(CC)
/*
 * skip zeros 8-at-a-time
 */
e1:
 AND.S	$(0xff<<24),R(Q), R(N)
 BNE	e2
 SLL	$8, R(Q)
 SUB.S	$8, R(CC)
 BNE	e1
 RET
e2:
 MOVW	$0, R(N)

loop:
/*
 * shift R(N||Q) left one
 */
 SLL	$1, R(N)
 CMP	$0, R(Q)
 ORR.LT	$1, R(N)
 SLL	$1, R(Q)

/*
 * compare numerator to denominator
 * if less, subtract and set quotent bit
 */
 CMP	R(D), R(N)
 ORR.HS	$1, R(Q)
 SUB.HS	R(D), R(N)
 SUB.S	$1, R(CC)
 BNE	loop
 RET

TEXT	_div(SB), 1, $16
 BL	save<>(SB)
 CMP	$0, R(Q)
 BGE	d1
 RSB	$0, R(Q), R(Q)
 CMP	$0, R(D)
 BGE	d2
 RSB	$0, R(D), R(D)
d0:
 BL	div<>(SB)		/* none/both neg */
 MOVW	R(Q), R(TMP)
 B	out
d1:
 CMP	$0, R(D)
 BGE	d0
 RSB	$0, R(D), R(D)
d2:
 BL	div<>(SB)		/* one neg */
 RSB	$0, R(Q), R(TMP)
 B	out

TEXT	_mod(SB), 1, $16
 BL	save<>(SB)
 CMP	$0, R(D)
 RSB.LT	$0, R(D), R(D)
 CMP	$0, R(Q)
 BGE	m1
 RSB	$0, R(Q), R(Q)
 BL	div<>(SB)		/* neg numerator */
 RSB	$0, R(N), R(TMP)
 B	out
m1:
 BL	div<>(SB)		/* pos numerator */
 MOVW	R(N), R(TMP)
 B	out

TEXT	_divu(SB), 1, $16
 BL	save<>(SB)
 BL	div<>(SB)
 MOVW	R(Q), R(TMP)
 B	out

TEXT	_modu(SB), 1, $16
 BL	save<>(SB)
 BL	div<>(SB)
 MOVW	R(N), R(TMP)
 B	out

out:
 BL	rest<>(SB)
 B	out

//---------------------------------
// end of principia/libc/arm/div.s
//---------------------------------
