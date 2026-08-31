// xexit_sparc.s -- Linux SPARC32 exit(status=0)
//
// Clobbers R1 (g1 = syscall number = exit) without saving/restoring
// it, unlike xwrite_sparc.s's own use of R1 -- safe here only because
// exit() never returns, so kc's REGSP convention for R1 never needs
// to be valid again afterward.

TEXT xexit+0(SB), $0
	MOVW	$0, R8     // o0 = status = 0
	MOVW	$1, R1     // g1 = syscall number = exit (1)
	MOVW	$0x10, R9  // scratch reg holding the trap number
	TA	R9
	RETURN             // not reached
