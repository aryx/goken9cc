
TEXT xwrite+0(SB), $0
	// xwrite(char *buf, int count) -> write(1, buf, count).
	// Linux 386 syscall convention: AX=number, BX/CX/DX=args, INT $0x80.
	// 8c passes both args on the stack (via FP); pointers are 4 bytes on
	// 386, so count (an int) sits at 4(FP), right after buf at 0(FP).
	MOVL	$4, AX             // syscall number for write
	MOVL	$1, BX             // fd = 1 (stdout)
	MOVL	buf+0(FP), CX      // 1st argument: buf
	MOVL	count+4(FP), DX    // 2nd argument: count
	INT	$0x80
	RET
