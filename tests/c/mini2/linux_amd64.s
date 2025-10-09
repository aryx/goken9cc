
TEXT    panic(SB), $0
        // syscall: exit(0)
        MOVQ    $60, AX
        XORQ    DI, DI
        SYSCALL


// was called exit1 in runtime/linux/amd64/sys.s
TEXT	exit(SB), $0-8
	MOVL	8(SP), DI
	MOVL	$60, AX	// exit - exit the current os thread
	SYSCALL
	RET

//alt?
//TEXT    exit0(SB), 7, $0
//        // syscall: exit(0)
//        MOVQ    $60, AX
//        XORQ    DI, DI
//        SYSCALL

TEXT	open(SB), $0-16
	MOVQ	8(SP), DI
	MOVL	16(SP), SI
	MOVL	20(SP), DX
	MOVL	$2, AX			// syscall entry
	SYSCALL
	RET

TEXT	write(SB), $0-24
	MOVL	8(SP), DI
	MOVQ	16(SP), SI
	MOVL	24(SP), DX
	MOVL	$1, AX			// syscall entry
	SYSCALL
	RET

//alt?
//TEXT	write0(SB), $0
//	MOVQ	$1, AX          // syscall number for write
//	MOVQ	fd+0(FP), DI     // fd (arg 1)
//	MOVQ	buf+8(FP), SI    // buf (arg 2)
//	MOVQ	n+16(FP), DX     // n (arg 3)
//	SYSCALL
//	RET

TEXT	gettime(SB), $32
	LEAQ	8(SP), DI
	MOVQ	$0, SI
	MOVQ	$0xffffffffff600000, AX
	CALL	AX

	MOVQ	8(SP), BX	// sec
	MOVQ	sec+0(FP), DI
	MOVQ	BX, (DI)

	MOVL	16(SP), BX	// usec
	MOVQ	usec+8(FP), DI
	MOVL	BX, (DI)
	RET

TEXT	·getcallerpc+0(SB), $0
	MOVQ	x+0(FP),AX		// addr of first arg
	MOVQ	-8(AX),AX		// get calling pc
	RET

TEXT	·setcallerpc+0(SB), $0
	MOVQ	x+0(FP),AX		// addr of first arg
	MOVQ	x+8(FP), BX
	MOVQ	BX, -8(AX)		// set calling pc
	RET

TEXT getcallersp(SB), $0
	MOVQ	sp+0(FP), AX
	RET



// bool cas(int32 *val, int32 old, int32 new)
// Atomically:
//	if(*val == old){
//		*val = new;
//		return 1;
//	} else
//		return 0;
TEXT cas(SB), $0
	MOVQ	8(SP), BX
	MOVL	16(SP), AX
	MOVL	20(SP), CX
	LOCK
	CMPXCHGL	CX, 0(BX)
	JZ 3(PC)
	MOVL	$0, AX
	RET
	MOVL	$1, AX
	RET
