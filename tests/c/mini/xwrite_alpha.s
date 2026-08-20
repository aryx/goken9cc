TEXT xwrite+0(SB), $0
	// xwrite(char *buf, int count) -> write(1, buf, count).
	// zc passes the first arg (buf) in R0 (REGARG=REGRET=0) and stores
	// count on the stack at count+4(FP) -- confirmed with 'zc -S' on a
	// two-arg function, not derived from the FP-offset formula the
	// other archs' xwrite_*.s comment (autosize+offset+ptrsize):
	// zc has no notion of ptrsize (unlike the dual-width ic/il), and
	// this offset came out 4 regardless of R0's pointer being 8 bytes
	// wide on this otherwise-64-bit machine.
	//
	// Alpha Linux syscall ABI: v0/R0 = syscall number in, result out;
	// a0-a5 = R16-R21. R0 must be moved out to a1 before it's
	// clobbered with the syscall number (same reason arm64's xwrite
	// moves its own R0 to R1 first).
	MOVL	count+4(FP), R18    // a2 = count
	MOVQ	R0, R17             // a1 = buf (arrives in R0)
	MOVQ	$1, R16             // a0 = fd = 1
	MOVQ	$4, R0              // v0 = syscall number: write (4)
	CALL_PAL $0x83              // callsys
	RET
