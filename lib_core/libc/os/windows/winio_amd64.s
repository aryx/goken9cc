// Claude Code: raw Windows (PE) amd64 kernel32 stubs backing the
// Plan9-shaped open/read/write/close/seek glue in open.c. Same
// __imp_* indirect-call / caller's-SP-save / 16-byte-realign / shadow-
// space pattern already established by tests/c/mini2/windows_amd64.s
// and tests/c/mini/xwrite_windows_amd64.s -- see those files' own
// comments for the Win64-ABI background (6c-generated callers only
// guarantee plan9-ABI stack args, no Win64 alignment/shadow space at
// the call site, so every stub here has to make its own room).
//
// Wired via lib_core/libc/mkfile's OSEXTRAFILES override (see that
// mkfile's own comment); arch/amd64/rt0.s's generic _main is reused
// unmodified as the windows/amd64 entry point too -- see this file's
// own exit() below for why no separate rt0_windows.s was needed.
// Verified with a real native build+run on a Windows/Cygwin host.

#define OPEN_EXISTING		3
// claude: CREATE_ALWAYS == "create it, and truncate it if it already
// exists", which is exactly Plan9 create(2)'s contract (see
// lib_core/libc/os/linux/open.c's create() for the same reasoning
// applied to O_CREAT|O_TRUNC). Used only by _wincreate below.
#define CREATE_ALWAYS		2
#define FILE_SHARE_READ		1
#define FILE_SHARE_WRITE	2
#define FILE_ATTRIBUTE_NORMAL	0x80
// claude: CreateFileA refuses to open a DIRECTORY unless this flag is
// set -- there is no dwDesiredAccess that makes a plain open work.
// Needed because Plan9's create() must hand back the new directory
// already open (see open.c's create()); _winopendir below is the only
// caller.
#define FILE_FLAG_BACKUP_SEMANTICS	0x02000000
// claude: _winopendir below is the only user, and this was missing --
// the same constant is #defined in open.c, but that is a C file and
// says nothing about this one, so the file did not assemble at all
// ("syntax error, last name: GENERIC_READ"). It went unnoticed because
// the windows build is only reachable via `mk test_windows`, which
// needs a real Windows host and so never runs on the Linux dev box or
// in CI. Found while adding _winalloc.
#define GENERIC_READ	0x80000000

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

