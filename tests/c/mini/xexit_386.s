TEXT xexit+0(SB), $0
	// xexit() -> exit(0). Linux 386: AX=number, BX=status, INT $0x80.
	MOVL	$1, AX             // syscall number for exit
	XORL	BX, BX             // status = 0
	INT	$0x80
