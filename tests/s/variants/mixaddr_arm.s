/* mixaddr_arm: the assembler side.
 * Takes the address of `shared` via $shared(SB), which the assembler
 * encodes as D_ADDR. mixaddr_arm.c takes the same address via &shared,
 * which the compiler encodes as D_CONST(+name). When linked together the
 * two must collapse to ONE literal-pool entry. See mixaddr_arm.c and the
 * mkfile rule for why (kernel bootstrap init.out pool-dedup mismatch).
 */
TEXT	_start(SB), $0
	MOVW	$shared+0(SB), R1
	BL	cfun(SB)
	MOVW	$shared+0(SB), R2
	RET

GLOBL	shared+0(SB), $8
