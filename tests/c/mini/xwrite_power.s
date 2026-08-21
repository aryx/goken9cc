// xwrite_power.s -- Linux PowerPC write(fd=1, buf, count)

TEXT xwrite+0(SB), $0
	// qc passes the first arg (buf) in R3 (REGARG=REGRET=3, confirmed
	// with 'qc -S' on a two-arg function) and the second arg (count)
	// on the stack at count+4(FP) -- pointers/ints are 4 bytes here
	// (SZ_IND=4), same offset convention as xwrite_arm.s/xwrite_mips.s.
	//
	// PowerPC Linux syscall ABI: r0 = syscall number, r3-r5 = args
	// (same convention as tests/s/exit/exit_linux_power.s). R3 must be
	// moved out to R4 before it's clobbered with fd, same reason
	// xwrite_arm.s/xwrite_alpha.s move their own incoming arg register
	// first.
	MOVW	R3, R4          // buf (arrives in R3)
	MOVW	count+4(FP), R5 // count (second arg)
	MOVW	$1, R3          // fd = 1 (stdout)
	MOVW	$4, R0          // syscall number = write (4)
	SYSCALL
	RETURN