// h = _wincreate(path, access)
//   CreateFileA(path, access, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
//               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)
// Byte-for-byte _winopen above except for dwCreationDisposition, which
// is the single Win32 parameter that separates Plan9's open() from its
// create(). A shared stub taking the disposition as a third argument
// would have been the obvious alternative; kept separate so _winopen's
// already-verified argument layout is untouched.
//
// Plan9's create() also takes a `perm` word, which has no CreateFileA
// equivalent worth emulating (Win32 ACLs are not Unix mode bits), so
// open.c drops it -- see that file's create() comment. NOT verified on
// a real Windows host, unlike the stubs above; this arrived with
// create/remove/chdir for the other GOOSes and no Windows machine was
// available to run it.
TEXT _wincreate+0(SB), $0
	MOVQ	SP, DI			// save caller's SP
	MOVQ	path+0(FP), CX		// 1st: lpFileName
	MOVQ	access+8(FP), DX	// 2nd: dwDesiredAccess

	ANDQ	$-16, SP
	SUBQ	$64, SP			// 32 shadow + 3 stack args (24), 16-aligned

	MOVQ	$(FILE_SHARE_READ|FILE_SHARE_WRITE), R8
	MOVQ	$0, R9			// lpSecurityAttributes = NULL
	MOVQ	$CREATE_ALWAYS, 32(SP)
	MOVQ	$FILE_ATTRIBUTE_NORMAL, 40(SP)
	MOVQ	$0, 48(SP)		// hTemplateFile = NULL
	MOVQ	__imp_CreateFileA(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET				// HANDLE (or INVALID_HANDLE_VALUE) in AX

// ok = _windelete(path) -- DeleteFileA(path), backing remove().
// Returns a BOOL, like _winclose below; open.c does the 0/-1
// translation. Unverified on a real Windows host, see _wincreate.
TEXT _windelete+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_DeleteFileA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// ok = _winchdir(path) -- SetCurrentDirectoryA(path), backing chdir().
// Same BOOL-returning shape as _windelete. Note this moves the
// process-wide current directory, the same thing chdir(2) does on the
// Unix targets. Unverified on a real Windows host, see _wincreate.
TEXT _winchdir+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_SetCurrentDirectoryA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// h = _winopendir(path) -- CreateFileA(path, GENERIC_READ, share, NULL,
//   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL)
// _winopen with the one flag that lets CreateFileA return a handle to a
// directory. Plan9's create() returns the new object already open, so
// its DMDIR path needs this after _winmkdir. Access is fixed at
// GENERIC_READ, not a parameter: create() only accepts OREAD for a
// directory anyway (os/linux/open.c's create() explains why).
// Unverified on a real Windows host, see _wincreate.
TEXT _winopendir+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX

	ANDQ	$-16, SP
	SUBQ	$64, SP

	MOVQ	$GENERIC_READ, DX
	MOVQ	$(FILE_SHARE_READ|FILE_SHARE_WRITE), R8
	MOVQ	$0, R9
	MOVQ	$OPEN_EXISTING, 32(SP)
	MOVQ	$FILE_FLAG_BACKUP_SEMANTICS, 40(SP)
	MOVQ	$0, 48(SP)
	MOVQ	__imp_CreateFileA(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET

// ok = _winmkdir(path) -- CreateDirectoryA(path, NULL), backing
// create()'s DMDIR bit. BOOL result; open.c does the 0/-1 translation.
// Note Win32 has no mode/perm argument at all (ACLs, not Unix bits), so
// Plan9's perm word is dropped here -- see open.c's create().
// Unverified on a real Windows host, see _wincreate.
TEXT _winmkdir+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	$0, DX			// lpSecurityAttributes = NULL
	MOVQ	__imp_CreateDirectoryA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// ok = _winrmdir(path) -- RemoveDirectoryA(path). Win32 splits file and
// directory removal exactly as POSIX splits unlink/rmdir, so remove()
// needs the same two-step bridge here that port/remove.c does on the
// POSIX GOOSes. Unverified on a real Windows host, see _wincreate.
TEXT _winrmdir+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_RemoveDirectoryA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// attrs = _winattrs(path) -- GetFileAttributesA(path), backing
// access(). Returns INVALID_FILE_ATTRIBUTES (0xffffffff) if the path
// does not exist; open.c tests for that and for FILE_ATTRIBUTE_READONLY.
// Unverified on a real Windows host, see _wincreate.
TEXT _winattrs+0(SB), $0
	MOVQ	SP, DI
	MOVQ	path+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetFileAttributesA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

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

// exit(status) -- ExitProcess(status), never returns. Lives here
// (rather than a separate rt0_windows.s) because arch/amd64/rt0.s's
// generic _main (CALL main(SB); MOVL $0,(SP); CALL exit(SB)) already
// works unmodified for windows: hello.c/io.c's main() takes no
// arguments, so there's no dyld-style register-vs-stack argc/argv
// bridging to do the way rt0_darwin.s needs -- the only GOOS-specific
// piece process startup actually needs is exit() itself having a real
// body instead of the raw-syscall one every other GOOS gets from
// syscall/os/$GOOS/zsyscall_*.c. Same shape as
// tests/c/mini2/windows_amd64.s's exit(), which predates this and was
// verified working native on this Windows host.
TEXT exit(SB), $0
	MOVL	8(SP), CX		// status
	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space
	MOVQ	__imp_ExitProcess(SB), AX
	CALL	AX
	RET

// claude: p = _winalloc(n)
//   VirtualAlloc(NULL, n, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE)
// Backs os/windows/sbrk.c -- the only stub in this file that is not a
// file operation. Four arguments, all register-passed on Win64
// (RCX/RDX/R8/R9), so this needs only the mandatory 32-byte shadow
// space and no stack arguments at all -- the simplest stub here after
// _winattrs.
//
// lpAddress is NULL rather than a requested address: letting the kernel
// choose is what makes this a legal sbrk without a break. MEM_COMMIT
// and MEM_RESERVE together in one call means "reserve the range and
// back it with real pages now", which is what a caller expecting sbrk
// memory to be immediately writable needs; reserving without committing
// would hand back an address that faults on first touch.
//
// VirtualAlloc returns NULL (not -1) on failure -- sbrk.c converts.
#define MEM_COMMIT	0x1000
#define MEM_RESERVE	0x2000
#define PAGE_READWRITE	0x04

TEXT _winalloc+0(SB), $0
	MOVQ	SP, DI			// save caller's SP
	MOVQ	n+0(FP), DX		// 2nd: dwSize

	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space only, no stack args

	MOVQ	$0, CX			// 1st: lpAddress = NULL
	MOVQ	$(MEM_COMMIT|MEM_RESERVE), R8	// 3rd: flAllocationType
	MOVQ	$PAGE_READWRITE, R9		// 4th: flProtect
	MOVQ	__imp_VirtualAlloc(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET				// LPVOID (or NULL) in AX

// claude: the "small tier" stubs -- getpid/getwd/time/sleep. All four
// are ordinary kernel32 calls here rather than syscalls, and all four
// take 0, 1 or 2 register arguments, so each needs only the mandatory
// 32-byte shadow space and no stack arguments.

// pid = _wingetpid()
//   GetCurrentProcessId(void)
// No arguments at all -- the shortest stub in this file. The shadow
// space is still mandatory even with zero arguments: the callee owns
// those 32 bytes unconditionally under the Win64 ABI.
TEXT _wingetpid+0(SB), $0
	MOVQ	SP, DI
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetCurrentProcessId(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// DWORD pid in AX

// n = _wingetcwd(nbuf, buf)
//   GetCurrentDirectoryA(nBufferLength, lpBuffer)
// Argument order is deliberately (nbuf, buf), matching Win32's own
// (length first) rather than the (buf, len) every other call in this
// tree uses -- so the two arguments land in RCX/RDX with no shuffling.
// os/windows/getwd.c does the reordering, where it is visible in C.
TEXT _wingetcwd+0(SB), $0
	MOVQ	SP, DI
	MOVL	nbuf+0(FP), CX		// 1st: nBufferLength (zero-extend)
	MOVQ	buf+8(FP), DX		// 2nd: lpBuffer
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetCurrentDirectoryA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// chars written, or 0 on failure

// _winfiletime(ft)
//   GetSystemTimeAsFileTime(lpSystemTimeAsFileTime)
// Writes a 64-bit FILETIME (100-nanosecond ticks since 1601) through
// the pointer; returns void, so callers cannot check for failure and
// this stub reports none. os/windows/time.c converts the epoch.
TEXT _winfiletime+0(SB), $0
	MOVQ	SP, DI
	MOVQ	ft+0(FP), CX		// 1st: lpSystemTimeAsFileTime
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetSystemTimeAsFileTime(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// _winsleep(ms)
//   Sleep(dwMilliseconds)
// Win32's Sleep already takes milliseconds, exactly like
// include/os/time.h's sleep(long) -- the only GOOS besides plan9 where
// the units need no conversion. Returns void.
TEXT _winsleep+0(SB), $0
	MOVQ	SP, DI
	MOVL	ms+0(FP), CX		// 1st: dwMilliseconds (zero-extend)
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_Sleep(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// claude: ok = _winfileinfo(handle, buf)
//   GetFileInformationByHandle(handle, lpFileInformation)
// Backs dirfstat() (os/windows/stat.c) -- the by-handle stat-family
// equivalent, chosen over GetFileAttributesA (already used by access(),
// see _winattrs above) because that call only returns the attributes
// bitmask, not size/inode/timestamps. Two register arguments, so shadow
// space only, same shape as _wingetpid. BOOL result; buf is a
// BY_HANDLE_FILE_INFORMATION-shaped Windows.c-local struct, not defined
// here -- same "raw layer stays struct-free" split as every _sysXxx
// syscall wrapper on the Linux/Darwin side. Unverified on a real
// Windows host, see _wincreate above.
TEXT _winfileinfo+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX	// 1st: hFile
	MOVQ	buf+8(FP), DX		// 2nd: lpFileInformation
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetFileInformationByHandle(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// claude: ok = _winseteof(handle) -- SetEndOfFile(handle)
// Backs dirfwstat()'s length field: Win32 has no direct by-handle
// ftruncate, only "truncate to wherever the file pointer currently is"
// -- os/windows/stat.c seeks to the target length with the existing
// _winseek stub first, then calls this. One register argument, shadow
// space only. Unverified on a real Windows host, see _wincreate above.
TEXT _winseteof+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX	// 1st: hFile
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_SetEndOfFile(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET

// claude: n = _wingetenv(name, buf, size)
//   GetEnvironmentVariableA(lpName, lpBuffer, nSize)
// Backs os/windows/getenv.c. Three register arguments (RCX/RDX/R8), so
// shadow space only. Called twice per lookup: first with buf = nil and
// size = 0 to learn the required length, then to fill. See getenv.c for
// the three-way overloading of the return value.
TEXT _wingetenv+0(SB), $0
	MOVQ	SP, DI			// save caller's SP
	MOVQ	name+0(FP), CX		// 1st: lpName
	MOVQ	buf+8(FP), DX		// 2nd: lpBuffer
	MOVL	size+16(FP), R8		// 3rd: nSize (zero-extend)

	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space only, no stack args

	MOVQ	__imp_GetEnvironmentVariableA(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET				// DWORD: chars written, or required size, or 0

// claude: ok = _winsetenv(name, value) -- SetEnvironmentVariableA(lpName,
// lpValue). Backs os/windows/putenv.c, the write-side counterpart of
// _wingetenv above. Two register arguments (RCX/RDX), so shadow space
// only, same shape as _windelete. Unverified on a real Windows host,
// like every _win* stub added without one available (see _wincreate's
// own comment) -- the two-argument, shadow-space-only shape is
// identical to several already-verified stubs above (_windelete,
// _winchdir), so the ABI mechanics here carry the same confidence,
// just not an actual run.
TEXT _winsetenv+0(SB), $0
	MOVQ	SP, DI			// save caller's SP
	MOVQ	name+0(FP), CX		// 1st: lpName
	MOVQ	value+8(FP), DX	// 2nd: lpValue

	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space only, no stack args

	MOVQ	__imp_SetEnvironmentVariableA(SB), AX
	CALL	AX

	MOVQ	DI, SP
	RET				// BOOL

// claude: Tier 4 process control (docs/claude_notes/plan_syscalls.txt).
// ok = _wincreatepipe(rd, wr)
//   CreatePipe(rd, wr, NULL, 0)
// A real, faithful Win32 primitive -- unlike fork()/wait() below, this
// one needs no design compromise: rd/wr are OUT HANDLE* pointers
// (os/windows/pipe.c supplies real storage for both), lpPipeAttributes
// = NULL (not inheritable by default -- os/windows/exec.c's
// bInheritHandles=TRUE on the child process is the actual inheritance
// mechanism, not this flag) and nSize = 0 asks for the default OS
// buffer size. Four register arguments, so shadow space only.
TEXT _wincreatepipe+0(SB), $0
	MOVQ	SP, R15
	MOVQ	rd+0(FP), SI		// 1st: hReadPipe*
	MOVQ	wr+8(FP), DI		// 2nd: hWritePipe*

	ANDQ	$-16, SP
	SUBQ	$32, SP			// shadow space only, no stack args

	MOVQ	SI, CX
	MOVQ	DI, DX
	MOVQ	$0, R8			// 3rd: lpPipeAttributes = NULL
	MOVQ	$0, R9			// 4th: nSize = 0 (default)
	MOVQ	__imp_CreatePipe(SB), AX
	CALL	AX

	MOVQ	R15, SP
	RET				// BOOL result in AX

// claude: ok = _wincreateprocess(app, cmdline, startupinfo, procinfo)
//   CreateProcessA(app, cmdline, NULL, NULL, TRUE, 0, NULL, NULL,
//                  startupinfo, procinfo)
// Backs exec()/execl() (os/windows/exec.c) -- the closest Win32 gets to
// fork()+exec() in one call, which is exactly why this project's
// Windows exec() is built directly on it instead of trying to fake a
// real fork() first (see exec.c's own comment on why fork() itself
// stays unimplemented here).
//
// This stub only takes the 4 arguments that actually vary per call;
// the other 6 CreateProcessA wants are hardcoded here rather than
// threaded through as C parameters, since none of them ever needs to
// differ for this project's one caller:
//   lpProcessAttributes/lpThreadAttributes = NULL (default security)
//   bInheritHandles = TRUE (the child must inherit this process's
//     stdin/stdout/stderr -- the same fd inheritance real fork()+exec()
//     gives for free, and what makes compilers/pcc/pcc.c's dopipe()
//     dup-onto-0/1-before-exec pattern meaningful at all)
//   dwCreationFlags = 0 (no special creation behavior)
//   lpEnvironment = NULL (inherit this process's environment)
//   lpCurrentDirectory = NULL (inherit this process's cwd)
// startupinfo/procinfo are raw pointers to caller-allocated,
// caller-zeroed STARTUPINFOA/PROCESS_INFORMATION buffers -- os/windows/
// exec.c builds and sizes both (see that file's own comment on their
// derived, unverified byte sizes); this stub never looks inside either,
// matching every other _winXxx stub's "raw layer stays struct-free"
// split. First 4 args register-passed (RCX/RDX/R8/R9), remaining 6 on
// the stack -- the largest argument list any stub in this file needs.
TEXT _wincreateprocess+0(SB), $0
	MOVQ	SP, R15
	MOVQ	app+0(FP), SI
	MOVQ	cmdline+8(FP), DI
	MOVQ	startupinfo+16(FP), BX
	MOVQ	procinfo+24(FP), R11

	ANDQ	$-16, SP
	SUBQ	$80, SP			// 32 shadow + 6 stack args (48), 16-aligned

	MOVQ	SI, CX			// 1st: lpApplicationName
	MOVQ	DI, DX			// 2nd: lpCommandLine
	MOVQ	$0, R8			// 3rd: lpProcessAttributes = NULL
	MOVQ	$0, R9			// 4th: lpThreadAttributes = NULL
	MOVQ	$1, 32(SP)		// 5th: bInheritHandles = TRUE
	MOVQ	$0, 40(SP)		// 6th: dwCreationFlags = 0
	MOVQ	$0, 48(SP)		// 7th: lpEnvironment = NULL
	MOVQ	$0, 56(SP)		// 8th: lpCurrentDirectory = NULL
	MOVQ	BX, 64(SP)		// 9th: lpStartupInfo
	MOVQ	R11, 72(SP)		// 10th: lpProcessInformation
	MOVQ	__imp_CreateProcessA(SB), AX
	CALL	AX

	MOVQ	R15, SP
	RET				// BOOL result in AX

// claude: r = _winwaitprocess(handle)
//   WaitForSingleObject(handle, INFINITE)
// Backs exec()'s internal wait -- blocks until the just-spawned child
// exits, since there is no separate wait() call for a caller to make
// afterward (see exec.c's own comment on why). dwMilliseconds is
// hardcoded to INFINITE (0xFFFFFFFF): exec() is meant to behave like
// the process it replaced, and a real exec() never times out waiting
// for itself to finish either.
TEXT _winwaitprocess+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	$0xFFFFFFFF, DX	// dwMilliseconds = INFINITE
	MOVQ	__imp_WaitForSingleObject(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// DWORD wait result (WAIT_OBJECT_0 == 0 on success) in AX

// claude: ok = _wingetexitcode(handle, code)
//   GetExitCodeProcess(handle, code)
// The last piece exec() needs: once _winwaitprocess above confirms the
// child is done, this reads its real exit code through the pointer, so
// exec()'s own final exit(status) can propagate it -- otherwise the
// calling process's exit status would always be 0 regardless of
// whether the spawned command actually succeeded.
TEXT _wingetexitcode+0(SB), $0
	MOVQ	SP, R15
	MOVQ	handle+0(FP), SI
	MOVQ	code+8(FP), DI
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	SI, CX
	MOVQ	DI, DX
	MOVQ	__imp_GetExitCodeProcess(SB), AX
	CALL	AX
	MOVQ	R15, SP
	RET				// BOOL result in AX

// claude: os/windows/spawn.c's own portable spawn() (docs/claude_notes/
// notes_libc_api_design.txt's "spawn(): a portable process-spawn
// primitive" section) needs these four new stubs beyond what exec()
// already had: marking a pipe end inheritable before CreateProcessA
// (CreatePipe's own handles are non-inheritable by default -- see
// _wincreatepipe's own comment above), waiting on however many children
// are actually in flight (not just one, unlike _winwaitprocess), and
// environ()'s own windows gap (os/windows/getenv.c only ever answered
// single-name lookups, GetEnvironmentVariableA has no "list them all"
// mode -- GetEnvironmentStrings is the real Win32 primitive for that).

// ok = _winsethandleinfo(handle, mask, flags)
//   SetHandleInformation(handle, mask, flags)
// Three register arguments, shadow space only. os/windows/spawn.c's
// only caller passes mask=HANDLE_FLAG_INHERIT(1), flags=1 -- kept
// general (not hardcoded here) since the raw layer stays free of
// interpreting its own arguments, same split as every stub above.
TEXT _winsethandleinfo+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	MOVL	mask+8(FP), DX
	MOVL	flags+16(FP), R8
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_SetHandleInformation(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL

// r = _winwaitmultiple(count, handles, waitall, ms)
//   WaitForMultipleObjects(count, handles, waitall, ms)
// Backs os/windows/wait.c's own wait() -- Win32 has no "wait for any
// child" call the way POSIX wait4(-1,...) is, only this explicit-list
// form (capped at MAXIMUM_WAIT_OBJECTS=64, matching wait.c's own table
// size), so wait() has to keep a live-HANDLE table and pass it here
// every call. Four register arguments (RCX/RDX/R8/R9), shadow space
// only. Raw DWORD result (WAIT_OBJECT_0+n on success, WAIT_TIMEOUT or
// WAIT_FAILED otherwise) -- wait.c does the interpreting, not this stub.
TEXT _winwaitmultiple+0(SB), $0
	MOVQ	SP, DI
	MOVL	count+0(FP), CX
	MOVQ	handles+8(FP), DX
	MOVL	waitall+16(FP), R8
	MOVL	ms+24(FP), R9
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_WaitForMultipleObjects(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// DWORD

// p = _wingetenvstrings()
//   GetEnvironmentStrings(void)
// Backs os/windows/getenv.c's environ() -- the one Win32 call that
// hands back the WHOLE environment as a double-nul-terminated block of
// "NAME=value\0...\0\0" strings, unlike GetEnvironmentVariableA (single-
// name lookups only, already wired via _wingetenv above). No arguments
// at all, same shape as _wingetpid. Note the real DLL export is named
// GetEnvironmentStrings, not GetEnvironmentStringsA -- MinGW-w64's own
// processenv.h #defines the A-suffixed name to the same unsuffixed
// symbol for this one call (confirmed against real headers, not
// assumed from the usual-A-suffix pattern every other stub here uses).
TEXT _wingetenvstrings+0(SB), $0
	MOVQ	SP, DI
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetEnvironmentStrings(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// LPCH (or NULL) in AX

// ok = _winfreeenvstrings(p) -- FreeEnvironmentStringsA(p)
// The write-side counterpart _wingetenvstrings needs: the block it
// returns is not caller-managed memory, Win32 owns it and this is how
// it gets released once os/windows/getenv.c has copied what it needs
// out of it. Real, A-suffixed export this time (unlike the get side).
TEXT _winfreeenvstrings+0(SB), $0
	MOVQ	SP, DI
	MOVQ	p+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_FreeEnvironmentStringsA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL

// claude: rc self-hosting's own os/windows/dirread.c/isatty.c/dup.c
// (docs/claude_notes/notes_libc_selfhost.txt's rc-on-windows entry)
// need these six more stubs -- directory enumeration (Win32 has no
// getdents64 equivalent, only a path-pattern-based Find*File family),
// telling a console handle apart from a redirected file/pipe, and the
// one honest slice of dup() this GOOS can support (see os/windows/
// dup.c's own header comment on why only fd 0/1/2 targets work).

// h = _winfindfirst(pattern, finddata) -- FindFirstFileA(pattern, finddata)
// Two register arguments, shadow space only.
TEXT _winfindfirst+0(SB), $0
	MOVQ	SP, DI
	MOVQ	pattern+0(FP), CX
	MOVQ	finddata+8(FP), DX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_FindFirstFileA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// HANDLE (or INVALID_HANDLE_VALUE) in AX

// ok = _winfindnext(handle, finddata) -- FindNextFileA(handle, finddata)
TEXT _winfindnext+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	MOVQ	finddata+8(FP), DX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_FindNextFileA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL

// ok = _winfindclose(handle) -- FindClose(handle)
TEXT _winfindclose+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_FindClose(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL

// n = _wingetfinalpath(handle, buf, buflen, flags)
//   GetFinalPathNameByHandleA(handle, buf, buflen, flags)
// The Win32 counterpart of os/windows/getwd.c's/os/darwin/dirread.c's
// own fcntl(F_GETPATH) trick: Opendir() (rc/goken.c) only ever has a
// HANDLE for the directory, but FindFirstFileA needs a real path
// pattern ("dir\*"), so os/windows/dirread.c asks the kernel for that
// handle's own path first. Four register arguments, shadow space only.
TEXT _wingetfinalpath+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	MOVQ	buf+8(FP), DX
	MOVL	buflen+16(FP), R8
	MOVL	flags+24(FP), R9
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetFinalPathNameByHandleA(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// DWORD: chars written, or 0 on failure

// ok = _wingetconsolemode(handle, mode) -- GetConsoleMode(handle, mode)
// Backs isatty(): succeeds only when handle is a real console, fails
// (mode never written) for a redirected file/pipe -- the standard
// Win32 isatty(3) technique (what MSVCRT's own _isatty() does
// internally), since Windows has no ioctl(TCGETS)/termios concept at
// all for os/linux/isatty.c's/os/darwin/isatty.c's own approach to
// carry over.
TEXT _wingetconsolemode+0(SB), $0
	MOVQ	SP, DI
	MOVQ	handle+0(FP), CX
	MOVQ	mode+8(FP), DX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetConsoleMode(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL

// h = _wingetcurrentprocess() -- GetCurrentProcess(void)
// A real DLL export (not a macro the way some CRTs inline it) that
// hands back the current process's own pseudo-handle -- os/windows/
// dup.c's own DuplicateHandle call needs it twice (source AND target
// process, since it's duplicating a handle within this same process).
TEXT _wingetcurrentprocess+0(SB), $0
	MOVQ	SP, DI
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_GetCurrentProcess(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// HANDLE (pseudo-handle) in AX

// ok = _winduphandle(src, dst)
//   DuplicateHandle(cur, src, cur, dst, 0, TRUE, DUPLICATE_SAME_ACCESS)
// Backs os/windows/dup.c. Unlike every other stub here, this hardcodes
// 5 of DuplicateHandle's real 7 arguments (both process handles to
// _wingetcurrentprocess()'s own result -- duplicating a handle within
// THIS process, never to another one; dwDesiredAccess=0+
// DUPLICATE_SAME_ACCESS, meaning "keep whatever access the source
// handle already has" rather than asking for a specific, possibly-
// mismatched set; bInheritHandle=TRUE, matching os/windows/spawn.c's
// own child-inheritance story) since dup.c's only caller never needs
// them to vary. Five register/stack arguments after the two real
// parameters (src, dst's target pointer) -- the largest stack frame
// in this file after _wincreateprocess.
#define DUPLICATE_SAME_ACCESS	2
TEXT _winduphandle+0(SB), $0
	MOVQ	SP, R15
	MOVQ	src+0(FP), SI
	MOVQ	dst+8(FP), DI

	CALL	_wingetcurrentprocess(SB) // AX = pseudo-handle, safe: no args to clobber

	ANDQ	$-16, SP
	SUBQ	$64, SP			// 32 shadow + 2 stack args (16), 16-aligned

	MOVQ	AX, CX			// 1st: hSourceProcessHandle
	MOVQ	SI, DX			// 2nd: hSourceHandle
	MOVQ	AX, R8			// 3rd: hTargetProcessHandle (same process)
	MOVQ	DI, R9			// 4th: lpTargetHandle
	MOVQ	$0, 32(SP)		// 5th: dwDesiredAccess = 0
	MOVQ	$1, 40(SP)		// 6th: bInheritHandle = TRUE
	MOVQ	$DUPLICATE_SAME_ACCESS, 48(SP)	// 7th: dwOptions
	MOVQ	__imp_DuplicateHandle(SB), AX
	CALL	AX

	MOVQ	R15, SP
	RET				// BOOL

// ok = _winsetstdhandle(std, handle) -- SetStdHandle(std, handle)
// The write-side counterpart of _wingetstdhandle -- os/windows/dup.c's
// own way to make a duplicated handle become THIS process's new
// fd-0/1/2-equivalent, the one case dup(old,new) can honestly support
// on this GOOS (see that file's own header comment).
TEXT _winsetstdhandle+0(SB), $0
	MOVQ	SP, DI
	MOVL	std+0(FP), CX
	MOVQ	handle+8(FP), DX
	ANDQ	$-16, SP
	SUBQ	$32, SP
	MOVQ	__imp_SetStdHandle(SB), AX
	CALL	AX
	MOVQ	DI, SP
	RET				// BOOL
