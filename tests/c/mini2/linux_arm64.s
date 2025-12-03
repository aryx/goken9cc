TEXT _main+0(SB), $0
	MOV    $setSB(SB), R28
	BL main+0(SB)


TEXT    exit+0(SB), $0
    // syscall: exit(0)
    //MOV    0(FP), R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    RETURN // never reached


TEXT write+0(SB), $0        // NOSPLIT | DUPOK | NOPROF
    //MOV fd+8(FP), R0
    MOV buf+16(FP), R1
    MOV count+24(FP), R2
    MOV $64, R8         // syscall number: write
    SVC $0
    RETURN

TEXT    panic(SB), $0
        // syscall: exit(0)
    MOV    $0, R0          // status = 0
    MOV    $93, R8         // syscall number: exit
    SVC     $0
    RETURN

// from 9front libc/arm64/getcallerpc.s
TEXT ·getcallerpc(SB), $0
	MOV	0(SP), R0
	RETURN



//TEXT	open(SB), $0-16
//	MOVQ	8(SP), DI
//	MOVL	16(SP), SI
//	MOVL	20(SP), DX
//	MOVL	$2, AX			// syscall entry
//	SYSCALL
//	RET
//
//
//TEXT	gettime(SB), $32
//	LEAQ	8(SP), DI
//	MOVQ	$0, SI
//	MOVQ	$0xffffffffff600000, AX
//	CALL	AX
//
//	MOVQ	8(SP), BX	// sec
//	MOVQ	sec+0(FP), DI
//	MOVQ	BX, (DI)
//
//	MOVL	16(SP), BX	// usec
//	MOVQ	usec+8(FP), DI
//	MOVL	BX, (DI)
//	RET
//
//
//TEXT	·setcallerpc+0(SB), $0
//	MOVQ	x+0(FP),AX		// addr of first arg
//	MOVQ	x+8(FP), BX
//	MOVQ	BX, -8(AX)		// set calling pc
//	RET
//
//TEXT getcallersp(SB), $0
//	MOVQ	sp+0(FP), AX
//	RET
//
//
//
//// bool cas(int32 *val, int32 old, int32 new)
//// Atomically:
////	if(*val == old){
////		*val = new;
////		return 1;
////	} else
////		return 0;
//TEXT cas(SB), $0
//	MOVQ	8(SP), BX
//	MOVL	16(SP), AX
//	MOVL	20(SP), CX
//	LOCK
//	CMPXCHGL	CX, 0(BX)
//	JZ 3(PC)
//	MOVL	$0, AX
//	RET
//	MOVL	$1, AX
//	RET
