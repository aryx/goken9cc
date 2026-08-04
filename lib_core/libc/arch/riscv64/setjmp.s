// setjmp()/longjmp() for linux/riscv64 (rv64). See arch/riscv/
// setjmp.s's own header comment for the general approach (identical
// registers and branch shape -- R8 for the unspilled first argument,
// R2/R1 for the real hardware sp/ra, BNE-against-R0 for the ansi
// longjmp(j, 0) special case). The one real difference from rv32: v
// (longjmp's second argument) lands at v+8(FP), not +4 -- confirmed
// via jc -S on the same probe, matching svc_riscv64.s's own 8-byte
// argument-slot convention -- and MOV (not MOVW) is used throughout
// to move full 64-bit sp/ra/pointer values rather than truncating them
// to 32 bits, the same distinction _syscall6 vs _syscall6v already
// makes on this arch.
TEXT setjmp(SB), $0
	MOV	R2, 0(R8)	// save real sp
	MOV	R1, 8(R8)	// save return address (the real ra register)
	MOVW	$0, R8		// setjmp() returns 0 on the direct call
	RET

TEXT longjmp(SB), $0
	MOVW	v+8(FP), R9
	BNE	R0, R9, notzero
	MOVW	$1, R9		// ansi: longjmp(j, 0) behaves as longjmp(j, 1)
notzero:
	MOV	0(R8), R10	// saved sp (R8 still holds j here)
	MOV	8(R8), R1	// saved return address -> restore R1 directly
	MOVW	R9, R8		// return value into R8 (frees R8, j no longer needed)
	MOV	R10, R2		// restore sp last
	RET			// branches via the now-restored R1, with R8=v
