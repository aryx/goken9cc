// "Hello, World!" for 6l -H 10 (PE32+ for Windows x86-64).
//
// 6l emits a kernel32.dll import table and exposes each thunk as an
// __imp_<name> symbol pointing at its IAT slot (see src/cmd/ld/pe.c). The
// slot holds the resolved function pointer once the loader binds it, so we
// load it and call indirectly (MOVQ __imp_<name>(SB), AX; CALL AX). A
// direct "CALL __imp_<name>(SB)" would be rejected by the linker's patch()
// pass, which expects direct call targets to be text symbols.
//
// Win64 calling convention: integer args in CX, DX, R8, R9; the caller
// reserves 32 bytes of shadow space plus room for any stack args, and RSP
// must be 16-byte aligned at the call. We RET into RtlUserThreadStart in
// case ExitProcess ever returns.

TEXT	_start(SB), $0
	SUBQ	$56, SP			// 32 shadow + 8 (5th arg) + 8 (written) + align

	// h = GetStdHandle(STD_OUTPUT_HANDLE = -11)
	MOVL	$-11, CX
	MOVQ	__imp_GetStdHandle(SB), AX
	CALL	AX
	MOVQ	AX, BX			// save handle

	// WriteFile(h, msg, 14, &written, NULL)
	MOVQ	BX, CX			// hFile
	LEAQ	msg(SB), DX		// lpBuffer
	MOVL	$14, R8			// nNumberOfBytesToWrite
	LEAQ	40(SP), R9		// lpNumberOfBytesWritten
	MOVQ	$0, 32(SP)		// lpOverlapped = NULL (5th arg)
	MOVQ	__imp_WriteFile(SB), AX
	CALL	AX

	// ExitProcess(0)
	MOVL	$0, CX
	MOVQ	__imp_ExitProcess(SB), AX
	CALL	AX
	RET

DATA	msg+0(SB)/8, $"Hello, W"
DATA	msg+8(SB)/6, $"orld!\n"
GLOBL	msg(SB), $14
