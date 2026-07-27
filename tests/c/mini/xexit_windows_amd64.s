// Windows (PE) amd64 stub for 6l -H 10; see xwrite_windows_amd64.s for the
// __imp_* indirect-call and stack-realignment background. RET after the
// call is defensive (ExitProcess isn't expected to return), matching
// tests/c/mini2/windows_amd64.s's exit().
TEXT	xexit+0(SB), $0
	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space
	MOVL	$0, CX			// status = 0
	MOVQ	__imp_ExitProcess(SB), AX
	CALL	AX
	RET
