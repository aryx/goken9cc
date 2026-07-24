// Windows (PE) amd64 syscall stubs for 6l -H 10. Mirrors linux_amd64.s /
// macos_amd64.s: same write(fd,buf,n)/exit(status)/panic()/_main() surface
// expected by print_nofloat_no64.c and helloprintf.c (see minilibc.h),
// but implemented via kernel32.dll (GetStdHandle/WriteFile/ExitProcess)
// instead of a raw syscall. See ../../s/mini/hello_windows_amd64.s for
// the entry-point version of the same trick and the Win64-ABI notes
// (indirect __imp_* calls, shadow space, arg registers CX/DX/R8/R9).
// No -z / PIE handling needed here unlike macos_amd64.s: 6l -H 10 (see
// linkers/lk/pe.c) always emits a fixed ImageBase, never ASLR-relocated.
//
// Unlike hello_windows_amd64.s (which IS the entry point, so it controls
// the initial stack layout), write()/exit()/panic() are called from
// ordinary plan9-ABI code (6c-generated: stack-args only, no Win64
// 16-byte alignment guarantee at the call site). So each one realigns SP
// before calling into kernel32 -- otherwise WriteFile/ExitProcess's
// internal SSE spills could fault on a misaligned stack. write() saves
// the incoming SP in R15 first (to still reach the stack-passed args
// after realigning); R15/DI/SI/BX are all callee-saved in the Win64 ABI,
// so they survive the CALLs below untouched, unlike CX/DX/R8/R9/AX.

TEXT	_main(SB), $0
	CALL	main(SB)

TEXT	panic(SB), $0
	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space
	MOVL	$2, CX			// exit(2)
	MOVQ	__imp_ExitProcess(SB), AX
	CALL	AX

TEXT	exit(SB), $0
	MOVL	8(SP), CX		// status
	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space
	MOVQ	__imp_ExitProcess(SB), AX
	CALL	AX
	RET

TEXT	write(SB), $0
	MOVQ	SP, R15			// save caller's SP (args live above it)
	ANDQ	$-16, SP
	SUBQ	$48, SP			// 32 shadow + 8 (5th arg slot) + 8 (written), 16-aligned

	MOVL	8(R15), DI		// fd
	MOVQ	16(R15), SI		// buf
	MOVL	24(R15), BX		// n

	// h = GetStdHandle(fd==2 ? STD_ERROR_HANDLE(-12) : STD_OUTPUT_HANDLE(-11))
	MOVL	$-11, CX
	CMPL	DI, $2
	JNE	gethandle
	MOVL	$-12, CX
gethandle:
	MOVQ	__imp_GetStdHandle(SB), AX
	CALL	AX

	// WriteFile(h, buf, n, &written, NULL)
	MOVQ	AX, CX			// hFile
	MOVQ	SI, DX			// lpBuffer
	MOVL	BX, R8			// nNumberOfBytesToWrite
	LEAQ	40(SP), R9		// lpNumberOfBytesWritten
	MOVQ	$0, 32(SP)		// lpOverlapped = NULL (5th arg)
	MOVQ	__imp_WriteFile(SB), AX
	CALL	AX

	MOVQ	R15, SP
	RET
