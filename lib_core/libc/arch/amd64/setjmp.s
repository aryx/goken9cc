// setjmp()/longjmp() for linux/amd64. See arch/386/setjmp.s's own
// header comment for the general approach (this compiler preserves no
// registers across a call, so only sp+return-address need saving) and
// the SP-pseudo-token-vs-real-hardware-register story -- confirmed
// directly for amd64 specifically via os/windows/winio_amd64.s's own
// `MOVQ SP, DI`/`MOVQ DI, SP` pair (real hardware RSP, verified with a
// real native Windows run), not merely inferred by analogy the way
// 386's own version has to be.
//
// Unlike 386, amd64's own first argument (j, a pointer) does NOT
// arrive unspilled in a register -- confirmed via 6c -S on a probe
// function, same "no argument, not even the first, arrives in a
// register" AMD64 SysV fact svc_amd64.s's own header comment already
// established for _syscall6. So j is read from j+0(FP) exactly like
// 386's version, just MOVQ instead of MOVL and 8-byte fields.
TEXT setjmp(SB), $0
	MOVQ	j+0(FP), AX
	MOVQ	SP, 0(AX)	// save real sp
	MOVQ	0(SP), BX	// return address currently on the stack top
	MOVQ	BX, 8(AX)
	MOVQ	$0, AX		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVQ	v+8(FP), AX
	CMPQ	AX, $0
	JNE	ok
	MOVQ	$1, AX		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
ok:
	MOVQ	j+0(FP), BX
	MOVQ	0(BX), SP	// restore real sp
	MOVQ	8(BX), BX	// saved return address
	MOVQ	BX, 0(SP)	// write it back onto the (now-restored) stack top
	RET			// RET pops that slot and jumps there, with AX=v
