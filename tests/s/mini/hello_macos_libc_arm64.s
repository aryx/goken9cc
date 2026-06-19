TEXT _start(SB), $0
	MOV	$setSB(SB), R28		// static base (PC-relative under -H6)
	MOV	$1, R0			// fd = 1 (stdout)
	MOV	$msg(SB), R1		// buf
	MOV	$13, R2			// count
	MOV	writep(SB), R3		// dyld-bound _write pointer (value at writep)
	BL	(R3)			// write(1, msg, 13) via libSystem
	MOV	$0, R0			// return 0
	RETURN				// LC_MAIN: dyld calls exit(0)

DATA	msg+0(SB)/8, $"Hello, w"
DATA	msg+8(SB)/5, $"orld\n"
GLOBL	msg(SB), $13

GLOBL	writep(SB), $8			// GOT slot; -I binds it to _write@libSystem
DATA	writep+0(SB)/8, $0
