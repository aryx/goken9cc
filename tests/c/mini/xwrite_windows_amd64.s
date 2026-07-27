// Windows (PE) amd64 stub for 6l -H 10. Mirrors xwrite_macos_amd64.s /
// xwrite_amd64.s but goes through kernel32.dll (GetStdHandle/WriteFile)
// instead of a raw syscall -- see ../../s/mini/hello_windows_amd64.s for
// the entry-point version of the same __imp_* indirect-call trick (6l
// exposes each kernel32 import as an __imp_<name> symbol pointing at its
// IAT slot) and tests/c/mini2/windows_amd64.s's write() for the same
// stub shape reused from ordinary (non-entry-point) plan9-ABI code.
//
// xwrite is called from 6c-generated code, which only guarantees
// plan9-ABI stack args -- no Win64 16-byte SP alignment or shadow space
// at the call site, and (unlike the entry point in hello_windows_amd64.s)
// xwrite is called more than once, so SP must come back exactly as found:
// the caller's SP is saved in DI and restored before RET, the same as
// R15 in tests/c/mini2/windows_amd64.s's write(). The FP-relative args
// are also read *before* SP is realigned: once SP moves, buf+0(FP)/
// len+8(FP) no longer resolve to the right place.
TEXT	xwrite+0(SB), $0
	MOVQ	SP, DI			// save caller's SP (restore before RET)
	MOVQ	buf+0(FP), SI		// 1st argument: buf
	// MOVL not MOVQ: len is an int and the caller (6c) stores only 32
	// bits on the stack; see xwrite_macos_amd64.s's note on the same
	// line for why a 64-bit load would risk stale garbage in DX/BX's
	// upper half. MOVL zero-extends into the full register instead.
	MOVL	len+8(FP), BX		// 2nd argument: len (32-bit, zero-extended)

	ANDQ	$-16, SP
	SUBQ	$48, SP			// 32 shadow + 8 (5th arg slot) + 8 (written), 16-aligned

	// h = GetStdHandle(STD_OUTPUT_HANDLE = -11)
	MOVL	$-11, CX
	MOVQ	__imp_GetStdHandle(SB), AX
	CALL	AX

	// WriteFile(h, buf, len, &written, NULL)
	MOVQ	AX, CX			// hFile
	MOVQ	SI, DX			// lpBuffer
	MOVL	BX, R8			// nNumberOfBytesToWrite
	LEAQ	40(SP), R9		// lpNumberOfBytesWritten
	MOVQ	$0, 32(SP)		// lpOverlapped = NULL (5th arg)
	MOVQ	__imp_WriteFile(SB), AX
	CALL	AX

	MOVQ	DI, SP			// restore caller's SP
	RET
