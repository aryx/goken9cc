// Exercise the float constant encoding path: assembler/linker convert
// $double via ieeedtod() into the Ieee{l,h} struct, historically declared
// with 'long' fields (see changes.txt long->int32 entries): on 64-bit
// hosts a long-based implementation can encode differently.
// See also tests/c/misc/f_1_e_126.c which caught this in 8c.

TEXT	_start+0(SB), $0
	// exit(0)
	MOVL	$1, AX
	XORL	BX, BX
	INT	$0x80

// 8-byte doubles, including denormal-ish and extreme exponents
DATA	dbl+0(SB)/8, $1e-126
DATA	dbl+8(SB)/8, $-0.5
DATA	dbl+16(SB)/8, $3.141592653589793
DATA	dbl+24(SB)/8, $1.7976931348623157e308
DATA	dbl+32(SB)/8, $5e-324
DATA	dbl+40(SB)/8, $-0.0
GLOBL	dbl(SB), $48
