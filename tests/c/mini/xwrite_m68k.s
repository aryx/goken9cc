// xwrite_m68k.s -- Linux m68k write(fd=1, buf, count)
//
// 2c passes both args on the stack (2c -S on a two-arg function shows
// buf+0(FP)/count+4(FP), unlike the register-based REGARG convention
// 5c/6c/7c/.../kc use) and expects the return value in R0 (D0) -- see
// helloc.c's "MOVL $1,R0"/"CLRL R0" pattern for how it sets its own
// return values. Linux m68k syscall ABI (see tests/s/exit/
// exit_linux_m68k.s's comment) already returns the syscall result in
// d0/R0 too, so unlike xwrite_sparc.s (whose REGSP collides with the
// syscall-number register and needs an explicit save/restore), no
// register juggling is needed here: R0 already holds the right value
// for both the trap and the C-level return.

TEXT xwrite+0(SB), $0
	MOVL	$1, R1           // d1 = fd = 1 (stdout)
	MOVL	buf+0(FP), R2    // d2 = buf (first arg, on the stack)
	MOVL	count+4(FP), R3  // d3 = count (second arg, on the stack)
	MOVL	$4, R0           // d0 = syscall number = write (4)
	TRAP	$0
	RTS
