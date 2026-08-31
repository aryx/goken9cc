// xwrite_sparc.s -- Linux SPARC32 write(fd=1, buf, count)
//
// kc passes the first arg (buf) in R7 (REGARG=REGRET=7, confirmed with
// 'kc -S' on a two-arg function) and the second arg (count) on the
// stack at count+4(FP), same FP-relative convention as xwrite_power.s.
// R7 must be moved out before it's clobbered with the syscall setup,
// same reason xwrite_power.s/xwrite_arm.s move their own incoming arg
// register first.
//
// Linux SPARC32 syscall ABI: g1 (R1) = syscall number, o0-o5 (R8-R13)
// = args, "ta 0x10" traps into the kernel (see tests/s/exit/
// exit_linux_sparc.s's comment) -- a different register namespace
// than kc's own REGARG/REGRET (R7), so the syscall's real return value
// (left in o0/R8 by the kernel) has to be moved into R7 before RETURN
// for kc's caller-side convention to see it.
//
// R1 is ALSO kc's own REGSP (this backend's logical stack pointer,
// live across the call in the caller's frame) -- a genuine ABI
// collision with the hardware's %g1, unlike exit_linux_sparc.s's own
// use of R1, which is safe there only because that file is leaf code
// with no stack in use. Clobbering it here without saving/restoring
// corrupts the caller's (main's) stack pointer for good, breaking its
// epilogue once execution returns -- found as a real segfault (main's
// "MOVW 0(R1),R8" link-restore reading from a garbage address) with
// helloc.c's second xwrite call.

TEXT xwrite+0(SB), $0
	MOVW	R7, R9           // buf (arrives in R7)
	MOVW	count+4(FP), R10 // count (second arg, on the stack)
	MOVW	R1, R12          // save REGSP before clobbering it below
	MOVW	$1, R8           // o0 = fd = 1 (stdout)
	MOVW	$4, R1           // g1 = syscall number = write (4)
	MOVW	$0x10, R11       // scratch reg holding the trap number
	TA	R11
	MOVW	R12, R1          // restore REGSP
	MOVW	R8, R7           // syscall return (o0) -> REGRET
	RETURN
