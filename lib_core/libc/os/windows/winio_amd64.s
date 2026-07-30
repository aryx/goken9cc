// Claude Code: raw Windows (PE) amd64 kernel32 stubs backing the
// Plan9-shaped open/read/write/close/seek glue in open.c. Same
// __imp_* indirect-call / caller's-SP-save / 16-byte-realign / shadow-
// space pattern already established by tests/c/mini2/windows_amd64.s
// and tests/c/mini/xwrite_windows_amd64.s -- see those files' own
// comments for the Win64-ABI background (6c-generated callers only
// guarantee plan9-ABI stack args, no Win64 alignment/shadow space at
// the call site, so every stub here has to make its own room).
//
// NOT wired into lib_core/libc/mkfile: there is no GOOS=windows
// rt0/_main entry point there yet (unlike linux/darwin -- see
// docs/claude_notes/notes_libc_selfhost.txt), so this is source only,
// not yet even compile-checked, let alone run. Best-effort per the
// user's own "I'll refine on my Windows machine" framing when this was
// requested.

#define OPEN_EXISTING		3
#define FILE_SHARE_READ		1
#define FILE_SHARE_WRITE	2
#define FILE_ATTRIBUTE_NORMAL	0x80

// h = _winopen(path, access)
//   CreateFileA(path, access, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
//               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)
// dwCreationDisposition is always OPEN_EXISTING, not a parameter:
// Plan9's open() (include/os/file.h) never creates -- no OCREATE bit
// exists in that API at all, files are created via a separate create()
// syscall this project hasn't wired up yet.
TEXT _winopen+0(SB), $0
	MOVQ	SP, DI			// save caller's SP
	MOVQ	path+0(FP), CX		// 1st: lpFileName
	MOVQ	access+8(FP), DX	// 2nd: dwDesiredAccess

	ANDQ	$-16, SP
	SUBQ	$64, SP			// 32 shadow + 3 stack args (24), 16-aligned

	MOVQ	$(FILE_SHARE_READ|FILE_SHARE_WRITE), R8
	MOVQ	$0, R9			// lpSecurityAttributes = NULL
	MOVQ	$OPEN_EXISTING, 32(SP)
	MOVQ	$FILE_ATTRIBUTE_NORMAL, 40(SP)
	MOVQ	$0, 48(SP)		// hTemplateFile = NULL
	MOVQ	__imp_CreateFileA(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET				// HANDLE (or INVALID_HANDLE_VALUE) in AX

// n = _winread(handle, buf, len) -- ReadFile(handle, buf, len, &nread, NULL)
TEXT _winread+0(SB), $0
	MOVQ	SP, R15			// save caller's SP
	MOVQ	handle+0(FP), SI
	MOVQ	buf+8(FP), DI
	MOVL	len+16(FP), BX		// zero-extend, see xwrite_windows_amd64.s

	ANDQ	$-16, SP
	SUBQ	$48, SP			// 32 shadow + 8 (5th arg) + 8 (nread out)

	MOVQ	SI, CX			// hFile
	MOVQ	DI, DX			// lpBuffer
	MOVL	BX, R8			// nNumberOfBytesToRead
	LEAQ	40(SP), R9		// lpNumberOfBytesRead
	MOVQ	$0, 32(SP)		// lpOverlapped = NULL
	MOVQ	__imp_ReadFile(SB), AX
	CALL	AX

	TESTL	AX, AX			// AX holds BOOL success, not the count
	JNE	winread_ok
	MOVQ	$-1, AX
	MOVQ	R15, SP
	RET
winread_ok:
	MOVL	40(SP), AX		// nNumberOfBytesRead, read before SP moves
	MOVQ	R15, SP
	RET

// n = _winwrite(handle, buf, len) -- same shape as _winread above
TEXT _winwrite+0(SB), $0
	MOVQ	SP, R15
	MOVQ	handle+0(FP), SI
	MOVQ	buf+8(FP), DI
	MOVL	len+16(FP), BX

	ANDQ	$-16, SP
	SUBQ	$48, SP

	MOVQ	SI, CX
	MOVQ	DI, DX
	MOVL	BX, R8
	LEAQ	40(SP), R9
	MOVQ	$0, 32(SP)
	MOVQ	__imp_WriteFile(SB), AX
	CALL	AX

	TESTL	AX, AX
	JNE	winwrite_ok
	MOVQ	$-1, AX
	MOVQ	R15, SP
	RET
winwrite_ok:
	MOVL	40(SP), AX
	MOVQ	R15, SP
	RET

// ok = _winclose(handle) -- CloseHandle(handle); BOOL result in AX,
// open.c's close() does the POSIX-style 0/-1 translation.
TEXT _winclose+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_CloseHandle(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// pos = _winseek(handle, offset, whence)
//   SetFilePointerEx(handle, offset, &newpos, whence)
// FILE_BEGIN/FILE_CURRENT/FILE_END (0/1/2) already match POSIX/Plan9's
// SEEK_SET/SEEK_CUR/SEEK_END numerically, same lucky coincidence
// port/seek.c documents for the Linux/Darwin lseek() -- no whence
// translation needed here either.
TEXT _winseek+0(SB), $0
	MOVQ	SP, R15
	MOVQ	handle+0(FP), SI
	MOVQ	offset+8(FP), DI	// LARGE_INTEGER passed by value (8 bytes)
	MOVL	whence+16(FP), BX

	ANDQ	$-16, SP
	SUBQ	$48, SP			// 32 shadow + 8 (lpNewFilePointer out)

	MOVQ	SI, CX			// hFile
	MOVQ	DI, DX			// liDistanceToMove
	LEAQ	40(SP), R8		// lpNewFilePointer
	MOVQ	BX, R9			// dwMoveMethod
	MOVQ	__imp_SetFilePointerEx(SB), AX
	CALL	AX

	TESTL	AX, AX
	JNE	winseek_ok
	MOVQ	$-1, AX
	MOVQ	R15, SP
	RET
winseek_ok:
	MOVQ	40(SP), AX		// new absolute position
	MOVQ	R15, SP
	RET

// h = _wingetstdhandle(std) -- GetStdHandle(std), std one of
// STD_{INPUT,OUTPUT,ERROR}_HANDLE (-10/-11/-12). open.c's read()/
// write() route the well-known fds 0/1/2 through this instead of
// treating them as a (truncated) real HANDLE value -- mirrors
// tests/c/mini2/windows_amd64.s's write(), which does the same
// GetStdHandle dispatch inline rather than as a separate stub.
TEXT _wingetstdhandle+0(SB), $0
	MOVQ	SP, DI
	MOVL	std+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetStdHandle(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET
