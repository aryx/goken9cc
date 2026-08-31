// Regression test for a real bug found while building tests/c/mini2's
// hellowrite_wasm.c: ea's `STOREx addr` convenience form (a.y's
// pushaddr()-fused grammar rule) emits the address-push *at this
// line*, right before the store opcode -- correct only when the value
// being stored is *also* pushed by this same line (a bare immediate).
// When the value instead comes from an *earlier* instruction (e.g. a
// LOCALGET of a real argument -- exactly what every ec-compiled
// function's own store codegen does, see cgen.c's OAS case), the
// fused form gets the stack order backwards: [value, address] instead
// of the [address, value] a real i32.store needs (e.out.h's own
// ALOADx/ASTOREx comment: address is always evaluated first). This
// wasn't caught earlier because no test exercised STOREx with a
// non-constant value -- el/asm.c's own top comment flagged loads/
// stores as implemented-but-unverified.
//
// storebyte(ptr, val) below stores `val` (arrived via LOCALGET,
// exactly the hazard case) at `ptr` (also via LOCALGET) using the
// fixed, honest `STOREB $0` form added alongside the fused one (see
// a.y's own comment on that grammar rule): push address, push value,
// then a bare store with a literal offset, taking both operands from
// whatever's already on the stack. Building "Hello, world\n" one byte
// at a time this way and comparing the real output against the same
// hello.expected.txt every other hello_*.exe in this directory uses
// means a regression here fails exactly like a silently-wrong hello
// program would, not just a narrow assertion.
TEXT	storebyte(SB), $0
SIGNATURE	storebyte(SB), $"WWV"

	LOCALGET	LOCAL(0)	// ptr
	LOCALGET	LOCAL(1)	// val
	STOREB	$0

	RET

TEXT	_start(SB), $0

	CONSTW	$buf+0(SB)
	CONSTW	$72		// 'H'
	CALL	storebyte(SB)

	CONSTW	$buf+1(SB)
	CONSTW	$101		// 'e'
	CALL	storebyte(SB)

	CONSTW	$buf+2(SB)
	CONSTW	$108		// 'l'
	CALL	storebyte(SB)

	CONSTW	$buf+3(SB)
	CONSTW	$108		// 'l'
	CALL	storebyte(SB)

	CONSTW	$buf+4(SB)
	CONSTW	$111		// 'o'
	CALL	storebyte(SB)

	CONSTW	$buf+5(SB)
	CONSTW	$44		// ','
	CALL	storebyte(SB)

	CONSTW	$buf+6(SB)
	CONSTW	$32		// ' '
	CALL	storebyte(SB)

	CONSTW	$buf+7(SB)
	CONSTW	$119		// 'w'
	CALL	storebyte(SB)

	CONSTW	$buf+8(SB)
	CONSTW	$111		// 'o'
	CALL	storebyte(SB)

	CONSTW	$buf+9(SB)
	CONSTW	$114		// 'r'
	CALL	storebyte(SB)

	CONSTW	$buf+10(SB)
	CONSTW	$108		// 'l'
	CALL	storebyte(SB)

	CONSTW	$buf+11(SB)
	CONSTW	$100		// 'd'
	CALL	storebyte(SB)

	CONSTW	$buf+12(SB)
	CONSTW	$10		// '\n'
	CALL	storebyte(SB)

	CONSTW	$1		// fd = 1 (stdout)
	CONSTW	$iov(SB)	// iovs_ptr
	CONSTW	$1		// iovs_len
	CONSTW	$nwritten(SB)	// nwritten_ptr

	CALL	fd_write(SB)
	DROP

	RET

GLOBL	nwritten(SB), $4
GLOBL	buf(SB), $13

DATA	iov+0(SB)/4, $buf(SB)
DATA	iov+4(SB)/4, $13
GLOBL	iov(SB), $8
