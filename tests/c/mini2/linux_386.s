//---------------------------------
// Entry and exit point
//---------------------------------
TEXT _main(SB), $0
	CALL	main(SB)
	CALL	exit(SB) // should not be reached

// Linux 386 syscall convention: AX=number, BX/CX/DX=args, INT $0x80.
// 8c passes args on the stack via FP; each slot is 4 bytes on 386.

TEXT exit+0(SB), $0
	MOVL	status+0(FP), BX
	MOVL	$1, AX          // syscall number 1 = sys_exit
	INT	$0x80
	RET // never reached

//---------------------------------
// Basic functions
//---------------------------------

TEXT panic+0(SB), $0
	MOVL	status+0(FP), BX
	MOVL	$1, AX          // syscall number 1 = sys_exit
	INT	$0x80
	RET // never reached

TEXT write+0(SB), $0
	MOVL	fd+0(FP), BX
	MOVL	buf+4(FP), CX
	MOVL	count+8(FP), DX
	MOVL	$4, AX          // syscall number 4 = sys_write
	INT	$0x80
	RET
