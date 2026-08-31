// "Hello, world" for 8l_ -H 10 (PE32 for Windows x86).
//
// 8l emits a kernel32.dll import table and exposes each thunk as an
// __imp_<name> symbol pointing at its IAT slot (see src/cmd/ld/pe.c). The
// slot holds the resolved function pointer once the loader binds it, so we
// load it and call indirectly (MOVL __imp_<name>(SB), AX; CALL AX). A
// direct "CALL __imp_<name>(SB)" would be rejected by the linker's patch()
// pass, which expects direct call targets to be text symbols.
//
// Win32 __stdcall: arguments go on the stack right-to-left and the callee
// pops them (RET n), so ESP is back where it started after each call. We
// reserve the argument area with SUBL and fill it with MOVL rather than
// PUSHL: 8l statically balances PUSH/POP within a function and can't see
// that the callee does the popping, so PUSHL would trip "unbalanced
// PUSH/POP". Results come back in AX.

TEXT	_start(SB), $0
	// h = GetStdHandle(STD_OUTPUT_HANDLE = -11)
	SUBL	$4, SP
	MOVL	$-11, 0(SP)
	MOVL	__imp_GetStdHandle(SB), AX
	CALL	AX			// stdcall pops its arg; SP restored
	MOVL	AX, BX			// save handle (EBX is nonvolatile)

	// WriteFile(h, msg, 13, &written, NULL)
	SUBL	$20, SP
	MOVL	BX, 0(SP)		// hFile
	MOVL	$msg(SB), 4(SP)		// lpBuffer
	MOVL	$13, 8(SP)		// nNumberOfBytesToWrite
	MOVL	$written(SB), 12(SP)	// lpNumberOfBytesWritten
	MOVL	$0, 16(SP)		// lpOverlapped = NULL
	MOVL	__imp_WriteFile(SB), AX
	CALL	AX

	// ExitProcess(0)
	SUBL	$4, SP
	MOVL	$0, 0(SP)
	MOVL	__imp_ExitProcess(SB), AX
	CALL	AX
	RET

DATA	msg+0(SB)/8, $"Hello, w"
DATA	msg+8(SB)/5, $"orld\n"
GLOBL	msg(SB), $13
GLOBL	written(SB), $4
