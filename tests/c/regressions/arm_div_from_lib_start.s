// Minimal _main/exit runtime for arm_div_from_lib.c -- deliberately
// does NOT define _div/_divu/_mod/_modu itself (see
// arm_div_from_lib_lib.s, built into a separate library and pulled in
// only via -l, the whole point of this regression test).

TEXT _main(SB), $0
	MOVW $setR12(SB), R12
	BL main(SB)
	BL exit(SB) // should not be reached

TEXT exit+0(SB), $0
	// status arrives in R0 already (5c passes the first argument in a
	// register, never spilled to FP+0 unless the callee takes its
	// address -- see lib_core/libc/arch/arm/rt0.s's own comment)
	MOVW	$1, R7          // syscall number 1 = sys_exit
	SWI	$0
	RET // never reached
