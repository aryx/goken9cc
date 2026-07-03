// Exercise large/negative constants and offsets: these flow through
// Gen/Adr offset fields and the linker's lput()/wput(), historically
// declared with 'long' (see changes.txt long->int32 entries): on 64-bit
// hosts sign extension and truncation can differ from a 32-bit long.

TEXT	_start+0(SB), $0
	MOVL	$0x7fffffff, AX
	MOVL	$0x80000000, BX
	MOVL	$0xdeadbeef, CX
	MOVL	$-1, DX
	MOVL	$-2147483647, SI
	ADDL	$0x12345678, DI
	CMPL	AX, $0x80000001
	// exit(0)
	MOVL	$1, AX
	XORL	BX, BX
	INT	$0x80

DATA	big+0(SB)/4, $0xcafebabe
DATA	big+4(SB)/4, $-559038737
DATA	big+8(SB)/4, $0x80000000
DATA	big+12(SB)/1, $0xff
DATA	big+13(SB)/2, $0xbeef
GLOBL	big(SB), $16
