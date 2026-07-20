TEXT xwrite+0(SB), $0
	// xwrite(char *buf, int count) -> write(1, buf, count).
	// ic passes the first arg (buf) in R8 (REGARG=REGRET=8) and stores
	// count on the stack. Linux RISC-V wants a0=R10, a1=R11, a2=R12 and
	// the syscall number in a7=R17; write is 64.
	//
	// count sits at 4(FP): FP params resolve to autosize+offset+ptrsize
	// (see linkers/il/span.c), and here autosize=0 (leaf) and ptrsize=4
	// (rv32), so the caller's 8(SP) store is reached with offset 4.
	// MOVW not MOV: count is an int, only 32 bits were stored.
	MOVW	count+4(FP), R12    // a2 = count
	MOVW	R8, R11             // a1 = buf (arrives in R8)
	MOVW	$1, R10             // a0 = fd = 1
	MOVW	$64, R17            // a7 = syscall number: write
	ECALL
	RET
