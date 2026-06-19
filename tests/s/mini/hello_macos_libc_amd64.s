TEXT _start(SB), $0
	MOVQ	writep(SB), AX		// dyld-bound _write pointer (RIP-relative load)
	MOVL	$1, DI			// fd = 1 (stdout)
	LEAQ	msg(SB), SI		// buf
	MOVL	$13, DX			// count
	CALL	AX			// write(1, msg, 13) via libSystem
	XORL	AX, AX			// return 0
	RET				// LC_MAIN: dyld calls exit(0)

DATA	msg+0(SB)/8, $"Hello, w"
DATA	msg+8(SB)/5, $"orld\n"
GLOBL	msg(SB), $13

GLOBL	writep(SB), $8			// GOT slot; -I binds it to _write@libSystem
DATA	writep+0(SB)/8, $0
